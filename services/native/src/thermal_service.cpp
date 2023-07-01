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
#include "securec.h"
#include "system_ability_definition.h"
#include <algorithm>
#include <fcntl.h>
#include <ipc_skeleton.h>
#include <thread>
#include <unistd.h>

#include "constants.h"
#include "permission.h"
#include "sysparam.h"
#include "thermal_common.h"
#include "thermal_mgr_dumper.h"
#include "thermal_srv_config_parser.h"
#include "ffrt_utils.h"
#include "xcollie/watchdog.h"

#include "config_policy_utils.h"

namespace OHOS {
namespace PowerMgr {
namespace {
const std::string THERMAL_SERVICE_CONFIG_PATH = "etc/thermal_config/thermal_service_config.xml";
const std::string VENDOR_THERMAL_SERVICE_CONFIG_PATH = "/vendor/etc/thermal_config/thermal_service_config.xml";
const std::string SYSTEM_THERMAL_SERVICE_CONFIG_PATH = "/system/etc/thermal_config/thermal_service_config.xml";
constexpr const char* THMERMAL_SERVICE_NAME = "ThermalService";
constexpr const char* HDI_SERVICE_NAME = "thermal_interface_service";
FFRTQueue g_queue("thermal_service");
constexpr uint32_t RETRY_TIME = 1000;
auto g_service = DelayedSpSingleton<ThermalService>::GetInstance();
const bool G_REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(g_service.GetRefPtr());
SysParam::BootCompletedCallback g_bootCompletedCallback;
} // namespace
std::atomic_bool ThermalService::isBootCompleted_ = false;
ThermalService::ThermalService() : SystemAbility(POWER_MANAGER_THERMAL_SERVICE_ID, true) {}

ThermalService::~ThermalService() {}

void ThermalService::OnStart()
{
    THERMAL_HILOGD(COMP_SVC, "Enter");
    if (ready_) {
        THERMAL_HILOGE(COMP_SVC, "OnStart is ready, nothing to do");
        return;
    }

    if (!(Init())) {
        THERMAL_HILOGE(COMP_SVC, "OnStart call init fail");
        return;
    }

    AddSystemAbilityListener(COMMON_EVENT_SERVICE_ID);
    if (!Publish(DelayedSpSingleton<ThermalService>::GetInstance())) {
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
    }
}

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
    char buf[MAX_PATH_LEN];
    char* path = GetOneCfgFile(THERMAL_SERVICE_CONFIG_PATH.c_str(), buf, MAX_PATH_LEN);
    if (path != nullptr && *path != '\0') {
        if (ThermalSrvConfigParser::GetInstance().ThermalSrvConfigInit(path)) {
            THERMAL_HILOGD(COMP_SVC, "match pliocy config file");
            return true;
        }
        THERMAL_HILOGE(COMP_SVC, "pliocy config file config init err");
        return false;
    }

    if (ThermalSrvConfigParser::GetInstance().ThermalSrvConfigInit(VENDOR_THERMAL_SERVICE_CONFIG_PATH)) {
        THERMAL_HILOGD(COMP_SVC, "thermal service config init suc:VENDOR_CONFIG");
        return true;
    }

    if (ThermalSrvConfigParser::GetInstance().ThermalSrvConfigInit(SYSTEM_THERMAL_SERVICE_CONFIG_PATH)) {
        THERMAL_HILOGD(COMP_SVC, "thermal service config init suc:SYSTEM_CONFIG");
        return true;
    }

