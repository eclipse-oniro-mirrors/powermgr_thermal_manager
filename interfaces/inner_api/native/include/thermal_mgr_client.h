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

#ifndef THERMAL_SERVICE_CLIENT_H
#define THERMAL_SERVICE_CLIENT_H

#include "ithermal_srv.h"
#include "thermal_srv_sensor_info.h"
#include <singleton.h>
#include <string>

namespace OHOS {
namespace PowerMgr {
class ThermalMgrClient final : public DelayedRefSingleton<ThermalMgrClient> {
    DECLARE_DELAYED_REF_SINGLETON(ThermalMgrClient)

public:
    DISALLOW_COPY_AND_MOVE(ThermalMgrClient);

    bool SubscribeThermalTempCallback(
        const std::vector<std::string>& typeList, const sptr<IThermalTempCallback>& callback);
    bool UnSubscribeThermalTempCallback(const sptr<IThermalTempCallback>& callback);
    bool SubscribeThermalLevelCallback(const sptr<IThermalLevelCallback>& callback, bool isSync = true);
    bool UnSubscribeThermalLevelCallback(const sptr<IThermalLevelCallback>& callback);
    bool SubscribeThermalActionCallback(const std::vector<std::string>& actionList, const std::string& desc,
        const sptr<IThermalActionCallback>& callback);
    bool UnSubscribeThermalActionCallback(const sptr<IThermalActionCallback>& callback);
    int32_t GetThermalSensorTemp(const SensorType type);
    ThermalLevel GetThermalLevel();
    bool SetScene(const std::string& scene);
    bool UpdateThermalPolicy();
    bool UpdateThermalState(const std::string& tag, const std::string& val, bool isImmed = false);
    std::string Dump(const std::vector<std::string>& args);

#ifndef THERMAL_SERVICE_DEATH_UT
private:
#endif
    class ThermalMgrDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        explicit ThermalMgrDeathRecipient(ThermalMgrClient& client) : client_(client) {}
        ~ThermalMgrDeathRecipient() = default;
        void OnRemoteDied(const wptr<IRemoteObject>& remote);

    private:
        DISALLOW_COPY_AND_MOVE(ThermalMgrDeathRecipient);
        ThermalMgrClient& client_;
    };

    ErrCode Connect();
    void ResetProxy(const wptr<IRemoteObject>& remote);
    sptr<IThermalSrv> GetThermalSrv();
    void GetLevel(ThermalLevel& level);
    bool GetThermalSrvSensorInfo(const SensorType& type, ThermalSrvSensorInfo& sensorInfo);
    sptr<IThermalSrv> thermalSrv_ {nullptr};
    sptr<IRemoteObject::DeathRecipient> deathRecipient_ {nullptr};
    std::mutex mutex_;
};
} // namespace PowerMgr
} // namespace OHOS
#endif // THERMAL_SERVICE_CLIENT_H
