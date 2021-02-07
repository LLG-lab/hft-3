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

#include <ctime>
#include <cstdio>

#include <hft_utils.hpp>
#include <session_transport.hpp>

#define hft_log(__X__) \
    CLOG(__X__, "transport")

void session_transport::handle_read(const boost::system::error_code &error)
{
    if (! error)
    {
        //
        // Update information about time when the
        // request arrived.
        //

        time_t t = time(0);
        struct tm *now = localtime(&t);

        request_time_ = boost::posix_time::time_duration(now -> tm_hour, now -> tm_min, now -> tm_sec, 0);

        //
        // Extract the newline-delimited message from the buffer.
        //

        std::string line;
        std::istream is(&input_buffer_);
        std::getline(is, line);

        std::ostringstream response_buffer;

        try
        {
            hft::generic_protocol_message msg = hft::req_parse_message(line);

            switch (msg.first)
            {
                case hft::tick_message::OPCODE:
                    tick_notify(boost::any_cast<hft::tick_message>(msg.second), response_buffer);
                    break;
                case hft::historical_tick_message::OPCODE:
                    historical_tick_notify(boost::any_cast<hft::historical_tick_message>(msg.second), response_buffer);
                    break;
                case hft::subscribe_instrument_message::OPCODE:
                    subscribe_notify(boost::any_cast<hft::subscribe_instrument_message>(msg.second), response_buffer);
                    break;
                case hft::hci_setup_message::OPCODE:
                    hci_setup_notify(boost::any_cast<hft::hci_setup_message>(msg.second), response_buffer);
                    break;
                default:
                    throw std::runtime_error("Transport error");
            }
        }
        catch (const hft::protocol_violation_error &e)
        {
            hft_log(ERROR) << "Protocol violation error: " << e.what()
                           << ". Client request: „" << line << "”";

            response_buffer << "ERROR;" << e.what();
        }
        catch (const std::exception &e)
        {
            hft_log(ERROR) << "Error occured: " << e.what()
                           << ". Going to close the session";

            socket_.close(); // FIXME: Ja bym tu dał: "delete this; return;"
        }

        response_buffer << std::endl;
        response_data_ = response_buffer.str();

        boost::asio::async_write(socket_,
                                 boost::asio::buffer(response_data_.c_str(), response_data_.length()),
                                 boost::bind(&session_transport::handle_write, this, boost::asio::placeholders::error));
    }
    else
    {
        hft_log(WARNING) << "Destroying session because of error: "
                         << error.message();

        delete this;
    }
}

void session_transport::handle_write(const boost::system::error_code& error)
{
    if (! error)
    {
        boost::asio::async_read_until(socket_, input_buffer_, '\n', boost::bind(&session_transport::handle_read, this, _1));
    }
    else
    {
        hft_log(WARNING) << "Destroying session because of error: "
                         << error.message();

        delete this;
    }
}
