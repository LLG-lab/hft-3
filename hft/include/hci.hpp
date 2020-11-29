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

#ifndef __HCI_HPP__
#define __HCI_HPP__

#include <boost/noncopyable.hpp>
#include <decision_trigger.hpp>

class hci : private boost::noncopyable
{
public:

    DEFINE_CUSTOM_EXCEPTION_CLASS(exception, std::runtime_error)

    hci(size_t capacity, double st, double it);
    hci(void) = delete;

    ~hci(void);

    void insert_pips_yield(int pips_yield);
    decision_type decision(decision_type d) const;

    void enable_cache(const std::string &file_name);
    std::string get_state_str(void) const;

    void debug(bool state) { debug_ = state; }

private:

    void load_object_state(void);
    void save_object_state(void);

    enum class state
    {
        STRAIGHT,
        INVERT
    };

    state  state_;
    const double straight_threshold_;
    const double invert_threshold_;
    size_t index_;
    const size_t capacity_;

    int *const buffer_;
    int *const integral_buffer_;

    std::string file_name_;

    bool debug_;
};

#endif /* __HCI_HPP__ */
