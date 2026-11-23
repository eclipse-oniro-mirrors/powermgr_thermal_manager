/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "thermal_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <fuzzer/FuzzedDataProvider.h>
#include <string>
#include <vector>
#include "thermal_service.h"
#include "thermal_action_callback_stub.h"
#include "thermal_level_callback_stub.h"
#include "thermal_temp_callback_stub.h"
#include "ithermal_srv.h"
#include "thermal_srv_sensor_info.h"
#undef private
#undef protected

#include "message_parcel.h"
#include "message_option.h"
#include "securec.h"

using namespace OHOS::PowerMgr;
using namespace OHOS;

namespace {
constexpr size_t MAX_STRING_LENGTH = 256;
constexpr size_t MAX_VECTOR_SIZE = 16;
constexpr int32_t REWIND_READ_DATA = 0;
} // namespace

namespace OHOS {
namespace PowerMgr {

ThermalFuzzer::ThermalFuzzer()
{
    service_ = ThermalService::GetInstance();
    if (service_ != nullptr) {
        service_->OnStart();
        service_->SetTempReportSwitch(false);
    }
}

ThermalFuzzer::~ThermalFuzzer()
{
    if (service_ != nullptr) {
        service_->OnStop();
    }
}

std::string ThermalFuzzer::GenerateString(FuzzedDataProvider& provider)
{
    size_t length = provider.ConsumeIntegralInRange<size_t>(0,
        std::min(MAX_STRING_LENGTH, provider.remaining_bytes()));
    return provider.ConsumeRandomLengthString(length);
}

std::vector<std::string> ThermalFuzzer::GenerateStringVector(FuzzedDataProvider& provider)
{
    std::vector<std::string> result;
    size_t size = provider.ConsumeIntegralInRange<size_t>(0,
        std::min(MAX_VECTOR_SIZE, provider.remaining_bytes() / sizeof(char)));
    for (size_t i = 0; i < size && provider.remaining_bytes() > 0; ++i) {
        result.push_back(GenerateString(provider));
    }
    return result;
}

void ThermalFuzzer::FuzzSubscribeThermalTempCallback(FuzzedDataProvider& provider)
{
    if (service_ == nullptr) {
        return;
    }
    std::vector<std::string> typeList = GenerateStringVector(provider);
    sptr<IThermalTempCallback> callback = sptr<ThermalFuzzerTempCallback>::MakeSptr();
    if (callback == nullptr) {
        return;
    }
    service_->SubscribeThermalTempCallback(typeList, callback);
    service_->UnSubscribeThermalTempCallback(callback);
}

void ThermalFuzzer::FuzzUnSubscribeThermalTempCallback(FuzzedDataProvider& provider)
{
    (void)provider;
    if (service_ == nullptr) {
        return;
    }
    sptr<IThermalTempCallback> callback = sptr<ThermalFuzzerTempCallback>::MakeSptr();
    if (callback == nullptr) {
        return;
    }
    service_->UnSubscribeThermalTempCallback(callback);
}

void ThermalFuzzer::FuzzSubscribeThermalLevelCallback(FuzzedDataProvider& provider)
{
    if (service_ == nullptr) {
        return;
    }
    sptr<IThermalLevelCallback> callback = sptr<ThermalFuzzerLevelCallback>::MakeSptr();
    if (callback == nullptr) {
        return;
    }
    service_->SubscribeThermalLevelCallback(callback, provider.ConsumeBool());
    service_->UnSubscribeThermalLevelCallback(callback);
}

void ThermalFuzzer::FuzzUnSubscribeThermalLevelCallback(FuzzedDataProvider& provider)
{
    (void)provider;
    if (service_ == nullptr) {
        return;
    }
    sptr<IThermalLevelCallback> callback = sptr<ThermalFuzzerLevelCallback>::MakeSptr();
    if (callback == nullptr) {
        return;
    }
    service_->UnSubscribeThermalLevelCallback(callback);
}

void ThermalFuzzer::FuzzSubscribeThermalActionCallback(FuzzedDataProvider& provider)
{
    if (service_ == nullptr) {
        return;
    }
    std::vector<std::string> actionList = GenerateStringVector(provider);
    std::string desc = GenerateString(provider);
    sptr<IThermalActionCallback> callback = sptr<ThermalFuzzerActionCallback>::MakeSptr();
    if (callback == nullptr) {
        return;
    }
    service_->SubscribeThermalActionCallback(actionList, desc, callback);
    service_->UnSubscribeThermalActionCallback(callback);
}

void ThermalFuzzer::FuzzUnSubscribeThermalActionCallback(FuzzedDataProvider& provider)
{
    (void)provider;
    if (service_ == nullptr) {
        return;
    }
    sptr<IThermalActionCallback> callback = sptr<ThermalFuzzerActionCallback>::MakeSptr();
    if (callback == nullptr) {
        return;
    }
    service_->UnSubscribeThermalActionCallback(callback);
}

void ThermalFuzzer::FuzzGetThermalSrvSensorInfo(FuzzedDataProvider& provider)
{
    if (service_ == nullptr) {
        return;
    }
    const auto type = static_cast<int32_t>(provider.ConsumeIntegral<uint8_t>());
    ThermalSrvSensorInfo sensorInfo;
    bool sensorInfoRet = false;
    service_->GetThermalSrvSensorInfo(type, sensorInfo, sensorInfoRet);
}

void ThermalFuzzer::FuzzGetThermalLevel(FuzzedDataProvider& provider)
{
    (void)provider;
    if (service_ == nullptr) {
        return;
    }
    int32_t level = 0;
    service_->GetThermalLevel(level);
}

void ThermalFuzzer::FuzzGetThermalInfo(FuzzedDataProvider& provider)
{
    (void)provider;
    if (service_ == nullptr) {
        return;
    }
    service_->GetThermalInfo();
}

void ThermalFuzzer::FuzzSetScene(FuzzedDataProvider& provider)
{
    if (service_ == nullptr) {
        return;
    }
    std::string scene = GenerateString(provider);
    service_->SetScene(scene);
}

void ThermalFuzzer::FuzzUpdateThermalState(FuzzedDataProvider& provider)
{
    if (service_ == nullptr) {
        return;
    }
    std::string tag = GenerateString(provider);
    std::string val = GenerateString(provider);
    bool isImmed = provider.ConsumeBool();
    service_->UpdateThermalState(tag, val, isImmed);
}

void ThermalFuzzer::FuzzShellDump(FuzzedDataProvider& provider)
{
    if (service_ == nullptr) {
        return;
    }
    std::vector<std::string> args = GenerateStringVector(provider);
    const auto argc = static_cast<uint32_t>(args.size());
    std::string result;
    service_->ShellDump(args, argc, result);
}

namespace {
int32_t GetRandomIpcCode(FuzzedDataProvider& provider)
{
    static const int32_t ipcCodes[] = {
        static_cast<int32_t>(ThermalFuzzIpcCode::SUBSCRIBE_THERMAL_TEMP),
        static_cast<int32_t>(ThermalFuzzIpcCode::UN_SUBSCRIBE_THERMAL_TEMP),
        static_cast<int32_t>(ThermalFuzzIpcCode::SUBSCRIBE_THERMAL_LEVEL),
        static_cast<int32_t>(ThermalFuzzIpcCode::UN_SUBSCRIBE_THERMAL_LEVEL),
        static_cast<int32_t>(ThermalFuzzIpcCode::SUBSCRIBE_THERMAL_ACTION),
        static_cast<int32_t>(ThermalFuzzIpcCode::UN_SUBSCRIBE_THERMAL_ACTION),
        static_cast<int32_t>(ThermalFuzzIpcCode::GET_THERMAL_SRV_SENSOR_INFO),
        static_cast<int32_t>(ThermalFuzzIpcCode::GET_THERMAL_LEVEL),
        static_cast<int32_t>(ThermalFuzzIpcCode::GET_THERMAL_INFO),
        static_cast<int32_t>(ThermalFuzzIpcCode::SET_SCENE),
        static_cast<int32_t>(ThermalFuzzIpcCode::UPDATE_THERMAL_STATE),
        static_cast<int32_t>(ThermalFuzzIpcCode::SHELL_DUMP),
    };
    return provider.PickValueInArray(ipcCodes);
}
} // namespace

void ThermalFuzzer::FuzzThermalService(FuzzedDataProvider& provider)
{
    if (service_ == nullptr) {
        return;
    }

    int32_t code = GetRandomIpcCode(provider);
    switch (static_cast<ThermalFuzzIpcCode>(code)) {
        case ThermalFuzzIpcCode::SUBSCRIBE_THERMAL_TEMP:
            FuzzSubscribeThermalTempCallback(provider);
            break;
        case ThermalFuzzIpcCode::UN_SUBSCRIBE_THERMAL_TEMP:
            FuzzUnSubscribeThermalTempCallback(provider);
            break;
        case ThermalFuzzIpcCode::SUBSCRIBE_THERMAL_LEVEL:
            FuzzSubscribeThermalLevelCallback(provider);
            break;
        case ThermalFuzzIpcCode::UN_SUBSCRIBE_THERMAL_LEVEL:
            FuzzUnSubscribeThermalLevelCallback(provider);
            break;
        case ThermalFuzzIpcCode::SUBSCRIBE_THERMAL_ACTION:
            FuzzSubscribeThermalActionCallback(provider);
            break;
        case ThermalFuzzIpcCode::UN_SUBSCRIBE_THERMAL_ACTION:
            FuzzUnSubscribeThermalActionCallback(provider);
            break;
        case ThermalFuzzIpcCode::GET_THERMAL_SRV_SENSOR_INFO:
            FuzzGetThermalSrvSensorInfo(provider);
            break;
        case ThermalFuzzIpcCode::GET_THERMAL_LEVEL:
            FuzzGetThermalLevel(provider);
            break;
        case ThermalFuzzIpcCode::GET_THERMAL_INFO:
            FuzzGetThermalInfo(provider);
            break;
        case ThermalFuzzIpcCode::SET_SCENE:
            FuzzSetScene(provider);
            break;
        case ThermalFuzzIpcCode::UPDATE_THERMAL_STATE:
            FuzzUpdateThermalState(provider);
            break;
        case ThermalFuzzIpcCode::SHELL_DUMP:
            FuzzShellDump(provider);
            break;
        default:
            break;
    }
}

void ThermalFuzzer::FuzzAllInterfaces(FuzzedDataProvider& provider)
{
    if (service_ == nullptr) {
        return;
    }

    // Fuzz all interfaces in sequence for maximum coverage
    while (provider.remaining_bytes() > 0) {
        FuzzThermalService(provider);
    }
}

void ThermalFuzzer::FuzzServiceStub(uint32_t code, const uint8_t* data, size_t size)
{
    if (service_ == nullptr) {
        return;
    }

    MessageParcel dataParcel;
    dataParcel.WriteInterfaceToken(ThermalService::GetDescriptor());
    dataParcel.WriteBuffer(data, size);
    dataParcel.RewindRead(REWIND_READ_DATA);
    MessageParcel reply;
    MessageOption option;
    service_->OnRemoteRequest(code, dataParcel, reply, option);
}

} // namespace PowerMgr
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }

    static OHOS::PowerMgr::ThermalFuzzer fuzzer;
    FuzzedDataProvider provider(data, size);

    // Method 1: Direct API fuzzing with random IPC code selection
    fuzzer.FuzzThermalService(provider);

    // Method 2: IPC stub fuzzing with raw data
    if (provider.remaining_bytes() > 0) {
        uint32_t code = provider.ConsumeIntegralInRange<uint32_t>(
            0, static_cast<uint32_t>(OHOS::PowerMgr::ThermalFuzzIpcCode::MAX_CODE) - 1);
        std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
        if (!remainingData.empty()) {
            fuzzer.FuzzServiceStub(code, remainingData.data(), remainingData.size());
        }
    }

    return 0;
}
