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

#include <instrument_handler.hpp>

#include <easylogging++.h>

#define hft_log(__X__) \
    CLOG(__X__, logger_id_.c_str()) << state2string()

#undef INSTRUMENT_HANDLER_TICK_TRACE

instrument_handler::instrument_handler(const boost::program_options::variables_map &config,
                                           const std::string &instrument)
    : config_(config),
      instrument_(instrument),
      logger_id_(std::string("handler_") + instrument),
      expert_advisor_(instrument),
      trade_time_(config),
      state_(handler_state::READING_MARKET),
      ticks_counter_(0),
      previous_price_(0),
      trade_start_price_(0)
{
    el::Loggers::getLogger(logger_id_.c_str(), true);

    hft_log(INFO) << "Initialize handler for instrument ‘"
                  << hft::get_instrument_description(hft::instrument2type(instrument))
                  << "’";

    if (config_["handlers.enable_cache"].as<bool>())
    {
        expert_advisor_.enable_cache();

        hft_log(INFO) << "Cache is [ENABLED] for handler";
    }
    else
    {
        hft_log(INFO) << "Cache is [DISABLED] for handler.";
    }

    if (config_["handlers.trade_positive_swaps_only"].as<bool>())
    {
        expert_advisor_.trade_on_positive_swaps_only(true);

        hft_log(INFO) << "Trading on direction with negative swaps ARE NOT allowed.";
    }
    else
    {
        hft_log(WARNING) << "Trading on direction with negative swaps ARE ALLOWED.";
    }
}

instrument_handler::~instrument_handler(void)
{
    hft_log(INFO) << "Destroying handler for instrument ["
                  << instrument_ << "]";
}

void instrument_handler::on_tick(const hft::tick_message &message, std::ostringstream &response)
{
    if (previous_price_ - message.ask == 0)
    {
        #ifdef INSTRUMENT_HANDLER_TICK_TRACE
        hft_log(INFO) << "No significant change on the market – skipping tick";
        #endif

        response << "OK";

        return;
    }

    previous_price_ = message.ask;

    ++ticks_counter_;

    #ifdef INSTRUMENT_HANDLER_TICK_TRACE

    //
    // Write to log info about each tick.
    //

    hft_log(INFO) << "Received 'tick' notify #" << ticks_counter_
                  << " (ASK=" << message.ask << ")";

    #else

    //
    // Write to log info about tick „sometimes”.
    //

    if (ticks_counter_ % 50 == 0)
    {
        hft_log(INFO) << "Received 'tick' notify #" << ticks_counter_
                      << " (ASK=" << message.ask << "), equity: "
                      << message.bankroll;
    }

    #endif /* INSTRUMENT_HANDLER_TICK_TRACE */

    expert_advisor_.poke_tick(message.ask);

    if (! expert_advisor_.has_sufficient_data())
    {
        response << "OK";

        return;
    }
    else
    {
        if (state_ == handler_state::READING_MARKET)
        {
            hft_log(INFO) << "Market informations has become sufficient.";
            hft_log(INFO) << "Switching state READING_MARKET → WATCHING.";

            state_ = handler_state::WATCHING;
        }
    }

    switch (state_)
    {
        case handler_state::WATCHING:
             if (! trade_time_.can_play(message.request_time))
             {
                 hft_log(INFO) << "No time frame to trade";
                 response << "OK";
             }
             else switch (expert_advisor_.provide_advice())
             {
                 case NO_DECISION:
                      hft_log(INFO) << "Decision is to stay out of the market";
                      response << "OK";
                      break;
                 case DECISION_SHORT:
                      hft_log(INFO) << "Decision is take a position SHORT";
                      trade_start_price_ = message.ask;
                      expert_advisor_.notify_start_position(position_control::position_type::SHORT, (int)(trade_start_price_));

                      hft_log(INFO) << "Switching state WATCHING → TRADE_SHORT, tick#"
                                    << ticks_counter_ << ", ASK=" << trade_start_price_;
                      state_ = handler_state::TRADE_SHORT;
                      response << short_response(); // "SHORT";
                      break;
                 case DECISION_LONG:
                      hft_log(INFO) << "Decision is take a position LONG";
                      trade_start_price_ = message.ask;
                      expert_advisor_.notify_start_position(position_control::position_type::LONG, (int)(trade_start_price_));

                      hft_log(INFO) << "Switching state WATCHING → TRADE_LONG, tick#"
                                    << ticks_counter_ << ", ASK=" << trade_start_price_;
                      state_ = handler_state::TRADE_LONG;
                      response << long_response(); // "LONG";
                      break;
             }

             break;
        case handler_state::TRADE_LONG:
             continue_long_position(message, response);
             break;
        case handler_state::TRADE_SHORT:
             continue_short_position(message, response);
             break;
        default:
             hft_log(ERROR) << "Invalid state!";
             response << "ERROR;Internal server error";
    }
}

