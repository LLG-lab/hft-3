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

#ifndef __POSITION_CONTROL_MANAGER_HPP__
#define __POSITION_CONTROL_MANAGER_HPP__

#include <memory>
#include <list>

#include <boost/noncopyable.hpp>

#include <position_control_driver.hpp>

class position_control_manager : public position_control
{
    friend class driver_supplier;
public:

    DEFINE_CUSTOM_EXCEPTION_CLASS(position_control_manager_exception, std::runtime_error)

    typedef position_control_driver *(*pcp_driver_generator)(int, int);

    class driver_supplier : private boost::noncopyable
    {
    public:
        driver_supplier(std::string label, pcp_driver_generator gen) { position_control_manager::register_driver(label, gen); }
    private:
        driver_supplier(void) {}
    };

    position_control_manager(int dpips_limit_loss, int dpips_limit_profit)
        : position_control(dpips_limit_loss, dpips_limit_profit) {}

    virtual control get_position_control_status(int cur_level_dpips);
    virtual void setup(position_type pt, int current);
    virtual setup_info get_setup_info(void) const;
    virtual void reset(void);
    virtual void configure_from_json(cJSON *json);

private:

    typedef struct supported_driver_info_
    {
        supported_driver_info_(std::string l, pcp_driver_generator dg)
            : label(l), create_driver(dg) {}

        std::string label;
        pcp_driver_generator create_driver;
    } supported_driver_info;

    typedef std::list<supported_driver_info> supported_driver_container;

    static void register_driver(const std::string &label, pcp_driver_generator gen);
    static pcp_driver_generator lookup_driver(const std::string &label);
    static supported_driver_container supported_drivers_;

    bool is_configured(void) const { return (driver_.get() != nullptr); }

    std::shared_ptr<position_control_driver> driver_;
};

#endif /* __POSITION_CONTROL_MANAGER_HPP__ */
