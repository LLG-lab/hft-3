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

#ifndef __EXPERT_ADVISOR_HPP__
#define __EXPERT_ADVISOR_HPP__

#include <list>
#include <utility>

#include <basic_artifical_inteligence.hpp>
#include <decision_trigger.hpp>
#include <position_control_manager.hpp>
#include <files_change_tracker.hpp>
#include <hci.hpp>

class expert_advisor : private basic_artifical_inteligence
{
public:

    DEFINE_CUSTOM_EXCEPTION_CLASS(exception, std::runtime_error)

    typedef struct _custom_handler_options
    {
        _custom_handler_options(void)
            : invert_engine_decision(false) {}

        bool invert_engine_decision;

    } custom_handler_options;

    expert_advisor(const std::string &instrument);
    expert_advisor(void) = delete;
    ~expert_advisor(void);

    //
    // Essential interface.
    //

    bool has_sufficient_data(void) const;
    void poke_tick(unsigned int tick);
    decision_type provide_advice(void);

    //
    // Position control
    // related routines.
    //

    void notify_start_position(position_control::position_type pt, int open_price);
    void notify_close_position(int close_price);
    bool should_close_position(int current_price);

    //
    // Auxiliary.
    //

    void enable_cache(void);
    void trade_on_positive_swaps_only(bool flag) { trade_on_positive_swaps_only_ = flag; }
    void setup_hci(bool state) { if (decision_compensate_inverter_.use_count()) decision_compensate_inverter_enabled_ = state; }
    const custom_handler_options &get_custom_handler_options(void) const { return custom_handler_options_; }

private:

    void factory_advisor(void);

    std::string instrument_;
    unsigned int dpips_limit_loss_;
    unsigned int dpips_limit_profit_;
    bool mutable_networks_;
    int eav_;
    decision_type ai_decision_;

    double vote_ratio_;

    std::shared_ptr<decision_trigger> decision_;
    std::shared_ptr<position_control_manager> position_controller_;
    std::shared_ptr<hci> decision_compensate_inverter_;
    bool decision_compensate_inverter_enabled_;

    files_change_tracker net_info_;

    bool trade_on_positive_swaps_only_;

    custom_handler_options custom_handler_options_;
};

#endif /* __EXPERT_ADVISOR_HPP__ */
