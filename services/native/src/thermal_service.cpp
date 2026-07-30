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

#include "thermal_service.h"

#include "file_ex.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "modulemgr.h"
#include "securec.h"
#include "system_ability_definition.h"
#include <algorithm>
#include <fcntl.h>
#include <ipc_skeleton.h>
#include <thread>
#include <unistd.h>

#ifdef HAS_THERMAL_CONFIG_POLICY_PART
#include "config_policy_utils.h"
#endif
#include "constants.h"
#include "ffrt_utils.h"
#include "permission.h"
#include "sysparam.h"
#include "soc_action_base.h"
#include "thermal_common.h"
#include "thermal_mgr_dumper.h"
#include "thermal_xcollie.h"
#include "xcollie/watchdog.h"

namespace OHOS {
namespace PowerMgr {
sptr<ThermalService> ThermalService::instance_ = nullptr;
std::mutex ThermalService::singletonMutex_;
namespace {
MODULE_MGR* g_moduleMgr = nullptr;
#if (defined(__aarch64__) || defined(__x86_64__))
constexpr const char* THERMAL_PLUGIN_AUTORUN_PATH = "/system/lib64/thermalplugin/autorun";
#else
constexpr const char* THERMAL_PLUGIN_AUTORUN_PATH = "/system/lib/thermalplugin/autorun";
#endif
const std::string THERMAL_SERVICE_CONFIG_PATH = "etc/thermal_config/thermal_service_config.xml";
const std::string VENDOR_THERMAL_SERVICE_CONFIG_PATH = "/vendor/etc/thermal_config/thermal_service_config.xml";
const std::string SYSTEM_THERMAL_SERVICE_CONFIG_PATH = "/system/etc/thermal_config/thermal_service_config.xml";
constexpr const char* HDI_SERVICE_NAME = "thermal_interface_service";
FFRTQueue g_queue("thermal_service");
constexpr uint32_t RETRY_TIME = 1000;
constexpr int32_t ERR_FAIL = -1;
auto g_service = ThermalService::GetInstance();
const bool G_REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(g_service.GetRefPtr());
SysParam::BootCompletedCallback g_bootCompletedCallback;
} // namespace
std::atomic_bool ThermalService::isBootCompleted_ = false;
std::string ThermalService::scene_;
#ifdef HAS_THERMAL_AIRPLANE_MANAGER_PART
bool ThermalService::userAirplaneState_ = false;
bool ThermalService::isThermalAirplane_ = false;
#endif
ThermalService::ThermalService() : SystemAbility(POWER_MANAGER_THERMAL_SERVICE_ID, true) {}

ThermalService::~ThermalService() {}

sptr<ThermalService> ThermalService::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock(singletonMutex_);
        if (instance_ == nullptr) {
            instance_ = new ThermalService();
        }
    }
    return instance_;
}

void ThermalService::OnStart()
{
    THERMAL_HILOGD(COMP_SVC, "Enter");
    if (ready_) {
        THERMAL_HILOGE(COMP_SVC, "OnStart is ready, nothing to do");
        return;
    }
    g_moduleMgr = ModuleMgrScan(THERMAL_PLUGIN_AUTORUN_PATH);
    if (!(Init())) {
        THERMAL_HILOGE(COMP_SVC, "OnStart call init fail");
#ifndef FUZZ_TEST
        ModuleMgrUninstall(g_moduleMgr, "thermal_decrypt");
#endif
        return;
    }
// This library occupies up to 300kB, dlclose after using it.
// Leave it be in fuzz tests
#ifndef FUZZ_TEST
        ModuleMgrUninstall(g_moduleMgr, "thermal_decrypt");
#endif
    AddSystemAbilityListener(COMMON_EVENT_SERVICE_ID);
    AddSystemAbilityListener(SOC_PERF_SERVICE_SA_ID);
    AddSystemAbilityListener(POWER_MANAGER_BATT_SERVICE_ID);
    if (!Publish(ThermalService::GetInstance())) {
        THERMAL_HILOGE(COMP_SVC, "OnStart register to system ability manager failed.");
        return;
    }
    RegisterBootCompletedCallback();
    ready_ = true;
    THERMAL_HILOGD(COMP_SVC, "OnStart and add system ability success");
}

