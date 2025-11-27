// CYThreadTest.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

#include "CYThread/ICYThread.hpp"
#include "CYThread/CYThreadFactory.hpp"

int main()
{
    cry::CYThreadFactory factory;
    std::unique_ptr<cry::ICYThreadPool> pool(factory.CreateThreadPool());

    const int maxThreads = std::thread::hardware_concurrency();
    pool->CreateThreadPool(cry::CY_PLATFORM_WINDOWS, maxThreads);

    cry::CYThreadTask task;
    task.funTaskToExecute = [](void* data, bool) {
        auto value = static_cast<int*>(data);
        printf("Task value = %d\n", *value);
        };
    int payload = 42;
    task.pArgList = &payload;

    pool->SubmitTask(task);

    class ImageJob final : public cry::ICYIThreadableObject
    {
    public:
        ImageJob(uint32_t id) { m_nObjectId = id; }

        void TaskToExecute() override
        {
            printf("TaskToExecute\n");
        }
    };

    ImageJob job{ 1 };
    pool->SubmitTask(&job);

    auto props = job.GetExecutionProps();
    props->SetTasksPriority(cry::CYThreadPriority::PRIORITY_THREAD_HIGH);
    props->CreateTasksProcessorAffinity();

    while (pool->AreAnyThreadsWorking())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    pool->SuspendAllWorkingThreads();
    pool->ResumeAllWorkingThreads();
    pool->TerminateAllWorkingThreads();

    std::cout << "Hello World!\n";

    return 0;
}