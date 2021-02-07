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

#include <position_control_manager.hpp>

struct pcp_driver_basic_private
{
    int dummy;
};

DECLARE_PCP_DRIVER(pcp_driver_basic, pcp_driver_basic_private, "basic")

position_control::control pcp_driver_basic::get_position_control_status(int cur_level_dpips)
{
    int pips_diff;

    if (get_position_type() == position_control::position_type::LONG)
    {
        pips_diff = cur_level_dpips - get_start_position_dpips();
    }
    else if (get_position_type() == position_control::position_type::SHORT)
    {
        pips_diff = get_start_position_dpips() - cur_level_dpips;
    }
    else
    {
        throw position_control_driver_exception("PCP driver „basic”: Illegal position type");
    }

    if (pips_diff >= get_dpips_limit_profit())
    {
        //
        // Profitable end of trade.
        //

        return control::CLOSE;
    }
    else if (pips_diff <= (-1) * get_dpips_limit_loss())
    {
        //
        // Loosy end of trade.
        //

        return control::CLOSE;
    }

    return control::HOLD;
}

void pcp_driver_basic::configure_from_json(cJSON *json)
{
    //
    // Nothing to do, since basic PCP driver has no options.
    //

    return;
}

void pcp_driver_basic::reset_driver(void)
{
    //
    // Nothing to do, everything to reset is already done
    // in position_control_driver class.
    //

    return;
}

void pcp_driver_basic::setup_driver(void)
{
    //
    // Nothing to do, everything to setup is already done
    // in position_control_driver class.
    //

    return;
}
