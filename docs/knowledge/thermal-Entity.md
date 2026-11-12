# thermal_manager 实体清单

> 每个实体按"职责 → 代码位置 → 数据结构 → 接口 → 协作关系"展开，行号基于当前主干。总览见 [architecture.md](architecture.md)；跨实体运行机制见 [thermal-Technology.md](thermal-Technology.md)。

## 实体一：ThermalService（SA 3303 服务主体）

### 职责与能力
系统服务入口，继承 SystemAbility 与 IPC 生成 stub（`class ThermalService final : SystemAbility, ThermalSrvStub`，thermal_service.h:56）。负责生命周期编排（配置解析 → 五大组件初始化 → HDI 订阅）、12 个 IPC 方法的落地、HDI 事件入口（温度/风扇/模拟）、场景值与状态上报的鉴权存储。单例（GetInstance + ready_ 防重入，OnStart :86-117）。

### 代码位置
| 文件 | 内容 |
|---|---|
| services/native/include/thermal_service.h | 类声明、Init 家族、成员清单 |
| services/native/src/thermal_service.cpp | OnStart :86、Init :210、InitConfigFile :269、InitModules :319、InitThermalObserver :351、InitThermalPolicy :402、RegisterHdiStatusListener :617、HandleThermalCallbackEvent :692、HandleTempEmulation :710 |

### 数据结构
成员即组件拓扑：observer_（ThermalObserver）、serviceSubscriber_（ThermalServiceSubscriber）、info_（ThermalSensorInfo）、policy_、baseInfo_、state_（StateMachine）、actionMgr_、fanFaultDetect_、configParser_、thermalInterface_（HDI 代理）、popup_（ActionPopup）；静态 scene_ + sceneMutex_；仿真开关 isSimulation_/isTempReport_；HAS_THERMAL_AIRPLANE_MANAGER_PART 下 userAirplaneState_/isThermalAirplane_（thermal_service.h）。

### 接口清单
- IPC 12 方法落地（订阅/退订三类回调、查询传感器/档位、GetThermalInfo、SetScene :587、UpdateThermalState :600、ShellDump）。
- HDI 事件入口：HandleThermalCallbackEvent :692（event.info → TypeTempMap → subscriber）、HandleFanCallbackEvent、HandleTempEmulation :710（isTempReport_ 关闭时吃掉模拟数据）。
- 编排：OnStart 扫描 thermal_decrypt 插件（ModuleMgrScan，用完卸载省 300KB）、AddSystemAbilityListener(COMMON_EVENT/SOC_PERF/POWER_BATT/DEVICE_SERVICE_MANAGER)（:95-113）。

### 协作关系
OnAddSystemAbility 中 COMMON_EVENT 到位才 InitStateMachine（晚于服务发布）；SOC_PERF 到位时对 SocActionBase::SocSet 内动作 ResetActionValue。配置加载顺序与兜底链见实体八；温度链路见 [thermal-Technology.md](thermal-Technology.md) 机制一。

## 实体二：IThermalSrv IPC 契约与回调通道

### 职责与能力
客户端与服务端之间的唯一 IPC 契约：主接口 12 方法 + 3 个回调接口 + 1 个 sequenceable。主接口 proxy/stub 由 idl_gen_interface 生成；回调方向（服务端 → 客户端推送）的 proxy/stub 手写维护。

