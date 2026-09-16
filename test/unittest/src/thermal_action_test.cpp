/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "thermal_action_test.h"

#ifdef THERMAL_GTEST
#define private   public
#define protected   public
#endif

#include <memory>
#include <singleton.h>
#include "action_airplane.h"
#include "action_application_process.h"
#include "action_charger.h"
#include "action_cpu_big.h"
#include "action_cpu_med.h"
#include "action_cpu_lit.h"
#include "action_cpu_isolate.h"
#include "action_display.h"
#include "action_gpu.h"
#include "action_node.h"
#include "action_shutdown.h"
#include "action_thermal_level.h"
#include "action_popup.h"
#include "action_socperf.h"
#include "action_socperf_resource.h"
#include "action_volume.h"
#include "action_voltage.h"
#include "file_operation.h"
#include "iremote_object.h"
#include "mock_thermal_remote_object.h"
#include "thermal_service.h"
#include "thermal_timer.h"

using namespace testing::ext;
using namespace OHOS::PowerMgr;
using namespace OHOS;
using namespace std;

namespace {
constexpr int32_t BUF_LEN = 32;
static sptr<ThermalService> g_service = nullptr;
std::shared_ptr<ActionApplicationProcess> g_actionApplicationProcess =
    std::make_shared<ActionApplicationProcess>("process_ctrl");
std::shared_ptr<ActionAirplane> g_actionAirplane = std::make_shared<ActionAirplane>("airplane");
std::shared_ptr<ActionCharger> g_actionCharger = std::make_shared<ActionCharger>("current_sc");
std::shared_ptr<ActionCpuBig> g_actionCpuBig = std::make_shared<ActionCpuBig>("cpu_big");
std::shared_ptr<ActionCpuMed> g_actionCpuMed = std::make_shared<ActionCpuMed>("cpu_med");
std::shared_ptr<ActionCpuLit> g_actionCpuLit = std::make_shared<ActionCpuLit>("cpu_lit");
std::shared_ptr<ActionCpuIsolate> g_actionCpuIsolate = std::make_shared<ActionCpuIsolate>("isolate");
std::shared_ptr<ActionDisplay> g_actionDisplay = std::make_shared<ActionDisplay>("lcd");
std::shared_ptr<ActionGpu> g_actionGpu = std::make_shared<ActionGpu>("gpu");
std::shared_ptr<ActionPopup> g_actionPopup = std::make_shared<ActionPopup>("popup");
std::shared_ptr<ActionShutdown> g_actionShutdown = std::make_shared<ActionShutdown>("shut_down");
std::shared_ptr<ActionSocPerf> g_actionSocPerf = std::make_shared<ActionSocPerf>("heca");
std::shared_ptr<ActionThermalLevel> g_actionThermalLevel = std::make_shared<ActionThermalLevel>("thermallevel");
std::shared_ptr<ActionSocPerfResource> g_actionSocPerfResource =
    std::make_shared<ActionSocPerfResource>("sustainable_power");
std::shared_ptr<ActionVolume> g_actionVolume = std::make_shared<ActionVolume>("volume");
std::shared_ptr<ActionVoltage> g_actionVoltage = std::make_shared<ActionVoltage>("voltage");
std::shared_ptr<ActionNode> g_actionNode = std::make_shared<ActionNode>("action_node");
}

void ThermalActionTest::SetUpTestCase()
{
    g_service = ThermalService::GetInstance();
    g_service->InitSystemTestModules();
    g_service->OnStart();
}

void ThermalActionTest::TearDownTestCase()
{
    g_service->OnStop();
}

void ThermalActionTest::SetUp() {}

void ThermalActionTest::TearDown() {}

