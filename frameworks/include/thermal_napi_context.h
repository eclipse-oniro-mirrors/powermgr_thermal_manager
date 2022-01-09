/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef THERMAL_NAPI_BASE_H
#define THERMAL_NAPI_BASE_H

#include <functional>
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace PowerMgr {
enum AsyncStatus {
    PENDING,
    RESOLVED,
    REJECTED,
};
class ThermalNapiContext {
using FreeFunc = std::function<void(ThermalNapiContext*)>;
using ExecuteFunc = std::function<bool()>;

public:
    ThermalNapiContext(napi_env env, napi_value* args, uint32_t argc,
        int32_t callbackArg, napi_value object = nullptr);
    virtual ~ThermalNapiContext();
    bool StartAsyncWork(const char* workName, ExecuteFunc exeFunc, FreeFunc freeFunc);
    napi_value GetPromise()
    {
        return promise_;
    }
    napi_env env_;
    napi_value outValue_;
    napi_value outError_;

protected:
    static void ExecuteAsyncWork(napi_env env, void *data);
    static void CompleteAsyncWork(napi_env env, napi_status status, void *data);

    virtual void Init(napi_value* args, uint32_t argc);
    virtual napi_value GetValue(); // default function is for Promise<void>
    virtual napi_value GetError();  // default function is always return undefined

    uint32_t cbParamCount_;
    napi_value object_;
    napi_deferred deferred_;
    napi_ref callbackRef_;
    napi_async_work asyncWork_;
    napi_value promise_;
    ExecuteFunc exeFunc_;
    FreeFunc freeFunc_;
    AsyncStatus status_;
};
} // namespace PowerMgr
} // namespace OHOS
#endif // THERMAL_NAPI_BASE_H
