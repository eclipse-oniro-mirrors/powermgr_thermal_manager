# AGENTS.md - OpenHarmony 热管理服务（Thermal Manager）

## 1. 代码地图

本仓库实现 OpenHarmony 热管理服务（Thermal Manager），SA ID 3303，进程名 `powermgr`，库 `libthermalservice.z.so`。核心职责是设备温度检测、温控档位仲裁、温控动作下发（限频/限流/降亮度/关机等）、非运行态温控（ protector 子模块）。最重要的架构边界是**`ThermalService` 通过 HDI 周期性采集传感器温度，经 `ThermalPolicy` 仲裁出档位，再由 `ThermalActionManager` 下发限频/限流动作到各子系统**。

### 非本项目维护的目录

以下目录属于其他团队或生成产物，不属于本项目维护范围，修改时请跳过：

- `frameworks/cj/`：CJ 语言绑定（由 CJ 团队维护）
- `frameworks/ets/taihe/`：taihe FFI 生成产物（由 `taihe_ffi_gen` 工具生成，不要手改）

### 嵌套指引

本仓目前无嵌套 AGENTS.md / CLAUDE.md / rules / skills 文件。以下子模块可按需新建嵌套 AGENTS.md 以提供更具体的指引：

- `application/protector/`：非运行态温控独立子模块（关机/重启期间的简化温控），含 `include/`、`src/`、`profile/`，与运行态服务互斥运行，可新建 `application/protector/AGENTS.md` 聚焦独立运行约束、init 拉起时序、HDI 就绪重试机制
- `application/init/`：init 拉起 protector 的配置目录，可新建嵌套 AGENTS.md 说明启动时序约束
- `utils/native/` + `utils/appmgr/` + `utils/hookmgr/`：跨仓共享工具层，被 `power_manager` 等下游仓引用，可新建 `utils/AGENTS.md` 聚焦跨仓工具变更影响评估
- `services/native/include/thermal_policy/`：档位仲裁策略子目录，可新建嵌套 AGENTS.md 聚焦 sensor_cluster/threshold/threshold_clr 防抖与档位抖动控制
- `services/native/include/thermal_action/`：温控动作执行器子目录，可新建嵌套 AGENTS.md 聚焦下游子系统调用时序与失败回滚
- `services/native/include/thermal_observer/`：温度采集核心，可新建嵌套 AGENTS.md 聚焦 HDI 上报频率与重连逻辑
- `services/native/profile/thermal_service_config.xml`：温控档位配置文件，schema 变更影响所有产品温控行为，可单独新建 AGENTS.md 聚焦配置 schema 与产品差异
- `test/`：测试组织目录，可新建 `test/AGENTS.md` 聚焦 common/fuzztest/systemtest/unittest 四类测试的运行约定

### 关键区域

- `interfaces/inner_api/native/include/`：公共 C++ API 头文件，含 `thermal_mgr_client.h` 入口、`thermal_srv_sensor_info.h` 传感器数据模型、`thermal_level_info.h` 档位信息、回调接口（`IThermalTempCallback` / `IThermalLevelCallback` / `IThermalActionCallback`）及对应 `*_ipc_interface_code.h` IPC 码枚举。
- `frameworks/napi/`：ArkTS NAPI 绑定。
- `frameworks/native/`：客户端 SDK 实现。
- `services/zidl/IThermalSrv.idl`：IPC 接口定义（**本项目使用 `.idl` 文件，含 callback 接口和 sequenceable**），含 12 个方法。
- `services/zidl/include/` + `services/zidl/src/`：IDL 生成的 proxy/stub。
- `services/native/include/thermal_service.h`：服务端核心类。
- `services/native/include/thermal_observer.h`：温度采集与档位计算。
- `services/native/include/thermal_policy/`：温控策略（档位仲裁、动作选择）。
- `services/native/include/thermal_action/`：温控动作执行器（限频/限流/降亮度等）。
- `services/native/include/thermal_callback.h`：温度事件回调。
- `services/native/include/thermal_mgr_dumper.h`：hidumper 入口。
- `services/native/include/fan_callback.h`：风扇控制回调。
- `services/native/include/hdi_service_status_listener.h`：HDI 上下线监听。
- `services/native/profile/`：`thermal_service_config.xml` 等配置文件。
- `application/protector/`：**非运行态温控独立子模块**（关机/重启期间的简化温控），含 `include/`、`src/`、`profile/`。
- `application/init/`：init 配置文件（启动 protector）。
- `sa_profile/3303.json`：SA 3303 注册配置，`min_hdi_proxy_version: ["libthermal_proxy_1.1.z.so", "libbattery_proxy_2.0.z.so"]`（依赖两个 HDI）。
- `thermalmgr.gni`：1 个特性开关 + 多个 part 检测。
- `thermalmgr.yaml`：HiSysEvent 配置。
- `utils/native/` + `utils/appmgr/` + `utils/hookmgr/`：跨仓共享工具（部分被 `power_manager`/`battery_manager` 等仓引用）。
- `test/`：`common/` + `fuzztest/`（16 个 fuzzer）+ `systemtest/` + `unittest/` + `resource/`。

