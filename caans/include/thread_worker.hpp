/**********************************************************************\
**                                                                    **
**       -=≡≣ Central Alerting And Notification Service  ≣≡=-        **
**                                                                    **
**          Copyright  2017 - 2021 by LLG Ryszard Gradowski          **
**                       All Rights Reserved.                         **
**                                                                    **
**  CAUTION! This application is an intellectual propery              **
**           of LLG Ryszard Gradowski. This application as            **
**           well as any part of source code cannot be used,          **
**           modified and distributed by third party person           **
**           without prior written permission issued by               **
**           intellectual property owner.                             **
**                                                                    **
\**********************************************************************/

#ifndef __THREAD_WORKER_HPP__
#define __THREAD_WORKER_HPP__

#undef THREAD_WORKER_DEBUG

#include <thread>
#include <condition_variable>

#ifdef THREAD_WORKER_DEBUG
#include <iostream>
#endif

#include "synchronized_queue.hpp"

template <typename T>
class thread_worker
{
public:

    thread_worker(void)
        : started_(false),
          terminate_(false)
    {}

    virtual ~thread_worker(void)
    {
        terminate();
    }

    void terminate(void)
    {
        terminate_ = true;

        if (started_)
        {
            std::unique_lock<std::mutex> lck(mtx_);
            cv_.notify_one();
        }

        if (thread_.joinable())
        {
            thread_.join();
        }

        started_ = false;
    }

    virtual void start_job(void)
    {
        if (started_)
        {
            return;
        }

        terminate_ = false;

        auto thread_function_wrapper = [this](void)
        {
            started_ = true;
            while (true)
            {
                std::unique_lock<std::mutex> lck(mtx_);

                while (! terminate_ && queue_.size() == 0)
                {
                    cv_.wait(lck);
                }

                if (terminate_)
                {
                    break;
                }

                T data;
                queue_.dequeue(data);
                work(data);
            }
        };

        thread_ = std::thread(thread_function_wrapper);
    }

    void enqueue(const T &data)
    {
        queue_.enqueue(data);

        for (int i = 0; i < 2; i++)
        {
            if (mtx_.try_lock())
            {
                mtx_.unlock();
                cv_.notify_one();

                return;
            }
        }
    }

protected:

    virtual void work(const T &data) {}

private:

    bool started_;
    bool terminate_;
    std::thread thread_;
    std::mutex mtx_;
    std::condition_variable cv_;
    synchronized_queue<T> queue_;
};

#endif /* __THREAD_WORKER_HPP__ */
