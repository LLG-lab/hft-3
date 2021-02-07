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

#include <sstream>

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

#include <hftr.hpp>

#undef HFTR_TRACE

void hftr::set_pin(unsigned int n, double value)
{
    inputs_[n] = value;
}

double hftr::get_pin(unsigned int n) const
{
    auto it = inputs_.find(n);

    if (it == inputs_.end())
    {
        std::ostringstream err_msg;

        err_msg << "HFTR error: Unable to find input of index „"
                << n << "”";

        throw exception(err_msg.str());
    }

    return it -> second;
}

void hftr::import_from_text_line(const std::string &text_line)
{
    static const boost::char_separator<char> sep1("\t");
    static const boost::char_separator<char> sep2("=" );

    std::string key, value;
    boost::tokenizer<boost::char_separator<char> > tokens(text_line, sep1);

    clear();

    for (auto &token : tokens)
    {
        boost::tokenizer<boost::char_separator<char> > keyval(token, sep2);

        auto it = keyval.begin();

        if (it == keyval.end())
        {
            throw exception("HFTR error: Required key on input data");
        }

        key = (*it);
        it++;

        if (it == keyval.end())
        {
            throw exception("HFTR error: Required value on input data");
        }

        value = (*it);

        if (key == "OUTPUT")
        {
            if (value == "-1")
            {
                output_ = DECREASE;
            }
            else if (value == "1")
            {
                output_ = INCREASE;
            }
            else if (value == "?")
            {
                output_ = UNDEFINED;
            }
            else
            {
                throw exception("Unrecognized output value");
            }
        }
        else
        {
            #ifdef HFTR_TRACE
            hft_log(TRACE) << "Setting pin #" << key << " with " << value;
            #endif

            set_pin(boost::lexical_cast<unsigned int>(key), boost::lexical_cast<double>(value));
        }
    }
}

std::string hftr::export_to_text_line(void) const
{
    std::ostringstream text_line;

    text_line << "OUTPUT=";

    switch (output_)
    {
        case DECREASE:
            text_line << "-1";
            break;
        case UNDEFINED:
            text_line << "?";
            break;
        case INCREASE:
            text_line << "1";
            break;
        default:
            throw exception("Unrecognized output value");
    }

    for (auto &inp : inputs_)
    {
        text_line << "\t" << inp.first << "=" << inp.second;
    }

    return text_line.str();
}
