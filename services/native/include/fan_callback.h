/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef FAN_CALLBACK_H
#define FAN_CALLBACK_H

#include <functional>
#include <mutex>
#include "v1_1/ifan_callback.h"
#include "v1_1/thermal_types.h"

namespace OHOS {
namespace PowerMgr {
using namespace OHOS::HDI::Thermal::V1_1;
class FanCallback : public IFanCallback {
public:
    virtual ~FanCallback() {}
    using FanEventCallback = std::function<int32_t(const HdfThermalCallbackInfo& event)>;
    static int32_t RegisterFanEvent(const FanEventCallback &eventCb);
    int32_t OnFanDataEvent(const HdfThermalCallbackInfo& event) override;
private:
    static FanEventCallback eventCb_;
    static std::mutex eventMutex_;
};
} // OHOS
} // PowerMgr
#endif // FAN_CALLBACK_H
