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

#ifndef __SESSION_TRANSPORT_HPP__
#define __SESSION_TRANSPORT_HPP__

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <iostream>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>

#include <easylogging++.h>

#include <auto_deallocator.hpp>
#include <hft_req_proto.hpp>

using boost::asio::ip::tcp;

class session_transport : public auto_deallocator
{
public:

    session_transport(boost::asio::io_service &io_service,
                          const boost::program_options::variables_map &config)
        : socket_(io_service), request_time_(0,0,0,0), config_(config)
    {
         el::Loggers::getLogger("transport", true);
    }

    virtual ~session_transport(void) = default;

    tcp::socket &socket(void)
    {
        return socket_;
    }

    void start(void)
    {
        boost::asio::async_read_until(socket_, input_buffer_, '\n', boost::bind(&session_transport::handle_read, this, _1));
    }

protected:

    const boost::posix_time::time_duration &get_request_time(void) const
    {
        return request_time_;
    }

    const boost::program_options::variables_map &get_config(void) const
    {
        return config_;
    }

    virtual void tick_notify(const hft::tick_message &msg,
                                 std::ostringstream &response) = 0;

    virtual void historical_tick_notify(const hft::historical_tick_message &msg,
                                            std::ostringstream &response) = 0;

    virtual void subscribe_notify(const hft::subscribe_instrument_message &msg,
                                      std::ostringstream &response) = 0;

    virtual void hci_setup_notify(const hft::hci_setup_message &msg,
                                      std::ostringstream &response) = 0;

private:

    void handle_read(const boost::system::error_code &error);

    void handle_write(const boost::system::error_code &error);

    tcp::socket socket_;
    boost::asio::streambuf input_buffer_;
    std::string response_data_;

    boost::posix_time::time_duration request_time_;

    const boost::program_options::variables_map &config_;
};

#endif /* __SESSION_TRANSPORT_HPP__ */
