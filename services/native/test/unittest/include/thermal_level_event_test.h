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

#ifndef THERMAL_LEVEL_EVENT_TEST
#define THERMAL_LEVEL_EVENT_TEST

#include <gtest/gtest.h>
#include "thermal_service.h"

namespace OHOS {
namespace PowerMgr {
namespace {
const uint32_t MAX_PATH = 256;
const uint32_t WAIT_TIME_5_SEC = 5;
std::string batteryPath = "/data/service/el0/thermal/sensor/battery/temp";
std::string shellPath = "/data/service/el0/thermal/sensor/shell/temp";
std::string chargerPath = "/data/service/el0/thermal/sensor/charger/temp";
std::string socPath = "/data/service/el0/thermal/sensor/soc/temp";
std::string ambientPath = "/data/service/el0/thermal/sensor/ambient/temp";
std::string cpuPath = "/data/service/el0/thermal/sensor/cpu/temp";
std::string paPath = "/data/service/el0/thermal/sensor/pa/temp";
std::string apPath = "/data/service/el0/thermal/sensor/ap/temp";
const std::string SIMULATION_TEMP_DIR = "/data/service/el0/thermal/sensor/%s/temp";
}
class ThermalLevelEventTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    int32_t InitNode();
};
} // namespace PowerMgr
} // namespace OHOS
#endif // THERMAL_LEVEL_EVENT_TEST