void ThermalService::RegisterBootCompletedCallback()
{
    g_bootCompletedCallback = []() {
        isBootCompleted_ = true;
    };
    SysParam::RegisterBootCompletedCallback(g_bootCompletedCallback);
}

void ThermalService::OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId)
{
    THERMAL_HILOGI(COMP_SVC, "systemAbilityId=%{public}d, deviceId=%{private}s", systemAbilityId, deviceId.c_str());
    if (systemAbilityId == COMMON_EVENT_SERVICE_ID) {
        InitStateMachine();
#ifdef HAS_THERMAL_AIRPLANE_MANAGER_PART
        SubscribeCommonEvent();
#endif
    } else if (systemAbilityId == SOC_PERF_SERVICE_SA_ID) {
        ThermalActionManager::ThermalActionMap actionMap = GetActionManagerObj()->GetActionMap();
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& iter : SocActionBase::SocSet) {
            auto actionIter = actionMap.find(iter);
            if (actionIter == actionMap.end() || actionIter->second == nullptr) {
                THERMAL_HILOGE(COMP_SVC, "can't find action [%{public}s] ability", iter.c_str());
                continue;
            }
            actionIter->second->ResetActionValue();
        }
    } else if (systemAbilityId == POWER_MANAGER_BATT_SERVICE_ID) {
#ifdef BATTERY_MANAGER_ENABLE
        auto csc = ChargerStateCollection::GetInstance();
        if (csc == nullptr) {
            THERMAL_HILOGE(COMP_SVC, "ChargerStateCollection GetInstance failed");
            return;
        }
        csc->InitChargeState();
#endif
    }
}

#ifdef HAS_THERMAL_AIRPLANE_MANAGER_PART
bool ThermalService::SubscribeCommonEvent()
{
    using namespace OHOS::EventFwk;
    bool result = false;
    MatchingSkills matchingSkills;
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_AIRPLANE_MODE_CHANGED);
    CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    subscribeInfo.SetThreadMode(CommonEventSubscribeInfo::ThreadMode::COMMON);
    if (!subscriberPtr_) {
        subscriberPtr_ = std::make_shared<AirplaneCommonEventSubscriber>(subscribeInfo);
    }
    result = CommonEventManager::SubscribeCommonEvent(subscriberPtr_);
    if (!result) {
        THERMAL_HILOGE(COMP_SVC, "Subscribe CommonEvent failed");
    } else {
        THERMAL_HILOGD(COMP_SVC, "Subscribe CommonEvent success");
    }
    return result;
}

void AirplaneCommonEventSubscriber::OnReceiveEvent(const OHOS::EventFwk::CommonEventData &data)
{
    if (!ThermalService::isThermalAirplane_) {
        int32_t code = data.GetCode();
        ThermalService::userAirplaneState_ = static_cast<bool>(code);
        THERMAL_HILOGD(COMP_SVC, "user %{public}s Airplane mode",
            ThermalService::userAirplaneState_ ? "open" : "close");
    } else {
        ThermalService::isThermalAirplane_ = false;
        THERMAL_HILOGD(COMP_SVC, "thermal change Airplane mode");
    }
}
#endif

bool ThermalService::Init()
{
    THERMAL_HILOGD(COMP_SVC, "Enter");
    if (!CreateConfigModule()) {
        return false;
    }
    if (!InitModules()) {
        return false;
    }
    RegisterHdiStatusListener();
    THERMAL_HILOGD(COMP_SVC, "Init success");
    return true;
}

