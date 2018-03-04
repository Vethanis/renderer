#pragma once

#include "store.h"
#include "array.h"

typedef bool (*UpdateFn)(void*);

struct AssetStoreUpdateFn
{
    UpdateFn m_updateFn;
    void* m_instance;

    bool Call()
    {
       return m_updateFn(m_instance);
    }
};

extern Array<AssetStoreUpdateFn, 64> g_AssetStoreUpdates;

inline void UpdateAssetStores()
{
    for(AssetStoreUpdateFn& fn : g_AssetStoreUpdates)
    {
        if(fn.Call())
            return;
    }
}

template<typename T, typename U, unsigned capacity>
class AssetStore
{
    typedef AssetStore<T, U, capacity> _ThisType;

    Store<T, capacity> m_store;
    Vector<unsigned> m_requests;
public:
    AssetStore()
    {
        g_AssetStoreUpdates.grow() = { _ThisType::update, this };
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
        else
        {
            m_requests.grow() = name;
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
    static bool update(void* instance)
    {
        _ThisType* pInstance = static_cast<_ThisType*>(instance);

        if(!pInstance->m_requests.count())
            return false;

        if(pInstance->m_store.full())
        {
            return false;
        }
        
        const unsigned name = pInstance->m_requests.pop();

        T* item = pInstance->m_store[name];
        if(item)
        {
            item->AddRef();
            return false;
        }

        T m;
        m.OnLoad(name);
        m.AddRef();
        pInstance->m_store.insert(name, m);

        return true;
    }
    U* operator[](unsigned name){ return get(name); }
};