### Where to look

| 任务类型 | 先看哪里 |
|---|---|
| 公共 API 变更 | `interfaces/inner_api/native/include/` 头文件 -> `frameworks/native/` 实现 -> `frameworks/napi/` ArkTS 绑定 -> `*.map` 版本脚本 |
| IPC 接口变更 | `services/zidl/IThermalSrv.idl` -> 构建生成 proxy/stub -> `interfaces/inner_api/native/include/thermal_mgr_client.h` 调用方 |
| 温度采集流程 | `services/native/include/thermal_observer.h` + `services/native/src/thermal_observer*.cpp` + HDI `drivers_interface_thermal` |
| 温控档位仲裁 | `services/native/include/thermal_policy/` + `services/native/src/thermal_policy/` + `services/native/profile/thermal_service_config.xml` |
| 温控动作下发 | `services/native/include/thermal_action/` + `services/native/src/thermal_action/` |
| 温度回调订阅 | `interfaces/inner_api/native/include/ithermal_temp_callback.h` + `IThermalSrv.idl` 中 `SubscribeThermalTempCallback` + `services/native/include/thermal_callback.h` |
| 档位回调订阅 | `interfaces/inner_api/native/include/ithermal_level_callback.h` + `IThermalSrv.idl` 中 `SubscribeThermalLevelCallback` |
| 动作回调订阅 | `interfaces/inner_api/native/include/ithermal_action_callback.h` + `IThermalSrv.idl` 中 `SubscribeThermalActionCallback` |
| 温控场景（SetScene） | `IThermalSrv.idl` 中 `SetScene` + `services/native/src/thermal_service.cpp` |
| 温控状态更新 | `IThermalSrv.idl` 中 `UpdateThermalState` + `services/native/src/thermal_service.cpp` |
| 非运行态温控 | `application/protector/include/` + `application/protector/src/` + `application/protector/profile/` + `application/init/` |
| 温控配置文件 | `services/native/profile/thermal_service_config.xml`（传感器簇、档位阈值、动作映射）|
| 风扇控制 | `services/native/include/fan_callback.h` + HDI `drivers_interface_thermal` |
| 跨仓依赖调用 | `bundle.json` 中 deps.components（依赖 `battery_statistics`/`battery_manager`/`display_manager`） |
| 特性开关 | `thermalmgr.gni` `declare_args()` + part 检测段 |
| 新增/修改测试 | `test/unittest/` + `test/systemtest/` + `test/fuzztest/`（16 个）+ `test/common/` + `test/resource/` |
| 构建配置 | `bundle.json` + `thermalmgr.gni` + 子目录 `BUILD.gn` |
| DFX（日志/事件） | `thermalmgr.yaml`（HiSysEvent 配置）+ grep `HiSysEvent` 调用点 |

### 架构分层