bool ThermalService::CreateConfigModule()
{
    if (!baseInfo_) {
        baseInfo_ = std::make_shared<ThermalConfigBaseInfo>();
        if (baseInfo_ == nullptr) {
            THERMAL_HILOGE(COMP_SVC, "failed to create base info");
            return false;
        }
    }

    if (!state_) {
        state_ = std::make_shared<StateMachine>();
        if (state_ == nullptr) {
            THERMAL_HILOGE(COMP_SVC, "failed to create state machine");
            return false;
        }
    }

    if (!actionMgr_) {
        actionMgr_ = std::make_shared<ThermalActionManager>();
        if (actionMgr_ == nullptr) {
            THERMAL_HILOGE(COMP_SVC, "failed to create action manager");
            return false;
        }
    }

    if (!policy_) {
        policy_ = std::make_shared<ThermalPolicy>();
        if (policy_ == nullptr) {
            THERMAL_HILOGE(COMP_SVC, "failed to create thermal policy");
            return false;
        }
    }

    if (!fanFaultDetect_) {
        fanFaultDetect_ = std::make_shared<FanFaultDetect>();
        if (fanFaultDetect_ == nullptr) {
            THERMAL_HILOGE(COMP_SVC, "failed to create fan fault detect");
            return false;
        }
    }

    return true;
}

bool ThermalService::InitConfigFile()
{
    if (serviceConfigParsed) {
        THERMAL_HILOGI(COMP_SVC, "system config file has parsed.");
        return true;
    }
#ifdef HAS_THERMAL_CONFIG_POLICY_PART
    char buf[MAX_PATH_LEN];
    char* path = GetOneCfgFile(THERMAL_SERVICE_CONFIG_PATH.c_str(), buf, MAX_PATH_LEN);
    if (path != nullptr && *path != '\0') {
        if (configParser_.ThermalSrvConfigInit(path)) {
            THERMAL_HILOGD(COMP_SVC, "match pliocy config file");
            return true;
        }
        THERMAL_HILOGE(COMP_SVC, "policy config file config init err");
        return false;
    }
#endif

    if (configParser_.ThermalSrvConfigInit(VENDOR_THERMAL_SERVICE_CONFIG_PATH)) {
        THERMAL_HILOGD(COMP_SVC, "thermal service config init suc:VENDOR_CONFIG");
        return true;
    }

    if (configParser_.ThermalSrvConfigInit(SYSTEM_THERMAL_SERVICE_CONFIG_PATH)) {
        THERMAL_HILOGD(COMP_SVC, "thermal service config init suc:SYSTEM_CONFIG");
        return true;
    }
    THERMAL_HILOGE(COMP_SVC, "policy config file config init fail");
    return false;
}


bool ThermalService::InitConfigModule()
{
    THERMAL_HILOGD(COMP_SVC, "InitVendor Enter");
    if (!CreateConfigModule()) {
        THERMAL_HILOGD(COMP_SVC, "CreateConfigModule fail");
    }

    if (!configParser_.ThermalSrvConfigInit(SYSTEM_THERMAL_SERVICE_CONFIG_PATH)) {
        THERMAL_HILOGE(COMP_SVC, "system thermal service config init failed.");
        return false;
    }
    serviceConfigParsed = true;

    THERMAL_HILOGE(COMP_SVC, "system thermal service config init suc.");
    return true;
}

bool ThermalService::InitModules()
{
    if (!InitConfigFile()) {
        return false;
    }

    if (popup_ == nullptr) {
        popup_ = std::make_shared<ActionPopup>(POPUP_ACTION_NAME);
    }

    if (!InitThermalObserver()) {
        THERMAL_HILOGE(COMP_SVC, "thermal observer start fail");
        return false;
    }

    if (!InitActionManager()) {
        THERMAL_HILOGE(COMP_SVC, "action manager init fail");
        return false;
    }

    if (!InitThermalPolicy()) {
        THERMAL_HILOGE(COMP_SVC, "thermal policy start fail");
        return false;
    }

    if (!InitThermalSubscriber()) {
        THERMAL_HILOGE(COMP_SVC, "thermal subscriber init fail");
        return false;
    }
    return true;
}

