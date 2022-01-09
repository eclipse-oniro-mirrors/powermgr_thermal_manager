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

#ifndef THERMAL_SIMULATION_NODE_H
#define THERMAL_SIMULATION_NODE_H

#include <string>
#include <vector>
#include <mutex>
#include <map>

namespace OHOS {
namespace HDI {
namespace THERMAL {
namespace V1_0 {
class ThermalSimulationNode {
public:
    ThermalSimulationNode() {};
    ~ThermalSimulationNode() {};

    int32_t NodeInit();
    int32_t CreateNodeDir(std::string dir);
    int32_t CreateNodeFile(std::string filePath);
    int32_t AddSensorTypeTemp();
    int32_t AddMitigationDevice();
    int32_t WriteFile(std::string path, std::string buf, size_t size);
    int32_t SetTempRequest(std::string type, int32_t temp);
private:
    std::mutex mutex_;
    std::vector<std::string> tempPath_;
};
} // namespace V1_0
} // namaespace THERMAL
} // namespace HDI
} // namespace OHOS
#endif // THERMAL_SIMULATION_NODE_H