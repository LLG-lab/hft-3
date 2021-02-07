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

#include <cmath>
#include <position_control_manager.hpp>

struct pcp_driver_antychiron_private
{
    pcp_driver_antychiron_private(void)
        : dpips_minmax_(0),
          summand_(0.0),
          multiplier_(0.0),
          scale_factor_(0.0),
          configured_(false)
    {}

    int pcp_function(int pips)
    {
        if (pips <= (max_loss_dpips_/10))
        {
            return (max_loss_dpips_/10) + pips;
        }

        return ::floor( summand_ + multiplier_ * ::sqrt(scale_factor_ * pips) );
    }

    int dpips_minmax_;
    double summand_;
    double multiplier_;
    double scale_factor_;
    bool configured_;
    int max_loss_dpips_;
};

DECLARE_PCP_DRIVER(pcp_driver_antychiron, pcp_driver_antychiron_private, "antychiron")

position_control::control pcp_driver_antychiron::get_position_control_status(int cur_level_dpips)
{
    if (! configured_)
    {
        throw position_control_driver_exception("PCP driver „antychiron” not configured");
    }

    if (get_position_type() == position_control::position_type::SHORT)
    {
        dpips_minmax_ = (cur_level_dpips > dpips_minmax_) ? cur_level_dpips : dpips_minmax_;

        if ((dpips_minmax_ - cur_level_dpips)/10 >= pcp_function((dpips_minmax_ - get_start_position_dpips())/10))
        {
            return control::CLOSE;
        }
    }
    else if (get_position_type() == position_control::position_type::LONG)
    {
        dpips_minmax_ = (cur_level_dpips < dpips_minmax_) ? cur_level_dpips : dpips_minmax_;

        if ((cur_level_dpips - dpips_minmax_)/10 >= pcp_function((get_start_position_dpips() - dpips_minmax_)/10))
        {
            return control::CLOSE;
        }
    }
    else
    {
        throw position_control_driver_exception("PCP driver „antychiron”: Illegal position type");
    }

    return control::HOLD;
}

void pcp_driver_antychiron::configure_from_json(cJSON *json)
{
    //
    // Obtain summand parameter.
    //

    cJSON *summand_json = cJSON_GetObjectItem(json, "summand");

    if (summand_json == nullptr)
    {
        throw position_control_driver_exception("PCP driver „antychiron”: Unspecified [summand] field");
    }

    summand_ = summand_json -> valuedouble;

    //
    // Obrain multiplier parameter.
    //

    cJSON *multiplier_json = cJSON_GetObjectItem(json, "multiplier");

    if (multiplier_json == nullptr)
    {
        throw position_control_driver_exception("PCP driver „antychiron”: Unspecified [multiplier] field");
    }

    multiplier_ = multiplier_json -> valuedouble;

    //
    // Obtain scale_factor parameter.
    //

    cJSON *scale_factor_json = cJSON_GetObjectItem(json, "scale_factor");

    if (scale_factor_json == nullptr)
    {
        throw position_control_driver_exception("PCP driver „antychiron”: Unspecified [scale_factor] field");
    }

    scale_factor_ = scale_factor_json -> valuedouble;

    max_loss_dpips_ = get_dpips_limit_loss();

    configured_ = true;
}

void pcp_driver_antychiron::reset_driver(void)
{
    dpips_minmax_ = 0;
}

void pcp_driver_antychiron::setup_driver(void)
{
    dpips_minmax_ = get_start_position_dpips();
}
