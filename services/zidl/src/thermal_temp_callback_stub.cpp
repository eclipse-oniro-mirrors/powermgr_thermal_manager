/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include "thermal_temp_callback_stub.h"

#include <message_parcel.h>
#include "errors.h"
#include "ipc_object_stub.h"
#include "constants.h"
#include "thermal_log.h"
#include "thermal_mgr_errors.h"
#include "thermal_common.h"
#include "thermal_temp_callback_ipc_interface_code.h"
#include "xcollie/xcollie.h"
#include "xcollie/xcollie_define.h"

namespace OHOS {
namespace PowerMgr {
constexpr uint32_t MAX_TEMP_MAP_NUM = 2000;
int ThermalTempCallbackStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    THERMAL_HILOGD(COMP_SVC,
        "ThermalTempCallbackStub::OnRemoteRequest, cmd = %{public}d, flags= %{public}d",
        code, option.GetFlags());
    std::u16string descripter = ThermalTempCallbackStub::GetDescriptor();
    std::u16string remoteDescripter = data.ReadInterfaceToken();
    if (descripter != remoteDescripter) {
        THERMAL_HILOGE(COMP_SVC, "ThermalTempCallbackStub::OnRemoteRequest failed, \
            descriptor is not matched!");
        return E_GET_THERMAL_SERVICE_FAILED;
    }
    const int DFX_DELAY_S = 60;
    int id = HiviewDFX::XCollie::GetInstance().SetTimer("ThermalLevelCallbackStub", DFX_DELAY_S, nullptr, nullptr,
        HiviewDFX::XCOLLIE_FLAG_LOG);
    int ret = ERR_OK;
    if (code == static_cast<uint32_t>(PowerMgr::ThermalTempCallbackInterfaceCode::THERMAL_TEMPERATURE_CHANGED)) {
        ret = OnThermalTempChangedStub(data);
    } else {
        ret = IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    HiviewDFX::XCollie::GetInstance().CancelTimer(id);
    return ret;
}

int32_t ThermalTempCallbackStub::OnThermalTempChangedStub(MessageParcel& data)
{
    std::map<std::string, int32_t> tempTypeCb;
    uint32_t size = 0;
    THERMAL_READ_PARCEL_WITH_RET(data, Uint32, size, E_READ_PARCEL_ERROR_THERMAL);
    if (size <= 0 || size > MAX_TEMP_MAP_NUM) {
        THERMAL_HILOGW(COMP_SVC, "size exceed limit, size=%{public}d", size);
        return E_EXCEED_PARAM_LIMIT;
    }
    for (uint32_t i = 0; i < size; i++) {
        std::string type;
        int32_t temp = INVAILD_TEMP;
        THERMAL_READ_PARCEL_WITH_RET(data, String, type, E_READ_PARCEL_ERROR_THERMAL);
        THERMAL_READ_PARCEL_WITH_RET(data, Int32, temp, E_READ_PARCEL_ERROR_THERMAL);
        tempTypeCb[type] = temp;
    }
    OnThermalTempChanged(tempTypeCb);
    return ERR_OK;
}
} // namespace PowerMgr
} // namespace OHOS