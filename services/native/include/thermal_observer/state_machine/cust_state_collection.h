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

#ifndef CUST_STATE_COLLECTION_H
#define CUST_STATE_COLLECTION_H

#include "istate_collection.h"

namespace OHOS {
namespace PowerMgr {
class CustStateCollection : public IStateCollection {
public:
    CustStateCollection() = default;
    ~CustStateCollection() = default;
    bool Init() override;
    bool InitParam(std::string& params) override;
    std::string GetState() override;
    virtual void SetState(const std::string& stateValue) override;
    virtual bool DecideState(const std::string& value) override;

private:
    unsigned long state_ = 0;
};
} // namespace PowerMgr
} // namespace OHOS
#endif // CUST_STATE_COLLECTION_H