# thermal_manager 编码规范

> 从现有代码归纳，修改代码前先对齐本节。通用背景见 [business-context.md](business-context.md)。

## 命名规范

| 项 | 约定 | 示例 |
|---|---|---|
| 类/结构体 | PascalCase | `ThermalObserver`（thermal_observer.h:34） |
| 成员变量 | trailing underscore | `policyState_`、`sensorsRateMap_`（thermal_observer.h:100-116） |
| 函数/方法 | PascalCase | `OnReceivedSensorInfo`（thermal_observer.h:42） |
| 常量 | `constexpr` + UPPER_SNAKE | `VOLUME_PATH`（action_volume.cpp:34） |
| 全局/静态 | `g_` 前缀 | `g_moduleMgr`、`g_bootCompletedCallback`（thermal_service.cpp:92-119） |
| 名空间 | `OHOS::PowerMgr` | 全仓统一 |
| 枚举 | 上划线 PascalCase | `ThermalStatus::STATUS_IDEAL`（thermal_config_sensor_cluster.h:27-32） |

## 头文件与依赖

- 公共 API 只暴露 interfaces/inner_api/native/include/ 下头文件；实现细节头文件不出 services/native。
- 服务端内部依赖 HDI 接口头写 `#include "v1_1/ithermal_interface.h"`、`"v1_1/thermal_types.h"`（thermal_service.h:42-43），版本随 sa_profile/3303.json 的 min_hdi_proxy_version 联动，禁止降版本。
- 复用 utils/native 公共件：日志 thermal_log.h、事件 thermal_hisysevent.h、错误码 thermal_mgr_errors.h、文件/字符串工具 file_operation.h、string_operation.h，不自造轮子。
- 新增仓外依赖必须同步 bundle.json 的 deps.components 与 thermalmgr.gni part 检测。

## IPC 与智能指针

- 跨进程回调对象一律 `sptr<IXxxCallback>`，并注册 `IRemoteObject::DeathRecipient`（thermal_observer.h:67-79 两个死亡通知类）。
- 进程内组件用 `std::shared_ptr`（wptr<ThermalService> 回引用避免环，thermal_observer.h:38/101）。
- 客户端 `ThermalMgrClient` 是 `DelayedRefSingleton`（thermal_mgr_client.h:26），成员 thermalSrv_ 惰性获取并挂死亡通知（:50、:65-66）。
- 不要手改 services/zidl 生成产物；回调方向 proxy/stub 是手写源码（services/zidl/include/thermal_temp_callback_proxy.h 等），新增回调接口需同步 idl 声明。

## 错误处理与日志

- 返回值用 `int32_t` 错误码（thermal_mgr_errors.h），成功 `ERR_OK`，业务失败返回具体 ERR_*，不用裸 -1。
- 日志用宏 `THERMAL_HILOGD/I/W/E(COMP_SVC, format, ...)`，温度值打 `%{public}d`，避免整段信息标 public（thermal_service.cpp:604-606 的做法）。
- 关键 IPC 入口创建看门狗护栏：`ThermalXCollie thermalXCollie("ThermalService::SetScene", false);`（thermal_service.cpp:588-589，工具在 thermal_xcollie.h）。
- 写操作先鉴权再落状态：`Permission::IsSystem()` 失败即返回（thermal_service.cpp:587-598、:600-610）。

## 并发规范

- 回调订阅表、档位缓存、场景值各持独立互斥锁：`mutexTempCallback_`、`mutexActionCallback_`、`mutex_`、`sceneMutex_`（thermal_observer.h:102-105、thermal_service.h:203-205）。
- HDI 温度回调内不做阻塞 IO/长计算，只做拷贝与转发（HandleThermalCallbackEvent，thermal_service.cpp:692-708），重活在下游线程继续。
- 定时任务走 `ThermalTimer`（MiscServices 定时器封装，thermal_timer.h:51-62），不裸起线程；protector 侧轮询用 epoll+timerfd 既有框架（thermal_sensor_provider.h:52-60），不自建轮询。

## 配置与兼容

- 温控行为调配置不调代码：改 thermal_service_config.xml（schema 见 architecture.md），新增字段必须向后兼容（旧版本可忽略）。
- 特性开关只经 thermalmgr.gni 的 declare_args（:17），配套 `has_*_part` 检测 + `HAS_THERMAL_*_PART` 宏包裹源码（如 HAS_THERMAL_AIRPLANE_MANAGER_PART，thermal_service.h:44-49），并同步 bundle.json features。
- IPC 码值只在 idl `[ipccode N]` 追加，不复用不改号（IThermalSrv.idl:22 起）。

## 测试规范

- 单测/系统测试统一 HWTEST_F 宏 + Gtest；测试入口在 test/unittest/src/（30+ 文件，含 mock_proxy/mock_stub 系列）。
- 动作注入用 `ThermalActionManager::EnableMock`（thermal_action_manager.h:60），温度注入用 dump EmulateTempReport/SwitchTempReport（thermal_mgr_dumper.h:34-37）或 mock 配置 XML，不桩私有成员。
- 新增回调接口必须配套 test/fuzztest/ 下对应 fuzzer（现 16 个，BUILD.gn 聚合在 test/fuzztest/BUILD.gn）。
