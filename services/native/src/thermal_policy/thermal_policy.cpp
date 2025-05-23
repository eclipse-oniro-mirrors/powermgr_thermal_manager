/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "thermal_policy.h"

#include <algorithm>
#include "action_charger.h"
#include "action_voltage.h"
#include "constants.h"
#include "file_operation.h"
#include "thermal_common.h"
#include "thermal_service.h"
#include "securec.h"
#include "ffrt_utils.h"
#include "string_operation.h"

using namespace OHOS::AppExecFwk;
namespace OHOS {
namespace PowerMgr {
namespace {
constexpr const char* LEVEL_PATH = "/data/service/el0/thermal/config/configLevel";
const int MAX_PATH = 256;
const int MAX_DELAY_FLAG_SIZE = 2;
constexpr const char* DELAY_TIME_FLAG = "delaytime";
}

bool ThermalPolicy::Init()
{
    SortLevel();
    auto tms = ThermalService::GetInstance();
    if (tms == nullptr) {
        return false;
    }
    RegisterObserver();
    return true;
}

void ThermalPolicy::OnSensorInfoReported(const TypeTempMap& info)
{
    typeTempMap_ = info;
    LevelDecision();
    WriteLevel();
    PolicyDecision();
}

void ThermalPolicy::ExecutePolicy()
{
    LevelDecision();
    PolicyDecision();
}

void ThermalPolicy::SetPolicyMap(PolicyConfigMap& pcm)
{
    clusterPolicyMap_ = pcm;
}

void ThermalPolicy::SetSensorClusterMap(SensorClusterMap& scm)
{
    sensorClusterMap_ = scm;
}

std::map<std::string, uint32_t> ThermalPolicy::GetClusterLevelMap()
{
    return clusterLevelMap_;
}


void ThermalPolicy::SortLevel()
{
    for (auto clusterPolicy = clusterPolicyMap_.begin(); clusterPolicy != clusterPolicyMap_.end(); clusterPolicy++) {
        sort(clusterPolicy->second.begin(), clusterPolicy->second.end(), LevelCompare);
    }
}

void ThermalPolicy::RegisterObserver()
{
    ThermalObserver::Callback callback = [this](const TypeTempMap& info) { this->OnSensorInfoReported(info); };
    auto tms = ThermalService::GetInstance();
    tms->GetObserver()->SetRegisterCallback(callback);
}

void ThermalPolicy::LevelDecision()
{
    for (auto cluster = sensorClusterMap_.begin(); cluster != sensorClusterMap_.end(); cluster++) {
        THERMAL_HILOGD(COMP_SVC, "update [%{public}s] level", cluster->first.c_str());
        cluster->second->UpdateThermalLevel(typeTempMap_);
        uint32_t level = cluster->second->GetCurrentLevel();
        clusterLevelMap_[cluster->first] = level;
    }
}

void ThermalPolicy::WriteLevel()
{
    auto tms = ThermalService::GetInstance();
    if (!tms->GetSimulationXml()) {
        return;
    }
    std::list<uint32_t> levelList;
    int32_t ret = -1;
    char levelBuf[MAX_PATH] = {0};
    for (auto iter = clusterLevelMap_.begin(); iter != clusterLevelMap_.end(); iter++) {
        levelList.push_back(iter->second);
    }
    auto level = *max_element(levelList.begin(), levelList.end());

    ret = snprintf_s(levelBuf, MAX_PATH, sizeof(levelBuf) - 1, LEVEL_PATH);
    if (ret < EOK) {
        return;
    }
    std::string valueString = std::to_string(level) + "\n";
    ret = FileOperation::WriteFile(levelBuf, valueString, valueString.length());
    if (ret != ERR_OK) {
        return;
    }
}

void ThermalPolicy::PolicyDecision()
{
    for (auto clusterPolicy = clusterPolicyMap_.begin(); clusterPolicy != clusterPolicyMap_.end(); clusterPolicy++) {
        const std::string& clusterName = clusterPolicy->first;
        const std::vector<PolicyConfig>& policyConfig = clusterPolicy->second;
        if (clusterName.empty() || policyConfig.empty()) {
            continue;
        }
        auto clusterIter = clusterLevelMap_.find(clusterName);
        if (clusterIter == clusterLevelMap_.end()) {
            continue;
        }
        uint32_t clusterCurrLev = clusterIter->second;
        for (auto levelAction = policyConfig.rbegin(); levelAction != policyConfig.rend(); levelAction++) {
            if (clusterCurrLev >= levelAction->level) {
                ActionDecision(levelAction->policyActionList);
                break;
            }
        }
    }

    PrintPolicyState();

    if (!ActionExecution()) {
        THERMAL_HILOGW(COMP_SVC, "failed to execute action");
        return;
    }

    ActionCharger::ExecuteCurrentLimit();
    ActionVoltage::ExecuteVoltageLimit();
}

void ThermalPolicy::ActionDecision(const std::vector<PolicyAction>& actionList)
{
    std::unordered_map<std::string, PolicyAction> actionPolicyInfos;
    for (auto action = actionList.begin(); action != actionList.end(); action++) {
        if (action->isProp && !StateMachineDecision(action->actionPropMap)) {
            continue;
        }
        auto& actionName = action->actionName;
        actionPolicyInfos[actionName] = *action;
    }
    auto tms = ThermalService::GetInstance();
    ThermalActionManager::ThermalActionMap actionMap = tms->GetActionManagerObj()->GetActionMap();
    for (const auto& [actionName, actionPolicy] : actionPolicyInfos) {
        if (actionMap.count(actionName) == 0 || actionMap[actionName] == nullptr) {
            THERMAL_HILOGE(COMP_SVC, "can't find action [%{public}s] ability", actionPolicy.actionName.c_str());
            continue;
        }
        auto& thermalAction = actionMap[actionName];
        auto ite = actionPolicy.actionPropMap.find(DELAY_TIME_FLAG);
        if (actionPolicy.isProp && ite != actionPolicy.actionPropMap.end()) {
            THERMAL_HILOGI(COMP_SVC, "actionName = %{public}s, actionValue = %{public}s, delayTime = %{public}s",
                actionName.c_str(), actionPolicy.actionValue.c_str(), ite->second.c_str());
            std::vector<std::string> delayStr;
            StringOperation::SplitString(ite->second, delayStr, ",");
            if (delayStr.size() != MAX_DELAY_FLAG_SIZE) {
                THERMAL_HILOGE(COMP_SVC, "size check fail");
                continue;
            }
            PolicyDelayAction delayAction;
            uint32_t actionId = 0;
            StringOperation::StrToUint(delayStr[0], actionId);
            StringOperation::StrToUint(delayStr[1], delayAction.delayTime);
            thermalAction->AddActionDelayTime(actionId, delayAction);
            thermalAction->AddActionValue(actionId, actionPolicy.actionValue);
            continue;
        }
        thermalAction->AddActionValue(actionPolicy.actionValue);
    }
}

void ThermalPolicy::FindSubscribeActionValue()
{
    THERMAL_HILOGD(COMP_SVC, "Enter");
    auto tms = ThermalService::GetInstance();
    if (tms == nullptr) {
        THERMAL_HILOGI(COMP_SVC, "tms is nullptr");
        return;
    }
    if (tms->GetObserver() ==nullptr) {
        THERMAL_HILOGI(COMP_SVC, "tms->GetObserver() is nullptr");
        return;
    }

    tms->GetObserver()->FindSubscribeActionValue();
}

bool ThermalPolicy::StateMachineDecision(const std::map<std::string, std::string>& stateMap)
{
    auto tms = ThermalService::GetInstance();
    for (auto prop = stateMap.begin(); prop != stateMap.end(); prop++) {
        if (prop->first == DELAY_TIME_FLAG) continue;
        StateMachine::StateMachineMap stateMachineMap = tms->GetStateMachineObj()->GetStateCollectionMap();
        auto stateIter = stateMachineMap.find(prop->first);
        if (stateIter == stateMachineMap.end() || stateIter->second == nullptr) {
            THERMAL_HILOGE(COMP_SVC, "can't find state machine [%{public}s]", prop->first.c_str());
            return false;
        }
        if (stateIter->second->DecideState(prop->second)) {
            continue;
        } else {
            return false;
        }
    }
    return true;
}

bool ThermalPolicy::ActionExecution()
{
    auto tms = ThermalService::GetInstance();
    auto actionMgr = tms->GetActionManagerObj();
    if (actionMgr == nullptr) {
        return false;
    }

    ThermalActionManager::ThermalActionMap actionMap = actionMgr->GetActionMap();
    for (auto iter = actionMap.begin(); iter != actionMap.end(); iter++) {
        iter->second->Execute();
    }

    FFRTUtils::SubmitTask([this] {
        return FindSubscribeActionValue();
    });
    return true;
}

void ThermalPolicy::PrintPolicyState()
{
    std::string levInfo = "";
    for (auto clusterIter = clusterLevelMap_.begin(); clusterIter != clusterLevelMap_.end(); clusterIter++) {
        levInfo.append(clusterIter->first).append("-").append(std::to_string(clusterIter->second)).append(" ");
    }
    auto tms = ThermalService::GetInstance();
    tms->GetObserver()->UpdatePolicyState(levInfo);
    THERMAL_HILOGD(COMP_SVC, "current level: %{public}s", levInfo.c_str());
}

void ThermalPolicy::DumpLevel(std::string& result)
{
    for (auto iter = clusterLevelMap_.begin(); iter != clusterLevelMap_.end(); ++iter) {
        result.append("name: ");
        result.append(iter->first);
        result.append("\t");
        result.append("level: ");
        result.append(std::to_string(iter->second));
        result.append("\n");
    }
}

void ThermalPolicy::PrintPolicyAction(std::vector<PolicyAction> policyActionList, std::string& result)
{
    for (auto iter = policyActionList.begin(); iter != policyActionList.end(); ++iter) {
        result.append("actionName: ");
        result.append(iter->actionName);
        result.append("\t");
        result.append("actionValue: ");
        result.append(iter->actionValue);
        result.append("\t");
        for (auto it = iter->actionPropMap.begin(); it != iter->actionPropMap.end(); ++it) {
            result.append(it->first);
            result.append(": ");
            result.append(it->second);
            result.append("\t");
        }
        result.append("isProp: ");
        result.append(std::to_string(iter->isProp));
        result.append("\n");
    }
}

void ThermalPolicy::DumpPolicy(std::string& result)
{
    for (auto iter = clusterPolicyMap_.begin(); iter != clusterPolicyMap_.end(); ++iter) {
        result.append("name: ");
        result.append(iter->first);
        result.append("\t");
        for (auto it = iter->second.begin(); it != iter->second.end(); ++it) {
            result.append("level: ");
            result.append(std::to_string(it->level));
            result.append("\n");
            PrintPolicyAction(it->policyActionList, result);
            result.append("\n");
        }
        result.append("\n");
    }
}
} // namespace PowerMgr
} // namespace OHOS
