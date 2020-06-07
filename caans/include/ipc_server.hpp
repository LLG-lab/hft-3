/**********************************************************************\
**                                                                    **
**       -=≡≣ Central Alerting And Notification Service  ≣≡=-        **
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

#ifndef __IPC_SERVER_HPP__
#define __IPC_SERVER_HPP__

#include <boost/asio.hpp>

#include <sms_messenger.hpp>

namespace caans {

class ipc_server
{
public:

    ipc_server(void) = delete;

    ipc_server(boost::asio::io_context &ioctx, sms_messenger &messenger);

    ~ipc_server(void)
    {
        ::unlink(ipc_server::file_.c_str());
    }

private:

    void start_accept(void);
    void handle_accept(void *new_session, const boost::system::error_code &error);

    static const std::string file_;

    sms_messenger &messenger_;
    boost::asio::io_context &ioctx_;
    boost::asio::local::stream_protocol::acceptor acceptor_;
};

} /* namespace caans */

#endif /* __IPC_SERVER_HPP__ */
