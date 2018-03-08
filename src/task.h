#pragma once

#include "circular_queue.h"
#include <thread>
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
    CircularQueue<Task, 256> m_tasks;
    std::thread m_threads[4];
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
            while(!m_tasks.empty())
            {
                Task task = m_tasks.pop();
                task.call();
            }
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(30ms);
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
        assert(!m_tasks.full());
        m_tasks.push(task);
    }
};

extern TaskManager g_TaskManager;