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

#ifndef __INSTRUMENT_HANDLER_HPP__
#define __INSTRUMENT_HANDLER_HPP__

#include <sstream>

#include <boost/program_options.hpp>

#include <hft_req_proto.hpp>
#include <expert_advisor.hpp>
#include <trade_time_frame.hpp>

class instrument_handler
{
public:

    instrument_handler(const boost::program_options::variables_map &config,
                           const std::string &instrument);

    ~instrument_handler(void);

    void on_tick(const hft::tick_message &message, std::ostringstream &response);
    void on_historical_tick(const hft::historical_tick_message &message, std::ostringstream &response);
    void on_hci_setup(const hft::hci_setup_message &message, std::ostringstream &response);

private:

    enum class handler_state
    {
        READING_MARKET,
        WATCHING,
        TRADE_LONG,
        TRADE_SHORT
    };

    void continue_long_position(const hft::tick_message &message, std::ostringstream &response);
    void continue_short_position(const hft::tick_message &message, std::ostringstream &response);

    bool should_prolong_position(void);

    void trade_status(unsigned int current_price);

    std::string short_response(void) const;
    std::string long_response(void) const;

    std::string state2string(void) const;

    const boost::program_options::variables_map &config_;
    const std::string instrument_;
    const std::string logger_id_;

    expert_advisor expert_advisor_;
    trade_time_frame trade_time_;
    handler_state state_;

    unsigned int ticks_counter_;
    unsigned int previous_price_;

    unsigned int trade_start_price_;
};

#endif /* __INSTRUMENT_HANDLER_HPP__ */
