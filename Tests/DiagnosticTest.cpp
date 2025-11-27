#include <iostream>
#include <chrono>
#include <atomic>
#include <thread>

// Include CYThread headers
#include "../Src/CYThreadFoundation.hpp"
#include "../Inc/CYThread/ICYThread.hpp"

using namespace cry;

// Diagnostic test task
class DiagTask : public ICYIThreadableObject
{
private:
    int m_id;
    static std::atomic<int> s_count;

public:
    DiagTask(int id) : m_id(id) {}

    void TaskToExecute() override
    {
        std::cout << "[TASK " << m_id << "] Started execution" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        s_count.fetch_add(1);
        std::cout << "[TASK " << m_id << "] Completed (total: " << s_count.load() << ")" << std::endl;
    }

    CYThreadExecutionProps* GetExecutionProps() override
    {
        std::cout << "[TASK " << m_id << "] GetExecutionProps called" << std::endl;
        return nullptr;
    }

    static int GetCount() { return s_count.load(); }
    static void Reset() { s_count.store(0); }
};

std::atomic<int> DiagTask::s_count{0};

int main()
{
    std::cout << "=== CYThread Diagnostic Test ===" << std::endl;

    try
    {
        std::cout << "[MAIN] Creating CYThreadFoundation..." << std::endl;
        CYThreadFoundation foundation;
        
        std::cout << "[MAIN] Creating thread pool with 2 threads..." << std::endl;
        foundation.CreateThreadPool(2);
        
        std::cout << "[MAIN] Thread pool created" << std::endl;
        
        // Check initial status
        bool isEmpty = foundation.IsEmpty();
        bool anyWorking = foundation.AreAnyThreadsWorking();
        std::cout << "[MAIN] Pool empty: " << isEmpty << ", Any working: " << anyWorking << std::endl;
        
        // Create tasks
        std::cout << "[MAIN] Creating tasks..." << std::endl;
        DiagTask task1(1);
        DiagTask task2(2);
        
        std::cout << "[MAIN] Submitting task 1..." << std::endl;
        bool ok1 = foundation.SubmitTask(&task1);
        std::cout << "[MAIN] Task 1 submission result: " << ok1 << std::endl;
        
        std::cout << "[MAIN] Submitting task 2..." << std::endl;
        bool ok2 = foundation.SubmitTask(&task2);
        std::cout << "[MAIN] Task 2 submission result: " << ok2 << std::endl;
        
        if (!ok1 || !ok2)
        {
            std::cout << "[MAIN] ❌ Failed to submit tasks" << std::endl;
            return 1;
        }
        
        // Check status after submission
        anyWorking = foundation.AreAnyThreadsWorking();
        std::cout << "[MAIN] After submission - Any working: " << anyWorking << std::endl;
        
        // Wait and check progress
        std::cout << "[MAIN] Waiting for task completion..." << std::endl;
        
        for (int i = 0; i < 20; ++i) // Wait up to 2 seconds
        {
            int completed = DiagTask::GetCount();
            std::cout << "[MAIN] Check " << i << ": Completed " << completed << "/2 tasks" << std::endl;
            
            if (completed >= 2)
            {
                std::cout << "[MAIN] ✅ All tasks completed!" << std::endl;
                return 0;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::cout << "[MAIN] ❌ Timeout - tasks did not complete" << std::endl;
        std::cout << "[MAIN] Final completed count: " << DiagTask::GetCount() << std::endl;
        
        // Final status check
        anyWorking = foundation.AreAnyThreadsWorking();
        isEmpty = foundation.IsEmpty();
        std::cout << "[MAIN] Final status - Empty: " << isEmpty << ", Any working: " << anyWorking << std::endl;
        
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[MAIN] ❌ Exception: " << e.what() << std::endl;
        return 1;
    }
}
