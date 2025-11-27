# CYThread

CYThread 是一个以 C++20 为基础的跨平台线程池库，目标是在 Windows / Linux / (实验性) macOS 等平台提供一致、低开销的任务调度与线程控制能力。核心实现位于 `Src/`，公有头文件放在 `Inc/CYThread/`，并提供了 `Build/CYThread.sln` 供 Visual Studio 直接编译出 `CYThread.dll`/`.lib`。

---

## 特性速览

- **双任务模型**：既支持通过 `CYThreadTask` 提交任意 `std::function`，也支持提交实现了 `ICYIThreadableObject` 的对象，让任务可以自带执行属性、状态和生命周期。  
- **C++20 实现**：内部使用 `std::jthread`、`std::stop_token` 和 `std::condition_variable` 组合实现可中断的 worker，避免裸线程资源泄漏。  
- **执行参数可调**：`CYThreadExecutionProps` 提供 CPU 亲和性、优先级、理想核心等设置；Windows 通过 `SetThreadAffinityMask`/`SetThreadPriority`，Linux 通过 `pthread_setaffinity_np`/`pthread_setschedparam` 落地。  
- **线程生命周期管理**：线程状态由 `CYThreadStatus` 枚举跟踪，线程池暴露 `Pause/Resume/Terminate/WaitForSingleObject` 等 API，方便和游戏引擎、实时子系统整合。  
- **Foundation 单例**：`CYThreadFoundation` 封装了线程池的创建、任务分发和资源回收，适合需要全局线程池的应用。  
- **系统画像**：`CYSystemDescription` 能够查询 CPU 数、物理内存和 Hyper-Threading 支持，为运行时自适应提供数据。

---

## 目录结构

- `Inc/`：对外发布的头文件（`CYThreadDefine.hpp`, `ICYThread.hpp`, `CYThreadFactory.hpp` 等）。  
- `Src/`：库的全部实现（线程/线程池/系统检测/平台特化等）。  
- `Build/`：Visual Studio 2022 解决方案与生成产物。  
- `LICENSE`：MIT 许可文本。

---

## 环境与依赖

- C++20 编译器（使用了 `<stop_token>`、`std::jthread`）。  
- 可选平台宏：`CYTHREAD_WIN_OS`, `CYTHREAD_UNIX_OS`, `CYTHREAD_MAC_OS`（详见 `CYThreadDefine.hpp`）。  
- Windows 版本默认构建为 DLL，如需静态库可自行调整工程配置或手动编译源文件。

---

## 构建方式

### CMake + 自动化脚本

仓库根目录下提供 `CMakeLists.txt`，并在 `Build/` 下准备了多平台的批处理/脚本，用来一次性产出不同架构、Debug/Release、静态/动态库组合。所有产物会被收集到 `Build/Artifacts/<Platform>/...`。

| 平台 | 脚本 | 说明 |
| --- | --- | --- |
| Windows | `Build/build_windows.bat` | 使用 VS 2022 生成器，遍历 `Win32/x64 × Debug/Release × (MD 动/静, MT 静态)`，输出 `.dll/.lib`。可通过设置 `CMAKE_GENERATOR` 覆盖。 |
| macOS | `Build/build_macos.sh` | 需要 `bash` 和 `cmake`（默认 `Ninja` 生成器），对 `Debug/Release × 静/动` 生成 arm64+x86_64 通用库。 |
| iOS | `Build/build_ios.sh` | 生成 `iphoneos` 与 `iphonesimulator` 的 arm64/x86_64 通用库，支持静/动态。可通过 `IOS_DEPLOYMENT_TARGET` 指定最低版本。 |
| Linux | `Build/build_linux.sh` | 遍历 `x86_64`、`aarch64`。非本机架构需要预先导出 `CYTHREAD_LINUX_TOOLCHAIN_<ARCH>=/path/toolchain.cmake`。 |
| Android | `Build/build_android.sh` | 自动检测常见 SDK/NDK 安装目录；默认生成 `arm64-v8a`, `armeabi-v7a`, `x86_64` 的静/动态库，API Level 21（可通过 `ANDROID_PLATFORM` 覆盖）。 |
| 多平台聚合（Unix） | `Build/build_all.sh` | 在当前主机支持范围内依次执行 macOS/iOS/Linux/Android 构建，实现一键拉起所有脚本。 |
| 多平台聚合（Windows） | `Build/build_all.bat` | 顺序调用 `build_windows.bat`，并在检测到 `bash.exe` 时通过它执行 `build_android.sh`。 |

> 所有 shell 脚本默认使用 `Ninja` 生成器，可通过 `export CMAKE_GENERATOR="Unix Makefiles"` 等方式覆盖。

