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

#ifndef __HFT_REQ_PROTO_HPP__
#define __HFT_REQ_PROTO_HPP__

#include <utility>
#include <stdexcept>
#include <set>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/any.hpp>

#include <hft_utils.hpp>
#include <custom_except.hpp>

namespace hft {

    DEFINE_CUSTOM_EXCEPTION_CLASS(protocol_violation_error, std::runtime_error)

    typedef std::pair<int, boost::any> generic_protocol_message;

    typedef struct _tick_message
    {
        enum { OPCODE = 1 };

        std::string instrument;
        boost::posix_time::ptime request_time;
        unsigned int ask;
        double bankroll;
        std::string position_status;

    } tick_message;

    typedef struct _historical_tick_message
    {
        enum { OPCODE = 2 };

        std::string instrument;
        unsigned int ask;

    } historical_tick_message;

    typedef struct _subscribe_instrument_message
    {
        enum { OPCODE = 3 };

        std::set<std::string> instruments;

    } subscribe_instrument_message;

    typedef struct _hci_setup_message
    {
        enum { OPCODE = 4 };

        std::string instrument;
        bool enable;

    } hci_setup_message;

    generic_protocol_message req_parse_message(const std::string &data);

} /* namespace hft */

#endif /* __HFT_REQ_PROTO_HPP__ */
