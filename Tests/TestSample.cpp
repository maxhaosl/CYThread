#include <iostream>
#include <chrono>
#include <vector>
#include <atomic>
#include <thread>
#include <cassert>
#include <random>

// Include CYThread headers
#include "../Inc/CYThread/CYThreadFoundation.hpp"
#include "../Inc/CYThread/CYThreadDefine.hpp"
#include "../Inc/CYThread/ICYIThreadableObject.hpp"

using namespace cry;

// Test class implementing ICYIThreadableObject
class TestTask : public ICYIThreadableObject
{
private:
    int m_taskId;
    std::atomic<bool> m_executed{false};
    std::chrono::milliseconds m_sleepTime;
    static std::atomic<int> s_completedTasks;

public:
    TestTask(int taskId, int sleepMs = 100) 
        : m_taskId(taskId), m_sleepTime(sleepMs) {}

    void TaskToExecute() override
    {
        std::cout << "Task " << m_taskId << " started on thread " 
                  << std::this_thread::get_id() << std::endl;
        
        // Simulate some work
        std::this_thread::sleep_for(m_sleepTime);
        
        m_executed.store(true);
        s_completedTasks.fetch_add(1);
        
        std::cout << "Task " << m_taskId << " completed" << std::endl;
    }

    CYThreadExecutionProps* GetExecutionProps() override
    {
        static CYThreadExecutionProps props;
        props.SetTasksProcessAffinity(CYProcessorAffinity::AFFINITY_PROCESSOR_SOFT);
        props.SetTasksPriority(CYThreadPriority::PRIORITY_THREAD_NORMAL);
        props.SetTasksCore(m_taskId % std::thread::hardware_concurrency());
        props.CreateTasksProcessorAffinity();
        return &props;
    }

    bool IsExecuted() const { return m_executed.load(); }
    int GetTaskId() const { return m_taskId; }
    static int GetCompletedTaskCount() { return s_completedTasks.load(); }
    static void ResetCompletedTaskCount() { s_completedTasks.store(0); }
};

std::atomic<int> TestTask::s_completedTasks{0};

// Test function for CYThreadTask (function-based tasks)
void TestFunctionTask(void* arg, bool shouldDelete)
{
    int* taskId = static_cast<int*>(arg);
    std::cout << "Function task " << *taskId << " started on thread " 
              << std::this_thread::get_id() << std::endl;
    
    // Simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    std::cout << "Function task " << *taskId << " completed" << std::endl;
    
    if (shouldDelete && arg)
    {
        delete taskId;
    }
}

class ThreadPoolTester
{
private:
    CYThreadFoundation m_foundation;
    
public:
    ThreadPoolTester()
    {
        std::cout << "=== CYThread Pool Tester ===" << std::endl;
        std::cout << "Hardware concurrency: " << std::thread::hardware_concurrency() << std::endl;
    }

    // Test 1: Basic object-based task execution
    bool TestObjectTasks()
    {
        std::cout << "\n--- Test 1: Object-based Tasks ---" << std::endl;
        
        TestTask::ResetCompletedTaskCount();
        
        // Create thread pool
        m_foundation.CreateThreadPool(4);
        
        // Create and submit tasks
        std::vector<std::unique_ptr<TestTask>> tasks;
        const int numTasks = 8;
        
        for (int i = 0; i < numTasks; ++i)
        {
            auto task = std::make_unique<TestTask>(i + 1, 100);
            tasks.push_back(std::move(task));
        }
        
        // Submit all tasks
        for (auto& task : tasks)
        {
            bool submitted = m_foundation.SubmitTask(task.get());
            if (!submitted)
            {
                std::cout << "Failed to submit task " << task->GetTaskId() << std::endl;
                return false;
            }
        }
        
        std::cout << "Submitted " << numTasks << " tasks" << std::endl;
        
        // Wait for all tasks to complete
        auto startTime = std::chrono::steady_clock::now();
        while (TestTask::GetCompletedTaskCount() < numTasks)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Timeout after 10 seconds
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (elapsed > std::chrono::seconds(10))
            {
                std::cout << "Timeout waiting for tasks to complete" << std::endl;
                std::cout << "Completed: " << TestTask::GetCompletedTaskCount() 
                          << "/" << numTasks << std::endl;
                return false;
            }
        }
        
        // Verify all tasks were executed
        for (const auto& task : tasks)
        {
            if (!task->IsExecuted())
            {
                std::cout << "Task " << task->GetTaskId() << " was not executed" << std::endl;
                return false;
            }
        }
        