void instrument_handler::on_historical_tick(const hft::historical_tick_message &message, std::ostringstream &response)
{
    if (previous_price_ - message.ask == 0)
    {
        #ifdef INSTRUMENT_HANDLER_TICK_TRACE
        hft_log(INFO) << "No significant change on the market – skipping tick";
        #endif

        response << "OK";

        return;
    }

    previous_price_ = message.ask;

    ++ticks_counter_;

    #ifdef INSTRUMENT_HANDLER_TICK_TRACE

    //
    // Write to log info about each tick.
    //

    hft_log(INFO) << "Received 'historical_tick' notify #" << ticks_counter_
                  << " (ASK=" << message.ask << ")";

    #else

    //
    // Write to log info about tick „sometimes”.
    //

    if (ticks_counter_ % 50 == 0)
    {
        hft_log(INFO) << "Received 'historical_tick' notify #" << ticks_counter_
                      << " (ASK=" << message.ask << ")";
    }

    #endif /* INSTRUMENT_HANDLER_TICK_TRACE */

    expert_advisor_.poke_tick(message.ask);

    response << "OK";

    return;
}

void instrument_handler::on_hci_setup(const hft::hci_setup_message &message, std::ostringstream &response)
{
    if (message.enable)
    {
        hft_log(INFO) << "Enabling HCI controller.";
    }
    else
    {
        hft_log(INFO) << "Disabling HCI controller.";
    }

    expert_advisor_.setup_hci(message.enable);

    response << "OK";
}

void instrument_handler::continue_long_position(const hft::tick_message &message, std::ostringstream &response)
{
    if (! trade_time_.can_play(message.request_time))
    {
        expert_advisor_.notify_close_position(static_cast<int>(message.ask));
        response << "CLOSE";
        hft_log(WARNING) << "Closing position because time frame expired.";
        trade_status(message.ask);
        hft_log(INFO) << "Switching state TRADE_LONG → WATCHING";
        state_ = handler_state::WATCHING;

        return;
    }

    if (expert_advisor_.should_close_position(static_cast<int>(message.ask)))
    {
        expert_advisor_.notify_close_position(static_cast<int>(message.ask));
        trade_status(message.ask);

        if (should_prolong_position())
        {
            trade_start_price_ = message.ask;
            expert_advisor_.notify_start_position(position_control::position_type::LONG, static_cast<int>(trade_start_price_));

            hft_log(INFO) << "Position is prolonged for next trade [LONG], tick#"
                          << ticks_counter_ << ", ASK=" << trade_start_price_;
            response << "OK";
        }
        else
        {
            response << "CLOSE";
            hft_log(INFO) << "Switching state TRADE_LONG → WATCHING";
            state_ = handler_state::WATCHING;
        }
    }
    else
    {
        response << "OK";
    }
}

