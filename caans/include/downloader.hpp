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

#ifndef __BDM_DOWNLOADER_HPP__
#define __BDM_DOWNLOADER_HPP__

#include <curl/curl.h>
#include <boost/core/noncopyable.hpp>
#include <custom_except.hpp>

#include <list>
#include <stdexcept>
#include <mutex>

namespace caans {

class downloader : private boost::noncopyable
{
public:

    //
    // Downloader exceptions.
    //

    DEFINE_CUSTOM_EXCEPTION_CLASS(downloader_init_error, std::runtime_error)
    DEFINE_CUSTOM_EXCEPTION_CLASS(downloader_setup_error, std::runtime_error)
    DEFINE_CUSTOM_EXCEPTION_CLASS(downloader_transfer_error, std::runtime_error)

    //
    // Constructor.
    //

    explicit downloader(void);

    static void init_globals(void);

    ~downloader(void);

    void download_data(const std::string &uri, const std::string &file_name = std::string(""),
                           const std::list<std::string> &custom_headers = std::list<std::string>());

    void load_from_local_filesystem(const std::string &path);

    const std::string &get_buffer(void) const;

    void clear_buffer(void);

private:

    //
    // Destroys all curl stuff.
    //

    void destroy_curl(void);

    //
    // Exception wrapper for curl_easy_setopt.
    //

    void curl_easy_setopt_chk(CURLcode curl_code, const char *message);

    //
    // Callback for write data used by CURL.
    //

    static size_t curl_write_data_callback(char *ptr, size_t size,
                                               size_t nmemb, void *dev_private);

    //
    // Private connection context used by curl.
    //

    CURL *curl_ctx_;

    char curl_error_buffer_[1024];
    static bool curl_global_initialized_;

    //
    // Buffer, where curl_write_data_callback stores
    // data transfered from remote server.
    //

    std::string transfer_buffer_;

    static const char *const user_agent_fmt_;
    static std::mutex init_global_mtx_;
};

} /* namespace bdm */

#endif /* __BDM_DOWNLOADER_HPP__ */
