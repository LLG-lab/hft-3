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

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#include <hft_req_proto.hpp>

namespace hft {

typedef boost::tokenizer<boost::char_separator<char> >::iterator token_iterator;

//
// Helper routines.
//

static subscribe_instrument_message parse_subscribe_instrument_message(token_iterator begin, token_iterator end);
static tick_message parse_tick_message(token_iterator begin, token_iterator end);
static historical_tick_message parse_historical_tick_message(token_iterator begin, token_iterator end);
static hci_setup_message parse_hci_setup_message(token_iterator begin, token_iterator end);

static hft::instrument_type validate_instrument(const std::string &instrument);
static void validate_ask(const std::string &ask);

//
// Main protocol request parsing function.
//

generic_protocol_message req_parse_message(const std::string &data)
{
    generic_protocol_message message;

    boost::char_separator<char> sep(";");
    boost::tokenizer<boost::char_separator<char> > tokens(data, sep);

    auto it = tokens.begin();

    if (it == tokens.end())
    {
        throw protocol_violation_error("Missing code operation");
    }

    std::string code_operation = (*it);

    if (code_operation == "TICK")
    {
        message.first  = tick_message::OPCODE;
        message.second = parse_tick_message(++it, tokens.end());
    }
    else if (code_operation == "HISTORICAL_TICK")
    {
        message.first  = historical_tick_message::OPCODE;
        message.second = parse_historical_tick_message(++it, tokens.end());
    }
    else if (code_operation == "SUBSCRIBE")
    {
        message.first  = subscribe_instrument_message::OPCODE;
        message.second = parse_subscribe_instrument_message(++it, tokens.end());
    }
    else if (code_operation == "HCI_SETUP")
    {
        message.first  = hci_setup_message::OPCODE;
        message.second = parse_hci_setup_message(++it, tokens.end());
    }
    else
    {
        throw protocol_violation_error(std::string("Illegal code operation : ") + code_operation);
    }

    return message;
}

subscribe_instrument_message parse_subscribe_instrument_message(token_iterator begin, token_iterator end)
{
    // Dane przychodzące będą zawsze w formacie:
    // INSTR1;INSTR2;INSTR3;...
    subscribe_instrument_message msg;
    std::string instrument;

    auto it = begin;

    while (it != end)
    {
        instrument = (*it);
        validate_instrument(instrument);
        msg.instruments.insert(instrument);
        it++;
    }

    if (msg.instruments.empty())
    {
        throw protocol_violation_error("Empty instrument set");
    }

    return msg;
}

tick_message parse_tick_message(token_iterator begin, token_iterator end)
{
    // Dane przychodzace będą zawsze w formacie:
    // WALUTA;data_i_czas;kurs_ask;bankrol;pozycja
    //
    // gdzie:
    //    data_i_czas ma postać „YYYY-mm-dd hh:mm:ss.000”
    tick_message msg;

    auto it = begin;

    if (it == end)
    {
        throw protocol_violation_error("Missing instrument");
    }

    std::string instrument = (*it);

    if (++it == end)
    {
        throw protocol_violation_error("Missing date/time information");
    }

    std::string request_time = (*it);

    if (++it == end)
    {
        throw protocol_violation_error("Missing ASK information");
    }

    std::string ask = (*it);

    if (++it == end)
    {
        throw protocol_violation_error("Missing bankroll");
    }

    std::string bankroll = (*it);

    if (++it == end)
    {
        throw protocol_violation_error("Missing position status");
    }

    std::string position_status = (*it);

    if (++it != end)
    {
        throw protocol_violation_error(std::string("Unexpected data: ") + (*it));
    }

    //
    // Data validation for instrument.
    //

    hft::instrument_type itype = validate_instrument(instrument);

    msg.instrument = instrument;

    //
    // Data validation for request date/time.
    //

    try
    {
        msg.request_time = boost::posix_time::ptime(boost::posix_time::time_from_string(request_time));
    }
    catch (const std::exception &e)
    {
        throw protocol_violation_error(std::string("Bad request time: ") + request_time);
    }

    if (msg.request_time.is_not_a_date_time())
    {
        throw protocol_violation_error(std::string("Bad request time: ") + request_time);
    }

    validate_ask(ask);

    //
    // Here we have to convert ASK value from floating point
    // number to unsigned integer (value in pips).
    //

    msg.ask = hft::floating2dpips(ask, itype);

    //
    // Data validation for bankroll. For bankroll we check
    // if data is convertible to double and is non
    // negative number.
    //

    try
    {
        msg.bankroll = boost::lexical_cast<double>(bankroll);

        if (msg.bankroll < 0.0)
        {
            throw protocol_violation_error(std::string("Bad bankroll value: ") + bankroll);
        }
    }
    catch (const boost::bad_lexical_cast &e)
    {
        throw protocol_violation_error(std::string("Bad bankroll value: ") + bankroll);
    }

    //
    // Currently we don't check validity of position_status,
    // later we will check it.
    //

/*TODO: */

    msg.position_status = position_status;

    return msg;
}

historical_tick_message parse_historical_tick_message(token_iterator begin, token_iterator end)
{
    // Dane przychodzace będą zawsze w formacie:
    // WALUTA;kurs_ask
    historical_tick_message msg;

    auto it = begin;

    if (it == end)
    {
        throw protocol_violation_error("Missing instrument");
    }

    std::string instrument = (*it);

    if (++it == end)
    {
        throw protocol_violation_error("Missing ASK information");
    }

    std::string ask = (*it);

    if (++it != end)
    {
        throw protocol_violation_error(std::string("Unexpected data: ") + (*it));
    }

    //
    // Validation.
    //

    hft::instrument_type itype = validate_instrument(instrument);
    validate_ask(ask);

    msg.instrument = instrument;
    msg.ask = hft::floating2dpips(ask, itype);

    return msg;
}

hci_setup_message parse_hci_setup_message(token_iterator begin, token_iterator end)
{
    // Format dane przychodzących:
    // WALUTA;opcja_hci
    //
    // Gdzie opcja_hci to jeden z dwóch stringów:
    // hci_on albo hci_off

    hci_setup_message msg;

    auto it = begin;

    if (it == end)
    {
        throw protocol_violation_error("Missing instrument");
    }

    std::string instrument = (*it);

    if (++it == end)
    {
        throw protocol_violation_error("Missing HCI option");
    }

    std::string hci_option = (*it);

    if (++it != end)
    {
        throw protocol_violation_error(std::string("Unexpected data: ") + (*it));
    }

    //
    // Validation.
    //

    validate_instrument(instrument);

    msg.instrument = instrument;

    if (hci_option == "hci_on")
    {
        msg.enable = true;
    }
    else if (hci_option == "hci_off")
    {
        msg.enable = false;
    }
    else
    {
        throw protocol_violation_error(std::string("Invalid HCI option: ") + hci_option);
    }

    return msg;
}

hft::instrument_type validate_instrument(const std::string &instrument)
{
    hft::instrument_type itype = hft::instrument2type(instrument);

    if (itype == hft::UNRECOGNIZED_INSTRUMENT)
    {
        throw protocol_violation_error(std::string("Bad instrument: ‘") + instrument + std::string("’"));
    }

    return itype;

    //
    // Data validation for instrument. For instrument we
    // check if it has 6 capitalized letters.
    //

//    if (instrument.length() != 7)
//    {
//        throw protocol_violation_error(std::string("Bad instrument: ") + instrument);
//    }
//    else
//    {
//        for (auto strit = instrument.begin(); strit != instrument.end(); ++strit)
//        {
//            char c = (*strit);
//
//            if (c != '/' && (c < 'A' || c > 'Z'))
//            {
//                throw protocol_violation_error(std::string("Bad instrument: ") + instrument);
//            }
//        }
//    }
}

void validate_ask(const std::string &ask)
{
    //
    // Data validation for ASK. For ASK we check
    // if data is convertible to double and
    // is positive number.
    //

    try
    {
        double test_ask = boost::lexical_cast<double>(ask);

        if (test_ask <= 0.0)
        {
            throw protocol_violation_error(std::string("Bad ASK value: ") + ask);
        }
    }
    catch (const boost::bad_lexical_cast &e)
    {
        throw protocol_violation_error(std::string("Bad ASK value: ") + ask);
    }
}

} /* namespace hft */