namespace {
/**
 * @tc.name: ThermalActionTest001
 * @tc.desc: Action Volume Function Test
 * @tc.type: FUNC
 */
HWTEST_F(ThermalActionTest, ThermalActionTest001, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest001 function start!");
    g_actionVolume->InitParams("volume");
    g_actionVolume->SetEnableEvent(false);
    g_actionVolume->AddActionValue(0, "");
    g_actionVolume->AddActionValue(0, "1.0");
    g_actionVolume->Execute();
    g_actionVolume->SetStrict(true);
    g_actionVolume->AddActionValue(0, "2.0");
    g_actionVolume->GetActionValue();
    g_actionVolume->VolumeRequest(1.0);
    int32_t ret = g_actionVolume->VolumeExecution(1.0);
    EXPECT_TRUE(ret == ERR_OK);
    std::shared_ptr<IThermalAction> g_actionVolume2 = g_actionVolume;
    PolicyDelayAction delayAction;
    delayAction.delayTime = 10000;
    g_actionVolume2->AddActionDelayTime(1, delayAction);
    g_actionVolume2->AddActionValue(1, "1.0");
    PolicyDelayAction delayAction2;
    delayAction2.delayTime = 10000;
    g_actionVolume2->AddActionDelayTime(2, delayAction2);
    g_actionVolume2->AddActionValue(2, "1.0");
    g_actionVolume2->Execute();
    PolicyDelayAction delayAction3;
    delayAction3.delayTime = 10000;
    g_actionVolume2->AddActionDelayTime(2, delayAction3);
    g_actionVolume2->AddActionValue(2, "1.0");
    g_actionVolume2->Execute();
    PolicyDelayAction delayAction4;
    delayAction4.delayTime = 10000;
    g_actionVolume2->AddActionDelayTime(4, delayAction4);
    g_actionVolume2->AddActionValue(4, "1.0");
    g_actionVolume2->Execute();
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest001 function end!");
}

/**
 * @tc.name: ThermalActionTest002
 * @tc.desc: Action Voltage Function Test
 * @tc.type: FUNC
 */
HWTEST_F(ThermalActionTest, ThermalActionTest002, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest002 function start!");
    g_actionVoltage->AddActionValue(0, "");
    g_actionVoltage->AddActionValue(0, "1.0");
    g_actionVoltage->Execute();
    g_actionVoltage->SetStrict(true);
    g_actionVoltage->AddActionValue(0, "2.0");
    g_actionVoltage->GetActionValue();
    g_actionVoltage->SetVoltage(123456);
    g_actionVoltage->ExecuteVoltageLimit();
    int32_t ret = g_actionVoltage->WriteMockNode(123456);
    EXPECT_FALSE(ret == ERR_OK);
    PolicyDelayAction delayAction;
    delayAction.delayTime = 10000;
    std::shared_ptr<IThermalAction> g_actionVoltage2 = g_actionVoltage;
    g_actionVoltage2->AddActionDelayTime(1, delayAction);
    g_actionVoltage2->AddActionValue(1, "1.0");
    PolicyDelayAction delayAction2;
    delayAction2.delayTime = 10000;
    g_actionVoltage2->AddActionDelayTime(2, delayAction2);
    g_actionVoltage2->AddActionValue(2, "1.0");
    g_actionVoltage2->Execute();
    PolicyDelayAction delayAction3;
    delayAction3.delayTime = 10000;
    g_actionVoltage2->AddActionDelayTime(2, delayAction3);
    g_actionVoltage2->AddActionValue(2, "1.0");
    g_actionVoltage2->Execute();
    PolicyDelayAction delayAction4;
    delayAction4.delayTime = 10000;
    g_actionVoltage2->AddActionDelayTime(4, delayAction4);
    g_actionVoltage2->AddActionValue(4, "1.0");
    g_actionVoltage2->Execute();
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest002 function end!");
}

/**
 * @tc.name: ThermalActionTest003
 * @tc.desc: Action Thermal Level Function Test
 * @tc.type: FUNC
 */
HWTEST_F(ThermalActionTest, ThermalActionTest003, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest003 function start!");
    g_actionThermalLevel->AddActionValue(0, "");
    g_actionThermalLevel->AddActionValue(0, "1.0");
    g_actionThermalLevel->Execute();
    g_actionThermalLevel->GetThermalLevel();
    g_actionThermalLevel->LevelRequest(1);
    g_actionThermalLevel->LevelRequest(9);
    g_actionThermalLevel->LevelRequest(-1);
    g_actionThermalLevel->SetStrict(true);
    g_actionThermalLevel->AddActionValue(0, "2.0");
    g_actionThermalLevel->GetActionValue();
    g_actionThermalLevel->SubscribeThermalLevelCallback(nullptr);
    g_actionThermalLevel->UnSubscribeThermalLevelCallback(nullptr);
    std::shared_ptr<IRemoteObject::DeathRecipient> deathRecipient =
        std::make_shared<ActionThermalLevel::ThermalLevelCallbackDeathRecipient>();
    wptr<IRemoteObject> remoteObj = nullptr;
    EXPECT_NE(deathRecipient, nullptr);
    deathRecipient->OnRemoteDied(remoteObj);

    sptr<IRemoteObject> sptrRemoteObj = new MockThermalRemoteObject();
    EXPECT_FALSE(sptrRemoteObj == nullptr);
    deathRecipient->OnRemoteDied(sptrRemoteObj);

    g_actionThermalLevel->NotifyThermalLevelChanged(1);
    bool ret = g_actionThermalLevel->
        PublishLevelChangedEvents(ThermalCommonEventCode::CODE_THERMAL_LEVEL_CHANGED, 1);
    EXPECT_TRUE(ret);
    PolicyDelayAction delayAction;
    delayAction.delayTime = 10000;
    std::shared_ptr<IThermalAction> g_actionThermalLevel2 = g_actionThermalLevel;
    g_actionThermalLevel2->AddActionDelayTime(1, delayAction);
    g_actionThermalLevel2->AddActionValue(1, "1.0");
    PolicyDelayAction delayAction2;
    delayAction2.delayTime = 10000;
    g_actionThermalLevel2->AddActionDelayTime(2, delayAction2);
    g_actionThermalLevel2->AddActionValue(2, "1.0");
    g_actionThermalLevel2->Execute();
    PolicyDelayAction delayAction3;
    delayAction3.delayTime = 10000;
    g_actionThermalLevel2->AddActionDelayTime(2, delayAction3);
    g_actionThermalLevel2->AddActionValue(2, "1.0");
    g_actionThermalLevel2->Execute();
    PolicyDelayAction delayAction4;
    delayAction4.delayTime = 10000;
    g_actionThermalLevel2->AddActionDelayTime(4, delayAction4);
    g_actionThermalLevel2->AddActionValue(4, "1.0");
    g_actionThermalLevel2->Execute();
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest003 function end!");
}

/**
 * @tc.name: ThermalActionTest004
 * @tc.desc: Action Shutdown Function Test
 * @tc.type: FUNC
 */
HWTEST_F(ThermalActionTest, ThermalActionTest004, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest004 function start!");
    g_actionShutdown->AddActionValue(0, "");
    g_actionShutdown->AddActionValue(0, "1.0");
    g_actionShutdown->Execute();
    g_actionShutdown->SetStrict(true);
    g_actionShutdown->AddActionValue(0, "2.0");
    g_actionShutdown->GetActionValue();
    int32_t ret = g_actionShutdown->ShutdownRequest(false);
    EXPECT_TRUE(ret == ERR_OK);
    g_actionShutdown->ShutdownExecution(false);
    g_actionShutdown->ShutdownExecution(false);
    ret = g_actionShutdown->DelayShutdown(false, 0, 0);
    EXPECT_TRUE(ret == ERR_OK);
    PolicyDelayAction delayAction;
    delayAction.delayTime = 10000;
    std::shared_ptr<IThermalAction> g_actionShutdown2 = g_actionShutdown;
    g_actionShutdown2->AddActionDelayTime(1, delayAction);
    g_actionShutdown2->AddActionValue(1, "1.0");
    PolicyDelayAction delayAction2;
    delayAction2.delayTime = 10000;
    g_actionShutdown2->AddActionDelayTime(2, delayAction2);
    g_actionShutdown2->AddActionValue(2, "1.0");
    g_actionShutdown2->Execute();
    PolicyDelayAction delayAction3;
    delayAction3.delayTime = 10000;
    g_actionShutdown2->AddActionDelayTime(2, delayAction3);
    g_actionShutdown2->AddActionValue(2, "1.0");
    g_actionShutdown2->Execute();
    PolicyDelayAction delayAction4;
    delayAction4.delayTime = 10000;
    g_actionShutdown2->AddActionDelayTime(4, delayAction4);
    g_actionShutdown2->AddActionValue(4, "1.0");
    g_actionShutdown2->Execute();
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest004 function end!");
}

/**
 * @tc.name: ThermalActionTest005
 * @tc.desc: Action Display Function Test
 * @tc.type: FUNC
 * @tc.require: issueI6KRS8
 */
HWTEST_F(ThermalActionTest, ThermalActionTest005, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest005 function start!");
    g_actionDisplay->AddActionValue(0, "");
    g_actionDisplay->AddActionValue(0, "1.0");
    g_actionDisplay->Execute();
    g_actionDisplay->SetStrict(true);
    g_actionDisplay->AddActionValue(0, "2.0");
    g_actionDisplay->GetActionValue();
    EXPECT_FALSE(g_actionDisplay->valueList_.empty());
    PolicyDelayAction delayAction;
    delayAction.delayTime = 10000;
    std::shared_ptr<IThermalAction> g_actionDisplay2 = g_actionDisplay;
    g_actionDisplay2->AddActionDelayTime(1, delayAction);
    g_actionDisplay2->AddActionValue(1, "1.0");
    PolicyDelayAction delayAction2;
    delayAction2.delayTime = 10000;
    g_actionDisplay2->AddActionDelayTime(2, delayAction2);
    g_actionDisplay2->AddActionValue(2, "1.0");
    g_actionDisplay2->Execute();
    PolicyDelayAction delayAction3;
    delayAction3.delayTime = 10000;
    g_actionDisplay2->AddActionDelayTime(2, delayAction3);
    g_actionDisplay2->AddActionValue(2, "1.0");
    g_actionDisplay2->Execute();
    PolicyDelayAction delayAction4;
    delayAction4.delayTime = 10000;
    g_actionDisplay2->AddActionDelayTime(4, delayAction4);
    g_actionDisplay2->AddActionValue(4, "1.0");
    g_actionDisplay2->Execute();
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest005 function end!");
}

/**
 * @tc.name: ThermalActionTest006
 * @tc.desc: Action Charger Function Test
 * @tc.type: FUNC
 */
HWTEST_F(ThermalActionTest, ThermalActionTest006, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest006 function start!");
    g_actionCharger->AddActionValue(0, "");
    g_actionCharger->AddActionValue(0, "1.0");
    g_actionCharger->Execute();
    g_actionCharger->ChargerRequest(0);
    g_actionCharger->ExecuteCurrentLimit();
    int32_t ret = g_actionCharger->WriteSimValue(0);
    EXPECT_TRUE(ret);
    PolicyDelayAction delayAction;
    delayAction.delayTime = 10000;
    std::shared_ptr<IThermalAction> g_actionCharger2 = g_actionCharger;
    g_actionCharger2->AddActionDelayTime(1, delayAction);
    g_actionCharger2->AddActionValue(1, "1.0");
    PolicyDelayAction delayAction2;
    delayAction2.delayTime = 10000;
    g_actionCharger2->AddActionDelayTime(2, delayAction2);
    g_actionCharger2->AddActionValue(2, "1.0");
    g_actionCharger2->Execute();
    PolicyDelayAction delayAction3;
    delayAction3.delayTime = 10000;
    g_actionCharger2->AddActionDelayTime(2, delayAction3);
    g_actionCharger2->AddActionValue(2, "1.0");
    g_actionCharger2->Execute();
    PolicyDelayAction delayAction4;
    delayAction4.delayTime = 10000;
    g_actionCharger2->AddActionDelayTime(4, delayAction4);
    g_actionCharger2->AddActionValue(4, "1.0");
    g_actionCharger2->Execute();
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest006 function end!");
}

/**
 * @tc.name: ThermalActionTest007
 * @tc.desc: Action Application Process Test
 * @tc.type: FUNC
 */
HWTEST_F(ThermalActionTest, ThermalActionTest007, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest007 function start!");
    g_actionApplicationProcess->InitParams("");
    g_actionApplicationProcess->AddActionValue(0, "");
    g_actionApplicationProcess->AddActionValue(0, "1");
    g_actionApplicationProcess->Execute();
    g_actionApplicationProcess->KillApplicationAction("");
    g_actionApplicationProcess->ProcessAppActionRequest(0);
    g_actionApplicationProcess->ProcessAppActionRequest(1);
    g_actionApplicationProcess->ProcessAppActionRequest(2);
    g_actionApplicationProcess->ProcessAppActionRequest(3);
    g_actionApplicationProcess->ProcessAppActionExecution(0);
    EXPECT_TRUE(g_actionApplicationProcess->valueList_.empty());
    PolicyDelayAction delayAction;
    delayAction.delayTime = 10000;
    std::shared_ptr<IThermalAction> g_actionApplicationProcess2 = g_actionApplicationProcess;
    g_actionApplicationProcess2->AddActionDelayTime(1, delayAction);
    g_actionApplicationProcess2->AddActionValue(1, "1.0");
    PolicyDelayAction delayAction2;
    delayAction2.delayTime = 10000;
    g_actionApplicationProcess2->AddActionDelayTime(2, delayAction2);
    g_actionApplicationProcess2->AddActionValue(2, "1.0");
    g_actionApplicationProcess2->Execute();
    PolicyDelayAction delayAction3;
    delayAction3.delayTime = 10000;
    g_actionApplicationProcess2->AddActionDelayTime(2, delayAction3);
    g_actionApplicationProcess2->AddActionValue(2, "1.0");
    g_actionApplicationProcess2->Execute();
    PolicyDelayAction delayAction4;
    delayAction4.delayTime = 10000;
    g_actionApplicationProcess2->AddActionDelayTime(4, delayAction4);
    g_actionApplicationProcess2->AddActionValue(4, "1.0");
    g_actionApplicationProcess2->Execute();
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest007 function end!");
}

/**
 * @tc.name: ThermalActionTest008
 * @tc.desc: Action CPU Big Test
 * @tc.type: FUNC
 * @tc.require: issueI6KRS8
 */
HWTEST_F(ThermalActionTest, ThermalActionTest008, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest008 function start!");
    g_actionCpuBig->AddActionValue(0, "");
    g_actionCpuBig->AddActionValue(0, "1.0");
    g_actionCpuBig->Execute();
    EXPECT_TRUE(g_actionCpuBig->valueList_.empty());
    PolicyDelayAction delayAction;
    delayAction.delayTime = 10000;
    std::shared_ptr<IThermalAction> g_actionCpuBig2 = g_actionCpuBig;
    g_actionCpuBig2->AddActionDelayTime(1, delayAction);
    g_actionCpuBig2->AddActionValue(1, "1.0");
    PolicyDelayAction delayAction2;
    delayAction2.delayTime = 10000;
    g_actionCpuBig2->AddActionDelayTime(2, delayAction2);
    g_actionCpuBig2->AddActionValue(2, "1.0");
    g_actionCpuBig2->Execute();
    PolicyDelayAction delayAction3;
    delayAction3.delayTime = 10000;
    g_actionCpuBig2->AddActionDelayTime(2, delayAction3);
    g_actionCpuBig2->AddActionValue(2, "1.0");
    g_actionCpuBig2->Execute();
    PolicyDelayAction delayAction4;
    delayAction4.delayTime = 10000;
    g_actionCpuBig2->AddActionDelayTime(4, delayAction4);
    g_actionCpuBig2->AddActionValue(4, "1.0");
    g_actionCpuBig2->Execute();
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest008 function end!");
}

/**
 * @tc.name: ThermalActionTest009
 * @tc.desc: Action CPU Med Test
 * @tc.type: FUNC
 * @tc.require: issueI6KRS8
 */
HWTEST_F(ThermalActionTest, ThermalActionTest009, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest009 function start!");
    g_actionCpuMed->AddActionValue(0, "");
    g_actionCpuMed->AddActionValue(0, "1.0");
    g_actionCpuMed->Execute();
    EXPECT_TRUE(g_actionCpuMed->valueList_.empty());
    PolicyDelayAction delayAction;
    delayAction.delayTime = 10000;
    std::shared_ptr<IThermalAction> g_actionCpuMed2 = g_actionCpuMed;
    g_actionCpuMed2->AddActionDelayTime(1, delayAction);
    g_actionCpuMed2->AddActionValue(1, "1.0");
    PolicyDelayAction delayAction2;
    delayAction2.delayTime = 10000;
    g_actionCpuMed2->AddActionDelayTime(2, delayAction2);
    g_actionCpuMed2->AddActionValue(2, "1.0");
    g_actionCpuMed2->Execute();
    PolicyDelayAction delayAction3;
    delayAction3.delayTime = 10000;
    g_actionCpuMed2->AddActionDelayTime(2, delayAction3);
    g_actionCpuMed2->AddActionValue(2, "1.0");
    g_actionCpuMed2->Execute();
    PolicyDelayAction delayAction4;
    delayAction4.delayTime = 10000;
    g_actionCpuMed2->AddActionDelayTime(4, delayAction4);
    g_actionCpuMed2->AddActionValue(4, "1.0");
    g_actionCpuMed2->Execute();
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest009 function end!");
}

/**
 * @tc.name: ThermalActionTest010
 * @tc.desc: Action CPU Lit Test
 * @tc.type: FUNC
 * @tc.require: issueI6KRS8
 */
HWTEST_F(ThermalActionTest, ThermalActionTest010, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest010 function start!");
    g_actionCpuLit->AddActionValue(0, "");
    g_actionCpuLit->AddActionValue(0, "1.0");
    g_actionCpuLit->Execute();
    EXPECT_TRUE(g_actionCpuLit->valueList_.empty());
    PolicyDelayAction delayAction;
    delayAction.delayTime = 10000;
    std::shared_ptr<IThermalAction> g_actionCpuLit2 = g_actionCpuLit;
    g_actionCpuLit2->AddActionDelayTime(1, delayAction);
    g_actionCpuLit2->AddActionValue(1, "1.0");
    PolicyDelayAction delayAction2;
    delayAction2.delayTime = 10000;
    g_actionCpuLit2->AddActionDelayTime(2, delayAction2);
    g_actionCpuLit2->AddActionValue(2, "1.0");
    g_actionCpuLit2->Execute();
    PolicyDelayAction delayAction3;
    delayAction3.delayTime = 10000;
    g_actionCpuLit2->AddActionDelayTime(2, delayAction3);
    g_actionCpuLit2->AddActionValue(2, "1.0");
    g_actionCpuLit2->Execute();
    PolicyDelayAction delayAction4;
    delayAction4.delayTime = 10000;
    g_actionCpuLit2->AddActionDelayTime(4, delayAction4);
    g_actionCpuLit2->AddActionValue(4, "1.0");
    g_actionCpuLit2->Execute();
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest010 function end!");
}

/**
 * @tc.name: ThermalActionTest011
 * @tc.desc: Action GPU Test
 * @tc.type: FUNC
 * @tc.require: issueI6KRS8
 */
HWTEST_F(ThermalActionTest, ThermalActionTest011, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest011 function start!");
    g_actionGpu->AddActionValue(0, "");
    g_actionGpu->AddActionValue(0, "1.0");
    g_actionGpu->Execute();
    EXPECT_TRUE(g_actionGpu->valueList_.empty());
    PolicyDelayAction delayAction;
    delayAction.delayTime = 10000;
    std::shared_ptr<IThermalAction> g_actionGpu2 = g_actionGpu;
    g_actionGpu2->AddActionDelayTime(1, delayAction);
    g_actionGpu2->AddActionValue(1, "1.0");
    PolicyDelayAction delayAction2;
    delayAction2.delayTime = 10000;
    g_actionGpu2->AddActionDelayTime(2, delayAction2);
    g_actionGpu2->AddActionValue(2, "1.0");
    g_actionGpu2->Execute();
    PolicyDelayAction delayAction3;
    delayAction3.delayTime = 10000;
    g_actionGpu2->AddActionDelayTime(2, delayAction3);
    g_actionGpu2->AddActionValue(2, "1.0");
    g_actionGpu2->Execute();
    PolicyDelayAction delayAction4;
    delayAction4.delayTime = 10000;
    g_actionGpu2->AddActionDelayTime(4, delayAction4);
    g_actionGpu2->AddActionValue(4, "1.0");
    g_actionGpu2->Execute();
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest011 function end!");
}

/**
 * @tc.name: ThermalActionTest012
 * @tc.desc: Action Isolate CPU Test
 * @tc.type: FUNC
  */
HWTEST_F(ThermalActionTest, ThermalActionTest012, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest012 function start!");
    g_actionCpuIsolate->AddActionValue(0, "");
    g_actionCpuIsolate->AddActionValue(0, "1.0");
    g_actionCpuIsolate->Execute();
    g_actionCpuIsolate->SetStrict(true);
    g_actionCpuIsolate->AddActionValue(0, "2.0");
    g_actionCpuIsolate->GetActionValue();
    EXPECT_FALSE(g_actionCpuIsolate->valueList_.empty());
    PolicyDelayAction delayAction;
    delayAction.delayTime = 10000;
    std::shared_ptr<IThermalAction> g_actionCpuIsolate2 = g_actionCpuIsolate;
    g_actionCpuIsolate2->AddActionDelayTime(1, delayAction);
    g_actionCpuIsolate2->AddActionValue(1, "1.0");
    PolicyDelayAction delayAction2;
    delayAction2.delayTime = 10000;
    g_actionCpuIsolate2->AddActionDelayTime(2, delayAction2);
    g_actionCpuIsolate2->AddActionValue(2, "1.0");
    g_actionCpuIsolate2->Execute();
    PolicyDelayAction delayAction3;
    delayAction3.delayTime = 10000;
    g_actionCpuIsolate2->AddActionDelayTime(2, delayAction3);
    g_actionCpuIsolate2->AddActionValue(2, "1.0");
    g_actionCpuIsolate2->Execute();
    PolicyDelayAction delayAction4;
    delayAction4.delayTime = 10000;
    g_actionCpuIsolate2->AddActionDelayTime(4, delayAction4);
    g_actionCpuIsolate2->AddActionValue(4, "1.0");
    g_actionCpuIsolate2->Execute();
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest012 function end!");
}

/**
 * @tc.name: ThermalActionTest013
 * @tc.desc: Action node Test
 * @tc.type: FUNC
  */
HWTEST_F(ThermalActionTest, ThermalActionTest013, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest013 function start!");
    std::string input = "1";
    g_actionNode->InitParams("/data/service/el0/thermal/config/lcd");
    g_actionNode->AddActionValue(0, input);
    g_actionNode->Execute();
    char buf[BUF_LEN];
    FileOperation::ReadFile("/data/service/el0/thermal/config/lcd", buf, BUF_LEN);
    std::string ret = buf;
    EXPECT_EQ(input, ret);
    PolicyDelayAction delayAction;
    delayAction.delayTime = 10000;
    std::shared_ptr<IThermalAction> g_actionNode2 = g_actionNode;
    g_actionNode2->AddActionDelayTime(1, delayAction);
    g_actionNode2->AddActionValue(1, "1.0");
    PolicyDelayAction delayAction2;
    delayAction2.delayTime = 10000;
    g_actionNode2->AddActionDelayTime(2, delayAction2);
    g_actionNode2->AddActionValue(2, "1.0");
    g_actionNode2->Execute();
    PolicyDelayAction delayAction3;
    delayAction3.delayTime = 10000;
    g_actionNode2->AddActionDelayTime(2, delayAction3);
    g_actionNode2->AddActionValue(2, "1.0");
    g_actionNode2->Execute();
    PolicyDelayAction delayAction4;
    delayAction4.delayTime = 10000;
    g_actionNode2->AddActionDelayTime(4, delayAction4);
    g_actionNode2->AddActionValue(4, "1.0");
    g_actionNode2->Execute();
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest013 function end!");
}

/**
 * @tc.name: ThermalActionTest014
 * @tc.desc: Action Airplane Function Test
 * @tc.type: FUNC
 */
HWTEST_F(ThermalActionTest, ThermalActionTest014, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest014 function start!");
    g_actionAirplane->InitParams("airplane");
    g_actionAirplane->AddActionValue(0, "0");
    g_actionAirplane->AddActionValue(0, "0");
    g_actionAirplane->AddActionValue(0, "air");
    g_actionAirplane->Execute();
    EXPECT_TRUE(g_actionAirplane->valueList_.empty());
    std::string input = "1";
    g_actionAirplane->AddActionValue(0, input);
    g_actionCpuIsolate->SetStrict(true);
    uint32_t value = g_actionAirplane->GetActionValue();
    int32_t ret = g_actionAirplane->AirplaneRequest(value);
    g_actionAirplane->AirplaneRequest(0);
    g_actionAirplane->AirplaneRequest(1);
    EXPECT_TRUE(ret == ERR_OK);
    ret = g_actionAirplane->AirplaneExecution(value);
    EXPECT_TRUE(ret == ERR_OK);
    PolicyDelayAction delayAction;
    delayAction.delayTime = 10000;
    std::shared_ptr<IThermalAction> g_actionAirplane2 = g_actionAirplane;
    g_actionAirplane2->AddActionDelayTime(1, delayAction);
    g_actionAirplane2->AddActionValue(1, "1.0");
    PolicyDelayAction delayAction2;
    delayAction2.delayTime = 10000;
    g_actionAirplane2->AddActionDelayTime(2, delayAction2);
    g_actionAirplane2->AddActionValue(2, "1.0");
    g_actionAirplane2->Execute();
    PolicyDelayAction delayAction3;
    delayAction3.delayTime = 10000;
    g_actionAirplane2->AddActionDelayTime(2, delayAction3);
    g_actionAirplane2->AddActionValue(2, "1.0");
    g_actionAirplane2->Execute();
    PolicyDelayAction delayAction4;
    delayAction4.delayTime = 10000;
    g_actionAirplane2->AddActionDelayTime(4, delayAction4);
    g_actionAirplane2->AddActionValue(4, "1.0");
    g_actionAirplane2->Execute();
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest014 function end!");
}

/**
 * @tc.name: ThermalActionTest015
 * @tc.desc: Action Timer Test
 * @tc.type: FUNC
 */
HWTEST_F(ThermalActionTest, ThermalActionTest015, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest015 function start!");
    auto timerInfo = std::make_shared<ThermalTimerInfo>();
    ASSERT_NE(timerInfo, nullptr);
    OHOS::PowerMgr::ThermalTimerInfo::TimerInfoCallback callback;
    timerInfo->SetCallbackInfo(callback);
    timerInfo->OnTrigger();
    auto timer = std::make_shared<ThermalTimer>();
    ASSERT_NE(timer, nullptr);
    uint64_t triggerTime = 1;
    std::shared_ptr<ThermalTimerInfo> info;
    uint64_t timerId = timer->CreateTimer(info);
    EXPECT_EQ(timerId, 0);
    timer->StartTimer(timerId, triggerTime);
    timer->DestroyTimer(timerId);
    timer->StopTimer(timerId);
    auto timerUtils = std::make_shared<ThermalTimerUtils>();
    ASSERT_NE(timerUtils, nullptr);
    timerUtils->Stop();
    int delay = 1;
    OHOS::PowerMgr::ThermalTimerUtils::NotifyTask task;
    timerUtils->Start(delay, task);
    timerUtils->Start(delay, task);
    timerUtils->Stop();
    timerUtils->Stop();
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest015 function end!");
}

/**
 * @tc.name: ThermalActionTest016
 * @tc.desc: Action Popup Test
 * @tc.type: FUNC
 */
HWTEST_F(ThermalActionTest, ThermalActionTest016, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest016 function start!");
    g_actionPopup->AddActionValue(0, "pop");
    g_actionPopup->Execute();
    EXPECT_TRUE(g_actionPopup->valueList_.empty());
    PolicyDelayAction delayAction;
    delayAction.delayTime = 10000;
    std::shared_ptr<IThermalAction> g_actionPopup2 = g_actionPopup;
    g_actionPopup2->AddActionDelayTime(1, delayAction);
    g_actionPopup2->AddActionValue(1, "1.0");
    PolicyDelayAction delayAction2;
    delayAction2.delayTime = 10000;
    g_actionPopup2->AddActionDelayTime(2, delayAction2);
    g_actionPopup2->AddActionValue(2, "1.0");
    g_actionPopup2->Execute();
    PolicyDelayAction delayAction3;
    delayAction3.delayTime = 10000;
    g_actionPopup2->AddActionDelayTime(2, delayAction3);
    g_actionPopup2->AddActionValue(2, "1.0");
    g_actionPopup2->Execute();
    PolicyDelayAction delayAction4;
    delayAction4.delayTime = 10000;
    g_actionPopup2->AddActionDelayTime(4, delayAction4);
    g_actionPopup2->AddActionValue(4, "1.0");
    g_actionPopup2->Execute();
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest016 function end!");
}

/**
 * @tc.name: ThermalActionTest017
 * @tc.desc: Action SocPerf Test
 * @tc.type: FUNC
 */
HWTEST_F(ThermalActionTest, ThermalActionTest017, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest017 function start!");
    g_actionSocPerf->InitParams("12363");
    EXPECT_EQ(12363, g_actionSocPerf->cmdId_);
    g_actionSocPerf->SetStrict(false);
    g_actionSocPerf->SetEnableEvent(false);
    g_actionSocPerf->AddActionValue(0, "");
    g_actionSocPerf->Execute();
    EXPECT_TRUE(g_actionSocPerf->valueList_.empty());
    g_actionSocPerf->AddActionValue(0, "1");
    g_actionSocPerf->Execute();
    g_actionSocPerf->AddActionValue(1, "1");
    g_actionSocPerf->Execute();
    g_actionSocPerf->ResetActionValue();
    EXPECT_EQ(0, g_actionSocPerf->lastValue_);
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest017 function end!");
}
 
/*
 * @tc.name: ThermalActionTest018
 * @tc.desc: Action SocPerf Resource Test
 * @tc.type: FUNC
 */
HWTEST_F(ThermalActionTest, ThermalActionTest018, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest018 function start!");
    g_actionSocPerfResource->InitParams("1031");
    EXPECT_EQ(1031, g_actionSocPerfResource->resId_);
    g_actionSocPerfResource->InitParams("1031|40000");
    EXPECT_EQ(1031, g_actionSocPerfResource->resId_);
    EXPECT_EQ(40000, g_actionSocPerfResource->fallbackValue_);
    g_actionSocPerfResource->SetStrict(false);
    g_actionSocPerfResource->SetEnableEvent(false);
    g_actionSocPerfResource->AddActionValue(0, "");
    g_actionSocPerfResource->AddActionValue(0, "abc");
    g_actionSocPerfResource->Execute();
    EXPECT_TRUE(g_actionSocPerfResource->valueList_.empty());
    g_actionSocPerfResource->AddActionValue(0, "20000");
    g_actionSocPerfResource->Execute();
    g_actionSocPerfResource->SetStrict(true);
    g_actionSocPerfResource->AddActionValue(0, "20000");
    g_actionSocPerfResource->Execute();
    g_actionSocPerfResource->AddActionValue(1, "20000");
    g_actionSocPerfResource->Execute();
    PolicyDelayAction policyDelayAction;
    policyDelayAction.isCompleted = true;
    policyDelayAction.uintDelayValue =10000;
    g_actionSocPerfResource->AddActionDelayTime(1, policyDelayAction);
    g_actionSocPerfResource->AddActionValue(1, "20000");
    g_actionSocPerfResource->Execute();
    g_actionSocPerfResource->ResetActionValue();
    EXPECT_EQ(0, g_actionSocPerfResource->lastValue_);
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest018 function end!");
}

/**
 * @tc.name: ThermalActionTest019
 * @tc.desc: Action Thermal Level Function Test
 * @tc.type: FUNC
 */
HWTEST_F(ThermalActionTest, ThermalActionTest019, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest019 function start!");
    sptr<IThermalLevelCallback> levelCallback = new ThermalLevelCallbackStub();
    EXPECT_FALSE(levelCallback == nullptr);
    g_actionThermalLevel->SubscribeThermalLevelCallback(levelCallback, false);
    g_actionThermalLevel->NotifyThermalLevelChanged(1);
    EXPECT_FALSE(g_actionThermalLevel->asyncThermalLevelListeners_.empty());
    g_actionThermalLevel->SubscribeThermalLevelCallback(levelCallback, false);
    EXPECT_FALSE(g_actionThermalLevel->asyncThermalLevelListeners_.empty());
    g_actionThermalLevel->UnSubscribeThermalLevelCallback(levelCallback);
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest019 function end!");
}

/**
 * @tc.name: ThermalActionTest020
 * @tc.desc: Test sync callback subscription success
 * @tc.type: FUNC
 */
HWTEST_F(ThermalActionTest, ThermalActionTest020, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest020 function start!");
    sptr<IThermalLevelCallback> levelCallback = new ThermalLevelCallbackStub();
    EXPECT_FALSE(levelCallback == nullptr);

    size_t beforeSize = g_actionThermalLevel->syncThermalLevelListeners_.size();
    g_actionThermalLevel->SubscribeThermalLevelCallback(levelCallback, true);

    EXPECT_EQ(g_actionThermalLevel->syncThermalLevelListeners_.size(), beforeSize + 1);
    g_actionThermalLevel->UnSubscribeThermalLevelCallback(levelCallback);
    EXPECT_EQ(g_actionThermalLevel->syncThermalLevelListeners_.size(), beforeSize);
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest020 function end!");
}

/**
 * @tc.name: ThermalActionTest021
 * @tc.desc: Test subscribe with null callback does not crash
 * @tc.type: FUNC
 */
HWTEST_F(ThermalActionTest, ThermalActionTest021, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest021 function start!");
    sptr<IThermalLevelCallback> nullCallback = nullptr;
    size_t syncBefore = g_actionThermalLevel->syncThermalLevelListeners_.size();
    size_t asyncBefore = g_actionThermalLevel->asyncThermalLevelListeners_.size();

    g_actionThermalLevel->SubscribeThermalLevelCallback(nullCallback, true);
    g_actionThermalLevel->SubscribeThermalLevelCallback(nullCallback, false);

    EXPECT_EQ(g_actionThermalLevel->syncThermalLevelListeners_.size(), syncBefore);
    EXPECT_EQ(g_actionThermalLevel->asyncThermalLevelListeners_.size(), asyncBefore);
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest021 function end!");
}

/**
 * @tc.name: ThermalActionTest022
 * @tc.desc: Test duplicate sync subscription does not increase listener count
 * @tc.type: FUNC
 */
HWTEST_F(ThermalActionTest, ThermalActionTest022, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest022 function start!");
    sptr<IThermalLevelCallback> levelCallback = new ThermalLevelCallbackStub();
    EXPECT_FALSE(levelCallback == nullptr);

    g_actionThermalLevel->SubscribeThermalLevelCallback(levelCallback, true);
    size_t afterFirst = g_actionThermalLevel->syncThermalLevelListeners_.size();

    g_actionThermalLevel->SubscribeThermalLevelCallback(levelCallback, true);
    EXPECT_EQ(g_actionThermalLevel->syncThermalLevelListeners_.size(), afterFirst);

    g_actionThermalLevel->UnSubscribeThermalLevelCallback(levelCallback);
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest022 function end!");
}

/**
 * @tc.name: ThermalActionTest023
 * @tc.desc: Test duplicate async subscription does not increase listener count
 * @tc.type: FUNC
 */
HWTEST_F(ThermalActionTest, ThermalActionTest023, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest023 function start!");
    sptr<IThermalLevelCallback> levelCallback = new ThermalLevelCallbackStub();
    EXPECT_FALSE(levelCallback == nullptr);

    g_actionThermalLevel->SubscribeThermalLevelCallback(levelCallback, false);
    size_t afterFirst = g_actionThermalLevel->asyncThermalLevelListeners_.size();

    g_actionThermalLevel->SubscribeThermalLevelCallback(levelCallback, false);
    EXPECT_EQ(g_actionThermalLevel->asyncThermalLevelListeners_.size(), afterFirst);

    g_actionThermalLevel->UnSubscribeThermalLevelCallback(levelCallback);
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest023 function end!");
}

/**
 * @tc.name: ThermalActionTest024
 * @tc.desc: Test GetThermalLevelListeners returns correct copies for sync and async
 * @tc.type: FUNC
 */
HWTEST_F(ThermalActionTest, ThermalActionTest024, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest024 function start!");
    sptr<IThermalLevelCallback> syncCallback = new ThermalLevelCallbackStub();
    sptr<IThermalLevelCallback> asyncCallback = new ThermalLevelCallbackStub();
    EXPECT_FALSE(syncCallback == nullptr);
    EXPECT_FALSE(asyncCallback == nullptr);

    g_actionThermalLevel->SubscribeThermalLevelCallback(syncCallback, true);
    g_actionThermalLevel->SubscribeThermalLevelCallback(asyncCallback, false);

    auto syncListeners = g_actionThermalLevel->GetThermalLevelListeners(true);
    auto asyncListeners = g_actionThermalLevel->GetThermalLevelListeners(false);

    EXPECT_EQ(syncListeners.size(), g_actionThermalLevel->syncThermalLevelListeners_.size());
    EXPECT_EQ(asyncListeners.size(), g_actionThermalLevel->asyncThermalLevelListeners_.size());

    bool foundSync = false;
    for (auto& cb : syncListeners) {
        if (cb->AsObject() == syncCallback->AsObject()) {
            foundSync = true;
            break;
        }
    }
    EXPECT_TRUE(foundSync);

    bool foundAsync = false;
    for (auto& cb : asyncListeners) {
        if (cb->AsObject() == asyncCallback->AsObject()) {
            foundAsync = true;
            break;
        }
    }
    EXPECT_TRUE(foundAsync);

    g_actionThermalLevel->UnSubscribeThermalLevelCallback(syncCallback);
    g_actionThermalLevel->UnSubscribeThermalLevelCallback(asyncCallback);
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest024 function end!");
}

/**
 * @tc.name: ThermalActionTest025
 * @tc.desc: Test NotifyThermalLevelChanged triggers callbacks via copied listener sets
 * @tc.type: FUNC
 */
HWTEST_F(ThermalActionTest, ThermalActionTest025, TestSize.Level0)
{
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest025 function start!");
    sptr<IThermalLevelCallback> syncCallback = new ThermalLevelCallbackStub();
    sptr<IThermalLevelCallback> asyncCallback = new ThermalLevelCallbackStub();
    EXPECT_FALSE(syncCallback == nullptr);
    EXPECT_FALSE(asyncCallback == nullptr);

    g_actionThermalLevel->SubscribeThermalLevelCallback(syncCallback, true);
    g_actionThermalLevel->SubscribeThermalLevelCallback(asyncCallback, false);

    EXPECT_FALSE(g_actionThermalLevel->syncThermalLevelListeners_.empty());
    EXPECT_FALSE(g_actionThermalLevel->asyncThermalLevelListeners_.empty());

    g_actionThermalLevel->NotifyThermalLevelChanged(1);

    EXPECT_FALSE(g_actionThermalLevel->syncThermalLevelListeners_.empty());
    EXPECT_FALSE(g_actionThermalLevel->asyncThermalLevelListeners_.empty());

    g_actionThermalLevel->UnSubscribeThermalLevelCallback(syncCallback);
    g_actionThermalLevel->UnSubscribeThermalLevelCallback(asyncCallback);
    THERMAL_HILOGI(LABEL_TEST, "ThermalActionTest025 function end!");
}

} // namespace
