/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System  ≣≡=-              **
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

#ifndef __RANGE_HPP__
#define __RANGE_HPP__

#include <stdexcept>

#include <custom_except.hpp>

template <typename T>
class range
{
public:

    DEFINE_CUSTOM_EXCEPTION_CLASS(bad_range, std::runtime_error)

    range<T>(T begin, T end)
        : begin_(begin), end_(end) {}

    bool in_range(T val) const
    {
        return (val >= begin_ && val < end_);
    }

    bool empty(void) const
    {
        return (begin_ == end_);
    }

    bool operator ==(const range<T> &rhs) const
    {
        return (begin_ == rhs.begin_ && end_ == rhs.end_);
    }

    bool operator <(const range<T> &rhs) const
    {
        return begin_ < rhs.begin_;
    }

    bool has_common_range(const range<T> &r) const
    {
        return ( in_range(r.begin_) || in_range(r.end_) ||
                 r.in_range(begin_) || r.in_range(end_) );
    }

private:

    range<T>(void)
    {
        if (begin_ > end_)
        {
            throw bad_range("Invalid range:  end must be greater or equal begin.");
        }
    }

    T begin_;
    T end_;
};

#endif /* __RANGE_HPP__ */
