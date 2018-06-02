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

template<typename T>
inline void IncRef(T* item)
{
    item->m_refs++;
}

template<typename T>
inline void DecRef(T* item)
{
    item->m_refs--;
}

template<typename T>
inline s32 RefCount(const T* item)
{
    return item->m_refs;
}

template<typename T>
inline void OnLoadAsync(T* item, u32 name)
{

}

template<typename T>
inline void OnLoadSync(T* item, u32 name)
{

}

template<typename T>
inline void OnRelease(T* item, u32 name)
{

}

template<typename T, typename U, u32 capacity, u32 req_capacity=128>
class AssetStore
{
    typedef AssetStore<T, U, capacity, req_capacity> _ThisType;

    struct Placement
    {
        T m_item;
        u32 m_name;
    };

    Store<T, capacity> m_store;
    CircularQueue<u32, req_capacity> m_requests;
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
    U* get(u32 name)
    {
        T* pElement = m_store[name];
        if(pElement)
        {
            return &pElement->m_item;
        }
        return nullptr;
    }
    void request(u32 name)
    {
        T* pElement = m_store[name];
        if(pElement)
        {
            IncRef(pElement);
        }
        else if(!m_requests.full())
        {
            m_requests.push(name);
        }
    }
    void release(u32 name)
    {
        T* pElement = m_store[name];
        if(pElement)
        {
            DecRef(pElement);
            if(RefCount(pElement) <= 0)
            {
                OnRelease(pElement, name);
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
                u32 name = m_requests.pop();
                Placement p;
                p.m_name = name;
                OnLoadAsync(&p.m_item, name);
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
                IncRef(item);
            }
            else
            {
                OnLoadSync(&p.m_item, p.m_name);
                IncRef(&p.m_item);
                m_store.insert(p.m_name, p.m_item);
            }
            return 1;
        }
        return 0;
    }
    U* operator[](u32 name){ return get(name); }
};
