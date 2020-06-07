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

#include <position_control_manager.hpp>

#undef PCM_TEST

#ifdef PCM_TEST
#include <iostream>
#endif

position_control_manager::supported_driver_container position_control_manager::supported_drivers_;

void position_control_manager::register_driver(const std::string &label, position_control_manager::pcp_driver_generator gen)
{
    for (auto it : supported_drivers_)
    {
        if (it.label == label)
        {
            std::string msg = std::string("(position_control_manager) PCP driver [")
                              + label + std::string("] already registered");

            throw position_control_manager_exception(msg);
        }
    }

    supported_drivers_.push_back(supported_driver_info(label, gen));

    #ifdef PCM_TEST
    std::cout << "position_control_manager: Registered [" << label << "] PCP driver.\n";
    #endif
}

position_control_manager::pcp_driver_generator position_control_manager::lookup_driver(const std::string &label)
{
    for (auto it : supported_drivers_)
    {
        if (it.label == label)
        {
            return it.create_driver;
        }
    }

    return nullptr;
}

position_control::control position_control_manager::get_position_control_status(int cur_level_dpips)
{
    if (! is_configured())
    {
        throw position_control_manager_exception("(position_control_manager) No PCP driver selected");
    }

    return driver_ -> get_position_control_status(cur_level_dpips);
}

void position_control_manager::setup(position_control::position_type pt, int current)
{
    if (! is_configured())
    {
        throw position_control_manager_exception("(position_control_manager) No PCP driver selected");
    }

    driver_ -> setup(pt, current);
}

position_control_manager::setup_info position_control_manager::get_setup_info(void) const
{
    if (! is_configured())
    {
        throw position_control_manager_exception("(position_control_manager) No PCP driver selected");
    }

    return driver_ -> get_setup_info();
}

void position_control_manager::reset(void)
{
    if (! is_configured())
    {
        throw position_control_manager_exception("(position_control_manager) No PCP driver selected");
    }

    driver_ -> reset();
}

void position_control_manager::configure_from_json(cJSON *json)
{
    cJSON *model_json = cJSON_GetObjectItem(json, "model");

    if (model_json == nullptr)
    {
        throw position_control_manager_exception("(position_control_manager) Unspecified PCP driver model");
    }

    std::string pcp_driver_model = std::string(model_json -> valuestring);

    auto create_driver = lookup_driver(pcp_driver_model);

    if (create_driver == nullptr)
    {
        std::string msg = std::string("(position_control_manager) Unsupported PCP driver [")
                          + pcp_driver_model + std::string("]");

        throw position_control_manager_exception(msg);
    }

    driver_.reset(create_driver(get_dpips_limit_loss(), get_dpips_limit_profit()));
    driver_ -> configure_from_json(json);
}
