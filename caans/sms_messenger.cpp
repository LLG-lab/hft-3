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

#include <sms_messenger.hpp>

#include <easylogging++.h>

#define caans_log(__X__) \
    CLOG(__X__, "sms_messenger")

namespace caans {

sms_messenger::sms_messenger(const boost::program_options::variables_map &config)
    : sandbox_(config["sandbox"].as<bool>()),
      user_(config["user"].as<std::string>()),
      password_(config["password"].as<std::string>()),
      recipient_(config["recipient"].as<std::string>()),
      sender_(config["sender"].as<std::string>())
{
    //
    // Initialize logger.
    //

    el::Loggers::getLogger("sms_messenger", true);

    start_job();
}

void sms_messenger::work(const sms_messenger_data &data)
{
    caans_log(INFO) << "Work got message from process ["
                    << data.process << "], payload ["
                    << data.sms_data << "]";

    std::string url = "https://api.gsmservice.pl/v5/send.php?login="
                      + user_ + std::string("&pass=") + password_
                      + std::string("&recipient=") + recipient_
                      + std::string("&message=")
                      + url_encode(std::string("[") + data.process + std::string("] ") + data.sms_data)
                      + std::string("&msg_type=1&encoding=utf-8&unicode=0")
                      + (sandbox_ ? std::string("&sandbox=1") : std::string("&sandbox=0"));

    if (sender_.length() > 0)
    {
        url += (std::string("&sender=") + url_encode(sender_));
    }

    caans_log(INFO) << "Sending SMS";

    http_client_.clear_buffer();
    http_client_.download_data(url);

    caans_log(INFO) << "Remote server reply: [" << http_client_.get_buffer() << "]";
}

std::string sms_messenger::url_encode(const std::string &data)
{
    std::string ret;

    CURL *curl = curl_easy_init();

    if (curl)
    {
        char *output = curl_easy_escape(curl, data.c_str(), 0);

        if (output)
        {
            ret = output;
            curl_free(output);
            curl_easy_cleanup(curl);
        }
        else
        {
            caans_log(ERROR) << "url_encode: URL encoding error";

            ret = "EmptyMsg";
        }
    }
    else
    {
        caans_log(ERROR) << "url_encode: Failed to initialize curl";

        ret = "EmptyMsg";
    }

    return ret;
}

} /* namespace caans */