### 代码位置
| 文件 | 内容 |
|---|---|
| services/zidl/IThermalSrv.idl | :16-18 三回调接口声明，:19 sequenceable，:21-34 主接口 12 方法（[ipccode 0] 起，末位 ShellDump :34） |
| services/zidl/BUILD.gn | :30-54 idl_gen_interface("thermalmgr_interface")，proxy/stub 过滤 "*_proxy.cpp"/*"_stub.cpp" |
| services/zidl/include、src | thermal_{temp,level,action}_callback_{proxy,stub}（手写） |
| interfaces/inner_api/native/include/*_ipc_interface_code.h | 回调方向 IPC 码枚举 |

### 数据结构
12 方法：Subscribe/UnSubscribeThermalTempCallback(typeList, callback)、Subscribe/UnSubscribeThermalLevelCallback(callback, isSync)、Subscribe/UnSubscribeThermalActionCallback(actionList, desc, callback)、GetThermalSrvSensorInfo(type → sensorInfo, sensorInfoRet)、GetThermalLevel(→level)、GetThermalInfo()、SetScene(scene)、UpdateThermalState(tag, val, isImmed)、ShellDump(args, argc → dumpShell)。

### 接口清单
回调接口三件：IThermalTempCallback（温度推送）、IThermalLevelCallback（档位推送）、IThermalActionCallback（动作决策值推送）；sequenceable：ThermalSrvSensorInfo。

### 协作关系
[ipccode N] 是 ABI 契约只追加不改号；客户端消费见实体十五（客户端 SDK 与公共数据模型）；生成物与手写物的边界详见 [thermal-Technology.md](thermal-Technology.md) 机制七。

## 实体三：ThermalObserver（温度订阅与决策值缓存中心）

### 职责与能力
服务端"发布侧枢纽"：管理温度回调与动作回调的订阅表；缓存最新 TypeTempMap（callbackinfo_）与动作决策值（actionMap_/allActionMap_）；把 subscriber 送来的温度快照分发给策略（注册回调 callback_），把策略产出的动作值推送给动作订阅者。

### 代码位置
services/native/include/thermal_observer/thermal_observer.h（:34 类声明）+ services/native/src/thermal_observer/thermal_observer.cpp（OnReceivedSensorInfo :268、GetTemp :315）。

### 数据结构
订阅表：sensorTempListeners_ + callbackTypeMap_（回调→关注 type 列表）、actionListeners_ + callbackActionMap_（回调→关注动作列表），均按 classcomp 排序去重（:86-98）；缓存：callbackinfo_、actionMap_、allActionMap_、typeMap_（SensorType→字符串）；锁四把（:102-105）。

### 接口清单
- 订阅面：Subscribe/UnSubscribeThermalTempCallback :43-45、Subscribe/UnSubscribeThermalActionCallback :46-48（含死亡通知注册）。
- 数据面：OnReceivedSensorInfo :42、SetSensorTemp :55、GetTemp :62、GetThermalSrvSensorInfo :53、SetRegisterCallback :54。
- 动作面：SetDecisionValue :56、FindSubscribeActionValue :49、DecisionActionValue :51-52、NotifySensorTempChanged :50。
- 状态面：UpdatePolicyState :58-61、GetSensorType :63-66。

### 协作关系
由 ThermalService::InitThermalObserver 创建并把 ThermalPolicy::OnSensorInfoReported 注册为 callback_（thermal_service.cpp:351 起）；动作值由策略执行后经 SetDecisionValue 写入，供 FindSubscribeActionValue 过滤推送。机制细节见 [thermal-Technology.md](thermal-Technology.md) 机制一、六。

## 实体四：ThermalServiceSubscriber（温度历史与温升速率）

### 职责与能力
温度数据"预处理层"：接收 HDI 原始上报（OnTemperatureChanged），维护每 type 的历史温度队列与温升速率队列，为依赖 temp_rise_rate 的传感器簇（high_safe 类）提供判档输入。

### 代码位置
services/native/include/thermal_observer/thermal_service_subscriber.h（:28 类）+ services/native/src/thermal_observer/thermal_service_subscriber.cpp。

### 数据结构
typeTempMap_（最新值）、typeHistoryMap_（map<string, deque<int32_t>>，深度由 base 段 history_temp_count 控制，样例 10）、sensorsRateMap_（map<string, deque<double>>）、rateCount_{3}（速率窗口）、historyCount_、mutex_（:50-55）。

### 接口清单
Init :32、OnTemperatureChanged :33（HDI 温度落地唯一入口）、SetHistoryTypeTempMap :34（测试/模拟注入）、GetRateCount :35-38、GetSensorsRate :39-42、GetSubscriberInfo :43-46、私有 GetThermalRiseRate :48。

### 协作关系
上游是 ThermalService::HandleThermalCallbackEvent（thermal_service.cpp:692-708，转 TypeTempMap 后调用）；温度模拟 HandleTempEmulation 也走同一入口（:710-717）；下游经 ThermalObserver::OnReceivedSensorInfo 汇入。速率消费方是 ThermalConfigSensorCluster::IsTempRateTrigger（机制二）。

## 实体五：StateMachine 与状态采集器

### 职责与能力
温控决策的"条件系统"：把充电、亮屏、场景、用户自定义、开机延迟、充电延迟、扩展状态等离散条件收敛为 name→value 状态表，供策略行属性匹配（如 `<cpu_big scene="cam" charge="1">`）。

### 代码位置
services/native/include/thermal_observer/state_machine/state_machine.h（:37 类）+ services/native/src/thermal_observer/state_machine/ 下 9 个实现文件（7 个状态采集器 .cpp + state_machine.cpp + state_collection_factory.cpp，与 include/ 下头文件一一对应）。

### 数据结构
StateItem{name, params, isExistParam}（state_machine.h:31-35）来自 XML state 段；StateMachineMap = map<名称, shared_ptr<IStateCollection>>（:39）；IdleState 空闲态配置（:54-62）。

### 接口清单
Init :40（按 StateItem 用工厂实例化采集器）、UpdateState :41（外部状态上报落地）、DumpState/DumpIdle :42-43（hidumper）、SetStateItem :44-47、GetStateCollectionMap :49-52、GetCommonEventReceiver :64-67（公共事件订阅器挂在状态机上）。

### 协作关系
COMMON_EVENT 服务到位后 InitStateMachine（thermal_service.cpp OnAddSystemAbility 分支）；UpdateThermalState(tag, val, isImmed) 直接驱动 UpdateState；采集器各自订阅公共事件（充电/亮屏）或接收 IPC 上报；策略侧 StateMachineDecision 消费（thermal_policy.h:64）。展开见机制四。

## 实体六：ThermalPolicy（档位仲裁与策略决策）

### 职责与能力
温控"大脑"：温度到达时逐簇计算档位（委托传感器簇）、多簇取最严档位并对外发布（WriteLevel/LEVEL_CHANGED）、按档位查策略表并做状态属性过滤、把动作值分发给 ThermalActionManager 执行。

### 代码位置
services/native/include/thermal_policy/thermal_policy.h（:40 类）+ services/native/src/thermal_policy/thermal_policy.cpp。

### 数据结构
PolicyAction{actionName, actionValue, actionPropMap, isProp}（:28-33）；PolicyConfig{level, policyActionList}（:35-38）；PolicyConfigMap = map<簇名, vector<PolicyConfig>>（:42）；运行态：sensorClusterMap_、clusterLevelMap_（:74-77）。

### 接口清单
- Init :46、SetPolicyMap :49、SetSensorClusterMap :50（配置解析后注入）。
- 决策链：OnSensorInfoReported :47 → LevelDecision :61 → SortLevel :58 → WriteLevel :60 → PolicyDecision :62 → StateMachineDecision :64 → ActionDecision :63 → ActionExecution :65。
- 对外：ExecutePolicy :48、FindSubscribeActionValue :51（动作订阅推送）、GetClusterLevelMap/DumpLevel/DumpPolicy（测试与 dump）。

### 协作关系
Init 时向 ThermalObserver 注册采集回调（RegisterObserver :59）；档位变化通知 ActionThermalLevel；动作执行委托 ThermalActionManager。完整时序见 [thermal-Technology.md](thermal-Technology.md) 机制三、四。

## 实体七：ThermalConfigSensorCluster（传感器簇档位计算）

### 职责与能力
单簇判档算法载体：按传感器组阈值表（LevelItem）计算本簇当前档位；支持升降双阈值滞后、降序阈值（冷保护 desc）、温升速率门槛、辅助传感器区间、双传感器温差五类判型。

### 代码位置
services/native/include/thermal_policy/thermal_config_sensor_cluster.h（:59 类，:120-121 SensorClusterPtr/Map）+ services/native/src/thermal_policy/thermal_config_sensor_cluster.cpp。

### 数据结构
LevelItem{threshold, thresholdClr, tempRiseRate 默认 -1000000（不启用）, level}（:34-39）；AuxLevelItem{lowerTemp, upperTemp, level}（:41-45）；TempDiffItem{sensor1, sensor2, tempDiff, level}（:47-52）；四个行为开关 descFlag_/auxFlag_/rateFlag_/tempDiffFlag_（:109-112）；latestLevel_、stateMap_。

### 接口清单
CheckStandard :61（校验配置合法性）、UpdateThermalLevel :62（输入 TypeTempMap 更新 latestLevel_）、GetCurrentLevel :64；setter 家族 SetSensorLevelInfo/SetAuxSensorLevelInfo/SetTempDiffInfo/SetDescFlag/SetAuxFlag/SetRateFlag/SetTempDiffFlag/AddState（:65-75）。

### 协作关系
由配置解析器按 level 段每簇构造一个实例，经 ThermalPolicy::SetSensorClusterMap 注入；温升速率从 subscriber 的速率队列取；防抖算法展开见 [thermal-Technology.md](thermal-Technology.md) 机制二。

## 实体八：温控配置模型与解析器

### 职责与能力
把 thermal_service_config.xml 五段（base/level/state/action/policy）解析成内存模型：基础项、传感器簇表、状态项、动作项、策略表；负责产品差异化（config_policy 覆盖 → VENDOR → SYSTEM 三级兜底）。

### 代码位置
| 文件 | 内容 |
|---|---|
| services/native/profile/thermal_service_config.xml | 样例配置（product="lya"） |
| services/native/include/thermal_policy/thermal_srv_config_parser.h + src/*.cpp | ThermalSrvConfigParser（ThermalSrvConfigInit） |
| services/native/include/thermal_policy/thermal_config_base_info.h | BaseItem{tag, value}（:31-34）、BaseInfoMap（:26）、GetSensorsType（:51） |
| thermal_service.cpp:269-299 | InitConfigFile 三级兜底（GetOneCfgFile → VENDOR_THERMAL_SERVICE_CONFIG_PATH → SYSTEM_THERMAL_SERVICE_CONFIG_PATH） |

### 数据结构
base 段 → 键值对（history_temp_count、temperature_query_enum="soc,battery,shell,cpu,charger,ambient,ap,pa"、sim_tz）；level 段 → SensorClusterMap（含簇内 `<state screen="0" charge="1"/>` 条件）；state 段 → StateItem 列表；action 段 → ActionItem 列表（含 event/uid/protocol/param 属性）；policy 段 → PolicyConfigMap。

### 接口清单
ThermalSrvConfigInit（thermal_service.cpp:279/288/293 三处调用）；解析结果经 SetPolicyMap/SetSensorClusterMap/SetStateItem/SetActionItem/SetFaultInfoMap 分发给各组件。

### 协作关系
供电侧配置链见架构文档配置视角图；schema 与样例值速查见 [glossary.md](glossary.md)；修改配置的兼容约束见 AGENTS.md"约束边界"。

## 实体九：ThermalActionManager 与动作工厂

### 职责与能力
动作容器与注册中心：按 XML action 段创建动作对象（工厂 19 种映射）、维护 name→IThermalAction 表；承载档位回调订阅（转交特殊动作 ActionThermalLevel）；提供动作 mock 与 dump 能力。

### 代码位置
services/native/include/thermal_action/thermal_action_manager.h（:38 类）+ services/native/src/thermal_action/thermal_action_manager.cpp（:35 创建 actionThermalLevel_）+ thermal_action_factory.cpp（:50-86 注册 19 个创建 lambda，:90 Create）。

### 数据结构
ActionItem{name, params, uid, protocol, strict, enableEvent}（:29-36）；ThermalActionMap = map<string, shared_ptr<IThermalAction>>（:40）；actionThermalLevel_（特殊成员，:64）；actionNameList_、vActionItem_。

### 接口清单
Init :41（解析配置后逐项建动作）；Subscribe/UnSubscribeThermalLevelCallback(callback, isSync=true) :42-44（委托 actionThermalLevel_，thermal_action_manager.cpp:57-69）；GetThermalLevel :44（:75-78）；SetActionItem/GetActionItem/GetActionMap（:45-56）；测试与诊断：CreateActionMockFile :58、DumpAction :59、EnableMock(actionName, mockAction) :60。

### 协作关系
19 种动作：ActionAirplane、ActionCpuBig/Med/Lit、ActionGpu、ActionDisplay、ActionVolume、ActionShutdown、ActionThermalLevel、ActionPopup、ActionCharger、ActionVoltage、ActionCpuBoost、ActionCpuIsolate、ActionCpuNonVip、ActionApplicationProcess、ActionSocPerf、ActionSocPerfResource、ActionNode（thermal_action_factory.cpp:50-86）。与策略的握手见机制五、六。

## 实体十：IThermalAction 动作基类与 ThermalTimer

### 职责与能力
动作统一抽象：InitParams/SetStrict/SetEnableEvent 配置化；AddActionValue 累积多策略值；Execute 统一处理"策略级延迟执行"（delayTime + 精准唤醒定时器），到期/无延迟时调子类 ExecuteInner 落地。

### 代码位置
services/native/include/thermal_action/ithermal_action.h（:39 类，:63-101 Execute 默认实现）+ services/native/include/thermal_action/thermal_timer.h（:35 ThermalTimerInfo、:51 ThermalTimer，包装 MiscServices 定时器）。

### 数据结构
PolicyDelayAction{isCompleted, intDelayValue, uintDelayValue, floatDelayValue, delayTime, delayTimerId}（:30-37）；成员 isStrict_{true}、enableEvent_{false}、policyActionMap_（actionId→延迟项）、actionIdList_（本轮策略集合）（:105-109）。

### 接口清单
纯虚：InitParams :44、SetStrict :45、SetEnableEvent :46、AddActionValue(actionId, value) :47、ResetActionValue :48、ExecuteInner :103；非虚：AddActionValue(string) :50-53、AddActionDelayTime :55-61、Execute :63-101（先清理已消失策略的定时器，再为未完成延迟项建 WAKEUP|EXACT 定时器，末尾同步执行一次 ExecuteInner）。

### 协作关系
ThermalTimerInfo 回调内置位 isCompleted 并补一次 ExecuteInner（ithermal_action.h:86-92）；SocActionBase::SocSet 内动作在 SOC_PERF SA 就绪时被 ResetActionValue（thermal_service.cpp OnAddSystemAbility）。执行语义见机制五。

## 实体十一：温控动作家族（ActionXxx，含 ActionThermalLevel）

### 职责与能力
每个动作封装一种下游控制手段；ActionThermalLevel 例外——它不下发控制，而是持有档位值并管理档位回调订阅与推送。

### 代码位置
services/native/include/thermal_action/action/（action_airplane.h、action_application_process.h、action_charger.h、action_display.h、action_node.h、action_popup.h、action_shutdown.h、action_thermal_level.h、action_voltage.h、action_volume.h、action_soc/）+ services/native/src/thermal_action/action/。

### 数据结构
均实现基类六虚函数；差异在执行通道：
| 动作 | 通道 | 证据 |
|---|---|---|
| ActionDisplay | DisplayPowerMgrClient::DiscountBrightness(factor) | action_display.cpp:29、:115-119 |
| ActionShutdown | PowerMgrClient（power_mgr_client.h） | action_shutdown.cpp:21、:89 |
| ActionVolume | AudioSystemManager/AudioStreamManager + /data/service/el0/thermal/config/volume | action_volume.cpp:19-20、:34 |
| ActionAirplane | 飞行模式子系统（HAS_THERMAL_AIRPLANE_MANAGER_PART 门控） | action_airplane.cpp、thermalmgr.gni:28-33 |
| ActionCharger/ActionVoltage | 充电电流/电压（battery HDI） | action_charger.h、action_voltage.h |
| ActionNode | 通用设备节点写 | action_node.h |
| ActionThermalLevel | 档位缓存 + 回调表 | action_thermal_level.h |

### 接口清单
家族统一接口即基类接口；ActionThermalLevel 额外提供 Subscribe/UnSubscribeThermalLevelCallback(callback, isSync)、GetThermalLevel、NotifyLevelChanged（thermal_action_manager.cpp:57-78 转发面）。

### 协作关系
isSync=true 时订阅即推送当前档位（首报）；下游失联处理与 strict 取严仲裁见机制五、六。

## 实体十二：HDI 适配（ThermalCallback / FanCallback / HDIServiceStatusListener）

### 职责与能力
服务端与 HDI 驱动的三个接触面：温度事件接收、风扇数据接收、HDI 服务状态监听（掉线自动重订阅）。

### 代码位置
| 文件 | 内容 |
|---|---|
| services/native/include/thermal_callback.h | :26 类 : IThermalCallback（V1_1），RegisterThermalEvent 静态注册 :30，OnThermalDataEvent :31 |
| services/native/include/fan_callback.h | :27 类 : IFanCallback，RegisterFanEvent :30，OnFanDataEvent :31 |
| services/native/include/hdi_service_status_listener.h | :27 类 : ServStatListenerStub（V1_0），StatusCallback 模板构造 :30 |

### 数据结构
ThermalEventCallback = function<int32_t(HdfThermalCallbackInfo)>（thermal_callback.h:29）；FanEventCallback 同型（fan_callback.h:29）；HDI 数据类型来自 v1_1/thermal_types.h。

### 接口清单
RegisterThermalEvent/RegisterFanEvent（静态）；HdiServiceStatusListener 以回调方式把"服务上线/下线"事件送回 ThermalService::RegisterHdiStatusListener（thermal_service.cpp:617-660，多处失败路径重试）。

### 协作关系
温度回调 lambda 直连 HandleThermalCallbackEvent（thermal_service.cpp:671）；风扇回调喂 FanFaultDetect；重连链路见机制八。

## 实体十三：FanFaultDetect（风扇故障检测）

### 职责与能力
按配置的转速/温度上下限检查风扇上报数据，识别"转速过慢/过快"故障并发 FAULT 级 HiSysEvent（FAN_FAULT）。

### 代码位置
services/native/include/thermal_policy/fan_fault_detect.h（:35 类）+ services/native/src/thermal_policy/fan_fault_detect.cpp；配置来自 thermal_service_config.xml 的 fan 段（SetFaultInfoMap）。

### 数据结构
FanFaultInfoMap = map<int32_t, FanSensorInfo>（:33，故障号→门限）；成员 fanFaultInfoMap_（:52）。

### 接口清单
OnFanSensorInfoChanged :40（风扇数据入口）、SetFaultInfoMap :41、HasFanConfig :42（无风扇配置时跳过 HDI 风扇订阅，见 RegisterFanHdiCallback，thermal_service.cpp:720）、私有 CheckFanFault :45 / CheckFanTooSlow :46 / CheckFanTooFast :47 / CheckFanSensorInfo :48。

### 协作关系
上游 FanCallback；事件出口 thermal_hisysevent.h；机制见 [thermal-Technology.md](thermal-Technology.md) 机制九。

## 实体十四：ThermalMgrDumper（hidumper 入口）

### 职责与能力
系统诊断出口：响应 `hidumper -s 3303` 输出基础信息/状态/档位/动作/策略/空闲态；并提供两个调试命令——温度上报开关切换与温度模拟注入。

### 代码位置
services/native/include/thermal_mgr_dumper.h（:26 类）+ services/native/src/thermal_mgr_dumper.cpp；IPC 侧经 ShellDump（IThermalSrv.idl:34）进入。

### 数据结构
无状态静态工具类（方法全 static）。

### 接口清单
Dump(args, result) :31 总入口；SwitchTempReport :34（切换 isTempReport_，用户版默认关）；EmulateTempReport :36（构造 TypeTempMap → HandleTempEmulation，thermal_service.cpp:710）；DumpPolicy :38。

### 协作关系
各组件的 Dump* 方法（StateMachine::DumpState、ThermalPolicy::DumpPolicy、ThermalActionManager::DumpAction）在此汇聚；模拟链路见机制十。

## 实体十五：客户端 SDK 与公共数据模型

### 职责与能力
进程外消费入口：ThermalMgrClient 转发订阅/查询 IPC 并挂服务死亡通知；ThermalMgrListener 为 C/C++ 订阅者提供档位监听（内置 ThermalLevelCallbackStub 子类）；数据模型 ThermalSrvSensorInfo/SensorType/ThermalLevel 定义跨进程契约。

### 代码位置
| 文件 | 内容 |
|---|---|
| interfaces/inner_api/native/include/thermal_mgr_client.h | :26 类 final : DelayedRefSingleton，:50 ThermalMgrDeathRecipient，:65-66 thermalSrv_/deathRecipient_ |
| interfaces/inner_api/native/include/thermal_mgr_listener.h | :27 类，:37 ThermalLevelCallback : ThermalLevelCallbackStub，:41 OnThermalLevelChanged |
| interfaces/inner_api/native/include/thermal_srv_sensor_info.h | :22-73 sequenceable（type_/temp_，Marshalling :70/Unmarshalling :71），:78-89 SensorType（SOC=0…SENSOR7） |
| interfaces/inner_api/native/include/thermal_level_info.h | :28-70 ThermalLevel 8 值，:72+ ThermalCommonEventCode（CODE_THERMAL_LEVEL_CHANGED=0） |
| frameworks/native/ | thermal_mgr_client.cpp、thermal_mgr_listener.cpp、thermal_srv_sensor_info.cpp、thermal_level_callback_stub.cpp |

### 数据结构
ThermalSrvSensorInfo{type_, temp_} + SensorType{SOC=0, BATTERY, SHELL, SENSOR1~SENSOR7}（10 值）+ ThermalLevel{COOL~ESCAPE}（8 值）+ ThermalCommonEventCode{CODE_THERMAL_LEVEL_CHANGED=0}；客户端无业务缓存（ThermalMgrDeathRecipient 只做重连清零，机制六）。

### 接口清单
客户端：Subscribe/UnSubscribeThermalTempCallback(typeList, callback) :33-34、Subscribe/UnSubscribeThermalLevelCallback(callback, isSync=true) :35-36、Subscribe/UnSubscribeThermalActionCallback(actionList, desc, callback) :37-39、GetThermalSensorTemp(type) :40、GetThermalLevel :41；监听器：RegisterServiceEvent(isSync) :54、GetThermalLevel :50。

### 协作关系
NAPI 绑定（frameworks/napi/thermal_manager_napi.cpp:129-139 新旧两组导出、:140 起 ThermalLevel 常量 cool~escape、:297-301 模块名 "thermal"）、CJ、taihe 三套绑定共用本层；DeathRecipient 行为见机制六。

## 实体十六：ThermalProtector（非运行态温控独立进程）

### 职责与能力
独立可执行文件 /system/bin/thermal_protector：不依赖 samgr/HDI/运行态服务，直读 sysfs 热区，按 thermal_kernel_config.xml 独立判档，直接写充电电流/电压/CPU 频率控制节点，覆盖开机早期（on late-fs 起常驻）的充电温控保护。

### 代码位置
| 文件 | 内容 |
|---|---|
| application/protector/src/main.cpp | 入口：ThermalKernelService::GetInstance().OnStart() |
| application/protector/include/thermal_kernel_service.h | :27 类，成员 policy_/provision_/control_/timer_（:63-66） |
| application/protector/src/thermal_kernel_service.cpp | Init :39：配置三级兜底 :62-72、policy_->Init :79、control_->Init :84、provision_->InitProvision :89、timer_->Init :91 |
| application/protector/src/thermal_sensor_provision.cpp | sysfs 扫描："thermal_zone%d"、"/sys/class/thermal/%s"、"/temp"、"/type"（:46-49） |
| application/protector/include/thermal_sensor_provider.h | :37 ThermalSensorProvider : ThermalZoneManager，epoll 线程模型（:52-60） |
| application/protector/profile/thermal_kernel_config.xml | base + control 段（thermal_zone type="soc-thermal" interval="2000"，档位内联 current/voltage/cpu 值） |
| application/init/thermal_protector.rc / .cfg | on late-fs 拉起；rc：user root、seclabel u:r:thermal_protector:s0；cfg：uid thermal、gid [thermal, shell]、start-mode boot |

### 数据结构
ThermalKernelService 四组件：ThermalKernelPolicy（判档）、ThermalSensorProvision（sysfs 枚举）、ThermalDeviceControl + device_control_factory（cpu/current/voltage 三类节点控制）、ThermalProtectorTimer（轮询节拍）；配置模型 protector_base_info/protector_thermal_zone_info。

### 接口清单
进程内无对外 IPC；对外只有可执行体 + 两份 init 配置。

### 协作关系
与运行态服务零依赖、零互调（各自读同一批 sysfs 节点）；注意 rc 与 cfg 的运行身份不同（root vs thermal），SELinux 标签亦不同（thermal_protector:s0 vs thermal:s0），属部署差异点。运行机制展开见 [thermal-Technology.md](thermal-Technology.md) 机制十三。
