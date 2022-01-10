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
#include "sensor_info_config.h"

namespace OHOS {
namespace HDI {
namespace THERMAL {
namespace V1_0 {
void SensorInfoConfig::SetXMLThermalZoneInfo(std::vector<XMLThermalZoneInfo> &vXmlTz)
{
    vXmlTz_ = vXmlTz;
}

void SensorInfoConfig::SetXMLThermalNodeInfo(std::vector<XMLThermalNodeInfo> &vXmlTn)
{
    vXmlTn_ = vXmlTn;
}

std::vector<XMLThermalZoneInfo> SensorInfoConfig::GetXMLThermalZoneInfo()
{
    return vXmlTz_;
}

std::vector<XMLThermalNodeInfo> SensorInfoConfig::GetXMLThermalNodeInfo()
{
    return vXmlTn_;
}
} // V1_0
} // THERMAL
} // HDI
} // OHOS