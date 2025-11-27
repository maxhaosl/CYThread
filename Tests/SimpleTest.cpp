#include <iostream>
#include <chrono>
#include <vector>
#include <atomic>
#include <thread>
#include <memory>

// Include CYThread headers
#include "../Src/CYThreadFoundation.hpp"
#include "../Inc/CYThread/ICYThread.hpp"

using namespace cry;

// Simple test task
class SimpleTestTask : public ICYIThreadableObject
{
private:
    int m_taskId;
    std::atomic<bool> m_completed{false};
    static std::atomic<int> s_completedCount;

public:
    SimpleTestTask(int id) : m_taskId(id) {}

    void TaskToExecute() override
    {
        std::cout << "[Task " << m_taskId << "] Started on thread " 
                  << std::this_thread::get_id() << std::endl;
        
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(100 + (m_taskId % 5) * 50));
        
        m_completed.store(true);
        s_completedCount.fetch_add(1);
        
        std::cout << "[Task " << m_taskId << "] Completed" << std::endl;
    }

    CYThreadExecutionProps* GetExecutionProps() override
    {
        // Return nullptr for default properties
        return nullptr;
    }

    bool IsCompleted() const { return m_completed.load(); }
    int GetTaskId() const { return m_taskId; }
    
    static int GetCompletedCount() { return s_completedCount.load(); }
    static void ResetCount() { s_completedCount.store(0); }
};

std::atomic<int> SimpleTestTask::s_completedCount{0};

int main()
{
    std::cout << "=== CYThread Simple Test ===" << std::endl;
    std::cout << "Hardware concurrency: " << std::thread::hardware_concurrency() << std::endl;

    try
    {
        // Create thread foundation
        CYThreadFoundation foundation;
        
        // Create thread pool with 4 threads
        std::cout << "\nCreating thread pool with 4 threads..." << std::endl;
        foundation.CreateThreadPool(4);
        
        // Test 1: Basic task execution
        std::cout << "\n--- Test 1: Basic Task Execution ---" << std::endl;
        
        const int numTasks = 8;
        std::vector<std::unique_ptr<SimpleTestTask>> tasks;
        
        // Create tasks
        for (int i = 0; i < numTasks; ++i)
        {
            tasks.push_back(std::make_unique<SimpleTestTask>(i + 1));
        }
        
        // Submit tasks
        std::cout << "Submitting " << numTasks << " tasks..." << std::endl;
        for (auto& task : tasks)
        {
            bool submitted = foundation.SubmitTask(task.get());
            if (!submitted)
            {
                std::cout << "❌ Failed to submit task " << task->GetTaskId() << std::endl;
                return 1;
            }
        }
        
        std::cout << "All tasks submitted. Waiting for completion..." << std::endl;
        
        // Wait for all tasks to complete
        auto startTime = std::chrono::steady_clock::now();
        while (SimpleTestTask::GetCompletedCount() < numTasks)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // Progress indicator
            int completed = SimpleTestTask::GetCompletedCount();
            std::cout << "Progress: " << completed << "/" << numTasks << " tasks completed\r" << std::flush;
            
            // Timeout check (10 seconds)
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (elapsed > std::chrono::seconds(10))
            {
                std::cout << "\n❌ Timeout! Only " << completed << "/" << numTasks 
                          << " tasks completed" << std::endl;
                return 1;
            }
        }
        
        std::cout << "\n✅ All " << numTasks << " tasks completed successfully!" << std::endl;
        
        // Verify all tasks are marked as completed
        bool allCompleted = true;
        for (const auto& task : tasks)
        {
            if (!task->IsCompleted())
            {
                std::cout << "❌ Task " << task->GetTaskId() << " not marked as completed" << std::endl;
                allCompleted = false;
            }
        }
        
        if (allCompleted)
        {
            std::cout << "✅ All tasks verified as completed" << std::endl;
        }
        
        // Test 2: Thread pool status
        std::cout << "\n--- Test 2: Thread Pool Status ---" << std::endl;
        
        bool isEmpty = foundation.IsEmpty();
        bool anyWorking = foundation.AreAnyThreadsWorking();
        
        std::cout << "Thread pool is empty: " << (isEmpty ? "Yes" : "No") << std::endl;
        std::cout << "Any threads working: " << (anyWorking ? "Yes" : "No") << std::endl;
        
        // Test 3: Quick burst test
        std::cout << "\n--- Test 3: Quick Burst Test ---" << std::endl;
        
        SimpleTestTask::ResetCount();
        std::vector<std::unique_ptr<SimpleTestTask>> burstTasks;
        
        const int burstSize = 20;
        for (int i = 0; i < burstSize; ++i)
        {
            burstTasks.push_back(std::make_unique<SimpleTestTask>(100 + i));
        }
        
        // Submit all tasks quickly
        auto submitStart = std::chrono::steady_clock::now();
        for (auto& task : burstTasks)
        {
            foundation.SubmitTask(task.get());
        }
        auto submitEnd = std::chrono::steady_clock::now();
        
        auto submitTime = std::chrono::duration_cast<std::chrono::microseconds>(submitEnd - submitStart);
        std::cout << "Submitted " << burstSize << " tasks in " << submitTime.count() << " microseconds" << std::endl;
        
        // Wait for completion
        startTime = std::chrono::steady_clock::now();
        while (SimpleTestTask::GetCompletedCount() < burstSize)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (elapsed > std::chrono::seconds(15))
            {
                std::cout << "❌ Burst test timeout!" << std::endl;
                return 1;
            }
        }
        
        auto totalTime = std::chrono::steady_clock::now() - submitStart;
        auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(totalTime);
        
        std::cout << "✅ Burst test completed in " << totalMs.count() << " ms" << std::endl;
        
        std::cout << "\n=== All Tests Completed Successfully! ===" << std::endl;
        
        // Give some time for cleanup
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "❌ Unknown exception occurred" << std::endl;
        return 1;
    }
}
