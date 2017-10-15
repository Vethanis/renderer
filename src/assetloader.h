#pragma once

#include "array.h"
#include "store.h"
#include "circular_queue.h"
#include <thread>
#include <mutex>
#include <chrono>

template<bool _inplace, typename T, int _capacity, int _num_threads = 4, int _sleep_ms = 30>
struct AssetStore
{
    typedef AssetStore<_inplace, T, _capacity, _num_threads, _sleep_ms> ThisType;
    typedef void (*LoadFn)(T*, unsigned);
    typedef void (*ReuseFn)(T*);

    Store<T, _capacity> m_store;
    CircularQueue<unsigned, _capacity> m_queue;
    Array<std::thread, _num_threads> m_threads;
    std::mutex m_mutex;
    LoadFn m_loadCB;
    ReuseFn m_reuseCB;
    bool m_shouldRun;

    void processQueue()
    {
        while(m_shouldRun)
        {
            if(!m_queue.empty())
            {
                while(m_shouldRun && !m_queue.empty())
                {
                    unsigned name = m_queue.pop();
                    T m;
                    m_loadCB(&m, name);
                    
                    std::lock_guard<std::mutex> guard(m_mutex);
                    if(m_store.full())
                    {
                        T* p = m_store.reuse_near(name);
                        m_reuseCB(p);
                        *p = m;
                    }
                    else
                    {
                        m_store.insert(name, m);
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(_sleep_ms));
        }
    }

    void processInPlace()
    {
        while(m_shouldRun)
        {
            if(!m_queue.empty())
            {
                while(m_shouldRun && !m_queue.empty())
                {
                    if(m_mutex.try_lock())
                    {
                        unsigned name = m_queue.pop();
                        T* m;
                        if(m_store.full())
                        {
                            m = m_store.reuse_near(name);
                            m_reuseCB(m);
                        }
                        else
                        {
                            m_store.insert(name, {});
                            m = m_store[name];
                        }
                        m_loadCB(m, name);

                        m_mutex.unlock();
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(_sleep_ms));
        }
    }

    AssetStore(LoadFn loader, ReuseFn reuser)
    {
        m_loadCB = loader;
        m_reuseCB = reuser;
        m_shouldRun = true;

        if(_inplace)
        {
            m_threads.grow() = std::thread(&ThisType::processInPlace, this);
        }
        else
        {
            for(int i = 0; i < _num_threads; ++i)
            {
                m_threads.grow() = std::thread(&ThisType::processQueue, this);
            }
        }
    }
    ~AssetStore()
    {
        m_shouldRun = false;
        for(auto& i : m_threads)
        {
            i.join();
        }
    }

    T* get(unsigned name){
        T* m = m_store[name];
        if(m){return m;}

        if(m_queue.full() == false){
            m_queue.set_push(name);
        }
        return nullptr;
    }
    T* operator[](unsigned name){ return get(name); }
};