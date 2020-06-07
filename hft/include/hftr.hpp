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

#ifndef __HFTR_HPP__
#define __HFTR_HPP__

#include <stdexcept>
#include <map>

#include <custom_except.hpp>

class hftr
{
public:

    DEFINE_CUSTOM_EXCEPTION_CLASS(exception, std::runtime_error)

    typedef enum 
    {
        DECREASE = -1,
        UNDEFINED,
        INCREASE,
    } output_type;

    hftr(void) : output_(UNDEFINED) {}

    hftr(const std::string &text_line)
    {
        import_from_text_line(text_line);
    }

    void clear(void)
    {
        inputs_.clear();
        output_ = UNDEFINED;
    }

    size_t get_size(void) const
    {
        return inputs_.size();
    }

    //
    // I/O.
    //

    output_type get_output(void) const
    {
        return output_;
    }

    void set_output(output_type ot)
    {
        output_ = ot;
    }

    void set_pin(unsigned int n, double value);
    double get_pin(unsigned int n) const;

    void import_from_text_line(const std::string &text_line);
    std::string export_to_text_line(void) const;

private:

    output_type output_;
    std::map<unsigned int, double> inputs_;
};

#endif /* __HFTR_HPP__ */