```
应用层
  ├─ ArkTS 应用 -> frameworks/napi (NAPI 绑定)
  │                └─ frameworks/ets/taihe (taihe FFI 生成)
  ├─ CJ 应用 -> frameworks/cj (CJ 绑定)
  └─ C++ 应用/系统组件 -> interfaces/inner_api/native (完整 C++ API)
          ↓
客户端 SDK
  ThermalMgrClient (interfaces/inner_api/native/include/thermal_mgr_client.h)
    -> IThermalSrv proxy (services/zidl，IDL 生成)
        ↓ IPC (SystemAbility 3303)
服务端
  ThermalService (services/native/include/thermal_service.h, 继承 SystemAbility)
    ├─ ThermalObserver (周期采集传感器温度，HDI 上报入口)
    │    └─ ThermalCallback (温度事件分发)
    ├─ ThermalPolicy (档位仲裁：sensor_cluster -> threshold -> level)
    │    └─ 读取 thermal_service_config.xml
    ├─ ThermalActionManager (按档位下发动作：限频/限流/降亮度/关机)
    │    ├─ 调用 display_manager (降亮度)
    │    ├─ 调用 audio_framework (限音量)
    │    ├─ 调用 call_manager (限通话)
    │    └─ 调用 power_manager (关机)
    ├─ ThermalMgrDumper (hidumper 入口)
    ├─ FanCallback (风扇控制)
    └─ HDIServiceStatusListener (HDI 上下线监听)

非运行态温控（独立子模块）
  application/protector/
    ├─ 在系统启动早期由 init 拉起（init 配置在 application/init/）
    ├─ 简化版温度采集与档位仲裁
    └─ 关机/重启期间的温控兜底
```

## 2. 知识路由

在规划或编辑前，先对任务分类，读取对应的代码路径和文档。

### Task-based routing

| 任务类型 | 读取 |
|---|---|
| 公共 API 新增/修改 | `interfaces/inner_api/native/include/*.h` + `frameworks/native/` + `frameworks/napi/` + `frameworks/cj/` + `*.map` 版本脚本 |
| IPC 接口变更 | `services/zidl/IThermalSrv.idl`（IDL 源文件，含 callback 接口 + sequenceable 声明）+ `interfaces/inner_api/native/include/thermal_mgr_client.h` 调用方 + `interfaces/inner_api/native/include/*_ipc_interface_code.h` |
| 温度采集逻辑 | `services/native/include/thermal_observer.h` + `services/native/src/thermal_observer*.cpp` + HDI `drivers_interface_thermal` |
| 档位仲裁策略 | `services/native/include/thermal_policy/` + `services/native/src/thermal_policy/` + `services/native/profile/thermal_service_config.xml`（sensor_cluster/threshold/threshold_clr 定义）|
| 动作下发 | `services/native/include/thermal_action/` + `services/native/src/thermal_action/` + 调用下游子系统（display_manager/audio_framework/call_manager/power_manager）|
| 温度回调订阅 | `interfaces/inner_api/native/include/ithermal_temp_callback.h` + `IThermalSrv.idl` 中 `SubscribeThermalTempCallback`/`UnSubscribeThermalTempCallback` + `services/native/include/thermal_callback.h` |
| 档位回调订阅 | `interfaces/inner_api/native/include/ithermal_level_callback.h` + `thermal_level_callback_stub.h` + `IThermalSrv.idl` 中 `SubscribeThermalLevelCallback` |
| 动作回调订阅 | `interfaces/inner_api/native/include/ithermal_action_callback.h` + `IThermalSrv.idl` 中 `SubscribeThermalActionCallback` |
| 传感器信息查询 | `interfaces/inner_api/native/include/thermal_srv_sensor_info.h` + `IThermalSrv.idl` 中 `GetThermalSrvSensorInfo` |
| 档位查询 | `interfaces/inner_api/native/include/thermal_level_info.h` + `IThermalSrv.idl` 中 `GetThermalLevel` |
| 场景化温控 | `IThermalSrv.idl` 中 `SetScene` + `services/native/src/thermal_service.cpp`（场景会覆盖默认档位）|
| 温控状态上报 | `IThermalSrv.idl` 中 `UpdateThermalState` + 调用方（其他子系统上报状态）|
| 非运行态温控 | `application/protector/include/` + `application/protector/src/` + `application/protector/profile/` + `application/init/`（init 拉起配置）|
| 温控配置文件变更 | `services/native/profile/thermal_service_config.xml`（传感器簇、档位阈值、动作映射）+ `services/native/include/thermal_config*.h`（解析器）|
| 风扇控制 | `services/native/include/fan_callback.h` + HDI `drivers_interface_thermal` |
| 跨仓依赖调用 | `bundle.json` 中 `deps.components` + grep 调用 `battery_manager` / `battery_statistics` / `display_manager` 的位置 |
| 权限校验 | grep `AccessTokenKit` / `VerifyAccessToken` 调用点 |
| 特性开关 | `thermalmgr.gni` `declare_args()` + part 检测段（蓝牙/wifi/display_manager/audio_framework/config_policy/netmanager_base 等）|
| 新增/修改测试 | `test/unittest/` + `test/systemtest/` + `test/fuzztest/`（16 个）+ `test/common/` + `test/resource/` |
| 构建配置 | `bundle.json`（依赖、syscap、rom/ram）+ `thermalmgr.gni`（特性开关、part 检测）+ 各 `BUILD.gn` |
| DFX（日志/事件） | `thermalmgr.yaml`（HiSysEvent 配置）+ grep `HiSysEvent` 调用点 |

