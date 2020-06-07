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

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

#include <decision_trigger.hpp>

static boost::char_separator<char> sep1(";");
static boost::char_separator<char> sep2("=" );
static boost::char_separator<char> sep3("," );

decision_type decision_trigger::operator ()(double x)
{
    for (auto &rule : rules_)
    {
        if (rule.first.in_range(x))
        {
            return rule.second;
        }
    }

    return NO_DECISION;
}

void decision_trigger::factory_rules(const std::string &rules)
{
    boost::tokenizer<boost::char_separator<char> > tokens(rules, sep1);

    for (auto &token : tokens)
    {
        std::string s = token;
        boost::trim(s);
        parse_single_rule(s);
    }

    /*
     * FIXME: Dorobić sprawdzenie, czy przedziały sie nie pokrywają.
     */
}

void decision_trigger::parse_single_rule(const std::string &rule)
{
    //
    // Rule example:
    // <0.1, 0.5> = LONG
    //

    boost::tokenizer<boost::char_separator<char> > tokens(rule, sep2);
    std::string s;

    auto iterator = tokens.begin();

    if (iterator == tokens.end())
    {
        std::string msg = std::string("decision_trigger: Invalid rule ") + rule;

        throw decision_trigger_exception(msg);
    }

    std::string range_str = *iterator;
    boost::trim(range_str);

    iterator++;
    if (iterator == tokens.end())
    {
        std::string msg = std::string("decision_trigger: Invalid rule ") + rule;

        throw decision_trigger_exception(msg);
    }

    std::string decision_str = *iterator;
    boost::trim(decision_str);

    iterator++;
    if (iterator != tokens.end())
    {
        std::string msg = std::string("decision_trigger: Invalid rule ") + rule;

        throw decision_trigger_exception(msg);
    }

    decision_type dec;

    if (decision_str == "LONG" || decision_str == "long")
    {
        dec = DECISION_LONG;
    }
    else if (decision_str == "SHORT" || decision_str == "short")
    {
        dec = DECISION_SHORT;
    }
    else
    {
        std::string msg = std::string("decision_trigger: Illegal decision ") + decision_str;

        throw decision_trigger_exception(msg);
    }

    if (range_str.length() <= 2 || range_str.at(0) != '<'
            || range_str.at(range_str.length() - 1) != '>')
    {
        std::string msg = std::string("decision_trigger: Invalid range ") + range_str;

        throw decision_trigger_exception(msg);
    }

    range_str.at(0) = ' ';
    range_str.at(range_str.length() - 1) = ' ';

    boost::tokenizer<boost::char_separator<char> > minmax(range_str, sep3);

    iterator = minmax.begin();

    if (iterator == minmax.end())
    {
        std::string msg = std::string("decision_trigger: Invalid range ") + range_str;

        throw decision_trigger_exception(msg);
    }

    s = *iterator;
    boost::trim(s);
    double min = boost::lexical_cast<double>(s);

    iterator++;

    if (iterator == minmax.end())
    {
        std::string msg = std::string("decision_trigger: Invalid range ") + range_str;

        throw decision_trigger_exception(msg);
    }

    s = *iterator;
    boost::trim(s);
    double max = boost::lexical_cast<double>(s);

    rules_.push_back(std::pair<range<double>, decision_type>(range<double>(min, max), dec));
}
