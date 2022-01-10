/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef BASE_INFO_CONFIG_H
#define BASE_INFO_CONFIG_H

#include <string>
#include <vector>

namespace OHOS {
namespace HDI {
namespace THERMAL {
namespace V1_0 {
struct BaseItem {
    std::string tag;
    std::string value;
};

class BaseInfoConfig {
public:
    BaseInfoConfig() = default;
    ~BaseInfoConfig() = default;
    void SetBase(std::vector<BaseItem> &vBase);
    void Dump();
private:
    std::vector<BaseItem> vBase_;
};
} // V1_0
} // THERMAL
} // HDI
} // OHOS
#endif // BASE_INFO_CONFIG_H