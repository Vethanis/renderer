#pragma once

#include "store.h"
#include "array.h"
#include "circular_queue.h"
#include <chrono>
#include <thread>

typedef int (*UpdateFn)(void*);

struct AssetStoreUpdateFn
{
    UpdateFn m_updateFn;
    void* m_instance;

    int Call()
    {
       return m_updateFn(m_instance);
    }
};

extern Array<AssetStoreUpdateFn, 64> g_AssetStoreUpdates;

inline void UpdateAssetStores()
{
    for(AssetStoreUpdateFn& fn : g_AssetStoreUpdates)
    {
        fn.Call();
    }
}

template<typename T, typename U, unsigned capacity, unsigned req_capacity=128>
class AssetStore
{
    typedef AssetStore<T, U, capacity, req_capacity> _ThisType;

    struct Placement
    {
        T m_item;
        unsigned m_name;
    };

    Store<T, capacity> m_store;
    CircularQueue<unsigned, req_capacity> m_requests;
    CircularQueue<Placement, req_capacity> m_placements;
    std::thread m_thread;
    volatile bool m_asyncRunning;
public:
    AssetStore()
    {
        g_AssetStoreUpdates.grow() = { _ThisType::update, this };
        m_asyncRunning = true;
        m_thread = std::thread(_ThisType::updateAsync, this);
    }
    ~AssetStore()
    {
        m_asyncRunning = false;
        m_thread.join();
    }
    U* get(unsigned name)
    {
        T* pElement = m_store[name];
        if(pElement)
            return pElement->GetItem();
        return nullptr;
    }
    void request(unsigned name)
    {
        T* pElement = m_store[name];
        if(pElement)
        {
            pElement->AddRef();
        }
        else if(!m_requests.full())
        {
            m_requests.push(name);
        }
    }
    void release(unsigned name)
    {
        T* pElement = m_store[name];
        if(pElement)
        {
            pElement->RemoveRef();
            if(pElement->RefCount() <= 0)
            {
                pElement->OnRelease(name);
                m_store.remove(name);
            }
        }
    }
    static void updateAsync(void* instance)
    {
        static_cast<_ThisType*>(instance)->MupdateAsync();
    }
    void MupdateAsync()
    {
        while(m_asyncRunning)
        {
            while(!m_requests.empty() && !m_placements.full())
            {
                unsigned name = m_requests.pop();
                Placement p;
                p.m_name = name;
                p.m_item.OnLoadAsync(name);
                m_placements.push(p);
            }
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(30ms);
        }
    }
    static int update(void* instance)
    {
        return static_cast<_ThisType*>(instance)->Mupdate();
    }
    int Mupdate()
    {
        while(!m_store.full() && !m_placements.empty())
        {
            Placement p = m_placements.pop();
            T* item = m_store[p.m_name];
            if(item)
            {
                item->AddRef();
            }
            else
            {
                p.m_item.OnLoadSync(p.m_name);
                p.m_item.AddRef();
                m_store.insert(p.m_name, p.m_item);
            }
            return 1;
        }
        return 0;
    }
    U* operator[](unsigned name){ return get(name); }
};
