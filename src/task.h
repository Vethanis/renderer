#pragma once

#include "array.h"
#include <thread>
#include <mutex>
#include <chrono>

typedef void (*TaskFn)(void*);

struct Task
{
    TaskFn m_function;
    TaskFn m_destructor;
    char m_data[64];
    void call()
    {
        m_function((void*)m_data);
        m_destructor((void*)m_data);
    }
};

class TaskManager
{
    Vector<Task> m_tasks;
    std::thread m_threads[4];
    std::mutex m_lock;
    volatile bool m_running;
public:
    TaskManager()
    {
        m_running = true;
        for(std::thread& t : m_threads)
        {
            t = std::thread(&TaskManager::ThreadUpdate, this);
        }
    }
    ~TaskManager()
    {
        m_running = false;
        for(std::thread& t : m_threads)
        {
            t.join();
        }
    }
    void ThreadUpdate()
    {
        while(m_running)
        {
            while(m_tasks.count())
            {
                Task task;
                bool doTask = false;
                m_lock.lock();
                if(m_tasks.count())
                {
                    task = m_tasks.pop();
                    doTask = true;
                }
                m_lock.unlock();
                if(doTask)
                {
                    task.call();
                }
            }
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1ms);
        }
    }
    template<typename T>
    void CreateTask(const T& data)
    {
        Task task;
        assert(sizeof(T) <= sizeof(task.m_data));
        task.m_function = [](void* pVoid)
        {
            T* item = static_cast<T*>(pVoid);
            T& rItem = *item;
            rItem();
        };
        task.m_destructor = [](void* pVoid)
        {
            T* pItem = static_cast<T*>(pVoid);
            pItem->~T();
        };
        void* p = task.m_data;
        T* pT = (T*)p;
        new (pT) T();
        *pT = data;
        
        m_lock.lock();
        m_tasks.grow() = task;
        m_lock.unlock();
    }
};

extern TaskManager g_TaskManager;