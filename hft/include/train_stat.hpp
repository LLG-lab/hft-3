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

#ifndef __TRAIN_STAT_HPP__
#define __TRAIN_STAT_HPP__

#include <list>
#include <utility>
#include <string>

class train_stat
{
public:

    enum report_type
    {
        HUMAN_READABLE,
        JSON
    };

    void clear(void) { data_.clear(); };
    void store(double network_response, double expected);

    std::string generate_report(report_type rt) const;

    double get_pessimistic_ratio_indicator(void) const;

private:

    typedef std::pair<unsigned int, unsigned int> position_type_stats;

    unsigned int get_total_successes(void) const;

    position_type_stats get_long_information(void) const;

    position_type_stats get_short_information(void) const;

    double get_average_abs_error(void) const;

    double get_average_sqr_error(void) const;

    static double ratio(unsigned int k, unsigned int n);
    static double pessimistic_ratio_indicator(unsigned int k, unsigned int n);

    //
    // First is network output value,
    // second is an result.
    //

    std::list<std::pair<double, double> > data_;
};

#endif /* __TRAIN_STAT_HPP__ */

