/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef THERMAL_MGR_SYSTEM_TEST
#define THERMAL_MGR_SYSTEM_TEST

#include <gtest/gtest.h>
#include <string>

namespace OHOS {
namespace PowerMgr {
const int32_t BUFFER_SIZE = 512;
const uint32_t MAX_PATH = 256;
const uint32_t SLEEP_INTERVAL_SEC = 5;
const std::string AIRPLANE_PATH = "/data/service/el0/thermal/config/airplane";
const std::string BATTERY_PATH = "/data/service/el0/thermal/sensor/battery/temp";
const std::string SHELL_PATH = "/data/service/el0/thermal/sensor/shell/temp";
const std::string CHARGER_PATH = "/data/service/el0/thermal/sensor/charger/temp";
const std::string SOC_PATH = "/data/service/el0/thermal/sensor/soc/temp";
const std::string AMBIENT_PATH = "/data/service/el0/thermal/sensor/ambient/temp";
const std::string CPU_PATH = "/data/service/el0/thermal/sensor/cpu/temp";
const std::string PA_PATH = "/data/service/el0/thermal/sensor/pa/temp";
const std::string AP_PATH = "/data/service/el0/thermal/sensor/ap/temp";
const std::string CONFIG_LEVEL_PATH = "/data/service/el0/thermal/config/configLevel";
const std::string LCD_PATH = "/data/service/el0/thermal/config/lcd";
const std::string PROCESS_PATH = "/data/service/el0/thermal/config/process_ctrl";
const std::string SHUTDOWN_PATH = "/data/service/el0/thermal/config/shut_down";
const std::string STATE_SCREEN_PATH = "/data/service/el0/thermal/state/screen";
const std::string STATE_SCENE_PATH = "/data/service/el0/thermal/state/scene";
const std::string STATE_CHARGE_PATH = "/data/service/el0/thermal/state/charge";
const std::string VENDOR_CONFIG = "/vendor/etc/thermal_config/thermal_service_config.xml";
const std::string SIMUL_CPU_FREQ_PATH = "/data/service/el0/thermal/cooling/cpu/freq";
const std::string REAL_CPU_FREQ_PATH = "/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq";
const std::string GPU_FREQ_PATH = "/data/service/el0/thermal/cooling/gpu/freq";
const std::string SIMUL_BATTERY_CHARGER_CURRENT_PATH = "/data/service/el0/thermal/cooling/battery/current";
const std::string REAL_BATTERY_CHARGER_CURRENT_PATH = "/data/service/el0/battery/current_limit";
const std::string SIMULATION_TEMP_DIR = "/data/service/el0/thermal/sensor/%s/temp";
class ThermalMgrSystemTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    static int32_t WriteFile(std::string path, std::string buf, size_t size);
    static int32_t ReadFile(const char *path, char *buf, size_t size);
    static int32_t ConvertInt(const std::string &value);
    static int32_t InitNode();
};
} // namespace PowerMgr
} // namespace OHOS
#endif // THERMAL_MGR_SYSTEM_TEST