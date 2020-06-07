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

#ifndef __DECISION_TRIGGER_HPP__
#define __DECISION_TRIGGER_HPP__

#include <list>
#include <utility>

#include <range.hpp>

typedef enum _decision_type_
{
    DECISION_SHORT = -1,
    NO_DECISION = 0,
    DECISION_LONG = 1
} decision_type;

class decision_trigger
{
public:

    DEFINE_CUSTOM_EXCEPTION_CLASS(decision_trigger_exception, std::runtime_error)

    decision_trigger(const std::string &rules)
    {
        factory_rules(rules);
    }

    decision_type operator ()(double x);

private:

    void factory_rules(const std::string &rules);
    void parse_single_rule(const std::string &rule);

    std::list<std::pair<range<double>, decision_type> > rules_;

    decision_trigger(void) {}
};

#endif /* __DECISION_TRIGGER_HPP__ */
