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

#ifndef __POSITION_CONTROL_DRIVER_HPP__
#define __POSITION_CONTROL_DRIVER_HPP__

#include <custom_except.hpp>
#include <position_control.hpp>

class position_control_driver : public position_control
{
public:

    DEFINE_CUSTOM_EXCEPTION_CLASS(position_control_driver_exception, std::runtime_error)

    position_control_driver(int dpips_limit_loss, int dpips_limit_profit)
        : position_control(dpips_limit_loss, dpips_limit_profit),
          type_(position_type::UNDEFINED), start_position_dpips_(0) {}

    virtual void setup(position_type pt, int current)
    {
        if (position_underway())
        {
            throw position_control_driver_exception("(position_control_driver) Illegal attempt to re-setup position");
        }

        if (pt == position_type::UNDEFINED)
        {
            throw position_control_driver_exception("(position_control_driver) Bad position type");
        }

        if (current <= 0)
        {
            throw position_control_driver_exception("(position_control_driver) Bad stock price");
        }

        type_ = pt;
        start_position_dpips_ = current;

        //
        // Setup internal state (if any) also in derived class.
        //

        setup_driver();
    }

    virtual setup_info get_setup_info(void) const
    {
        setup_info ret = { .position = type_, .price = start_position_dpips_ };
        return ret;
    }

    virtual void reset(void)
    {
        type_ = position_type::UNDEFINED;
        start_position_dpips_ = 0;

        //
        // Reset internal state (if any) also in derived class.
        //

        reset_driver();
    }

protected:

    virtual void reset_driver(void) = 0;
    virtual void setup_driver(void) = 0;

    position_type get_position_type(void) const { return type_; }
    int get_start_position_dpips(void) const  { return start_position_dpips_; }
    bool position_underway(void) const { return (type_ != position_type::UNDEFINED && start_position_dpips_ > 0); }

private:

    position_type type_;
    int start_position_dpips_;
};

//
// Following macro can be used in PCP driver implementations.
//

#define DECLARE_PCP_DRIVER(__DRIVER_CLASS_NAME__, __DRIVER_PRIVATE_CLASS_NAME__, __DRIVER_STRING_LABEL__) \
    class __DRIVER_CLASS_NAME__ : public position_control_driver, private __DRIVER_PRIVATE_CLASS_NAME__ \
    { \
    public: \
        __DRIVER_CLASS_NAME__(int dpips_limit_loss, int dpips_limit_profit) \
            : position_control_driver(dpips_limit_loss, dpips_limit_profit) {} \
        virtual control get_position_control_status(int cur_level_dpips); \
        virtual void configure_from_json(cJSON *json); \
    protected: \
        virtual void reset_driver(void); \
        virtual void setup_driver(void); \
    private: \
        static position_control_driver *generator(int dpips_limit_loss, int dpips_limit_profit) \
        { \
            return new __DRIVER_CLASS_NAME__(dpips_limit_loss, dpips_limit_profit); \
        } \
        static position_control_manager::driver_supplier register_pcp_driver_; \
    }; \
    position_control_manager::driver_supplier __DRIVER_CLASS_NAME__::register_pcp_driver_(std::string(__DRIVER_STRING_LABEL__), __DRIVER_CLASS_NAME__::generator);

#endif /* __POSITION_CONTROL_DRIVER_HPP__ */