    return false;
}


bool ThermalService::InitConfigModule()
{
    THERMAL_HILOGD(COMP_SVC, "InitVendor Enter");
    if (!CreateConfigModule()) {
        THERMAL_HILOGD(COMP_SVC, "CreateConfigModule fail");
    }

    if (!InitConfigFile()) {
        return false;
    }

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
    return true;
}

bool ThermalService::InitThermalObserver()
{
    if (!InitBaseInfo()) {
        return false;
    }

    THERMAL_HILOGD(COMP_SVC, "Enter");
    if (observer_ == nullptr) {
        observer_ = std::make_shared<ThermalObserver>(g_service);
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

void ThermalService::OnStop()
{
    THERMAL_HILOGD(COMP_SVC, "Enter");
    if (!ready_) {
        return;
    }
    ready_ = false;
    isBootCompleted_ = false;
    RemoveSystemAbilityListener(COMMON_EVENT_SERVICE_ID);
    if (thermalInterface_) {
        thermalInterface_->Unregister();
        thermalInterface_->UnregisterFanCallback();
        thermalInterface_ = nullptr;
    }
    if (hdiServiceMgr_) {
        hdiServiceMgr_->UnregisterServiceStatusListener(hdiServStatListener_);
        hdiServiceMgr_ = nullptr;
    }
}

bool ThermalService::SubscribeThermalTempCallback(
    const std::vector<std::string>& typeList, const sptr<IThermalTempCallback>& callback)
{
    if (!Permission::IsSystem()) {
        return false;
    }
    THERMAL_HILOGD(COMP_SVC, "Enter");
    auto uid = IPCSkeleton::GetCallingUid();
    THERMAL_HILOGI(COMP_SVC, "uid %{public}d", uid);
    observer_->SubscribeThermalTempCallback(typeList, callback);
    return true;
}

bool ThermalService::UnSubscribeThermalTempCallback(const sptr<IThermalTempCallback>& callback)
{
    if (!Permission::IsSystem()) {
        return false;
    }
    THERMAL_HILOGD(COMP_SVC, "Enter");
    auto uid = IPCSkeleton::GetCallingUid();
    THERMAL_HILOGI(COMP_SVC, "uid %{public}d", uid);
    observer_->UnSubscribeThermalTempCallback(callback);
    return true;
}

bool ThermalService::GetThermalSrvSensorInfo(const SensorType& type, ThermalSrvSensorInfo& sensorInfo)
{
    THERMAL_HILOGD(COMP_SVC, "Enter");
    if (!(observer_->GetThermalSrvSensorInfo(type, sensorInfo))) {
        THERMAL_HILOGI(COMP_SVC, "failed to get temp for sensor type");
        return false;
    }
    return true;
}

bool ThermalService::SubscribeThermalLevelCallback(const sptr<IThermalLevelCallback>& callback)
{
    THERMAL_HILOGD(COMP_SVC, "Enter");
    auto uid = IPCSkeleton::GetCallingUid();
    THERMAL_HILOGI(COMP_SVC, "uid %{public}d", uid);
    actionMgr_->SubscribeThermalLevelCallback(callback);
    return true;
}

bool ThermalService::UnSubscribeThermalLevelCallback(const sptr<IThermalLevelCallback>& callback)
{
    THERMAL_HILOGD(COMP_SVC, "Enter");
    auto uid = IPCSkeleton::GetCallingUid();
    THERMAL_HILOGI(COMP_SVC, "uid %{public}d", uid);
    actionMgr_->UnSubscribeThermalLevelCallback(callback);
    return true;
}

bool ThermalService::SubscribeThermalActionCallback(
    const std::vector<std::string>& actionList, const std::string& desc, const sptr<IThermalActionCallback>& callback)
{
    if (!Permission::IsSystem()) {
        return false;
    }
    THERMAL_HILOGD(COMP_SVC, "Enter");
    auto pid = IPCSkeleton::GetCallingPid();
    auto uid = IPCSkeleton::GetCallingUid();
    THERMAL_HILOGI(COMP_SVC, "pid %{public}d", pid);
    THERMAL_HILOGI(COMP_SVC, "uid %{public}d", uid);
    observer_->SubscribeThermalActionCallback(actionList, desc, callback);
    return true;
}

bool ThermalService::UnSubscribeThermalActionCallback(const sptr<IThermalActionCallback>& callback)
{
    if (!Permission::IsSystem()) {
        return false;
    }
    THERMAL_HILOGD(COMP_SVC, "Enter");
    auto pid = IPCSkeleton::GetCallingPid();
    auto uid = IPCSkeleton::GetCallingUid();
    THERMAL_HILOGI(COMP_SVC, "pid %{public}d", pid);
    THERMAL_HILOGI(COMP_SVC, "uid %{public}d", uid);
    observer_->UnSubscribeThermalActionCallback(callback);
    return true;
}

bool ThermalService::GetThermalLevel(ThermalLevel& level)
{
    uint32_t levelValue = actionMgr_->GetThermalLevel();
    level = static_cast<ThermalLevel>(levelValue);
    return true;
}

bool ThermalService::GetThermalInfo()
{
    THERMAL_HILOGD(COMP_SVC, "Enter");
    HdfThermalCallbackInfo thermalInfo;
    bool ret = false;
    if (thermalInterface_ == nullptr) {
        thermalInterface_ = IThermalInterface::Get();
        if (thermalInterface_ == nullptr) {
            THERMAL_HILOGD(COMP_SVC, "thermalInterface_ is nullptr");
            return ret;
        }
    }

    if (thermalInterface_ != nullptr) {
        int32_t res = thermalInterface_->GetThermalZoneInfo(thermalInfo);
        HandleThermalCallbackEvent(thermalInfo);
        if (!res) {
            ret = true;
        }
    }
    return ret;
}

bool ThermalService::SetScene(const std::string& scene)
{
    if (!Permission::IsSystem()) {
        return false;
    }
    scene_ = scene;
    return true;
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
            } else if (status.status == SERVIE_STATUS_STOP && thermalInterface_) {
                thermalInterface_->Unregister();
                thermalInterface_->UnregisterFanCallback();
                thermalInterface_ = nullptr;
                THERMAL_HILOGW(COMP_SVC, "thermal interface service, unregister interface");
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
    THERMAL_HILOGD(COMP_SVC, "Enter");
    if (serviceSubscriber_ == nullptr) {
        serviceSubscriber_ = std::make_shared<ThermalServiceSubscriber>();
        THERMAL_RETURN_IF_WITH_LOG(!(serviceSubscriber_->Init()), "thermal service suvscriber start fail");
    }

    THERMAL_HILOGD(COMP_SVC, "register thermal hdi callback");
    if (thermalInterface_ == nullptr) {
        thermalInterface_ = IThermalInterface::Get();
        THERMAL_RETURN_IF_WITH_LOG(thermalInterface_ == nullptr, "failed to get thermal hdi interface");
    }

    sptr<IThermalCallback> callback = new ThermalCallback();
    ThermalCallback::ThermalEventCallback eventCb =
        std::bind(&ThermalService::HandleThermalCallbackEvent, this, std::placeholders::_1);
    ThermalCallback::RegisterThermalEvent(eventCb);
    int32_t ret = thermalInterface_->Register(callback);
    THERMAL_HILOGI(COMP_SVC, "register thermal hdi callback end, ret: %{public}d", ret);
}

int32_t ThermalService::HandleThermalCallbackEvent(const HdfThermalCallbackInfo& event)
{
    TypeTempMap typeTempMap;
    if (!event.info.empty()) {
        for (auto iter = event.info.begin(); iter != event.info.end(); iter++) {
            typeTempMap.insert(std::make_pair(iter->type, iter->temp));
        }
    }
    serviceSubscriber_->OnTemperatureChanged(typeTempMap);
    return ERR_OK;
}

void ThermalService::RegisterFanHdiCallback()
{
    if (!fanFaultDetect_->HasFanConfig()) {
        return;
    }

    if (thermalInterface_ == nullptr) {
        thermalInterface_ = IThermalInterface::Get();
        THERMAL_RETURN_IF_WITH_LOG(thermalInterface_ == nullptr, "failed to get thermal hdi interface");
    }

    sptr<IFanCallback> callback = new FanCallback();
    FanCallback::FanEventCallback eventCb =
        std::bind(&ThermalService::HandleFanCallbackEvent, this, std::placeholders::_1);
    FanCallback::RegisterFanEvent(eventCb);
    int32_t ret = thermalInterface_->RegisterFanCallback(callback);
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

std::string ThermalService::ShellDump(const std::vector<std::string>& args, uint32_t argc)
{
    if (!Permission::IsSystem() || !isBootCompleted_) {
        return "";
    }
    std::lock_guard<std::mutex> lock(mutex_);
    pid_t pid = IPCSkeleton::GetCallingPid();
    THERMAL_HILOGI(COMP_SVC, "PID: %{public}d", pid);
    std::string result;
    bool ret = ThermalMgrDumper::Dump(args, result);
    THERMAL_HILOGI(COMP_SVC, "ThermalMgrDumper :%{public}d", ret);
    return result;
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
} // namespace PowerMgr
} // namespace OHOS
