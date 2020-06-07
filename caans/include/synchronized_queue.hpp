/**********************************************************************\
**                                                                    **
**       -=≡≣ Central Alerting And Notification Service  ≣≡=-        **
**                                                                    **
**          Copyright  2017 - 2020 by LLG Ryszard Gradowski          **
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

#ifndef __SYNCHRONIZED_QUEUE_HPP__
#define __SYNCHRONIZED_QUEUE_HPP__

#include <list>
#include <mutex>

template <typename T>
class synchronized_queue
{
public:

    size_t size(void) const
    {
        std::unique_lock<std::mutex> lck(mtx_);

        return lst_.size();
    }

    void enqueue(const T &data)
    {
        std::unique_lock<std::mutex> lck(mtx_);

        lst_.push_back(data);
    }

    bool dequeue(T &data)
    {
        std::unique_lock<std::mutex> lck(mtx_);

        if (lst_.empty())
        {
            return false;
        }

        data = lst_.front();
        lst_.pop_front();

        return true;
    }

private:

    mutable std::mutex mtx_;
    std::list<T> lst_;
};

#endif /* __SYNCHRONIZED_QUEUE_HPP__ */
