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

#include <fx_account.hpp>
#include <hft_utils.hpp>

#include <iostream>
#include <unistd.h>

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

const double fx_account::leverage_ = 100;

void fx_account::proceed_operation(const std::string &operation,
                                       const csv_loader::csv_record &market_info)
{
    if (operation == "OK")
    {
        //
        // Nothing to do.
        //

        return;
    }
    else if (operation == "LONG")
    {
        if (invert_hft_decision_)
        {
            open_short(market_info);
        }
        else
        {
            open_long(market_info);
        }
    }
    else if (operation == "SHORT")
    {
        if (invert_hft_decision_)
        {
            open_long(market_info);
        }
        else
        {
            open_short(market_info);
        }
    }
    else if (operation == "CLOSE")
    {
        if (position_ == LONG)
        {
            close_long(market_info);
        }
        else if (position_ == SHORT)
        {
            close_short(market_info);
        }
        else
        {
            throw exception("Unable to close unknown position type");
        }

        auto last_position = position_history_.rbegin();
        last_position -> display(itype_);
    }
    else
    {
        std::string error_msg = std::string("**** PROTOCOL ERROR: Unrecognized operation from server „")
                                + operation + std::string("”");

        throw exception(error_msg);
    }
}

void fx_account::forcibly_close_position(const csv_loader::csv_record &market_info)
{
    switch (position_)
    {
        case NONE:
            return;
            break;
        case LONG:
            close_long(market_info, true);
            break;
        case SHORT:
            close_short(market_info, true);
            break;
        default:
            throw std::logic_error("Bad position type");
    }

    auto last_position = position_history_.rbegin();
    last_position -> display(itype_);
}

std::string fx_account::get_position_status(const csv_loader::csv_record &market_info) const
{
    std::string open_price_str  = boost::lexical_cast<std::string>(open_price_);
    std::string current_price_str;

    std::string ret;
    double value;

    switch (position_)
    {
        case LONG:
             current_price_str = boost::lexical_cast<std::string>(market_info.bid);
             value = (double(hft::floating2dpips(current_price_str, itype_)) - double(hft::floating2dpips(open_price_str, itype_)))/10.0;
             ret = boost::lexical_cast<std::string>(value);
             break;
        case SHORT:
             current_price_str = boost::lexical_cast<std::string>(market_info.ask);
             value = (double(hft::floating2dpips(open_price_str, itype_)) - double(hft::floating2dpips(current_price_str, itype_)))/10.0;
             ret = boost::lexical_cast<std::string>(value);
             break;
        default:
             ret = "N/A";
    }

    return ret;
}

void fx_account::open_long(const csv_loader::csv_record &market_info)
{
    if (position_ != NONE)
    {
        throw exception("**** PROTOCOL ERROR: Unable to open position. Some position is already opened");
    }

    open_at_ = market_info.request_time;
    open_price_ = market_info.ask;
    position_ = LONG;
}

void fx_account::open_short(const csv_loader::csv_record &market_info)
{
    if (position_ != NONE)
    {
        throw exception("**** PROTOCOL ERROR: Unable to open position. Some position is already opened");
    }

    open_at_ = market_info.request_time;
    open_price_ = market_info.bid;
    position_ = SHORT;
}

void fx_account::close_short(const csv_loader::csv_record &market_info, bool forcibly)
{
    if (position_ != SHORT)
    {
        throw exception("Opened position is not a SHORT position.");
    }

    position_history_.push_back(position_result(SHORT, open_at_, open_price_, market_info.request_time, market_info.ask, forcibly));
    position_ = NONE;
    open_at_ = "";
}

void fx_account::close_long(const csv_loader::csv_record &market_info, bool forcibly)
{
    if (position_ != LONG)
    {
        throw exception("Opened position is not a LONG position.");
    }

    position_history_.push_back(position_result(LONG, open_at_, open_price_, market_info.request_time, market_info.bid, forcibly));
    position_ = NONE;
    open_at_ = "";
}

//
// Calculates profit/loss of position.
//

double fx_account::position_result::get_pips_pl(hft::instrument_type itype) const
{
    std::string open_price_str  = boost::lexical_cast<std::string>(open_price);
    std::string close_price_str = boost::lexical_cast<std::string>(close_price);

    double value = 0.0;

    switch (type)
    {
        case LONG:
            value = (double(hft::floating2dpips(close_price_str, itype)) - double(hft::floating2dpips(open_price_str, itype)))/10.0;
            break;
        case SHORT:
            value = (double(hft::floating2dpips(open_price_str, itype)) - double(hft::floating2dpips(close_price_str, itype)))/10.0;
            break;
        default:
            throw std::logic_error("Bad position type");
    }

    return value;
}

void fx_account::position_result::display(hft::instrument_type itype) const
{
    switch (type)
    {
        case LONG:
             std::cout << "LONG ";
             break;
        case SHORT:
             std::cout << "SHORT ";
             break;
        default:
            throw std::logic_error("Bad position type");
    }

    bool tty_output = isatty(fileno(stdout));

    double profit_loss = get_pips_pl(itype);

    if (profit_loss >= 0.0 && tty_output)
    {
        std::cout << "\033[0;32m";
    }
    else if (profit_loss < 0.0 && tty_output)
    {
        std::cout << "\033[0;31m";
    }

    std::cout << profit_loss;

    if (tty_output)
    {
        std::cout << "\033[0m";
    }

    std::cout << " [open: " << open_price << " (@ "
              << open_at << "), close: "
              << close_price << " (@ "
              << close_at
              << ")]";

    if (forcibly_closed)
    {
        std::cout << " (position closed forcibly)";
    }

    std::cout << std::endl;
}

void fx_account::dispaly_statistics(void) const
{
    //
    // TODO: Tutej bedzie można w przyszłości zaimplementować
    //       obliczanie oraz wyswietalnie statystyk
    //       liczonych na bazie „position_history_”.
    //

    return;
}
