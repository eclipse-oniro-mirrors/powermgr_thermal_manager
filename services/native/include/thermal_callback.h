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

#ifndef POWERMGR_THERMAL_MANAGER_THERMAL_CALLBACK_H
#define POWERMGR_THERMAL_MANAGER_THERMAL_CALLBACK_H

#include <functional>
#include <mutex>
#include "v1_1/ithermal_callback.h"
#include "v1_1/thermal_types.h"

namespace OHOS {
namespace PowerMgr {
using namespace OHOS::HDI::Thermal::V1_1;
class ThermalCallback : public IThermalCallback {
public:
    virtual ~ThermalCallback() {}
    using ThermalEventCallback = std::function<int32_t(const HdfThermalCallbackInfo& event)>;
    static int32_t RegisterThermalEvent(const ThermalEventCallback &eventCb);
    int32_t OnThermalDataEvent(const HdfThermalCallbackInfo& event) override;
private:
    static ThermalEventCallback eventCb_;
    static std::mutex eventMutex_;
};
} // OHOS
} // PowerMgr
#endif // POWERMGR_THERMAL_MANAGER_THERMAL_CALLBACK_H
