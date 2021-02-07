/**********************************************************************\
**                                                                    **
**       -=≡≣ Central Alerting And Notification Service  ≣≡=-        **
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

#include <ipc_server.hpp>

#include <easylogging++.h>
#include <cJSON/cJSON.h>

#include <boost/bind.hpp>

#define caans_log(__X__) \
    CLOG(__X__, "ipc_server")

using boost::asio::local::stream_protocol;

namespace caans {

const std::string ipc_server::file_ = "/var/run/caans.socket";

class ipc_session
{
public:

    ipc_session(void) = delete;
    ipc_session(boost::asio::io_context &ioctx, sms_messenger &messenger);
    ~ipc_session(void) = default;

    stream_protocol::socket &socket(void)
    {
        return socket_;
    }

    void start(void)
    {
        boost::asio::async_read_until(socket_, input_buffer_, '\n', boost::bind(&ipc_session::handle_read, this, _1));
    }

private:

    void handle_read(const boost::system::error_code &error);

    void handle_write(const boost::system::error_code &error);

    static sms_messenger_data json2sms_messenger_data(const std::string &json_str);

    boost::asio::io_context &ioctx_;
    sms_messenger &messenger_;

    stream_protocol::socket socket_;
    boost::asio::streambuf input_buffer_;
    std::string response_data_;
};

ipc_session::ipc_session(boost::asio::io_context &ioctx, sms_messenger &messenger)
    : ioctx_(ioctx), messenger_(messenger), socket_(ioctx)
{
    // Nothing to do.
}

void ipc_session::handle_read(const boost::system::error_code &error)
{
    if (! error)
    {
        //
        // Extract the newline-delimited message from the buffer.
        //

        std::string line;
        std::istream is(&input_buffer_);
        std::getline(is, line);

        std::ostringstream response_buffer;

        try
        {
            sms_messenger_data sms = json2sms_messenger_data(line);
            messenger_.enqueue(sms);
            response_buffer << "{\"status\":\"OK\"}";
        }
        catch (const std::runtime_error &e)
        {
            caans_log(ERROR) << "Got invalid message from client process: "
                             << e.what();

            response_buffer << "{\"status\":\"ERROR\",\"message\":\""
                            << e.what() << "\"}";
        }

        response_buffer << std::endl;
        response_data_ = response_buffer.str();

        boost::asio::async_write(socket_,
                                 boost::asio::buffer(response_data_.c_str(), response_data_.length()),
                                 boost::bind(&ipc_session::handle_write, this, boost::asio::placeholders::error));
    }
    else
    {
        caans_log(ERROR) << "Closing connection because of error: "
                         << error.message();

        delete this;
    }
}

void ipc_session::handle_write(const boost::system::error_code &error)
{
    caans_log(INFO) << "Sent response to the process, closing connection.";

    delete this;
}

sms_messenger_data ipc_session::json2sms_messenger_data(const std::string &json_str)
{
    sms_messenger_data ret;

    cJSON *json = cJSON_Parse(json_str.c_str());

    if (json == NULL)
    {
        std::string msg = std::string("Failed to parse json data");

        throw std::runtime_error(msg.c_str());
    }

    cJSON *process_json = cJSON_GetObjectItem(json, "process");

    if (process_json == NULL)
    {
        std::string err_msg = std::string("Undefined 'process' attribute in json data");

        cJSON_Delete(json);

        throw std::runtime_error(err_msg);
    }
    else if (process_json -> type != cJSON_String)
    {
        std::string err_msg = std::string("Attribute 'process' in json data must be a string");

        cJSON_Delete(json);

        throw std::runtime_error(err_msg);
    }

    cJSON *message_json = cJSON_GetObjectItem(json, "message");

    if (message_json == NULL)
    {
        std::string err_msg = std::string("Undefined 'message' attribute in json data");

        cJSON_Delete(json);

        throw std::runtime_error(err_msg);
    }
    else if (message_json -> type != cJSON_String)
    {
        std::string err_msg = std::string("Attribute 'message' in json data must be a string");

        cJSON_Delete(json);

        throw std::runtime_error(err_msg);
    }

    ret.process  = process_json -> valuestring;
    ret.sms_data = message_json -> valuestring;

    cJSON_Delete(json);

    return ret;
}

ipc_server::ipc_server(boost::asio::io_context &ioctx, sms_messenger &messenger)
    : messenger_(messenger),
      ioctx_(ioctx),
      acceptor_(ioctx, stream_protocol::endpoint(ipc_server::file_))
{
    //
    // Loger initialization.
    //

    el::Loggers::getLogger("ipc_server", true);

    start_accept();
}

void ipc_server::start_accept(void)
{
    ipc_session *new_session = new ipc_session(ioctx_, messenger_);

    acceptor_.async_accept(new_session -> socket(),
                           boost::bind(&ipc_server::handle_accept, this, new_session, boost::asio::placeholders::error));
}

void ipc_server::handle_accept(void *new_session, const boost::system::error_code &error)
{
    ipc_session *session = reinterpret_cast<ipc_session *>(new_session);

    if (! error)
    {
        caans_log(INFO) << "New connection from process.\n";
        session -> start();
    }
    else
    {
        caans_log(ERROR) << "Terminate session because of system error: "
                         << error.message();

        delete session;
    }

    start_accept();
}

} /* namespace caans */