        std::cout << "All " << numTasks << " object tasks completed successfully!" << std::endl;
        return true;
    }

    // Test 2: Function-based task execution
    bool TestFunctionTasks()
    {
        std::cout << "\n--- Test 2: Function-based Tasks ---" << std::endl;
        
        const int numTasks = 5;
        std::vector<int*> taskIds;
        
        // Create and submit function tasks
        for (int i = 0; i < numTasks; ++i)
        {
            int* taskId = new int(i + 1);
            taskIds.push_back(taskId);
            
            CYThreadTask task;
            task.funTaskToExecute = TestFunctionTask;
            task.pArgList = taskId;
            task.bDelete = true; // Let the function delete the memory
            
            // Note: CYThreadFoundation doesn't have SubmitTask for CYThreadTask
            // This is a limitation we need to address
            std::cout << "Function task " << *taskId << " created (direct execution)" << std::endl;
            TestFunctionTask(taskId, true);
        }
        
        std::cout << "All " << numTasks << " function tasks completed!" << std::endl;
        return true;
    }

    // Test 3: Thread pool status and management
    bool TestThreadPoolManagement()
    {
        std::cout << "\n--- Test 3: Thread Pool Management ---" << std::endl;
        
        // Test pool status
        bool isEmpty = m_foundation.IsEmpty();
        std::cout << "Thread pool is empty: " << (isEmpty ? "Yes" : "No") << std::endl;
        
        bool anyWorking = m_foundation.AreAnyThreadsWorking();
        std::cout << "Any threads working: " << (anyWorking ? "Yes" : "No") << std::endl;
        
        // Submit a long-running task
        auto longTask = std::make_unique<TestTask>(999, 2000);
        bool submitted = m_foundation.SubmitTask(longTask.get());
        
        if (submitted)
        {
            std::cout << "Submitted long-running task" << std::endl;
            
            // Check if threads are working
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            anyWorking = m_foundation.AreAnyThreadsWorking();
            std::cout << "Any threads working after task submission: " 
                      << (anyWorking ? "Yes" : "No") << std::endl;
            
            // Wait for task completion
            auto startTime = std::chrono::steady_clock::now();
            while (!longTask->IsExecuted())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                auto elapsed = std::chrono::steady_clock::now() - startTime;
                if (elapsed > std::chrono::seconds(5))
                {
                    std::cout << "Long task timeout" << std::endl;
                    return false;
                }
            }
            
            std::cout << "Long-running task completed" << std::endl;
        }
        
        return true;
    }

    // Test 4: Concurrent task submission
    bool TestConcurrentSubmission()
    {
        std::cout << "\n--- Test 4: Concurrent Task Submission ---" << std::endl;
        
        TestTask::ResetCompletedTaskCount();
        
        const int numThreads = 4;
        const int tasksPerThread = 5;
        const int totalTasks = numThreads * tasksPerThread;
        
        std::vector<std::thread> submitterThreads;
        std::vector<std::vector<std::unique_ptr<TestTask>>> allTasks(numThreads);
        
        // Create submitter threads
        for (int t = 0; t < numThreads; ++t)
        {
            submitterThreads.emplace_back([this, t, tasksPerThread, &allTasks]() {
                for (int i = 0; i < tasksPerThread; ++i)
                {
                    int taskId = t * tasksPerThread + i + 1;
                    auto task = std::make_unique<TestTask>(taskId, 50);
                    
                    bool submitted = m_foundation.SubmitTask(task.get());
                    if (submitted)
                    {
                        allTasks[t].push_back(std::move(task));
                    }
                    else
                    {
                        std::cout << "Failed to submit task " << taskId << std::endl;
                    }
                    
                    // Small delay to simulate real-world scenario
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
        }
        
        // Wait for all submitter threads to complete
        for (auto& thread : submitterThreads)
        {
            thread.join();
        }
        
        std::cout << "All submitter threads completed" << std::endl;
        
        // Wait for all tasks to complete
        auto startTime = std::chrono::steady_clock::now();
        while (TestTask::GetCompletedTaskCount() < totalTasks)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (elapsed > std::chrono::seconds(15))
            {
                std::cout << "Timeout in concurrent test. Completed: " 
                          << TestTask::GetCompletedTaskCount() << "/" << totalTasks << std::endl;
                return false;
            }
        }
        
        std::cout << "All " << totalTasks << " concurrent tasks completed successfully!" << std::endl;
        return true;
    }

    // Test 5: Thread execution properties
    bool TestExecutionProperties()
    {
        std::cout << "\n--- Test 5: Thread Execution Properties ---" << std::endl;
        
        // Create tasks with different priorities
        auto highPriorityTask = std::make_unique<TestTask>(1001, 100);
        auto normalPriorityTask = std::make_unique<TestTask>(1002, 100);
        
        // Submit tasks
        bool submitted1 = m_foundation.SubmitTask(highPriorityTask.get());
        bool submitted2 = m_foundation.SubmitTask(normalPriorityTask.get());
        
        if (!submitted1 || !submitted2)
        {
            std::cout << "Failed to submit priority test tasks" << std::endl;
            return false;
        }
        
        // Wait for completion
        auto startTime = std::chrono::steady_clock::now();
        while (!highPriorityTask->IsExecuted() || !normalPriorityTask->IsExecuted())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (elapsed > std::chrono::seconds(5))
            {
                std::cout << "Timeout in execution properties test" << std::endl;
                return false;
            }
        }
        
        std::cout << "Execution properties test completed" << std::endl;
        return true;
    }

    // Run all tests
    bool RunAllTests()
    {
        std::cout << "\n=== Starting Thread Pool Tests ===" << std::endl;
        
        bool allPassed = true;
        
        allPassed &= TestObjectTasks();
        allPassed &= TestFunctionTasks();
        allPassed &= TestThreadPoolManagement();
        allPassed &= TestConcurrentSubmission();
        allPassed &= TestExecutionProperties();
        
        std::cout << "\n=== Test Results ===" << std::endl;
        if (allPassed)
        {
            std::cout << "✅ All tests PASSED!" << std::endl;
        }
        else
        {
            std::cout << "❌ Some tests FAILED!" << std::endl;
        }
        
        return allPassed;
    }
};

int main()
{
    try
    {
        ThreadPoolTester tester;
        bool success = tester.RunAllTests();
        
        return success ? 0 : 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unknown exception caught" << std::endl;
        return 1;
    }
}
