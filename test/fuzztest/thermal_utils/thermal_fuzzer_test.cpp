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

/* This files contains faultlog fuzzer test modules. */

#include "thermal_fuzzer_test.h"

#include <cstddef>
#include <cstdint>

#include "message_parcel.h"
#include "securec.h"

#include "thermal_policy.h"
#include "thermal_config_sensor_cluster.h"

using namespace OHOS::PowerMgr;
using namespace OHOS;

namespace {
const int32_t REWIND_READ_DATA = 0;
} // namespace

auto g_service = ThermalService::GetInstance();

ThermalFuzzerTest::ThermalFuzzerTest()
{
    g_service->OnStart();
}

ThermalFuzzerTest::~ThermalFuzzerTest()
{
    if (g_service != nullptr) {
        g_service->OnStop();
    }
    g_service = nullptr;
}

void ThermalFuzzerTest::TestThermalPolicy()
{
    ThermalPolicy policy;
    std::string result = "fuzz_test";
    const TypeTempMap& info = {{"cpu", 30}, {"gpu", 45}};
    policy.OnSensorInfoReported(info);
    policy.DumpLevel(result);
    policy.DumpPolicy(result);
}

void ThermalFuzzerTest::TestThermalServiceStub(const uint32_t code, const uint8_t* data, size_t size)
{
    MessageParcel datas;
    datas.WriteInterfaceToken(ThermalService::GetDescriptor());
    datas.WriteBuffer(data, size);
    datas.RewindRead(REWIND_READ_DATA);
    MessageParcel reply;
    MessageOption option;
    g_service->OnRemoteRequest(code, datas, reply, option);
}
