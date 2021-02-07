/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System  ≣≡=-              **
**                              b                                     **
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

#ifndef __BINOMIAL_APPROXIMATION_HPP__
#define __BINOMIAL_APPROXIMATION_HPP__

#include <custom_except.hpp>
#include <stdexcept>
#include <map>

class binomial_approximation
{
public:

    DEFINE_CUSTOM_EXCEPTION_CLASS(exception, std::runtime_error)

    binomial_approximation(void) : amount_(0.0) {}
    binomial_approximation(const std::string &approximation_file_name)
        : amount_(0.0)
    {
        load_data_from_file(approximation_file_name);
    }

    double get_amount(void) const
    {
        return amount_;
    }

    double get_draw(int oscillation) const;

    void load_data_from_file(const std::string &approximation_file_name);
    void load_data_from_string(const std::string &data);

private:

    double amount_;
    std::map<unsigned int, double> distribution_;
};

#endif /* __BINOMIAL_APPROXIMATION_HPP__ */