bool ThermalService::InitThermalObserver()
{
    if (!InitBaseInfo()) {
        return false;
    }

    THERMAL_HILOGD(COMP_SVC, "Enter");
    if (observer_ == nullptr) {
        observer_ = std::make_shared<ThermalObserver>();
        if (!(observer_->Init())) {
            THERMAL_HILOGE(COMP_SVC, "InitThermalObserver: thermal observer start fail");
            return false;
        }
    }
    if (info_ == nullptr) {
        info_ = std::make_shared<ThermalSensorInfo>();
    }
    THERMAL_HILOGI(COMP_SVC, "InitThermalObserver: Init Success");
    return true;
}

bool ThermalService::InitBaseInfo()
{
    THERMAL_HILOGD(COMP_SVC, "Enter");
    if (!baseInfo_->Init()) {
        THERMAL_HILOGE(COMP_SVC, "InitBaseInfo: base info init failed");
        return false;
    }
    return true;
}

bool ThermalService::InitStateMachine()
{
    THERMAL_HILOGD(COMP_SVC, "Enter");
    if (!state_->Init()) {
        THERMAL_HILOGE(COMP_SVC, "InitStateMachine: state machine init failed");
        return false;
    }
    return true;
}

bool ThermalService::InitActionManager()
{
    THERMAL_HILOGD(COMP_SVC, "Enter");
    if (!actionMgr_->Init()) {
        THERMAL_HILOGE(COMP_SVC, "InitActionManager: action manager init failed");
        return false;
    }
    return true;
}

bool ThermalService::InitThermalPolicy()
{
    THERMAL_HILOGD(COMP_SVC, "Enter");
    if (!policy_->Init()) {
        THERMAL_HILOGE(COMP_SVC, "InitThermalPolicy: policy init failed");
        return false;
    }
    return true;
}

bool ThermalService::InitThermalSubscriber()
{
    if (serviceSubscriber_ == nullptr) {
        serviceSubscriber_ = std::make_shared<ThermalServiceSubscriber>();
        if (!(serviceSubscriber_->Init())) {
            THERMAL_HILOGE(COMP_SVC, "InitThermalSubscriber: thermal subscriber init failed");
            return false;
        }
    }
    return true;
}

void ThermalService::OnStop()
{
    THERMAL_HILOGD(COMP_SVC, "Enter");
    if (!ready_) {
        return;
    }
    ready_ = false;
    isBootCompleted_ = false;
    RemoveSystemAbilityListener(COMMON_EVENT_SERVICE_ID);
    {
        std::lock_guard lock(interfaceMutex_);
        if (thermalInterface_) {
            thermalInterface_->Unregister();
            thermalInterface_->UnregisterFanCallback();
            thermalInterface_ = nullptr;
        }
    }
    if (hdiServiceMgr_) {
        hdiServiceMgr_->UnregisterServiceStatusListener(hdiServStatListener_);
        hdiServiceMgr_ = nullptr;
    }
#ifdef HAS_THERMAL_AIRPLANE_MANAGER_PART
    if (!OHOS::EventFwk::CommonEventManager::UnSubscribeCommonEvent(subscriberPtr_)) {
        THERMAL_HILOGE(COMP_SVC, "Thermal Onstop unregister to commonevent manager failed!");
    } else {
        THERMAL_HILOGD(COMP_SVC, "Thermal Onstop unregister to commonevent manager success!");
    }
#endif
#ifndef FUZZ_TEST
    if (g_moduleMgr != nullptr) {
        ModuleMgrDestroy(g_moduleMgr);
        g_moduleMgr = nullptr;
    }
#endif
}

