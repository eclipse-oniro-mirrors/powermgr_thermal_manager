/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#include "action_voltage.h"

#include <map>
#ifdef DRIVERS_INTERFACE_BATTERY_ENABLE
#include "v2_0/ibattery_interface.h"
#endif
#include "thermal_hisysevent.h"
#include "thermal_service.h"
#include "file_operation.h"
#include "securec.h"
#include "constants.h"

namespace OHOS {
namespace PowerMgr {
namespace {
constexpr const char* SC_VOLTAGE_PATH = "/data/service/el0/thermal/config/sc_voltage";
constexpr const char* BUCK_VOLTAGE_PATH = "/data/service/el0/thermal/config/buck_voltage";
const int32_t MAX_PATH = 256;
}

#ifdef DRIVERS_INTERFACE_BATTERY_ENABLE
std::vector<ChargingLimit> ActionVoltage::chargeLimitList_;
#endif

ActionVoltage::ActionVoltage(const std::string& actionName)
{
    actionName_ = actionName;
}

void ActionVoltage::InitParams(const std::string& protocol)
{
    protocol_ = protocol;
}

void ActionVoltage::SetStrict(bool enable)
{
    isStrict_ = enable;
}

void ActionVoltage::SetEnableEvent(bool enable)
{
    enableEvent_ = enable;
}

void ActionVoltage::AddActionValue(uint32_t actionId, std::string value)
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

void ActionVoltage::ExecuteInner()
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
        SetVoltage(value);
        WriteMockNode(value);
        WriteActionTriggeredHiSysEvent(enableEvent_, actionName_, value);
        tms->GetObserver()->SetDecisionValue(actionName_, std::to_string(value));
        lastValue_ = value;
        THERMAL_HILOGD(COMP_SVC, "action execute: {%{public}s = %{public}u}", actionName_.c_str(), lastValue_);
    }
    valueList_.clear();
}

void ActionVoltage::ResetActionValue()
{
    lastValue_ = 0;
}

uint32_t ActionVoltage::GetActionValue()
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

int32_t ActionVoltage::SetVoltage(int32_t voltage)
{
#ifdef DRIVERS_INTERFACE_BATTERY_ENABLE
    sptr<IBatteryInterface> iBatteryInterface = IBatteryInterface::Get();
    if (iBatteryInterface == nullptr) {
        THERMAL_HILOGE(COMP_SVC, "iBatteryInterface_ is nullptr");
        return ERR_FLATTEN_OBJECT;
    }
    ChargingLimit chargingLimit;
    chargingLimit.type = TYPE_VOLTAGE;
    chargingLimit.protocol = protocol_;
    chargingLimit.value = voltage;
    chargeLimitList_.push_back(chargingLimit);
#endif
    return ERR_OK;
}

void ActionVoltage::ExecuteVoltageLimit()
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

int32_t ActionVoltage::WriteMockNode(int32_t mockValue)
{
    int32_t ret = -1;
    char buf[MAX_PATH] = {0};
    if (protocol_ == SC_PROTOCOL) {
        ret = snprintf_s(buf, MAX_PATH, sizeof(buf) - 1, SC_VOLTAGE_PATH);
        if (ret < EOK) {
            return ret;
        }
    } else if (protocol_ == BUCK_PROTOCOL) {
        ret = snprintf_s(buf, MAX_PATH, sizeof(buf) - 1, BUCK_VOLTAGE_PATH);
        if (ret < EOK) {
            return ret;
        }
    }

    std::string valueString = std::to_string(mockValue) + "\n";
    ret = FileOperation::WriteFile(buf, valueString, valueString.length());
    if (ret != ERR_OK) {
        return ret;
    }
    return ERR_OK;
}
} // namespace PowerMgr
} // namespace OHOS
