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

#ifndef ACTION_THERMAL_LEVEL_H
#define ACTION_THERMAL_LEVEL_H


#include <functional>
#include <set>
#include <mutex>
#include <common_event_publish_info.h>
#include <want.h>

#include "ithermal_level_callback.h"
#include "thermal_level_info.h"
#include "ithermal_action.h"

namespace OHOS {
namespace PowerMgr {
class ThermalService;

class ActionThermalLevel : public IThermalAction {
public:
    ActionThermalLevel(const std::string& actionName);
    ~ActionThermalLevel() = default;

    void InitParams(const std::string& params) override;
    void SetStrict(bool enable) override;
    void SetEnableEvent(bool enable) override;
    void AddActionValue(uint32_t actionId, std::string value) override;
    void ExecuteInner() override;
    void ResetActionValue() override;
    void SubscribeThermalLevelCallback(const sptr<IThermalLevelCallback>& callback, bool isSync = true);
    void UnSubscribeThermalLevelCallback(const sptr<IThermalLevelCallback>& callback);
    static int32_t GetThermalLevel();

public:
    class ThermalLevelCallbackDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        ThermalLevelCallbackDeathRecipient() = default;
        virtual void OnRemoteDied(const wptr<IRemoteObject>& remote);
        virtual ~ThermalLevelCallbackDeathRecipient() = default;
    };
private:
    struct classcomp {
        bool operator()(const sptr<IThermalLevelCallback>& l, const sptr<IThermalLevelCallback>& r) const
        {
            return l->AsObject() < r->AsObject();
        }
    };

    static int32_t lastValue_;

    int32_t GetActionValue();
    void LevelRequest(int32_t level);
    void NotifyThermalLevelChanged(int32_t level);
    bool PublishLevelChangedEvents(ThermalCommonEventCode code, int32_t level);
    bool AddThermalLevelListeners(const sptr<IThermalLevelCallback>& callback, bool isSync);
    auto GetThermalLevelListeners(bool isSync)
        -> std::set<const sptr<IThermalLevelCallback>, classcomp>;
    std::mutex mutex_;
    sptr<IRemoteObject::DeathRecipient> thermalLevelCBDeathRecipient_;
    std::vector<uint32_t> valueList_;
    std::string params_;
    std::set<const sptr<IThermalLevelCallback>, classcomp> syncThermalLevelListeners_;
    std::set<const sptr<IThermalLevelCallback>, classcomp> asyncThermalLevelListeners_;
};
} // namespace PowerMgr
} // namespace OHOS
#endif // ACTION_APPLICATION_PROCESS_H
