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

#ifndef __HFT_EXPERT_ENGINE_INTERFACE_HPP__
#define __HFT_EXPERT_ENGINE_INTERFACE_HPP__

class hft_expert_engine_interface
{
public:

    virtual void enable_cache(void) = 0;
    virtual void trade_on_positive_swaps_only(bool flag) = 0;
    virtual void poke_tick(unsigned int tick) = 0;
    virtual bool has_sufficient_data(void) const = 0;
    virtual decision_type provide_advice(void) = 0;  // XXX Skąd typ?
    virtual void notify_start_position(position_control::position_type pt, int open_price) = 0; // XXX skąd typ?
    virtual void notify_close_position(int close_price) = 0;
    virtual bool should_close_position(int current_price) = 0;
};

#endif /* __HFT_EXPERT_ENGINE_INTERFACE_HPP__ */