int32_t ThermalService::SubscribeThermalTempCallback(
    const std::vector<std::string>& typeList, const sptr<IThermalTempCallback>& callback)
{
    ThermalXCollie thermalXCollie("ThermalService::SubscribeThermalTempCallback", false);
    if (!Permission::IsSystem()) {
        THERMAL_HILOGE(COMP_SVC, "Permission::IsSystem() failed");
        return ERR_FAIL;
    }
    auto uid = IPCSkeleton::GetCallingUid();
    THERMAL_HILOGI(COMP_SVC, "ScbTempCb uid=%{public}d", uid);
    observer_->SubscribeThermalTempCallback(typeList, callback);
    return ERR_OK;
}

int32_t ThermalService::UnSubscribeThermalTempCallback(const sptr<IThermalTempCallback>& callback)
{
    ThermalXCollie thermalXCollie("ThermalService::UnSubscribeThermalTempCallback", false);
    if (!Permission::IsSystem()) {
        THERMAL_HILOGE(COMP_SVC, "Permission::IsSystem() failed");
        return ERR_FAIL;
    }
    auto uid = IPCSkeleton::GetCallingUid();
    THERMAL_HILOGI(COMP_SVC, "UnScbTempCb uid=%{public}d", uid);
    observer_->UnSubscribeThermalTempCallback(callback);
    return ERR_OK;
}

int32_t ThermalService::GetThermalSrvSensorInfo(int32_t type, ThermalSrvSensorInfo& sensorInfo, bool& sensorInfoRet)
{
    ThermalXCollie thermalXCollie("ThermalService::GetThermalSrvSensorInfo", false);
    THERMAL_HILOGD(COMP_SVC, "Enter");
    sensorInfoRet = observer_->GetThermalSrvSensorInfo(static_cast<SensorType>(type), sensorInfo);
    if (!(sensorInfoRet)) {
        THERMAL_HILOGW(COMP_SVC, "failed to get sensor temp, type enum: %{public}u", static_cast<uint32_t>(type));
        return ERR_FAIL;
    }
    return ERR_OK;
}

int32_t ThermalService::SubscribeThermalLevelCallback(const sptr<IThermalLevelCallback>& callback)
{
    ThermalXCollie thermalXCollie("ThermalService::SubscribeThermalLevelCallback", false);
    auto uid = IPCSkeleton::GetCallingUid();
    THERMAL_HILOGI(COMP_SVC, "ScbLevelCb uid=%{public}d", uid);
    actionMgr_->SubscribeThermalLevelCallback(callback);
    return ERR_OK;
}

int32_t ThermalService::UnSubscribeThermalLevelCallback(const sptr<IThermalLevelCallback>& callback)
{
    ThermalXCollie thermalXCollie("ThermalService::UnSubscribeThermalLevelCallback", false);
    auto uid = IPCSkeleton::GetCallingUid();
    THERMAL_HILOGI(COMP_SVC, "UnScbLevelCb uid=%{public}d", uid);
    actionMgr_->UnSubscribeThermalLevelCallback(callback);
    return ERR_OK;
}

int32_t ThermalService::SubscribeThermalActionCallback(
    const std::vector<std::string>& actionList, const std::string& desc, const sptr<IThermalActionCallback>& callback)
{
    ThermalXCollie thermalXCollie("ThermalService::SubscribeThermalActionCallback", false);
    if (!Permission::IsSystem()) {
        THERMAL_HILOGE(COMP_SVC, "Permission::IsSystem() failed");
        return ERR_FAIL;
    }
    auto pid = IPCSkeleton::GetCallingPid();
    auto uid = IPCSkeleton::GetCallingUid();
    THERMAL_HILOGI(COMP_SVC, "ScbActionCb pid=%{public}d,uid=%{public}d", pid, uid);
    observer_->SubscribeThermalActionCallback(actionList, desc, callback);
    return ERR_OK;
}

