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

#include "action_charger.h"

#include <map>
#include "constants.h"
#include "file_operation.h"
#include "securec.h"
#include "thermal_hisysevent.h"
#include "thermal_service.h"
#ifdef DRIVERS_INTERFACE_BATTERY_ENABLE
#include "v2_0/ibattery_interface.h"
#endif

namespace OHOS {
namespace PowerMgr {
namespace {
constexpr const char* SC_CURRENT_PATH = "/data/service/el0/thermal/config/sc_current";
constexpr const char* BUCK_CURRENT_PATH = "/data/service/el0/thermal/config/buck_current";
const int MAX_PATH = 256;
}
#ifdef DRIVERS_INTERFACE_BATTERY_ENABLE
std::vector<ChargingLimit> ActionCharger::chargeLimitList_;
#endif

ActionCharger::ActionCharger(const std::string& actionName)
{
    actionName_ = actionName;
}

void ActionCharger::InitParams(const std::string& protocol)
{
    protocol_ = protocol;
}

void ActionCharger::SetStrict(bool enable)
{
    isStrict_ = enable;
}

void ActionCharger::SetEnableEvent(bool enable)
{
    enableEvent_ = enable;
}

void ActionCharger::AddActionValue(uint32_t actionId, std::string value)
{
    if (value.empty()) {
        return;
    }
    if (actionId > 0) {
        auto iter = policyActionMap_.find(actionId);
        if (iter != policyActionMap_.end()) {
            iter->second.uintDelayValue = static_cast<uint32_t>(static_cast<uint32_t>(strtol(value.c_str(),
                nullptr, STRTOL_FORMART_DEC)));
        }
    } else {
        valueList_.push_back(static_cast<uint32_t>(strtol(value.c_str(), nullptr, STRTOL_FORMART_DEC)));
    }
}

void ActionCharger::ExecuteInner()
{
    auto tms = ThermalService::GetInstance();
    THERMAL_RETURN_IF (tms == nullptr);
    for (auto &policyAction : policyActionMap_) {
        if (policyAction.second.isCompleted) {
            valueList_.push_back(policyAction.second.uintDelayValue);
        }
    }

    uint32_t value = GetActionValue();
    if (value != lastValue_) {
        ChargerRequest(value);
        WriteSimValue(value);
        WriteActionTriggeredHiSysEvent(enableEvent_, actionName_, value);
        tms->GetObserver()->SetDecisionValue(actionName_, std::to_string(value));
        lastValue_ = value;
        THERMAL_HILOGD(COMP_SVC, "action execute: {%{public}s = %{public}u}", actionName_.c_str(), lastValue_);
    }
    valueList_.clear();
}

void ActionCharger::ResetActionValue()
{
    lastValue_ = 0;
}

uint32_t ActionCharger::GetActionValue()
{
    uint32_t value = FALLBACK_VALUE_UINT_ZERO;
    if (!valueList_.empty()) {
        if (isStrict_) {
            value = *min_element(valueList_.begin(), valueList_.end());
        } else {
            value = *max_element(valueList_.begin(), valueList_.end());
        }
    }
    return value;
}

int32_t ActionCharger::ChargerRequest(int32_t current)
{
#ifdef DRIVERS_INTERFACE_BATTERY_ENABLE
    ChargingLimit chargingLimit;
    chargingLimit.type = TYPE_CURRENT;
    chargingLimit.protocol = protocol_;
    chargingLimit.value = current;
    chargeLimitList_.push_back(chargingLimit);
#endif

    auto tms = ThermalService::GetInstance();
    auto thermalInterface = tms->GetThermalInterface();
    if (thermalInterface != nullptr) {
        int32_t ret = thermalInterface->SetBatteryCurrent(current);
        if (ret != ERR_OK) {
            THERMAL_HILOGE(COMP_SVC, "failed to set charger current to thermal hdf");
            return ret;
        }
    }
    return ERR_OK;
}

void ActionCharger::ExecuteCurrentLimit()
{
#ifdef DRIVERS_INTERFACE_BATTERY_ENABLE
    if (chargeLimitList_.empty()) {
        return;
    }
    sptr<IBatteryInterface> iBatteryInterface = IBatteryInterface::Get();
    if (iBatteryInterface == nullptr) {
        THERMAL_HILOGE(COMP_SVC, "iBatteryInterface_ is nullptr");
        return;
    }
    int32_t result = iBatteryInterface->SetChargingLimit(chargeLimitList_);
    if (result != ERR_OK) {
        THERMAL_HILOGE(COMP_SVC, "failed to set charge limit");
        return;
    }
    chargeLimitList_.clear();
#endif
}

int32_t ActionCharger::WriteSimValue(int32_t simValue)
{
    int32_t ret = -1;
    char buf[MAX_PATH] = {0};
    if (protocol_ == SC_PROTOCOL) {
        ret = snprintf_s(buf, MAX_PATH, sizeof(buf) - 1, SC_CURRENT_PATH);
        if (ret < EOK) {
            return ret;
        }
    } else if (protocol_ == BUCK_PROTOCOL) {
        ret = snprintf_s(buf, MAX_PATH, sizeof(buf) - 1, BUCK_CURRENT_PATH);
        if (ret < EOK) {
            return ret;
        }
    }
    std::string valueString = std::to_string(simValue) + "\n";
    ret = FileOperation::WriteFile(buf, valueString, valueString.length());
    if (ret != ERR_OK) {
        return ret;
    }
    return ERR_OK;
}
} // namespace PowerMgr
} // namespace OHOS
