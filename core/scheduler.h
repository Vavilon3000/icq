#ifndef __SCHEDULER_H_
#define __SCHEDULER_H_

#pragma once

namespace core
{
    class async_executer;

    class scheduler
    {
        struct scheduler_timer_task
        {
            std::chrono::system_clock::time_point last_execute_time_;
            uint32_t id_;
            std::chrono::milliseconds timeout_;
            std::function<void()> function_;

            scheduler_timer_task() : id_(0), timeout_(0) {}
        };

        std::unique_ptr<std::thread> thread_;
        std::list<std::shared_ptr<scheduler_timer_task>> timed_tasks_;

        std::mutex mutex_;
        std::condition_variable condition_;
        std::atomic<bool> is_stop_;

    public:

        uint32_t push_timer(std::function<void()> _function, std::chrono::milliseconds _timeout);
        uint32_t push_timer(std::function<void()> _function, uint32_t _timeout_msec)
        {
            return push_timer(std::move(_function), std::chrono::milliseconds(_timeout_msec));
        }
        void stop_timer(uint32_t _id);

        scheduler();
        virtual ~scheduler();
    };

}

#endif //__SCHEDULER_H_