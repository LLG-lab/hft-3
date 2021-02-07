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

#ifndef __FX_ACCOUNT_HPP__
#define __FX_ACCOUNT_HPP__

#include <list>
#include <stdexcept>

#include <custom_except.hpp>
#include <csv_loader.hpp>
#include <hft_utils.hpp>

class fx_account
{
public:

    DEFINE_CUSTOM_EXCEPTION_CLASS(exception, std::runtime_error)

    fx_account(void) = delete;

    fx_account(hft::instrument_type itype)
        : itype_(itype), position_(NONE), open_price_(0.0),
          open_at_(""), invert_hft_decision_(false) {};

    void proceed_operation(const std::string &operation,
                               const csv_loader::csv_record &market_info);

    void forcibly_close_position(const csv_loader::csv_record &market_info);

    std::string get_position_status(const csv_loader::csv_record &market_info) const;

    void dispaly_statistics(void) const;

    void invert_hft_decision(void) { invert_hft_decision_ = true; }

private:

    void open_long(const csv_loader::csv_record &market_info);
    void open_short(const csv_loader::csv_record &market_info);
    void close_short(const csv_loader::csv_record &market_info, bool forcibly = false);
    void close_long(const csv_loader::csv_record &market_info, bool forcibly = false);

    enum position_type
    {
        NONE,
        LONG,
        SHORT
    };

    struct position_result
    {
        position_result(position_type pt, const std::string &oa, double op,
                            const std::string &ca, double cp, bool forcibly = false)
            : type(pt), open_at(oa), open_price(op),
              close_at(ca),  close_price(cp),
              forcibly_closed(forcibly)
        {}

        void display(hft::instrument_type itype) const;

        double get_pips_pl(hft::instrument_type itype) const;

        position_type type;
        double open_price;
        double close_price;
        bool   forcibly_closed;

        std::string open_at;
        std::string close_at;
    };

    typedef std::list<position_result> position_result_container;

    static const double leverage_;

    position_result_container position_history_;

    const hft::instrument_type itype_;

    //
    // Position information.
    //

    position_type position_;
    double open_price_;
    std::string open_at_;

    //
    // Additional flag - if set, emulator inverts HFT trade decision.
    //

    bool invert_hft_decision_;
};

#endif /* __FX_ACCOUNT_HPP__ */
 
