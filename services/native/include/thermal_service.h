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

#ifndef THERMAL_SERVICE_H
#define THERMAL_SERVICE_H

#include <atomic>
#include <memory>
#include "iremote_object.h"
#include "system_ability.h"

#include "action_popup.h"
#include "fan_callback.h"
#include "fan_fault_detect.h"
#include "hdi_service_status_listener.h"
#include "ithermal_level_callback.h"
#include "ithermal_temp_callback.h"
#include "state_machine.h"
#include "thermal_action_manager.h"
#include "thermal_callback.h"
#include "thermal_config_base_info.h"
#include "thermal_config_sensor_cluster.h"
#include "thermal_observer.h"
#include "thermal_policy.h"
#include "thermal_sensor_info.h"
#include "thermal_service_subscriber.h"
#include "thermal_srv_config_parser.h"
#include "thermal_srv_sensor_info.h"
#include "thermal_srv_stub.h"
#include "v1_1/ithermal_interface.h"
#include "v1_1/thermal_types.h"
#ifdef HAS_THERMAL_AIRPLANE_MANAGER_PART
#include "common_event_data.h"
#include "common_event_manager.h"
#include "common_event_subscribe_info.h"
#include "common_event_support.h"
#endif

namespace OHOS {
namespace PowerMgr {
using TypeTempMap = std::map<std::string, int32_t>;
using namespace OHOS::HDI::Thermal::V1_1;
using namespace OHOS::HDI::ServiceManager::V1_0;
class ThermalService final : public SystemAbility, public ThermalSrvStub {
    DECLARE_SYSTEM_ABILITY(ThermalService);
    DISALLOW_COPY_AND_MOVE(ThermalService);

public:
    ThermalService();
    virtual ~ThermalService();
    virtual void OnStart() override;
    virtual void OnStop() override;
    virtual void OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId) override;
    virtual int32_t Dump(int fd, const std::vector<std::u16string>& args) override;
    int32_t SubscribeThermalTempCallback(
        const std::vector<std::string>& typeList, const sptr<IThermalTempCallback>& callback) override;
    int32_t UnSubscribeThermalTempCallback(const sptr<IThermalTempCallback>& callback) override;
    int32_t SubscribeThermalLevelCallback(const sptr<IThermalLevelCallback>& callback) override;
    int32_t UnSubscribeThermalLevelCallback(const sptr<IThermalLevelCallback>& callback) override;
    int32_t SubscribeThermalActionCallback(const std::vector<std::string>& actionList, const std::string& desc,
        const sptr<IThermalActionCallback>& callback) override;
    int32_t UnSubscribeThermalActionCallback(const sptr<IThermalActionCallback>& callback) override;
    int32_t GetThermalSrvSensorInfo(int32_t type, ThermalSrvSensorInfo& sensorInfo, bool& sensorInfoRet) override;
    int32_t GetThermalLevel(int32_t& level) override;
    int32_t GetThermalInfo() override;
    int32_t SetScene(const std::string& scene) override;
    int32_t UpdateThermalState(const std::string& tag, const std::string& val, bool isImmed = false) override;
    bool CreateConfigModule();
    int32_t ShellDump(const std::vector<std::string>& args, uint32_t argc, std::string& dumpShell) override;

    int32_t HandleThermalCallbackEvent(const HdfThermalCallbackInfo& event);
    int32_t HandleFanCallbackEvent(const HdfThermalCallbackInfo& event);
    bool HandleTempEmulation(const TypeTempMap& typeTempMap);
    static sptr<ThermalService> GetInstance();
    static void DestroyInstance();

    void SetTempReportSwitch(bool enable)
    {
        isTempReport_ = enable;
    }

    bool IsServiceReady() const
    {
        return ready_;
    }

    std::shared_ptr<ThermalConfigBaseInfo> GetBaseinfoObj() const
    {
        return baseInfo_;
    }
    std::shared_ptr<StateMachine> GetStateMachineObj() const
    {
        return state_;
    }
    std::shared_ptr<ThermalActionManager> GetActionManagerObj() const
    {
        return actionMgr_;
    }
    std::shared_ptr<ThermalPolicy> GetPolicy() const
    {
        return policy_;
    }
    std::shared_ptr<ThermalObserver> GetObserver() const
    {
        return observer_;
    }
    std::shared_ptr<ThermalSensorInfo> GetSensorInfo() const
    {
        return info_;
    }
    std::shared_ptr<ThermalServiceSubscriber> GetSubscriber() const
    {
        return serviceSubscriber_;
    }

