/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef THERMAL_XCOLLIE_H
#define THERMAL_XCOLLIE_H

#include <functional>
#include <string>

namespace OHOS {
namespace PowerMgr {
class ThermalXCollie {
public:
    ThermalXCollie(const std::string &logTag, bool isRecovery = false);
    ~ThermalXCollie();

protected:
    void CancelThermalXCollie();

    int32_t id_;
    std::string logTag_;
    bool isCanceled_;
};

} // namespace PowerMgr
} // namespace OHOS

#endif // THERMAL_XCOLLIE_H