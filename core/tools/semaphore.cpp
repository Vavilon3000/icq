#include "stdafx.h"
#include "semaphore.h"

using namespace core;
using namespace tools;

semaphore::semaphore(unsigned long count)
    : count_(count)
{
}


semaphore::~semaphore()
{
}

void semaphore::notify()
{
    boost::unique_lock<boost::mutex> lock(mtx_);
    ++count_;
    cv_.notify_one();
}

void semaphore::wait()
{
    boost::unique_lock<boost::mutex> lock(mtx_);

    cv_.wait(lock, [&]{ return count_ > 0; });
    --count_;
}