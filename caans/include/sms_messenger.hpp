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

#ifndef __SMS_MESSENGER_HPP__
#define __SMS_MESSENGER_HPP__

#include <thread_worker.hpp>
#include <downloader.hpp>

#include <boost/program_options.hpp>

namespace caans {

typedef struct _sms_messenger_data
{
    std::string process;
    std::string sms_data;

} sms_messenger_data;

class sms_messenger : public thread_worker<sms_messenger_data>
{
public:

    sms_messenger(void) = delete;
    sms_messenger(const boost::program_options::variables_map &config);
    ~sms_messenger(void) {  terminate(); }

private:

    void work(const sms_messenger_data &data);
    static std::string url_encode(const std::string &data);

    downloader http_client_;

    const bool sandbox_;
    const std::string user_;
    const std::string password_;
    const std::string recipient_;
    const std::string sender_;
};

} /* namespace caans */

#endif /* __SMS_MESSENGER_HPP__ */