### Path-based routing

| 修改路径 | 需了解的上下文 |
|---|---|
| `interfaces/inner_api/native/` | 所有 C++ 消费者的公共 API，变更需同步 `frameworks/napi/`、`frameworks/cj/`、`frameworks/ets/taihe/` 三套绑定并检查 `*.map` 版本脚本 |
| `services/zidl/IThermalSrv.idl` | IDL 源文件，含 3 个 callback 接口声明 + 1 个 sequenceable + 主接口 12 个方法，修改后由构建系统生成 proxy/stub |
| `services/zidl/include/` + `services/zidl/src/` | **IDL 生成产物**，不要手改 |
| `services/native/include/thermal_service.h` | 服务端核心类，所有温控流程的入口 |
| `services/native/include/thermal_observer.h` | 温度采集核心，HDI 上报频率和采集策略影响系统负载 |
| `services/native/include/thermal_policy/` | 策略核心，档位仲裁规则变更影响所有温控动作 |
| `services/native/include/thermal_action/` | 动作执行器，每个动作对应一个下游子系统调用 |
| `services/native/profile/thermal_service_config.xml` | **温控档位配置**，定义传感器簇、档位阈值、动作映射，修改直接影响产品温控行为 |
| `services/native/include/thermal_callback.h` | 温度事件分发，影响所有订阅者 |
| `application/protector/` | **独立子模块**，非运行态温控，有自己的执行入口和配置，与运行态服务互斥 |
| `application/init/` | init 拉起 protector 的配置，修改影响启动时序 |
| `frameworks/ets/taihe/` | taihe FFI **生成产物**，不要手改 |
| `thermalmgr.gni` | 1 个特性开关 + 多个 part 检测，part 检测变更需同步 `bundle.json` |
| `utils/native/` + `utils/appmgr/` + `utils/hookmgr/` | 跨仓共享工具，修改影响 `power_manager` 等下游仓 |
| `thermalmgr.yaml` | HiSysEvent 事件定义，新增事件需在此声明 |

### Vocabulary-based routing

当任务、issue、日志、API 名称中出现以下术语时，先理解其含义和风险再动手：

| 术语 | 含义与风险 | 读取 |
|---|---|---|
| ThermalSrv | Thermal Service，本服务简称，SA 3303 | 本文件 + `sa_profile/3303.json` |
| ThermalLevel | 温控档位（0-6 级），档位变更触发限频/限流等动作 | `interfaces/inner_api/native/include/thermal_level_info.h` |
| ThermalSrvSensorInfo | 传感器温度信息数据模型，跨进程传输（sequenceable） | `interfaces/inner_api/native/include/thermal_srv_sensor_info.h` |
| sensor_cluster | 传感器簇，`thermal_service_config.xml` 中定义一组传感器组合对应的档位阈值 | `services/native/profile/thermal_service_config.xml` |
| threshold / threshold_clr | 档位进入阈值/退出阈值，滞后防抖，修改影响档位抖动 | `thermal_service_config.xml` |
| ThermalAction | 温控动作（限频/限流/降亮度/关机等），每个动作对应一个下游子系统调用 | `services/native/include/thermal_action/` |
| ThermalObserver | 温度采集核心，HDI 周期上报入口 | `services/native/include/thermal_observer.h` |
| ThermalPolicy | 档位仲裁策略 | `services/native/include/thermal_policy/` |
| ThermalProtector | 非运行态温控子模块，关机/重启期间的温控兜底 | `application/protector/` |
| FanCallback | 风扇控制回调 | `services/native/include/fan_callback.h` |
| SetScene | 场景化温控接口，调用方设置当前场景覆盖默认档位 | `IThermalSrv.idl` |
| UpdateThermalState | 其他子系统上报温控相关状态（如充电状态、屏幕状态） | `IThermalSrv.idl` |
| IThermalTempCallback / IThermalLevelCallback / IThermalActionCallback | 三类回调接口（温度/档位/动作） | `interfaces/inner_api/native/include/ithermal_*_callback.h` |
| Sequenceable | IDL 中声明的可序列化数据模型 | `services/zidl/IThermalSrv.idl` |
| SystemAbility / SA | OpenHarmony 系统服务框架，本服务 SA ID 3303 | `sa_profile/3303.json` |
| sptr | OpenHarmony 共享指针（`refbase.h`），非 `std::shared_ptr` | 全项目 |
| ErrCode | 错误码返回类型 | `interfaces/inner_api/native/include/thermal_errors.h`（如有）|
| HWTEST_F | OpenHarmony 测试用例宏 | 全项目测试代码 |
| 特性开关 | `thermalmgr.gni` 中 `declare_args()` 控制条件编译 | `thermalmgr.gni` |
| part 检测 | `thermalmgr.gni` 中检测 `global_parts_info` 决定是否编译某 part 支持 | `thermalmgr.gni` |
| min_hdi_proxy_version | `sa_profile/3303.json` 中声明的 HDI 代理版本依赖（`libthermal_proxy_1.1.z.so` + `libbattery_proxy_2.0.z.so`），降级会破坏兼容性 | `sa_profile/3303.json` |
| powermgr 进程 | 本仓与 power_manager/battery_manager/display_manager 共享同一进程，多 SA 共进程 | `sa_profile/3303.json` |
| 跨仓依赖 | 本仓调用 `battery_statistics` / `battery_manager` / `display_manager` 等子系统获取耗电/电池/亮度信息 | `bundle.json` 中 `deps.components` |

