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

#define FUZZ_PROJECT_NAME "thermallevelcallbackstub_fuzzer"

#include <cstddef>
#include <cstdint>

#include "message_parcel.h"
#include "message_option.h"
#include "securec.h"
#include "thermal_level_callback_stub.h"
#include "thermal_level_callback_ipc_interface_code.h"

using namespace OHOS::PowerMgr;
using namespace OHOS;

namespace {
const int32_t REWIND_READ_DATA = 0;
}

class ThermalLevelCallbackStubTest : public ThermalLevelCallbackStub {
public:
    ThermalLevelCallbackStubTest() = default;
    virtual ~ThermalLevelCallbackStubTest() = default;
    bool OnThermalLevelChanged(ThermalLevel level) override
    {
        return true;
    }
};

bool DoSomethingInterestingWithMyAPI(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return false;
    }

    sptr<ThermalLevelCallbackStubTest> stub = new ThermalLevelCallbackStubTest();
    if (stub == nullptr) {
        return false;
    }

    MessageParcel datas;
    datas.WriteInterfaceToken(ThermalLevelCallbackStub::GetDescriptor());
    datas.WriteBuffer(data, size);
    datas.RewindRead(REWIND_READ_DATA);
    MessageParcel reply;
    MessageOption option;

    stub->OnRemoteRequest(
        static_cast<uint32_t>(ThermalLevelCallbackInterfaceCode::THERMAL_LEVEL_CHANGED),
        datas, reply, option);

    MessageParcel datas2;
    datas2.WriteInterfaceToken(ThermalLevelCallbackStub::GetDescriptor());
    if (size >= sizeof(int32_t)) {
        int32_t level = *(reinterpret_cast<const int32_t*>(data));
        datas2.WriteInt32(level);
    }
    datas2.RewindRead(REWIND_READ_DATA);
    MessageParcel reply2;
    stub->OnRemoteRequest(
        static_cast<uint32_t>(ThermalLevelCallbackInterfaceCode::THERMAL_LEVEL_CHANGED),
        datas2, reply2, option);

    return true;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    DoSomethingInterestingWithMyAPI(data, size);
    return 0;
}
