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

#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <easylogging++.h>
#include <trade_time_frame.hpp>

#define hft_log(__X__) \
    CLOG(__X__, "trade_time_frame")

//
// On production must be undefined.
//

#undef TTF_DEBUG

const char *const trade_time_frame::days_[] = { "Monday", "Tuesday", "Wednesday", "Thurstday", "Friday" };

trade_time_frame::trade_time_frame(const boost::program_options::variables_map &config)
    : enabled_(config["trade_time_frame.enabled"].as<bool>())
{
    el::Loggers::getLogger("trade_time_frame", true);

    if (enabled_)
    {
        tfr_[MON] = parse_restriction(config["trade_time_frame.Mon"].as<std::string>());
        tfr_[TUE] = parse_restriction(config["trade_time_frame.Tue"].as<std::string>());
        tfr_[WED] = parse_restriction(config["trade_time_frame.Wed"].as<std::string>());
        tfr_[THU] = parse_restriction(config["trade_time_frame.Thu"].as<std::string>());
        tfr_[FRI] = parse_restriction(config["trade_time_frame.Fri"].as<std::string>());

        for (int i = 0; i < 5; i++)
        {
            hft_log(INFO) << "Timeframe restriction for [" << days_[i] << "] is ["
                          << boost::posix_time::to_simple_string(tfr_[i].trade_period_from)
                          << " - "
                          << boost::posix_time::to_simple_string(tfr_[i].trade_period_to)
                          << "].";
        }
    }
    else
    {
        hft_log(INFO) << "Trade timeframe restrictions are disabled.";
    }
}

trade_time_frame::~trade_time_frame(void)
{
    // Nothing to do.
}

bool trade_time_frame::can_play(const boost::posix_time::ptime &current_time_point)
{
    if (! enabled_)
    {
        return true;
    }

    boost::gregorian::date today = current_time_point.date();
    int dow = today.day_of_week();

    if (dow == boost::date_time::Saturday || dow == boost::date_time::Sunday)
    {
        return false;
    }

    boost::posix_time::time_duration td = current_time_point.time_of_day();

    return can_play_today(tfr_[dow-1], td);
}

bool trade_time_frame::can_play(const boost::posix_time::time_duration &td)
{
    if (! enabled_)
    {
        return true;
    }

    boost::gregorian::date today = boost::gregorian::day_clock::local_day();
    int dow = today.day_of_week();

    if (dow == boost::date_time::Saturday || dow == boost::date_time::Sunday)
    {
        return false;
    }

    return can_play_today(tfr_[dow-1], td);
}

trade_time_frame::timeframe_restriction trade_time_frame::parse_restriction(std::string restriction)
{
    std::vector<std::string> time_points;
    timeframe_restriction ret;

    #ifdef TTF_DEBUG
    hft_log(DEBUG) << "Start parsing [" << restriction << "].";
    #endif

    boost::trim_if(restriction, boost::is_any_of("\""));
    boost::split(time_points, restriction, boost::is_any_of("-"));

    if (time_points.size() != 2)
    {
        hft_log(WARNING) << "Ignoring errorneous time frame restriction option: ["
                         << restriction << "]. Fallback to defaults.";

        ret.trade_period_from = boost::posix_time::duration_from_string("00:00:00.000");
        ret.trade_period_to   = boost::posix_time::duration_from_string("23:59:59.999");
    }
    else
    {
        try
        {
            ret.trade_period_from = boost::posix_time::duration_from_string(time_points.at(0));
        }
        catch (const std::exception &e)
        {
            hft_log(WARNING) << "Inavlid trade period (from) parameter: ["
                             << time_points.at(0) << "]. Using default [00:00:00.000]";

            ret.trade_period_from = boost::posix_time::duration_from_string("00:00:00.000");
        }

        try
        {
            ret.trade_period_to = boost::posix_time::duration_from_string(time_points.at(1));
        }
        catch (const std::exception &e)
        {
            hft_log(WARNING) << "Invalid trade period (to) parameter: ["
                             << time_points.at(1) << "]. Using default [23:59:59.999]";

            ret.trade_period_to = boost::posix_time::duration_from_string("23:59:59.999");
        }

        if (ret.trade_period_from > ret.trade_period_to)
        {
            hft_log(WARNING) << "Inconsistent data: trade period (to) ["
                             << boost::posix_time::to_simple_string(ret.trade_period_to)
                             << "] is before trade period (from) ["
                             << boost::posix_time::to_simple_string(ret.trade_period_from)
                             << "]. Ignore customized settings and fallback to defaults";

            ret.trade_period_from = boost::posix_time::duration_from_string("00:00:00.000");
            ret.trade_period_to   = boost::posix_time::duration_from_string("23:59:59.999");
        }
    }

    return ret;
}

bool trade_time_frame::can_play_today(const trade_time_frame::timeframe_restriction &restriction,
                                          const boost::posix_time::time_duration &td)
{
    return (restriction.trade_period_from <= td && restriction.trade_period_to >= td);
}
