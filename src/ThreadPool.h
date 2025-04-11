#pragma once

#include "bshoshany/BS_thread_pool.hpp"

class ThreadPoolContainer
{
    public:
        inline static ThreadPoolContainer* instance = nullptr;
        BS::thread_pool pool;
        ThreadPoolContainer()
        {
            pool = BS::thread_pool();
        }
        ThreadPoolContainer(int threadCount)
        {
            pool = BS::thread_pool(threadCount);
        }
}
#define dtThreadPool *instance.pool