int32_t ThermalService::UnSubscribeThermalActionCallback(const sptr<IThermalActionCallback>& callback)
{
    ThermalXCollie thermalXCollie("ThermalService::UnSubscribeThermalActionCallback", false);
    if (!Permission::IsSystem()) {
        THERMAL_HILOGE(COMP_SVC, "Permission::IsSystem() failed");
        return ERR_FAIL;
    }
    auto pid = IPCSkeleton::GetCallingPid();
    auto uid = IPCSkeleton::GetCallingUid();
    THERMAL_HILOGI(COMP_SVC, "UnScbActionCb pid=%{public}d, uid=%{public}d", pid, uid);
    observer_->UnSubscribeThermalActionCallback(callback);
    return ERR_OK;
}

int32_t ThermalService::GetThermalLevel(int32_t& level)
{
    ThermalXCollie thermalXCollie("ThermalService::GetThermalLevel", false);
    if (actionMgr_ == nullptr) {
        THERMAL_HILOGD(COMP_SVC, "actionMgr_ is nullptr");
        return ERR_FAIL;
    }
    level = static_cast<int32_t>(actionMgr_->GetThermalLevel());
    return ERR_OK;
}

sptr<IThermalInterface> ThermalService::GetThermalInterfaceInner()
{
    std::lock_guard lock(interfaceMutex_);
    if (thermalInterface_ == nullptr) {
        thermalInterface_ = IThermalInterface::Get();
        if (thermalInterface_ == nullptr) {
            THERMAL_HILOGE(COMP_SVC, "thermalInterface_ is nullptr");
            return nullptr;
        }
    }
    return thermalInterface_;
}

int32_t ThermalService::GetThermalInfo()
{
    ThermalXCollie thermalXCollie("ThermalService::GetThermalInfo", false);
    THERMAL_HILOGD(COMP_SVC, "Enter");
    HdfThermalCallbackInfo thermalInfo;
    int32_t ret = ERR_FAIL;
    sptr<IThermalInterface> thermalInterface = GetThermalInterfaceInner();
    if (thermalInterface != nullptr) {
        int32_t res = thermalInterface->GetThermalZoneInfo(thermalInfo);
        HandleThermalCallbackEvent(thermalInfo);
        if (!res) {
            ret = ERR_OK;
        }
    }
    return ret;
}

int32_t ThermalService::SetScene(const std::string& scene)
{
    ThermalXCollie thermalXCollie("ThermalService::SetScene", false);
    THERMAL_HILOGD(COMP_SVC, "Enter");
    if (!Permission::IsSystem()) {
        THERMAL_HILOGE(COMP_SVC, "Permission::IsSystem() failed");
        return ERR_FAIL;
    }
    std::lock_guard<std::mutex> lock(sceneMutex_);
    scene_ = scene;
    return ERR_OK;
}

int32_t ThermalService::UpdateThermalState(const std::string& tag, const std::string& val, bool isImmed)
{
    ThermalXCollie thermalXCollie("ThermalService::UpdateThermalState", false);
    if (!Permission::IsSystem()) {
        THERMAL_HILOGE(COMP_SVC, "Permission::IsSystem() failed");
        return ERR_FAIL;
    }
    THERMAL_HILOGI(COMP_SVC, "tag %{public}s, val %{public}s, isImmed %{public}d",
        tag.c_str(), val.c_str(), isImmed);
    std::lock_guard<std::mutex> lock(mutex_);
    state_->UpdateState(tag, val);
    if (isImmed) {
        policy_->ExecutePolicy();
    }
    return ERR_OK;
}

