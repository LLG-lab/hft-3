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

#ifndef __POSITION_CONTROL_HPP__
#define __POSITION_CONTROL_HPP__

#include <stdexcept>
#include <cJSON/cJSON.h>

class position_control
{
public:

    enum class position_type
    {
        UNDEFINED,
        SHORT,
        LONG
    };

    enum class control
    {
        HOLD,
        CLOSE
    };

    struct setup_info
    {
        position_type position;
        int price;
    };

    virtual ~position_control(void) {}

    position_control(int dpips_limit_loss, int dpips_limit_profit)
        : dpips_limit_loss_(dpips_limit_loss),
          dpips_limit_profit_(dpips_limit_profit) {}

    virtual control get_position_control_status(int cur_level_dpips) = 0;
    virtual void setup(position_type pt, int current) = 0;
    virtual setup_info get_setup_info(void) const = 0;
    virtual void reset(void) = 0;
    virtual void configure_from_json(cJSON *json) = 0;

protected:

    int get_dpips_limit_loss(void)   { return dpips_limit_loss_;   }
    int get_dpips_limit_profit(void) { return dpips_limit_profit_; }

private:

    position_control(void)
        : dpips_limit_loss_(-1),
          dpips_limit_profit_(-1)
    {
        throw std::logic_error("position_control: Illegal default constructor call");
    }

    const int dpips_limit_loss_;
    const int dpips_limit_profit_;
};

#endif /* __POSITION_CONTROL_HPP__ */