运行 `build_all.sh`/`.bat` 即可一次性产出当前主机能构建的全部组合，同时会确保 Android SDK/NDK 自动定位，无需手动导出 `ANDROID_SDK_ROOT/ANDROID_NDK_HOME`。对于 Linux 交叉编译，仍需要提前提供对应的 toolchain 文件。

仍保留原有 Visual Studio 解决方案，可根据需要直接打开 `Build/Windows/CYThread.sln`。

---

## 快速上手

### 1. 通过工厂创建线程池

```cpp
#include "CYThread/CYThreadFactory.hpp"

cry::CYThreadFactory factory;
std::unique_ptr<cry::ICYThreadPool> pool(factory.CreateThreadPool());

const int maxThreads = std::thread::hardware_concurrency();
pool->CreateThreadPool(cry::CY_PLATFORM_WINDOWS, maxThreads);
```

### 2. 提交 `std::function` 形式的任务

```cpp
cry::CYThreadTask task;
task.funTaskToExecute = [](void* data, bool) {
    auto value = static_cast<int*>(data);
    fmt::print("Task value = {}\n", *value);
};
int payload = 42;
task.pArgList = &payload;

pool->SubmitTask(task);
```

### 3. 提交对象任务（具备独立执行参数）

```cpp
class ImageJob final : public cry::ICYIThreadableObject {
public:
    ImageJob(uint32_t id) { m_nObjectId = id; }

    void TaskToExecute() override {
        // 实际工作...
    }
};

ImageJob job{1};
pool->SubmitTask(&job);
```

对象任务可以通过 `GetExecutionProps()` 修改亲和性/优先级，例如：

```cpp
auto props = job.GetExecutionProps();
props->SetTasksPriority(cry::CYThreadPriority::PRIORITY_THREAD_HIGH);
props->CreateTasksProcessorAffinity();
```

### 4. 控制线程池生命周期

```cpp
while (pool->AreAnyThreadsWorking()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

pool->SuspendAllWorkingThreads();
pool->ResumeAllWorkingThreads();
pool->TerminateAllWorkingThreads();
```

### 5. 使用 `CYThreadFoundation`

`CYThreadFoundation` 提供了“单例线程池 + 分发”封装，适合模块化场景：

```cpp
#include "CYThreadFoundation.hpp"

cry::CYThreadFoundation foundation;
foundation.CreateThreadPool(8);

foundation.SubmitTask(&job);
foundation.Distribute(); // 将等待队列里的对象推送给空闲线程
```

---

## 关键类型概览

```60:152:Inc/CYThread/CYThreadDefine.hpp
enum class CYThreadStatus : uint8_t {
    STATUS_THREAD_NOT_EXECUTING,
    STATUS_THREAD_EXECUTING,
    STATUS_THREAD_PURGING,
    STATUS_THREAD_PAUSING,
    STATUS_THREAD_NONE,
};
```

- `CYThread`：包装 `std::jthread` 的 worker，负责任务切换、暂停/恢复、执行属性同步。  
- `CYThreadPool`：维护任务队列、miss 队列以及 worker 集合，可以按状态统计或等待特定对象完成。  
- `CYThreadPoolProperties`：约束最大线程/任务数并提供任务提交锁。  
- `CYThreadFactory`：生成/释放 `ICYThreadPool` 实例，隔离调用方与具体实现。  
- `CYThreadFoundation`：线程池单例 + 分发助手。  
- `CYSystemDescription`：查询硬件信息，辅助决定线程池规模。  
- `CYThreadedTask<T>`：模板工具，允许绑定成员函数作为 `ICYIThreadableObject`。

---

## 调试与排障

- **任务堆积**：调用 `GetTaskCount()`/`GetTasksMissedCount()` 判断队列压力，并考虑提升最大线程数 (`CYThreadPoolProperties::SetMaxThreads`)。  
- **线程未恢复**：确认提交任务后调用了 `Distribute()`（Foundation 模式）或 `SubmitTask()`（直接模式）后有对应的 `ResumeThread()` 发生；必要时查看 `GetSpecificThreadStatusCount(CYThreadStatus::STATUS_THREAD_PAUSING)`。  
- **CPU 亲和设置无效**：在非 Windows/Linux 平台目前默认为软亲和，你需要在实现中扩展 `ChangeThreadsExecutionProperties`。  
- **等待接口**：`WaitForSingleObject` 返回 `0` 表示任务结束，`1` 表示超时。

---

## 许可

CYThread 使用 MIT 许可证，详见 [`LICENSE`](LICENSE)。