void ThermalService::RegisterHdiStatusListener()
{
    THERMAL_HILOGD(COMP_SVC, "Enter");
    hdiServiceMgr_ = IServiceManager::Get();
    if (hdiServiceMgr_ == nullptr) {
        FFRTTask retryTask = [this] {
            return RegisterHdiStatusListener();
        };
        FFRTUtils::SubmitDelayTask(retryTask, RETRY_TIME, g_queue);
        THERMAL_HILOGW(COMP_SVC, "hdi service manager is nullptr, try again after %{public}u ms", RETRY_TIME);
        return;
    }

    hdiServStatListener_ = new HdiServiceStatusListener(
        HdiServiceStatusListener::StatusCallback([&](const OHOS::HDI::ServiceManager::V1_0::ServiceStatus& status) {
            THERMAL_RETURN_IF(status.serviceName != HDI_SERVICE_NAME || status.deviceClass != DEVICE_CLASS_DEFAULT)
            if (status.status == SERVIE_STATUS_START) {
                FFRTTask task = [this] {
                    RegisterThermalHdiCallback();
                    RegisterFanHdiCallback();
                };
                FFRTUtils::SubmitTask(task);
                THERMAL_HILOGD(COMP_SVC, "thermal interface service start");
            } else if (status.status == SERVIE_STATUS_STOP) {
                std::lock_guard lock(interfaceMutex_);
                if (thermalInterface_) {
                    thermalInterface_->Unregister();
                    thermalInterface_->UnregisterFanCallback();
                    thermalInterface_ = nullptr;
                    THERMAL_HILOGW(COMP_SVC, "thermal interface service, unregister interface");
                }
            }
        }));

    int32_t status = hdiServiceMgr_->RegisterServiceStatusListener(hdiServStatListener_, DEVICE_CLASS_DEFAULT);
    if (status != ERR_OK) {
        THERMAL_HILOGW(COMP_SVC, "Register hdi failed, try again after %{public}u ms", RETRY_TIME);
        FFRTTask retryTask = [this] {
            return RegisterHdiStatusListener();
        };
        FFRTUtils::SubmitDelayTask(retryTask, RETRY_TIME, g_queue);
    }
}

void ThermalService::RegisterThermalHdiCallback()
{
    THERMAL_HILOGD(COMP_SVC, "register thermal hdi callback");
    sptr<IThermalInterface> thermalInterface = GetThermalInterfaceInner();
    if (thermalInterface == nullptr) {
        THERMAL_HILOGE(COMP_SVC, "the thermalInterface is null");
        return;
    }
    sptr<IThermalCallback> callback = new ThermalCallback();
    ThermalCallback::ThermalEventCallback eventCb =
        [this](const HdfThermalCallbackInfo& event) -> int32_t { return this->HandleThermalCallbackEvent(event); };
    ThermalCallback::RegisterThermalEvent(eventCb);
    int32_t ret = thermalInterface->Register(callback);
    THERMAL_HILOGI(COMP_SVC, "register thermal hdi callback end, ret: %{public}d", ret);
}

void ThermalService::UnRegisterThermalHdiCallback()
{
    sptr<IThermalInterface> thermalInterface = GetThermalInterfaceInner();
    if (thermalInterface == nullptr) {
        FFRTTask retryTask = [this] {
            return UnRegisterThermalHdiCallback();
        };
        FFRTUtils::SubmitDelayTask(retryTask, RETRY_TIME, g_queue);
        THERMAL_HILOGW(COMP_SVC, "thermalInterface is nullptr, try again after %{public}u ms", RETRY_TIME);
        return;
    }
    int32_t ret = thermalInterface->Unregister();
    THERMAL_HILOGI(COMP_SVC, "unregister thermal hdi callback end, ret: %{public}d", ret);
}

int32_t ThermalService::HandleThermalCallbackEvent(const HdfThermalCallbackInfo& event)
{
#ifndef THERMAL_USER_VERSION
    if (!isTempReport_) {
        return ERR_OK;
    }
#endif
    TypeTempMap typeTempMap;
    if (!event.info.empty()) {
        for (auto iter = event.info.begin(); iter != event.info.end(); iter++) {
            typeTempMap.insert(std::make_pair(iter->type, iter->temp));
        }
    }
    std::lock_guard<std::mutex> lock(mutex_);
    serviceSubscriber_->OnTemperatureChanged(typeTempMap);
    return ERR_OK;
}

