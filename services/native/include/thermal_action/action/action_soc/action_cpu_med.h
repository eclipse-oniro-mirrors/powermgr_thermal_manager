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

#ifndef ACTION_CPU_MED_H
#define ACTION_CPU_MED_H

#include "soc_action_base.h"

namespace OHOS {
namespace PowerMgr {
class ActionCpuMed : public IThermalAction, public SocActionBase {
public:
    ActionCpuMed(const std::string& actionName);
    ~ActionCpuMed() = default;

    void InitParams(const std::string& params) override;
    void SetStrict(bool enable) override;
    void SetEnableEvent(bool enable) override;
    void AddActionValue(uint32_t actionId, std::string value) override;
    void ExecuteInner() override;
    void ResetActionValue() override;

private:
    uint32_t GetActionValue();
    uint32_t lastValue_ {0};
    std::vector<uint32_t> valueList_;
};
} // namespace PowerMgr
} // namespace OHOS
#endif // ACTION_CPU_MED_H
