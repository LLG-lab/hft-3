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

#ifndef __GRANULARITY_COUNTER_HPP__
#define __GRANULARITY_COUNTER_HPP__

class granularity_counter
{
public:

    granularity_counter(const unsigned int &granularity)
        : granularity_(granularity), skipped_ticks_(0),
          prev_ask_(0) {}

    bool is_important_tick(unsigned int ask)
    {
        if (skipped_ticks_++ % granularity_ != 0)
        {
            return false;
        }

        if (prev_ask_ == ask)
        {
            skipped_ticks_--;
            return false;
        }

        prev_ask_ = ask;
        return true;
    }

private:

    const unsigned int &granularity_;
    unsigned int skipped_ticks_;
    unsigned int prev_ask_;
};

#endif /* __GRANULARITY_COUNTER_HPP__ */

