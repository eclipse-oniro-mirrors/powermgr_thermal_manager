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

#include "action_thermal_level.h"

#include <common_event_data.h>
#include <common_event_manager.h>
#include <common_event_support.h>
#include <map>

#include "constants.h"
#include "thermal_hisysevent.h"
#include "thermal_service.h"
#include "thermal_common.h"
#include "ffrt_utils.h"

using namespace OHOS::AAFwk;
using namespace OHOS::EventFwk;
using namespace OHOS::HiviewDFX;

namespace OHOS {
namespace PowerMgr {
namespace {
constexpr int32_t MIN_THERMAL_LEVEL = static_cast<int32_t>(ThermalLevel::COOL);
constexpr int32_t MAX_THERMAL_LEVEL = static_cast<int32_t>(ThermalLevel::ESCAPE);
}
int32_t ActionThermalLevel::lastValue_ = INT_MIN;

ActionThermalLevel::ActionThermalLevel(const std::string& actionName)
{
    actionName_ = actionName;
}

void ActionThermalLevel::InitParams(const std::string& params)
{
    if (thermalLevelCBDeathRecipient_ == nullptr) {
        thermalLevelCBDeathRecipient_ = new ThermalLevelCallbackDeathRecipient();
    }
}

void ActionThermalLevel::SetStrict(bool enable)
{
    isStrict_ = enable;
}

void ActionThermalLevel::SetEnableEvent(bool enable)
{
    enableEvent_ = enable;
}

void ActionThermalLevel::AddActionValue(uint32_t actionId, std::string value)
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

void ActionThermalLevel::ExecuteInner()
{
    auto tms = ThermalService::GetInstance();
    THERMAL_RETURN_IF (tms == nullptr);
    for (auto &policyAction : policyActionMap_) {
        if (policyAction.second.isCompleted) {
            valueList_.push_back(policyAction.second.uintDelayValue);
        }
    }

    int32_t value = GetActionValue();
    if (value != lastValue_) {
        THERMAL_HILOGI(COMP_SVC, "new lvl:%{public}d,old lvl:%{public}d", value, lastValue_);
        lastValue_ = value;
        LevelRequest(value);
        tms->GetObserver()->SetDecisionValue(actionName_, std::to_string(value));
        THERMAL_HILOGD(COMP_SVC, "action execute: {%{public}s = %{public}u}", actionName_.c_str(), lastValue_);
    }
    valueList_.clear();
}

void ActionThermalLevel::ResetActionValue()
{
    lastValue_ = INT_MIN;
}

int32_t ActionThermalLevel::GetActionValue()
{
    uint32_t value = MIN_THERMAL_LEVEL;
    if (!valueList_.empty()) {
        if (isStrict_) {
            value = *min_element(valueList_.begin(), valueList_.end());
        } else {
            value = *max_element(valueList_.begin(), valueList_.end());
        }
    }
    return value;
}

int32_t ActionThermalLevel::GetThermalLevel()
{
    return lastValue_;
}

void ActionThermalLevel::LevelRequest(int32_t level)
{
    if (level > MAX_THERMAL_LEVEL) {
        level = MAX_THERMAL_LEVEL;
    }
    if (level < MIN_THERMAL_LEVEL) {
        level = MIN_THERMAL_LEVEL;
    }
    NotifyThermalLevelChanged(level);
}

void ActionThermalLevel::SubscribeThermalLevelCallback(const sptr<IThermalLevelCallback>& callback, bool isSync)
{
    THERMAL_RETURN_IF(callback == nullptr);
    if (isSync) {
        if (AddThermalLevelListeners(callback, true)) {
            callback->OnThermalLevelChanged(static_cast<ThermalLevel>(lastValue_));
        }
    } else {
        if (AddThermalLevelListeners(callback, false)) {
            callback->OnAsyncThermalLevelChanged(static_cast<ThermalLevel>(lastValue_));
        }
    }
}

void ActionThermalLevel::UnSubscribeThermalLevelCallback(const sptr<IThermalLevelCallback>& callback)
{
    std::lock_guard lock(mutex_);
    THERMAL_RETURN_IF(callback == nullptr);
    auto object = callback->AsObject();
    THERMAL_RETURN_IF(object == nullptr);

    size_t eraseNum = 0;
    if (syncThermalLevelListeners_.find(callback) != syncThermalLevelListeners_.end()) {
        eraseNum = syncThermalLevelListeners_.erase(callback);
        THERMAL_HILOGI(COMP_SVC, "sync size=%{public}zu,eraseNum=%{public}zu",
            syncThermalLevelListeners_.size(), eraseNum);
    }
    if (asyncThermalLevelListeners_.find(callback) != asyncThermalLevelListeners_.end()) {
        eraseNum = asyncThermalLevelListeners_.erase(callback);
        THERMAL_HILOGI(COMP_SVC, "async size=%{public}zu,eraseNum=%{public}zu",
            asyncThermalLevelListeners_.size(), eraseNum);
    }
    if (eraseNum != 0) {
        object->RemoveDeathRecipient(thermalLevelCBDeathRecipient_);
    }
}

void ActionThermalLevel::ThermalLevelCallbackDeathRecipient::OnRemoteDied(const wptr<IRemoteObject>& remote)
{
    if (remote == nullptr || remote.promote() == nullptr) {
        return;
    }
    auto tms = ThermalService::GetInstance();
    if (tms == nullptr) {
        return;
    }
    sptr<IThermalLevelCallback> callback = iface_cast<IThermalLevelCallback>(remote.promote());
    FFRTTask task = [tms, callback] { tms->UnSubscribeThermalLevelCallback(callback); };
    FFRTUtils::SubmitTask(task);
}

void ActionThermalLevel::NotifyThermalLevelChanged(int32_t level)
{
    // Send Notification event
    PublishLevelChangedEvents(ThermalCommonEventCode::CODE_THERMAL_LEVEL_CHANGED, level);

    // Call back all level listeners
    auto asyncThermalLevelListeners = GetThermalLevelListeners(false);
    for (auto& listener : asyncThermalLevelListeners) {
        listener->OnAsyncThermalLevelChanged(static_cast<ThermalLevel>(level));
    }
    auto syncThermalLevelListeners = GetThermalLevelListeners(true);
    for (auto& listener : syncThermalLevelListeners) {
        listener->OnThermalLevelChanged(static_cast<ThermalLevel>(level));
    }
}

bool ActionThermalLevel::PublishLevelChangedEvents(ThermalCommonEventCode code, int32_t level)
{
    Want want;
    want.SetParam(ToString(static_cast<int32_t>(code)), level);
    want.SetAction(CommonEventSupport::COMMON_EVENT_THERMAL_LEVEL_CHANGED);
    CommonEventData commonData;
    commonData.SetWant(want);
    CommonEventPublishInfo publishInfo;
    publishInfo.SetOrdered(false);
    if (!CommonEventManager::PublishCommonEvent(commonData, publishInfo)) {
        THERMAL_HILOGE(COMP_SVC, "failed to publish thermal level change event");
        return false;
    }
    return true;
}

bool ActionThermalLevel::AddThermalLevelListeners(const sptr<IThermalLevelCallback>& callback, bool isSync)
{
    THERMAL_RETURN_IF_WITH_RET(callback == nullptr, false);
    auto object = callback->AsObject();
    THERMAL_RETURN_IF_WITH_RET(object == nullptr, false);
    if (isSync) {
        std::lock_guard lock(mutex_);
        auto retIt = syncThermalLevelListeners_.insert(callback);
        if (retIt.second) {
            object->AddDeathRecipient(thermalLevelCBDeathRecipient_);
        }
        THERMAL_HILOGI(COMP_SVC, "sync size=%{public}zu,insertOk=%{public}d",
            syncThermalLevelListeners_.size(), retIt.second);
        return retIt.second;
    } else {
        std::lock_guard lock(mutex_);
        auto retIt = asyncThermalLevelListeners_.insert(callback);
        if (retIt.second) {
            object->AddDeathRecipient(thermalLevelCBDeathRecipient_);
        }
        THERMAL_HILOGI(COMP_SVC, "async size=%{public}zu,insertOk=%{public}d",
            asyncThermalLevelListeners_.size(), retIt.second);
        return retIt.second;
    }
}

auto ActionThermalLevel::GetThermalLevelListeners(bool isSync)
    -> std::set<const sptr<IThermalLevelCallback>, classcomp>
{
    std::lock_guard lock(mutex_);
    if (isSync) {
        return syncThermalLevelListeners_;
    } else {
        return asyncThermalLevelListeners_;
    }
}
} // namespace PowerMgr
} // namespace OHOS
