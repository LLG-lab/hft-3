/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System  ≣≡=-              **
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

#ifndef __TRACER_HPP__
#define __TRACER_HPP__

#include <iostream>
#include <list>

#include <range.hpp>

class value_tracer
{
public:

    DEFINE_CUSTOM_EXCEPTION_CLASS(value_tracer_exception, std::runtime_error)

    value_tracer(void)
        : up_(false), down_(false), new_(true), prev_range_(0, 0) {}

    void add_range(const range<unsigned int> &r)
    {
        for (auto it = ranges_.begin(); it != ranges_.end(); ++it)
        {
            if (it -> has_common_range(r))
            {
                throw value_tracer_exception("Common range detected.");
            }
        }

        ranges_.push_back(r);
        ranges_.sort();
    }

    bool is_up(void) const
    {
        return up_;
    }

    bool is_down(void) const
    {
        return down_;
    }

    double get_trace(void)
    {
        if (is_up())
        {
            return 1.0;
        }
        else if (is_down())
        {
            return -1.0;
        }

        return 0.0;
    }

    void put_value(unsigned int v)
    {
        if (new_)
        {
            prev_range_ = get_range(v);
            new_ = false;
            return;
        }

        range<unsigned int> curr_r = get_range(v);

        if (prev_range_ == curr_r)
        {
            //
            // Nothing has changed.
            //

            return;
        }
        else if (prev_range_ < curr_r)
        {
            up_ = true;
            down_ = false;
            prev_range_ = curr_r;
            return;
        }
        else /* prev_range_ > curr_r*/
        {
            up_ = false;
            down_ = true;
            prev_range_ = curr_r;
            return;
        }
    }

private:

    range<unsigned int> get_range(unsigned int v)
    {
        range<unsigned int> null_range = range<unsigned int>(0, 0);

        for (auto it = ranges_.begin(); it != ranges_.end(); ++it)
        {
            if (it -> in_range(v))
            {
                return (*it);
            }
        }

        return null_range;
    }

    std::list<range<unsigned int> > ranges_;

    range<unsigned int> prev_range_;
    bool up_;
    bool down_;
    bool new_;
};

//
// Example test:
//

// int main(int argc, char *argv[])
// {
//    value_tracer tc;
//    try
//    {
//        tc.add_range(range(0, 1));
//        tc.add_range(range(-100, -0.0001));
//        tc.add_range(range(1.001, 3));
//        tc.add_range(range(3.001, 4));
//        tc.add_range(range(4.001, 100));
//    }
//    catch (const std::runtime_error &e)
//    {
//        std::cout << e.what() << "\n";
//    }
//
//    double values[] = {-3.4, -0.1, 0.34, 0.99, 3.5, 3.2, 1.8, 1.4};
//
//    for (int i = 0; i < sizeof(values) / sizeof(double); i++)
//    {
//        tc.put_value(values[i]);
//        std::cout << "v= " << values[i] << "is_down: [" << tc.is_down() << "], is_up [" << tc.is_up() << "]\n";
//    }
// }

#endif /* __TRACER_HPP__ */
