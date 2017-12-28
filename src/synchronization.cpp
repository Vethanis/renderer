#include "synchronization.h"

#include <mutex>

SyncGuard::SyncGuard(std::mutex& mtx) : lock(mtx)
{
    lock.lock();
}

SyncGuard::~SyncGuard()
{
    lock.unlock();
}