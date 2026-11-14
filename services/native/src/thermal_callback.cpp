/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "thermal_callback.h"

#include "hdf_base.h"

namespace OHOS {
namespace PowerMgr {
ThermalCallback::ThermalEventCallback ThermalCallback::eventCb_ = nullptr;
std::mutex ThermalCallback::eventMutex_;
int32_t ThermalCallback::OnThermalDataEvent(const HdfThermalCallbackInfo& event)
{
    std::lock_guard<std::mutex> lock(eventMutex_);
    if (eventCb_ == nullptr) {
        return HDF_FAILURE;
    }
    return eventCb_(event);
}

int32_t ThermalCallback::RegisterThermalEvent(const ThermalEventCallback &eventCb)
{
    std::lock_guard<std::mutex> lock(eventMutex_);
    eventCb_ = eventCb;
    return HDF_SUCCESS;
}
} // OHOS
} // PowerMgr