bool ThermalService::HandleTempEmulation(const TypeTempMap& typeTempMap)
{
    if (isTempReport_) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    serviceSubscriber_->OnTemperatureChanged(typeTempMap);
    return true;
}

void ThermalService::RegisterFanHdiCallback()
{
    if (!fanFaultDetect_->HasFanConfig()) {
        return;
    }
    sptr<IThermalInterface> thermalInterface = GetThermalInterfaceInner();
    if (thermalInterface == nullptr) {
        THERMAL_HILOGE(COMP_SVC, "the thermalInterface is null");
        return;
    }

    sptr<IFanCallback> callback = new FanCallback();
    FanCallback::FanEventCallback eventCb =
        [this](const HdfThermalCallbackInfo& event) -> int32_t { return this->HandleFanCallbackEvent(event); };
    FanCallback::RegisterFanEvent(eventCb);
    int32_t ret = thermalInterface->RegisterFanCallback(callback);
    THERMAL_HILOGI(COMP_SVC, "register fan hdi callback end, ret: %{public}d", ret);

    return;
}

int32_t ThermalService::HandleFanCallbackEvent(const HdfThermalCallbackInfo& event)
{
    FanSensorInfo report;
    if (!event.info.empty()) {
        for (auto iter = event.info.begin(); iter != event.info.end(); iter++) {
            report.insert(std::make_pair(iter->type, iter->temp));
        }
    }

    fanFaultDetect_->OnFanSensorInfoChanged(report);
    return ERR_OK;
}

int32_t ThermalService::ShellDump(const std::vector<std::string>& args, uint32_t argc, std::string& result)
{
    ThermalXCollie thermalXCollie("ThermalService::ShellDump", false);
    if (!Permission::IsSystem() || !isBootCompleted_) {
        THERMAL_HILOGE(COMP_SVC, "Permission::IsSystem() failed or isBootCompleted_ is false");
        result = "";
        return ERR_FAIL;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    pid_t pid = IPCSkeleton::GetCallingPid();
    THERMAL_HILOGI(COMP_SVC, "PID: %{public}d", pid);
    bool ret = ThermalMgrDumper::Dump(args, result);
    THERMAL_HILOGI(COMP_SVC, "ThermalMgrDumper :%{public}d", ret);
    return ERR_OK;
}

int32_t ThermalService::Dump(int fd, const std::vector<std::u16string>& args)
{
    if (!isBootCompleted_) {
        return ERR_NO_INIT;
    }
    if (!Permission::IsSystem()) {
        return ERR_PERMISSION_DENIED;
    }
    std::vector<std::string> argsInStr;
    std::transform(args.begin(), args.end(), std::back_inserter(argsInStr), [](const std::u16string& arg) {
        std::string ret = Str16ToStr8(arg);
        THERMAL_HILOGI(COMP_SVC, "arg: %{public}s", ret.c_str());
        return ret;
    });
    std::lock_guard<std::mutex> lock(mutex_);
    std::string result;
    ThermalMgrDumper::Dump(argsInStr, result);
    if (!SaveStringToFd(fd, result)) {
        THERMAL_HILOGE(COMP_SVC, "ThermalService::Dump failed, save to fd failed.");
        THERMAL_HILOGE(COMP_SVC, "Dump Info:\n");
        THERMAL_HILOGE(COMP_SVC, "%{public}s", result.c_str());
        return ERR_OK;
    }
    return ERR_OK;
}

void ThermalService::EnableMock(const std::string& actionName, void* mockObject)
{
    std::lock_guard<std::mutex> lock(mutex_);
    actionMgr_->EnableMock(actionName, mockObject);
}

void ThermalService::DestroyInstance()
{
    std::lock_guard<std::mutex> lock(singletonMutex_);
    if (instance_) {
        instance_.clear();
        instance_ = nullptr;
    }
}
} // namespace PowerMgr
} // namespace OHOS
