# thermal_manager 术语表

> 按拼音/字母排序。首次接触某个术语时先查此表，再按"读取"栏深入。

## 业务术语

| 术语 | 英文/标识符 | 含义 | 读取 |
|---|---|---|---|
| 档位 | ThermalLevel | 对外温控等级，8 值枚举 COOL/NORMAL/WARM/HOT/OVERHEATED/WARNING/EMERGENCY/ESCAPE（0-7） | thermal_level_info.h:28-70 |
| 辅助传感器 | aux_sensor / AuxLevelItem | 簇内除主传感器外、按 lowerTemp~upperTemp 区间参与判档的传感器 | thermal_config_sensor_cluster.h:41-45 |
| 降档阈值 | threshold_clr | 温度回落多少才允许退出当前档位的滞后阈值，与 threshold 成对防抖 | thermal_service_config.xml（level 段） |
| 传感器簇 | sensor_cluster | 一组传感器及其阈值表的组合（如 base_safe、warm_5G），每簇独立算档，多簇取最严 | thermal_config_sensor_cluster.h:59 |
| 温升速率 | temp_rise_rate | 单位时间温度变化率门槛，历史速率队列满足趋势才允许升档（high_safe 簇样例 1.5） | thermal_service_subscriber.h:39-54 |
| 温差判档 | temp_diff | 两个传感器（sensor1/sensor2）温度差作为升档条件（TempDiffItem） | thermal_config_sensor_cluster.h:47-52 |
| 升档阈值 | threshold | 进入某档位的温度阈值（逗号分隔对应多传感器），单位 0.001℃ | thermal_service_config.xml |
| 场景 | scene | 业务通过 SetScene 上报的使用场景（cam/call/game 等），策略表按 scene 属性差异化取值 | thermal_service.cpp:587-598 |
| 状态机 | StateMachine | 温控决策的状态条件集合（充电、亮屏、场景等）与采集器容器 | state_machine.h:37-75 |
| 非运行态温控 | ThermalProtector | init 在 on late-fs 拉起的独立进程 /system/bin/thermal_protector，直读 sysfs 控制设备节点 | application/protector/ |

## 技术标识

| 术语 | 含义 | 读取 |
|---|---|---|
| ActionItem | 动作声明（name/params/uid/protocol/strict/enableEvent），XML action 段解析产物 | thermal_action_manager.h:29-36 |
| ActionPopup | "popup" 弹窗动作，服务 OnStart 即预创建（POPUP_ACTION_NAME） | thermal_service.cpp:325-327 |
| DeathRecipient | IPC 死亡通知，客户端断链后自动清理订阅（temp/action 各一个） | thermal_observer.h:67-79 |
| EmulateTempReport | hidumper 命令注入模拟温度，测试/仿真入口 | thermal_mgr_dumper.h:36 |
| FanCallback | 风扇数据 HDI 回调（IFanCallback 实现），喂给 FanFaultDetect | fan_callback.h:27-34 |
| FanFaultDetect | 风扇转速过慢/过快检测，产出 FAN_FAULT FAULT 事件 | fan_fault_detect.h:35-52 |
| HdfThermalCallbackInfo | HDI 温度上报数据（info: vector<{type, temp}>） | v1_1/thermal_types.h（HDI 仓） |
| HdiServiceStatusListener | HDI 服务上下线监听，掉线后自动重订阅温控回调 | hdi_service_status_listener.h:27-31 |
| IThermalAction | 动作基类：AddActionValue/Execute/ExecuteInner，含延迟定时逻辑 | ithermal_action.h:39-110 |
| IThermalLevelCallback | 档位回调接口（zidl 声明 + 客户端手写 stub 继承） | ithermal_level_callback.h、thermal_level_callback_stub.h |
| IThermalSrv | 主 IPC 接口，idl 声明 12 方法（第 12 个 ShellDump） | services/zidl/IThermalSrv.idl:21-34 |
| IThermalTempCallback / IThermalActionCallback | 温度回调/动作回调接口（zidl 声明 + 手写 proxy/stub） | services/zidl/IThermalSrv.idl:16-17 |
| min_hdi_proxy_version | SA 配置中声明的 HDI 代理库最低版本（thermal 1.1 + battery 2.0） | sa_profile/3303.json:12-15 |
| PolicyAction / PolicyConfig | 策略表行：档位 + 动作列表 + prop 匹配（scene/charge/screen） | thermal_policy.h:28-38 |
| SA 3303 | 热管理服务在 samgr 的注册号，宿主进程 powermgr | sa_profile/3303.json:3-8 |
| ShellDump | IPC 第 12 方法：hidumper 请求透传（args/argc → dumpShell） | IThermalSrv.idl:34 |
| SensorType | 客户端侧传感器枚举 SOC/BATTERY/SHELL/SENSOR1~7 | thermal_srv_sensor_info.h:78-89 |
| ThermalCallback | 温度 HDI 回调（IThermalCallback 实现），静态注册到驱动 | thermal_callback.h:26-34 |
| ThermalMgrClient / ThermalMgrListener | 客户端单例与监听器（含 ThermalLevelCallbackStub 子类） | thermal_mgr_client.h:26、thermal_mgr_listener.h:27 |
| ThermalService | 服务主体，SystemAbility + 生成 stub 双继承 | thermal_service.h:56 |
| ThermalSrvSensorInfo | sequenceable 传感器信息 {type, temp} | thermal_srv_sensor_info.h:22-73 |
| ThermalTimer | MiscServices 定时器封装，动作延迟执行载体 | thermal_timer.h:51-62 |

## 事件与配置

| 术语 | 含义 | 读取 |
|---|---|---|
| ACTION_TRIGGERED | HiSysEvent STATISTIC 事件：动作触发（ACTION/VALUE/RATIO） | thermalmgr.yaml |
| FAN_FAULT | HiSysEvent FAULT 事件：风扇故障（ID/MSG） | thermalmgr.yaml |
| LEVEL_CHANGED | HiSysEvent STATISTIC 事件：档位变化（LEVEL） | thermalmgr.yaml |
| POWER_TEMPERATURE | HiSysEvent STATISTIC 事件：设备温度（NAME/TEMPERATURE） | thermalmgr.yaml |
| thermal_kernel_config.xml | protector 独立配置（base + control 段，thermal_zone 按 interval 轮询） | application/protector/profile/ |
| thermal_service_config.xml | 运行态温控配置（base/level/state/action/policy 五段，product 属性区分产品） | services/native/profile/ |

相关机制展开见 [thermal-Technology.md](thermal-Technology.md)；实体职责见 [thermal-Entity.md](thermal-Entity.md)。