在计划阶段，必须声明：
- **任务分类**（如：公共 API 变更 / IPC 接口变更 / 温度采集变更 / 档位仲裁变更 / 动作下发变更 / 非运行态温控变更 / 配置文件变更 / 新增特性 / 测试修改）
- **已读取的代码路径和文档**（具体到文件路径）
- **发现的约束**（架构不变量、禁止事项、特性开关依赖、HDI 版本依赖、跨仓依赖）
- **是否需要同步修改其他层**：
  - 公共 API 变更 -> 同步 NAPI / CJ / taihe 三套绑定 + `*.map` 版本脚本
  - `IThermalSrv.idl` 变更 -> 由构建系统重生成 proxy/stub + 同步 `thermal_mgr_client.h` 调用方
  - 特性开关变更 -> 同步 `bundle.json` 的 `features` 列表
  - 跨仓依赖调用变更 -> 检查 `battery_statistics` / `battery_manager` / `display_manager` 是否受影响
  - `thermalmgr.yaml` 变更 -> 同步 HiSysEvent 事件定义
  - `thermal_service_config.xml` 变更 -> 评估对产品温控行为的影响

## 3. 约束边界

### 架构不变量

- **客户端不持有业务状态**：`ThermalMgrClient` 仅做 IPC 转发，所有温度和档位由服务端 `ThermalService` 持有
- **HDI 是温度事件唯一真相源**：所有温度数据必须经 `ThermalObserver` 从 HDI 采集，禁止业务直接构造 `ThermalSrvSensorInfo`
- **档位仲裁必须经 `ThermalPolicy`**：禁止业务直接修改档位字段，所有档位变更必须经策略仲裁
- **动作下发必须经 `ThermalActionManager`**：禁止业务直接调用下游子系统（`display_manager` / `audio_framework` / `power_manager`）的限频接口
- **公共 API 表达稳定能力意图，不暴露 HDI 句柄或服务端内部字段**
- **权限校验在服务端入口完成**：`SetScene` / `UpdateThermalState` 等写操作必须有权限校验
- **非运行态温控（protector）与运行态服务互斥**：`application/protector/` 在系统启动早期/关机期间运行，不能依赖运行态服务
- **IPC 调用必须设置死亡通知**，回调订阅必须支持客户端异常断开后自动清理
- **跨进程回调必须用 Parcelable 序列化**，`ThermalSrvSensorInfo` 是 sequenceable
- **DFX（日志、HiSysEvent、HiCollie、错误码）必须观测所有档位变更和动作下发**
- **HDI 上下线必须能自动重连**：`HDIServiceStatusListener` 监听 HDI 重启，自动重新订阅
- **配置文件变更必须向后兼容**：`thermal_service_config.xml` 的新增字段必须可被旧版本忽略

### 禁止事项