    std::shared_ptr<ActionPopup> GetActionPopup() const
    {
        return popup_;
    }

    sptr<IThermalInterface> GetThermalInterface() const
    {
        return thermalInterface_;
    }

    bool GetSimulationXml()
    {
        return isSimulation_;
    }

    void SetSimulationXml(bool isSimulation)
    {
        isSimulation_ = isSimulation;
    }

    std::shared_ptr<FanFaultDetect> GetFanFaultDetect() const
    {
        return fanFaultDetect_;
    }

    ThermalSrvConfigParser& GetConfigParser()
    {
        return configParser_;
    }

    void InitSystemTestModules()
    {
        InitConfigModule();
    }

    const std::string& GetScene()
    {
        std::lock_guard<std::mutex> lock(sceneMutex_);
        return scene_;
    }
#ifdef HAS_THERMAL_AIRPLANE_MANAGER_PART
    static bool userAirplaneState_;
    static bool isThermalAirplane_;
#endif

private:
    bool Init();
    bool InitThermalDriver();
    bool InitThermalObserver();
    bool InitThermalSubscriber();
    bool InitThermalAction();
    bool InitThermalPolicy();
    bool InitBaseInfo();
    bool InitSensorCluser();
    bool InitActionManager();
    bool InitStateMachine();
    bool InitModules();
    bool InitConfigFile();
    bool InitConfigModule();
    void RegisterHdiStatusListener();
    void RegisterThermalHdiCallback();
    void UnRegisterThermalHdiCallback();
    void RegisterFanHdiCallback();
    void RegisterBootCompletedCallback();
    void EnableMock(const std::string& actionName, void* mockObject);
#ifdef HAS_THERMAL_AIRPLANE_MANAGER_PART
    bool SubscribeCommonEvent();
#endif
    sptr<IThermalInterface> GetThermalInterfaceInner();
    bool ready_ {false};
    static std::atomic_bool isBootCompleted_;
    bool isSimulation_ {false};
    bool isTempReport_ {true};
    std::mutex mutex_;
    std::mutex interfaceMutex_;
    std::mutex sceneMutex_;
    std::shared_ptr<ThermalServiceSubscriber> serviceSubscriber_ {nullptr};
    std::shared_ptr<ThermalObserver> observer_ {nullptr};
    std::shared_ptr<ThermalSensorInfo> info_ {nullptr};
    std::shared_ptr<ThermalPolicy> policy_ {nullptr};
    std::shared_ptr<ThermalConfigBaseInfo> baseInfo_ {nullptr};
    std::shared_ptr<StateMachine> state_ {nullptr};
    std::shared_ptr<ThermalActionManager> actionMgr_ {nullptr};
    std::shared_ptr<FanFaultDetect> fanFaultDetect_ {nullptr};
    ThermalSrvConfigParser configParser_;
    sptr<IThermalInterface> thermalInterface_ {nullptr};
    sptr<IServiceManager> hdiServiceMgr_ {nullptr};
    sptr<HdiServiceStatusListener::IServStatListener> hdiServStatListener_ {nullptr};
    std::shared_ptr<ActionPopup> popup_;
    std::atomic<bool> serviceConfigParsed {false};
    static std::string scene_;
    static sptr<ThermalService> instance_;
    static std::mutex singletonMutex_;
#ifdef HAS_THERMAL_AIRPLANE_MANAGER_PART
    std::shared_ptr<EventFwk::CommonEventSubscriber> subscriberPtr_;
#endif
};
#ifdef HAS_THERMAL_AIRPLANE_MANAGER_PART
class AirplaneCommonEventSubscriber : public EventFwk::CommonEventSubscriber {
public:
    explicit AirplaneCommonEventSubscriber(const EventFwk::CommonEventSubscribeInfo& subscribeInfo)
        : EventFwk::CommonEventSubscriber(subscribeInfo) {}
    virtual ~AirplaneCommonEventSubscriber() {}
    void OnReceiveEvent(const EventFwk::CommonEventData &data) override;
};
#endif
} // namespace PowerMgr
} // namespace OHOS
#endif // THERMAL_SERVICE_H
