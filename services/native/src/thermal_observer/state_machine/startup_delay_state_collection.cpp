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

#include "startup_delay_state_collection.h"

#include "common_event_subscriber.h"
#include "common_event_data.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "file_operation.h"
#include "securec.h"
#include "string_ex.h"
#include "string_operation.h"
#include "thermal_service.h"
#include "thermal_common.h"

using namespace OHOS::EventFwk;
namespace OHOS {
namespace PowerMgr {

bool StartupDelayStateCollection::Init()
{
    THERMAL_HILOGD(COMP_SVC, "StartupDelayStateCollection Init...");
    return this->RegisterEvent();
}

bool StartupDelayStateCollection::InitParam(std::string& params)
{
    THERMAL_HILOGD(COMP_SVC, "Enter");
    params_ = params;
    return true;
}

std::string StartupDelayStateCollection::GetState()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return ToString(state_);
}

bool StartupDelayStateCollection::RegisterEvent()
{
    auto tms = ThermalService::GetInstance();
    if (tms == nullptr) {
        return false;
    }
    auto receiver = tms->GetStateMachineObj()->GetCommonEventReceiver();
    if (receiver == nullptr) {
        return false;
    }
 
    EventHandle handlerPowerOn =
        [this](const CommonEventData& data) { this->HandlerPowerOnState(data); };
    receiver->AddEvent(CommonEventSupport::COMMON_EVENT_BOOT_COMPLETED, handlerPowerOn);
 
    return true;
}

void StartupDelayStateCollection::HandlerPowerOnState(const CommonEventData& data __attribute__((__unused__)))
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (delayTimerId_ > 0) {
        StopDelayAction();
    }
    state_ = STARTUP_DELAY_STATE;
    StartDelayTimer();
}

bool StartupDelayStateCollection::StartDelayTimer()
{
    auto thermalTimer = std::make_shared<ThermalTimer>();
    auto timerInfo = std::make_shared<ThermalTimerInfo>();
    timerInfo->SetType(timerInfo->TIMER_TYPE_WAKEUP | timerInfo->TIMER_TYPE_EXACT);
    timerInfo->SetCallbackInfo([this] { ResetState(); });
    
    delayTimerId_ = thermalTimer->CreateTimer(timerInfo);
    int64_t curMsecTimestam = MiscServices::TimeServiceClient::GetInstance()->GetWallTimeMs();
 
    return thermalTimer->StartTimer(delayTimerId_, static_cast<uint64_t>(delayTime_ + curMsecTimestam));
}

void StartupDelayStateCollection::StopDelayAction()
{
    if (delayTimerId_ > 0) {
        auto thermalTimer = std::make_shared<ThermalTimer>();
        if (!thermalTimer->StopTimer(delayTimerId_)) {
            THERMAL_HILOGE(COMP_SVC, "failed to stop delay timer, timerId = %{public}llu", delayTimerId_);
        }
        thermalTimer->DestroyTimer(delayTimerId_);
        delayTimerId_ = 0;
    }
}

void StartupDelayStateCollection::SetState(const std::string& stateValue)
{
}

void StartupDelayStateCollection::ResetState()
{
    std::lock_guard<std::mutex> lock(mutex_);
    state_ = NON_STARTUP_DELAY_STATE;
    delayTimerId_ = 0;
}

bool StartupDelayStateCollection::InitDelayTime(std::string& delaytime)
{
    THERMAL_HILOGD(COMP_SVC, "init power on delay time info");
    delayTime_ = static_cast<uint32_t>(strtol(delaytime.c_str(), nullptr, STRTOL_FORMART_DEC));
    return true;
}

bool StartupDelayStateCollection::DecideState(const std::string& value)
{
    THERMAL_HILOGD(COMP_SVC, "Enter: Consider the impact of the power-on delay status.");
    std::lock_guard<std::mutex> lock(mutex_);
    if ((value == ToString(NON_STARTUP_DELAY_STATE) && state_ == NON_STARTUP_DELAY_STATE) ||
        (value == ToString(STARTUP_DELAY_STATE) && state_ == STARTUP_DELAY_STATE)) {
        THERMAL_HILOGD(COMP_SVC, "current power on delay state = %{public}d", state_);
        return true;
    }
    return false;
}
} // namespace PowerMgr
} // namespace OHOS