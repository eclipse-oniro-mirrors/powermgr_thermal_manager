/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef SP_SINGLETON_H
#define SP_SINGLETON_H

#include <memory>
#include <mutex>
#include <refbase.h>
#include "nocopyable.h"

namespace OHOS {
namespace PowerMgr {
#define DECLARE_DELAYED_SP_SINGLETON(MyClass) \
public: \
    MyClass(); \
    ~MyClass(); \
private: \
    friend DelayedSpSingleton<MyClass>;

template<typename T>
class DelayedSpSingleton : public NoCopyable {
public:
    static std::shared_ptr<T> GetInstance();
    static void DestroyInstance();

private:
    static std::shared_ptr<T> instance_;
    static std::mutex mutex_;
};

template<typename T>
std::shared_ptr<T> DelayedSpSingleton<T>::instance_ = nullptr;

template<typename T>
std::mutex DelayedSpSingleton<T>::mutex_;

template<typename T>
std::shared_ptr<T> DelayedSpSingleton<T>::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (instance_ == nullptr) {
            instance_ = std::make_shared<T>();
        }
    }

    return instance_;
}

template<typename T>
void DelayedSpSingleton<T>::DestroyInstance()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_) {
        instance_.reset();
        instance_ = nullptr;
    }
}
} // namespace ThermalMgr
} // namespace OHOS
#endif // SP_SINGLETON_H
