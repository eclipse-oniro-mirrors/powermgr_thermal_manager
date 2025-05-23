/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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

#include "cust_state_collection.h"
#include "constants.h"
#include "string_operation.h"
#include "thermal_service.h"

namespace OHOS {
namespace PowerMgr {
namespace {
}
bool CustStateCollection::Init()
{
    return true;
}

bool CustStateCollection::InitParam(std::string& params)
{
    return true;
}

std::string CustStateCollection::GetState()
{
    return ToString(state_);
}

void CustStateCollection::SetState(const std::string& stateValue)
{
    unsigned long result = 0;
    if (!StringOperation::ParseStrtoulResult(stateValue, result)) {
        return;
    }
    state_ = result;
    THERMAL_HILOGI(COMP_SVC, "cust state has set to %{public}s", stateValue.c_str());
}

bool CustStateCollection::DecideState(const std::string& value)
{
    unsigned long result = 0;
    if (!StringOperation::ParseStrtoulResult(value, result)) {
        return false;
    }
    if (state_ & result) {
        return true;
    } else {
        return false;
    }
}
} //namespace PowerMgr
} //namespace OHOS