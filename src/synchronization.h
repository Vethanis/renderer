#pragma once

namespace std
{
    class mutex;
};

class SyncGuard
{
    std::mutex& lock;
public:
    SyncGuard(std::mutex& mtx);
    ~SyncGuard();
};