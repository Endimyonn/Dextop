#pragma once

#include "BS_thread_pool.hpp"

class ThreadPoolContainer
{
    public:
        inline static ThreadPoolContainer* instance = nullptr;
        inline static BS::thread_pool pool {0};
        ThreadPoolContainer()
        {
            instance = this;
        }
};
#define dtThreadPool ThreadPoolContainer::pool