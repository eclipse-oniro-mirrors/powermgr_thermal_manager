/*
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#ifndef SIMPLE_FUZZER_TEST_H
#define SIMPLE_FUZZER_TEST_H

#define FUZZ_PROJECT_NAME "simple_fuzzer"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include "thermal_service.h"
#include "thermal_action_callback_stub.h"
#include "thermal_level_callback_stub.h"
#include "thermal_temp_callback_stub.h"
#include "ithermal_srv.h"
#include "thermal_srv_sensor_info.h"

namespace OHOS {
namespace PowerMgr {

enum class ThermalFuzzIpcCode : int32_t {
    SUBSCRIBE_THERMAL_TEMP = 0,
    UN_SUBSCRIBE_THERMAL_TEMP = 1,
    SUBSCRIBE_THERMAL_LEVEL = 2,
    UN_SUBSCRIBE_THERMAL_LEVEL = 3,
    SUBSCRIBE_THERMAL_ACTION = 4,
    UN_SUBSCRIBE_THERMAL_ACTION = 5,
    GET_THERMAL_SRV_SENSOR_INFO = 6,
    GET_THERMAL_LEVEL = 7,
    GET_THERMAL_INFO = 8,
    SET_SCENE = 9,
    UPDATE_THERMAL_STATE = 10,
    SHELL_DUMP = 11,
    MAX_CODE
};

class SimpleFuzzerTempCallback : public ThermalTempCallbackStub {
public:
    bool OnThermalTempChanged(TempCallbackMap& tempCbMap) override
    {
        (void)tempCbMap;
        return true;
    }
};

class SimpleFuzzerLevelCallback : public ThermalLevelCallbackStub {
public:
    bool OnThermalLevelChanged(ThermalLevel level) override
    {
        (void)level;
        return true;
    }
};

class SimpleFuzzerActionCallback : public ThermalActionCallbackStub {
public:
    bool OnThermalActionChanged(ActionCallbackMap& actionCbMap) override
    {
        (void)actionCbMap;
        return true;
    }
};

class SimpleFuzzer {
public:
    SimpleFuzzer();
    ~SimpleFuzzer();

    void FuzzThermalService(FuzzedDataProvider& provider);
    void FuzzServiceStub(uint32_t code, const uint8_t* data, size_t size);

private:
    void FuzzSubscribeThermalTempCallback(FuzzedDataProvider& provider);
    void FuzzUnSubscribeThermalTempCallback(FuzzedDataProvider& provider);
    void FuzzSubscribeThermalLevelCallback(FuzzedDataProvider& provider);
    void FuzzUnSubscribeThermalLevelCallback(FuzzedDataProvider& provider);
    void FuzzSubscribeThermalActionCallback(FuzzedDataProvider& provider);
    void FuzzUnSubscribeThermalActionCallback(FuzzedDataProvider& provider);
    void FuzzGetThermalSrvSensorInfo(FuzzedDataProvider& provider);
    void FuzzGetThermalLevel(FuzzedDataProvider& provider);
    void FuzzGetThermalInfo(FuzzedDataProvider& provider);
    void FuzzSetScene(FuzzedDataProvider& provider);
    void FuzzUpdateThermalState(FuzzedDataProvider& provider);
    void FuzzShellDump(FuzzedDataProvider& provider);

    std::vector<std::string> GenerateStringVector(FuzzedDataProvider& provider);
    std::string GenerateString(FuzzedDataProvider& provider);

    sptr<ThermalService> service_ {nullptr};
};

} // namespace PowerMgr
} // namespace OHOS

#endif // SIMPLE_FUZZER_TEST_H
