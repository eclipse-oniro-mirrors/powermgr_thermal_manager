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

#include "simple_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <string>
#include <vector>

#include "message_parcel.h"
#include "message_option.h"

using namespace OHOS::PowerMgr;
using namespace OHOS;

namespace {
constexpr size_t MAX_STRING_LENGTH = 256;
constexpr size_t MAX_VECTOR_SIZE = 16;
constexpr int32_t REWIND_READ_DATA = 0;
} // namespace

namespace OHOS {
namespace PowerMgr {

SimpleFuzzer::SimpleFuzzer()
{
    service_ = ThermalService::GetInstance();
    if (service_ != nullptr) {
        service_->OnStart();
        service_->SetTempReportSwitch(false);
    }
}

SimpleFuzzer::~SimpleFuzzer()
{
    if (service_ != nullptr) {
        service_->OnStop();
    }
}

std::string SimpleFuzzer::GenerateString(FuzzedDataProvider& provider)
{
    size_t length = provider.ConsumeIntegralInRange<size_t>(0,
        std::min(MAX_STRING_LENGTH, provider.remaining_bytes()));
    return provider.ConsumeRandomLengthString(length);
}

std::vector<std::string> SimpleFuzzer::GenerateStringVector(FuzzedDataProvider& provider)
{
    std::vector<std::string> result;
    size_t size = provider.ConsumeIntegralInRange<size_t>(0,
        std::min(MAX_VECTOR_SIZE, provider.remaining_bytes() / sizeof(char)));
    for (size_t i = 0; i < size && provider.remaining_bytes() > 0; ++i) {
        result.push_back(GenerateString(provider));
    }
    return result;
}

void SimpleFuzzer::FuzzSubscribeThermalTempCallback(FuzzedDataProvider& provider)
{
    if (service_ == nullptr) {
        return;
    }
    std::vector<std::string> typeList = GenerateStringVector(provider);
    sptr<IThermalTempCallback> callback = sptr<SimpleFuzzerTempCallback>::MakeSptr();
    service_->SubscribeThermalTempCallback(typeList, callback);
}

void SimpleFuzzer::FuzzUnSubscribeThermalTempCallback(FuzzedDataProvider& provider)
{
    (void)provider;
    if (service_ == nullptr) {
        return;
    }
    sptr<IThermalTempCallback> callback = sptr<SimpleFuzzerTempCallback>::MakeSptr();
    service_->UnSubscribeThermalTempCallback(callback);
}

void SimpleFuzzer::FuzzSubscribeThermalLevelCallback(FuzzedDataProvider& provider)
{
    if (service_ == nullptr) {
        return;
    }
    sptr<IThermalLevelCallback> callback = sptr<SimpleFuzzerLevelCallback>::MakeSptr();
    service_->SubscribeThermalLevelCallback(callback);
}

void SimpleFuzzer::FuzzUnSubscribeThermalLevelCallback(FuzzedDataProvider& provider)
{
    (void)provider;
    if (service_ == nullptr) {
        return;
    }
    sptr<IThermalLevelCallback> callback = sptr<SimpleFuzzerLevelCallback>::MakeSptr();
    service_->UnSubscribeThermalLevelCallback(callback);
}

void SimpleFuzzer::FuzzSubscribeThermalActionCallback(FuzzedDataProvider& provider)
{
    if (service_ == nullptr) {
        return;
    }
    std::vector<std::string> actionList = GenerateStringVector(provider);
    std::string desc = GenerateString(provider);
    sptr<IThermalActionCallback> callback = sptr<SimpleFuzzerActionCallback>::MakeSptr();
    service_->SubscribeThermalActionCallback(actionList, desc, callback);
}

void SimpleFuzzer::FuzzUnSubscribeThermalActionCallback(FuzzedDataProvider& provider)
{
    (void)provider;
    if (service_ == nullptr) {
        return;
    }
    sptr<IThermalActionCallback> callback = sptr<SimpleFuzzerActionCallback>::MakeSptr();
    service_->UnSubscribeThermalActionCallback(callback);
}

void SimpleFuzzer::FuzzGetThermalSrvSensorInfo(FuzzedDataProvider& provider)
{
    if (service_ == nullptr) {
        return;
    }
    int32_t type = provider.ConsumeIntegral<int32_t>();
    ThermalSrvSensorInfo sensorInfo;
    bool sensorInfoRet = false;
    service_->GetThermalSrvSensorInfo(type, sensorInfo, sensorInfoRet);
}

void SimpleFuzzer::FuzzGetThermalLevel(FuzzedDataProvider& provider)
{
    (void)provider;
    if (service_ == nullptr) {
        return;
    }
    int32_t level = 0;
    service_->GetThermalLevel(level);
}

void SimpleFuzzer::FuzzGetThermalInfo(FuzzedDataProvider& provider)
{
    (void)provider;
    if (service_ == nullptr) {
        return;
    }
    service_->GetThermalInfo();
}

void SimpleFuzzer::FuzzSetScene(FuzzedDataProvider& provider)
{
    if (service_ == nullptr) {
        return;
    }
    std::string scene = GenerateString(provider);
    service_->SetScene(scene);
}

void SimpleFuzzer::FuzzUpdateThermalState(FuzzedDataProvider& provider)
{
    if (service_ == nullptr) {
        return;
    }
    std::string tag = GenerateString(provider);
    std::string val = GenerateString(provider);
    bool isImmed = provider.ConsumeBool();
    service_->UpdateThermalState(tag, val, isImmed);
}

void SimpleFuzzer::FuzzShellDump(FuzzedDataProvider& provider)
{
    if (service_ == nullptr) {
        return;
    }
    std::vector<std::string> args = GenerateStringVector(provider);
    uint32_t argc = provider.ConsumeIntegral<uint32_t>();
    std::string result;
    service_->ShellDump(args, argc, result);
}

void SimpleFuzzer::FuzzThermalService(FuzzedDataProvider& provider)
{
    if (service_ == nullptr) {
        return;
    }

    uint32_t code = provider.ConsumeIntegralInRange<uint32_t>(0,
        static_cast<uint32_t>(ThermalFuzzIpcCode::MAX_CODE) - 1);

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

void SimpleFuzzer::FuzzServiceStub(uint32_t code, const uint8_t* data, size_t size)
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

    OHOS::PowerMgr::SimpleFuzzer simpleFuzzer;
    FuzzedDataProvider provider(data, size);

    // Method 1: Direct API fuzzing
    simpleFuzzer.FuzzThermalService(provider);

    // Method 2: IPC stub fuzzing with raw data
    if (provider.remaining_bytes() > 0) {
        uint32_t code = provider.ConsumeIntegralInRange<uint32_t>(
            0, static_cast<uint32_t>(OHOS::PowerMgr::ThermalFuzzIpcCode::MAX_CODE) - 1);
        std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
        if (!remainingData.empty()) {
            simpleFuzzer.FuzzServiceStub(code, remainingData.data(), remainingData.size());
        }
    }

    return 0;
}
