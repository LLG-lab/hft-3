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
#include <sstream>
#include <train_stat.hpp>

void train_stat::store(double network_response, double expected)
{
    data_.push_back(std::make_pair(network_response, expected));
}

std::string train_stat::generate_report(train_stat::report_type rt) const
{
    //
    // +---------------------------------------------
    // | Total statistics:
    // +---------------------------------------------
    // |   n ............................ 1000
    // |   success....................... 500
    // |   ratio ........................ 0.5000
    // |   pessimistic ratio indicator .. 0.4900
    // +---------------------------------------------
    // | LONG statistics:
    // +---------------------------------------------
    // |   n ............................ 300
    // |   success ...................... 150
    // |   ratio ........................ 0.5000
    // |   pessimistic ratio indicator .. 0.4900
    // +---------------------------------------------
    // | SHORT statistics:
    // +---------------------------------------------
    // |   n ............................ 700
    // |   success ...................... 350
    // |   ratio ........................ 0.5000
    // |   pessimistic ratio indicator .. 0.4970
    // +----------------------------------------------
    // | Additional:
    // +----------------------------------------------
    // |   (1/n)·Σ|y-e| ................. 0.2122
    // |   (1/n)·Σ(y-e)² ................ 0.543
    // +----------------------------------------------
    //

    std::ostringstream oss;

    unsigned int total_success = get_total_successes();
    position_type_stats long_info  = get_long_information();
    position_type_stats short_info = get_short_information();

    switch (rt)
    {
        case HUMAN_READABLE:

        oss << "+---------------------------------------------\n"
            << "| Total statistics:\n"
            << "+---------------------------------------------\n"
            << "|   n ............................ " << data_.size() << "\n"
            << "|   success ...................... " << total_success << "\n"
            << "|   ratio ........................ " << train_stat::ratio(total_success, data_.size()) << "\n"
            << "|   pessimistic ratio indicator .. " << train_stat::pessimistic_ratio_indicator(total_success, data_.size()) << "\n"
            << "+---------------------------------------------\n"
            << "| LONG statistics:\n"
            << "+---------------------------------------------\n"
            << "|   n ............................ " << long_info.second << "\n"
            << "|   success ...................... " << long_info.first << "\n"
            << "|   ratio ........................ " << train_stat::ratio(long_info.first, long_info.second) << "\n"
            << "|   pessimistic ratio indicator .. " << train_stat::pessimistic_ratio_indicator(long_info.first, long_info.second) << "\n"
            << "+---------------------------------------------\n"
            << "| SHORT statistics:\n"
            << "+---------------------------------------------\n"
            << "|   n ............................ " << short_info.second << "\n"
            << "|   success ...................... " << short_info.first << "\n"
            << "|   ratio ........................ " << train_stat::ratio(short_info.first, short_info.second) << "\n"
            << "|   pessimistic ratio indicator .. " << train_stat::pessimistic_ratio_indicator(short_info.first, short_info.second) << "\n"
            << "+---------------------------------------------\n"
            << "| Additional:\n"
            << "+---------------------------------------------\n"
            << "|   (1/n)·Σ|y-e| ................. " << get_average_abs_error() << "\n"
            << "|   (1/n)·Σ(y-e)² ................ " << get_average_sqr_error() << "\n"
            << "+---------------------------------------------";
        break;

        case JSON:

        oss << "JSON\t{"
            << "\"n\":" << data_.size()
            << ",\"success\":" << total_success
            << ",\"ratio\":" << train_stat::ratio(total_success, data_.size())
            << ",\"pri\":" << train_stat::pessimistic_ratio_indicator(total_success, data_.size())
            << ",\"short_n\":" << short_info.second
            << ",\"short_success\":" << short_info.first
            << ",\"short_ratio\":" << train_stat::ratio(short_info.first, short_info.second)
            << ",\"short_pri\":" << train_stat::pessimistic_ratio_indicator(short_info.first, short_info.second)
            << ",\"long_n\":" << long_info.second
            << ",\"long_success\":" << long_info.first
            << ",\"long_ratio\":" << train_stat::ratio(long_info.first, long_info.second)
            << ",\"long_pri\":" << train_stat::pessimistic_ratio_indicator(long_info.first, long_info.second)
            << ",\"average_square_error\":" << get_average_sqr_error()
            << ",\"average_absolute_error\":" << get_average_abs_error()
            << "}\n";
        break;
    }

    return oss.str();
}

double train_stat::get_pessimistic_ratio_indicator(void) const
{
    return train_stat::pessimistic_ratio_indicator(get_total_successes(), data_.size());
}

unsigned int train_stat::get_total_successes(void) const
{
    unsigned int success = 0;

    for (auto &element : data_)
    {
        if ((element.first > 0.0 && element.second > 0.0) ||
            (element.first < 0.0 && element.second < 0.0))
        {
            success++;
        }
    }

    return success;
}

train_stat::position_type_stats train_stat::get_long_information(void) const
{
    position_type_stats ret;
    ret.first = 0;
    ret.second = 0;

    for (auto &element : data_)
    {
        if (element.first > 0.0)
        {
            ret.second++;

            if (element.second > 0.0)
            {
                ret.first++;
            }
        }
    }

    return ret;
}

train_stat::position_type_stats train_stat::get_short_information(void) const
{
    position_type_stats ret;
    ret.first = 0;
    ret.second = 0;

    for (auto &element : data_)
    {
        if (element.first < 0.0)
        {
            ret.second++;

            if (element.second < 0.0)
            {
                ret.first++;
            }
        }
    }

    return ret;
}

double train_stat::get_average_abs_error(void) const
{
    double v = 0.0;
    double n = data_.size() > 0 ? data_.size() : 1;

    for (auto &element : data_)
    {
        v += fabs(element.first - element.second);
    }

    return (v / n);
}

double train_stat::get_average_sqr_error(void) const
{
    double v = 0.0;
    double n = data_.size() > 0 ? data_.size() : 1;

    for (auto &element : data_)
    {
        v += (element.first - element.second)*(element.first - element.second);
    }

    return (v / n);
}

double train_stat::ratio(unsigned int k, unsigned int n)
{
    if (n == 0)
    {
        return 0.0;
    }

    return (static_cast<double>(k) / static_cast<double>(n));
}

double train_stat::pessimistic_ratio_indicator(unsigned int k, unsigned int n)
{
    if (n < 100)
    {
        return 0.0;
    }

    double estimator = train_stat::ratio(k, n);

    return estimator - 2.576*sqrt((estimator*(1.0 - estimator)) / static_cast<double>(n));
}
