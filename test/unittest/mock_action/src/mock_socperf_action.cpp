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

#include "mock_socperf_action.h"

namespace OHOS {
namespace PowerMgr {
bool MockSocPerfAction::tag_ = false;
uint32_t MockSocPerfAction::counter_ = 0;

void MockSocPerfAction::BoostRequest()
{
    counter_++;
}

uint32_t MockSocPerfAction::GetBoostRequestCounter()
{
    return counter_;
}

void MockSocPerfAction::ClearBoost()
{
    counter_ = 0;
}
} // namespace Power
} // namespace OHOS
