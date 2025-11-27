#include <iostream>
#include <chrono>
#include <atomic>
#include <thread>

// Include CYThread headers
#include "../Src/CYThreadFoundation.hpp"
#include "../Inc/CYThread/ICYThread.hpp"

using namespace cry;

// Quick test task
class QuickTask : public ICYIThreadableObject
{
private:
    int m_id;
    static std::atomic<int> s_count;

public:
    QuickTask(int id) : m_id(id) {}

    void TaskToExecute() override
    {
        std::cout << "Task " << m_id << " executing on thread " 
                  << std::this_thread::get_id() << std::endl;
        
        // Very short work
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        s_count.fetch_add(1);
        std::cout << "Task " << m_id << " completed (total: " << s_count.load() << ")" << std::endl;
    }

    CYThreadExecutionProps* GetExecutionProps() override
    {
        return nullptr; // Use default properties
    }

    static int GetCount() { return s_count.load(); }
    static void Reset() { s_count.store(0); }
};

std::atomic<int> QuickTask::s_count{0};

int main()
{
    std::cout << "=== CYThread Quick Test ===" << std::endl;

    try
    {
        // Create foundation and thread pool
        CYThreadFoundation foundation;
        foundation.CreateThreadPool(2);
        
        std::cout << "Created thread pool with 2 threads" << std::endl;
        
        // Create and submit a few tasks
        QuickTask task1(1);
        QuickTask task2(2);
        QuickTask task3(3);
        
        std::cout << "Submitting tasks..." << std::endl;
        
        bool ok1 = foundation.SubmitTask(&task1);
        bool ok2 = foundation.SubmitTask(&task2);
        bool ok3 = foundation.SubmitTask(&task3);
        
        std::cout << "Task submission results: " << ok1 << ", " << ok2 << ", " << ok3 << std::endl;
        
        if (!ok1 || !ok2 || !ok3)
        {
            std::cout << "❌ Failed to submit some tasks" << std::endl;
            return 1;
        }
        
        // Wait a bit for tasks to complete
        std::cout << "Waiting for tasks to complete..." << std::endl;
        
        for (int i = 0; i < 50; ++i) // Wait up to 5 seconds
        {
            if (QuickTask::GetCount() >= 3)
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        int completed = QuickTask::GetCount();
        std::cout << "Completed tasks: " << completed << "/3" << std::endl;
        
        if (completed == 3)
        {
            std::cout << "✅ All tasks completed successfully!" << std::endl;
            return 0;
        }
        else
        {
            std::cout << "❌ Not all tasks completed" << std::endl;
            return 1;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }
}
