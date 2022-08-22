/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "action_display.h"

#include <map>
#include "display_power_mgr_client.h"
#include "file_operation.h"
#include "thermal_hisysevent.h"
#include "thermal_service.h"
#include "securec.h"

using namespace OHOS::DisplayPowerMgr;
namespace OHOS {
namespace PowerMgr {
namespace {
auto g_service = DelayedSpSingleton<ThermalService>::GetInstance();
constexpr const char* LCD_PATH = "/data/service/el0/thermal/config/lcd";
const int MAX_PATH = 256;
std::map<std::string, std::string> g_sceneMap;
}
ActionDisplay::ActionDisplay(const std::string& actionName)
{
    actionName_ = actionName;
}

void ActionDisplay::InitParams(const std::string& params)
{
}

void ActionDisplay::SetStrict(bool flag)
{
    flag_ = flag;
}

void ActionDisplay::SetEnableEvent(bool enable)
{
    enableEvent_ = enable;
}

void ActionDisplay::SetXmlScene(const std::string& scene, const std::string& value)
{
    THERMAL_HILOGD(COMP_SVC, "Enter");
    for (auto iter = g_sceneMap.begin(); iter != g_sceneMap.end(); ++iter) {
        if (iter->first == scene) {
            if (iter->second != value) {
                iter->second = value;
            }
            return;
        }
    }

    g_sceneMap.insert(std::make_pair(scene, value));
}

void ActionDisplay::AddActionValue(std::string value)
{
    THERMAL_HILOGD(COMP_SVC, "value=%{public}s", value.c_str());
    if (value.empty()) return;
    valueList_.push_back(atof(value.c_str()));
}

void ActionDisplay::Execute()
{
    THERMAL_HILOGD(COMP_SVC, "Enter");
    float value;

    for (auto iter = g_sceneMap.begin(); iter != g_sceneMap.end(); ++iter) {
        if (g_service->GetScene() == iter->first) {
            value = atof(iter->second.c_str());
            if ((value != lastValue_) && (!g_service->GetSimulationXml())) {
                DisplayRequest(value);
            } else if (value != lastValue_) {
                DisplayExecution(value);
            } else {
                THERMAL_HILOGD(COMP_SVC, "value is not change");
            }
            WriteActionTriggeredHiSysEventWithRatio(enableEvent_, actionName_, value);
            lastValue_ = value;
            return;
        }
    }

    if (valueList_.empty()) {
        value = 0;
    } else {
        if (flag_) {
            value = *max_element(valueList_.begin(), valueList_.end());
        } else {
            value = *min_element(valueList_.begin(), valueList_.end());
        }
        valueList_.clear();
    }

    if (value != lastValue_) {
        if (!g_service->GetSimulationXml()) {
            DisplayRequest(value);
        } else {
            DisplayExecution(value);
        }
        WriteActionTriggeredHiSysEventWithRatio(enableEvent_, actionName_, value);
        lastValue_ = value;
    }
}

uint32_t ActionDisplay::DisplayRequest(float brightness)
{
    THERMAL_HILOGD(COMP_SVC, "Enter");
    return ERR_OK;
}

int32_t ActionDisplay::DisplayExecution(float brightness)
{
    THERMAL_HILOGD(COMP_SVC, "Enter");
    int32_t ret = -1;
    char lcdBuf[MAX_PATH] = {0};
    ret = snprintf_s(lcdBuf, MAX_PATH, sizeof(lcdBuf) - 1, LCD_PATH);
    if (ret < EOK) {
        return ret;
    }
    std::string valueString = std::to_string(brightness) + "\n";
    ret = FileOperation::WriteFile(lcdBuf, valueString, valueString.length());
    if (ret != ERR_OK) {
        return ret;
    }
    return ERR_OK;
}
} // namespace PowerMgr
} // namespace OHOS