- **不要修改公共 API 签名、错误码、权限行为或 `ThermalSrvSensorInfo`/`ThermalLevelInfo` 字段语义**，除非任务明确要求；修改 `*.map` 中已有符号的可见性属于破坏性变更
- **不要直接编辑 `frameworks/ets/taihe/` 下的生成文件**，应修改 `.taihe` 源文件后由构建系统重生成
- **不要直接修改 `services/zidl/` 下 IDL 生成的 proxy/stub 代码**，应修改 `IThermalSrv.idl` 后重新生成
- **不要为通过测试而删除日志、HiSysEvent 事件、错误码或诊断信息**
- **不要绕过现有的 DFX、安全、兼容性检查**
- **不要直接修改 HDI 接口**：本仓是 HDI 消费方，HDI 接口由 `drivers_interface_thermal` 仓维护
- **不要降级 `sa_profile/3303.json` 中的 `min_hdi_proxy_version`**：降级 HDI 版本会破坏与旧设备的兼容性
- **不要绕过 `ThermalPolicy` 直接修改档位**：会导致档位不一致
- **不要绕过 `ThermalActionManager` 直接调用下游子系统**：会导致动作未审计、无法回滚
- **不要在 `application/protector/` 中引入对运行态服务的依赖**：protector 独立运行，不能依赖 `ThermalService`
- **不要在 `ThermalObserver` 的 HDI 回调中执行阻塞 IO 或长耗时操作**：HDI 上报频率高，阻塞会导致采集延迟
- **不要在 `powermgr` 进程中执行长耗时同步操作**：本进程与 power/battery/display 共享
- **不要引入新的生产依赖**而不经过 `bundle.json` 评审
- **不要修改 `thermal_service_config.xml` 的 schema**而不评估对旧版本兼容性的影响

### 需确认后再修改

- **公共 API 签名变更**（需确认兼容性影响和版本策略，更新 `*.map` 版本脚本）
- **`ThermalSrvSensorInfo` / `ThermalLevelInfo` 字段新增/删除**（需评估跨进程序列化兼容性）
- **IPC 码值变更**（`*_ipc_interface_code.h`，新增接口追加新码值，不能复用或调整已有码值）
- **`sa_profile/3303.json` 中 `min_hdi_proxy_version` 调整**（需确认 HDI 团队和设备兼容性）
- **`thermal_service_config.xml` schema 变更**（需确认产品温控团队评审，影响所有产品形态）
- **档位阈值调整**（需评估对散热体验和性能的影响，特别是 `threshold` / `threshold_clr` 防抖逻辑）
- **动作下发策略变更**（需确认下游子系统团队评审：`display_manager` / `audio_framework` / `call_manager` / `power_manager`）
- **跨仓依赖调用变更**（需确认 `battery_statistics` / `battery_manager` / `display_manager` 接口稳定性）
- **`thermalmgr.gni` part 检测段变更**（需确认 `bundle.json` 的 `deps.components` 同步）
- **新增外部依赖**（需确认许可证、包大小、`bundle.json` 同步）
- **`utils/` 跨仓工具的接口变更**（需确认 `power_manager` 等下游仓是否受影响）
- **`thermalmgr.yaml` 中 HiSysEvent 事件变更**（需确认 DFX 团队评审）
- **`application/protector/` 行为变更**（需确认非运行态温控团队评审）

### 项目特定陷阱

- **`IThermalSrv.idl` 的 IPC 码值稳定性**：IDL 中 `[ipccode N]` 是 ABI 契约，新增接口必须追加新码值，复用或调整会让旧客户端调用错误接口
- **`thermal_service_config.xml` 的产品差异**：不同产品有不同的配置文件（通过 `product` 属性区分），修改时需评估对其他产品的影响
- **档位抖动**：`threshold` / `threshold_clr` 阈值设置不当会导致档位频繁切换，影响用户体验
- **跨子系统调用的时序**：`ThermalActionManager` 调用下游子系统时，下游可能未启动或已重启，需处理调用失败
- **`protector` 子模块的启动时序**：protector 在系统启动早期运行，依赖的 HDI 可能尚未就绪，需有重试机制
- **`ThermalSrvSensorInfo` 的 sequenceable 实现**：IDL 中声明的 sequenceable 必须实现 Marshalling/Unmarshalling，字段顺序是 ABI 一部分
- **`SetScene` 的覆盖语义**：场景化温控会覆盖默认档位，业务异常退出未清理场景会导致温控被持续压制
- **`UpdateThermalState` 的调用方**：调用方在多个子系统中（电池、屏幕等），需保证调用时序和参数一致
- **特性开关 `thermal_manager_audio_framework_enable` 的 part 检测**：开关默认关闭，开启需同步 `bundle.json` 中 `audio_framework` 依赖
- **`powermgr` 多 SA 共进程**：SA 3301/3302/3303/3308 在同一进程，一个 SA 崩溃会拖死全部
- **风扇控制的硬件差异**：`fan_callback.h` 仅在支持风扇的设备上启用，需处理无风扇设备的 fallback