void instrument_handler::continue_short_position(const hft::tick_message &message, std::ostringstream &response)
{
    if (! trade_time_.can_play(message.request_time))
    {
        expert_advisor_.notify_close_position(static_cast<int>(message.ask));
        response << "CLOSE";
        hft_log(WARNING) << "Closing position because time frame expired.";
        trade_status(message.ask);
        hft_log(INFO) << "Switching state TRADE_LONG → WATCHING";
        state_ = handler_state::WATCHING;

        return;
    }

    if (expert_advisor_.should_close_position(static_cast<int>(message.ask)))
    {
        expert_advisor_.notify_close_position(static_cast<int>(message.ask));
        trade_status(message.ask);

        if (should_prolong_position())
        {
            trade_start_price_ = message.ask;
            expert_advisor_.notify_start_position(position_control::position_type::SHORT, static_cast<int>(trade_start_price_));

            hft_log(INFO) << "Position is prolonged for next trade [SHORT], tick#"
                          << ticks_counter_ << ", ASK=" << trade_start_price_;
            response << "OK";
        }
        else
        {
            response << "CLOSE";
            hft_log(INFO) << "Switching state TRADE_SHORT → WATCHING";
            state_ = handler_state::WATCHING;
        }
    }
    else
    {
        response << "OK";
    }
}

bool instrument_handler::should_prolong_position(void)
{
    //
    // Does prolongation is enabled at all?
    //

    if (! config_["handlers.prolong_position_to_next_setup"].as<bool>())
    {
        return false;
    }

    decision_type decision = expert_advisor_.provide_advice();

    if ((decision == DECISION_LONG && state_ == handler_state::TRADE_LONG) ||
        (decision == DECISION_SHORT && state_ == handler_state::TRADE_SHORT))
    {
        return true;
    }

    return false;
}

void instrument_handler::trade_status(unsigned int current_price)
{
    if (expert_advisor_.get_custom_handler_options().invert_engine_decision)
    {
        if (state_ == handler_state::TRADE_LONG)
        {
            hft_log(INFO) << "Trade status: "
                          << ((current_price > trade_start_price_) ? "LOSS" : "GAINS");
        }
        else if (state_ == handler_state::TRADE_SHORT)
        {
            hft_log(INFO) << "Trade status: "
                          << ((current_price < trade_start_price_) ? "LOSS" : "GAINS");
        }
    }
    else
    {
        if (state_ == handler_state::TRADE_LONG)
        {
            hft_log(INFO) << "Trade status: "
                          << ((current_price > trade_start_price_) ? "GAINS" : "LOSS");
        }
        else if (state_ == handler_state::TRADE_SHORT)
        {
            hft_log(INFO) << "Trade status: "
                          << ((current_price < trade_start_price_) ? "GAINS" : "LOSS");
        }
    }
}

std::string instrument_handler::short_response(void) const
{
    if (expert_advisor_.get_custom_handler_options().invert_engine_decision)
    {
        hft_log(WARNING) << "Inverting decision to [LONG] on handler layer because of custom handler option ‘invert_engine_decision’";

        return std::string("LONG");
    }

    return std::string("SHORT");
}

std::string instrument_handler::long_response(void) const
{
    if (expert_advisor_.get_custom_handler_options().invert_engine_decision)
    {
        hft_log(WARNING) << "Inverting decision to [SHORT] on handler layer because of custom handler option ‘invert_engine_decision’";

        return std::string("SHORT");
    }

    return std::string("LONG");
}

std::string instrument_handler::state2string(void) const
{
    std::string ret;

    switch (state_)
    {
        case handler_state::READING_MARKET:
             ret = "(READING MARKET) ";
             break;
        case handler_state::WATCHING:
             /* No string label for WATCHING state */
             break;
        case handler_state::TRADE_LONG:
             if (expert_advisor_.get_custom_handler_options().invert_engine_decision)
             {
                 ret = "(SHORT) ";
             }
             else
             {
                 ret = "(LONG) ";
             }
             break;
        case handler_state::TRADE_SHORT:
             if (expert_advisor_.get_custom_handler_options().invert_engine_decision)
             {
                 ret = "(LONG) ";
             }
             else
             {
                 ret = "(SHORT) ";
             }
             break;
    }

    return ret;
}