## 4. 验证闭环

### 最小验证

```bash
# 构建 thermal_manager 子系统（从 OpenHarmony 根目录执行）
./build.sh --product-name rk3568 --build-target thermal_manager

# 构建全部测试
./build.sh --product-name rk3568 --build-target thermal_manager_test
```

### 任务特定验证

| 任务类型 | 验证命令 |
|---|---|
| 公共 API 变更 | `./build.sh --product-name rk3568 --build-target thermal_manager` + 同步构建依赖本仓的下游仓 + 跑 `test/unittest/` |
| IPC 接口变更 | 重新生成 proxy/stub + 跑 `test/unittest/` + `test/fuzztest/thermalmgrclient_fuzzer` / `thermalservice_fuzzer` 等所有 16 个 fuzzer |
| 温度采集流程 | 跑 `test/systemtest/` HDI 上报场景 + 验证 `ThermalObserver` 重连逻辑 + `test/fuzztest/thermalsrvsensorinfo_fuzzer` |
| 档位仲裁变更 | 跑 `test/unittest/` 档位仲裁用例 + `test/systemtest/` 多传感器场景 + 验证档位防抖 |
| 动作下发变更 | 跑 `test/systemtest/` 动作下发场景 + 验证下游子系统调用 + `test/fuzztest/thermalactioncallback_fuzzer` |
| 回调订阅变更 | `test/fuzztest/{thermallevelcallback,thermaltempcallback,thermalactioncallback,thermallevelcallbackstub}_fuzzer` |
| 非运行态温控 | `./build.sh --build-target thermal_protector` + 跑 `application/protector/` 内测试 + 真机非运行态验证 |
| 配置文件变更 | 真机验证档位阈值和动作映射 + `test/systemtest/` 多档位场景 |
| 跨仓依赖调用 | 同步构建 `battery_statistics` / `battery_manager` / `display_manager` + 验证调用接口未变 |
| 特性开关翻转 | 重新构建 `thermal_manager` 全量 + 验证 `bundle.json` 中 `features` / `deps.components` 同步 |
| HDI 适配层变更 | 验证 `sa_profile/3303.json` 的 `min_hdi_proxy_version` 未被降级 |

### Done 定义

- 构建通过（子系统 + 单元测试 + 模糊测试 + protector）
- 无新增编译警告
- 变更范围与任务要求一致，未夹带未关联的重构
- IPC 接口变更已同步 `IThermalSrv.idl` + 重生成 proxy/stub + 调用方
- 特性开关变更已同步 `thermalmgr.gni` + `bundle.json`
- 公共 API 变更已同步 NAPI / CJ / taihe / `*.map` 版本脚本
- `thermal_service_config.xml` 变更已评估对产品温控行为的影响
- 跨仓依赖调用变更已评估对下游仓的影响
- 涉及非运行态温控的变更已评估独立运行能力

### 最终响应期望

完成报告必须包含：
1. 修改的文件清单（按 `file:line` 引用）
2. 任务分类与对应验证命令的执行结果
3. 是否触发跨层同步修改（NAPI / CJ / taihe / `*.map` / IDL / `bundle.json` / `utils/` / `thermalmgr.yaml` / `thermal_service_config.xml`）
4. 是否影响特性开关默认值、HDI 版本依赖或跨仓依赖
5. 是否触及架构不变量或需确认事项
6. 涉及非运行态温控/档位仲裁/动作下发的变更需额外说明风险评估

### 无法验证时

如果构建环境不可用，列出应执行的命令并说明预期结果，明确标注「未验证」字样，不能假称已通过。涉及温控档位/动作下发/非运行态温控的变更，必须人工复核代码逻辑并说明无法在沙箱验证的限制。
