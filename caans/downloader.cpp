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

#include <iostream>
#include <cstring>
#include <cerrno>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <downloader.hpp>
#include "caans-config.h"

namespace caans {

#define CURL_SETOPT_SAFE(__OPTION__, __PARAMETER__) \
           curl_easy_setopt_chk(curl_easy_setopt(curl_ctx_, \
                                              __OPTION__, \
                                              __PARAMETER__), \
                             #__OPTION__);

bool downloader::curl_global_initialized_ = false;

const char *const downloader::user_agent_fmt_ = "CAANS client for Linux, version %d.%d";
std::mutex downloader::init_global_mtx_;

downloader::downloader(void)
    : curl_ctx_(NULL)
{
    memset(curl_error_buffer_, 0, 1024);

    //
    // Initialize curl library.
    //

    try
    {
        std::lock_guard<std::mutex> lck(init_global_mtx_);

        init_globals();
    }
    catch (std::runtime_error &e)
    {
        throw e;
    }


    //
    // Per-object initialization part.
    //

    curl_ctx_ = curl_easy_init();

    if (curl_ctx_ == NULL)
    {
        //
        // Uninit curl library.
        //

        destroy_curl();

        std::string err_msg = "Unable to initialize curl context.";

        throw downloader_init_error(err_msg);
    }

    //
    // General setup of curl context.
    //

    char user_agent_name[strlen(downloader::user_agent_fmt_) + 16];

    sprintf(user_agent_name, downloader::user_agent_fmt_,
                CAANS_VERSION_MAJOR, CAANS_VERSION_MINOR);

    CURL_SETOPT_SAFE(CURLOPT_ERRORBUFFER, curl_error_buffer_);
    CURL_SETOPT_SAFE(CURLOPT_NOSIGNAL, 1)
    CURL_SETOPT_SAFE(CURLOPT_USERAGENT, user_agent_name)
    CURL_SETOPT_SAFE(CURLOPT_WRITEDATA, this)
    CURL_SETOPT_SAFE(CURLOPT_WRITEFUNCTION, &curl_write_data_callback)
    CURL_SETOPT_SAFE(CURLOPT_CONNECTTIMEOUT, 5)
    CURL_SETOPT_SAFE(CURLOPT_SSL_VERIFYPEER, false)
    CURL_SETOPT_SAFE(CURLOPT_PROXY, "")

    //
    // Initial capacity for transfer buffer.
    //

    transfer_buffer_.reserve(4096);
}

void downloader::init_globals(void)
{
    if (! downloader::curl_global_initialized_)
    {
        //
        // This part of initialization
        // have to be done once.
        //

        if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0)
        {
            std::string err_msg = "Unable to initialize curl library.";

            throw std::runtime_error(err_msg);
        }

        downloader::curl_global_initialized_ = true;
    }
}

downloader::~downloader(void)
{
    destroy_curl();
}

void downloader::download_data(const std::string &uri,
                                   const std::string &file_name,
                                       const std::list<std::string> &custom_headers)
{
    transfer_buffer_.clear();
    struct curl_slist *list = NULL;

    if (! custom_headers.empty())
    {
        for (auto &header : custom_headers)
        {
            list = curl_slist_append(list, header.c_str());
        }
    }

    CURL_SETOPT_SAFE(CURLOPT_URL, uri.c_str())
    CURL_SETOPT_SAFE(CURLOPT_HTTPGET, 1)
    CURL_SETOPT_SAFE(CURLOPT_HTTPHEADER, list)

    CURLcode status = curl_easy_perform(curl_ctx_);

    if (list != NULL)
    {
        curl_slist_free_all(list);
        list = NULL;
    }

    //
    // Check for connection error.
    //

    if (status != CURLE_OK)
    {
        char err_code_buff[16] = {'\0'};
        sprintf(err_code_buff, "%d", (int)(status));

        std::string err_msg = std::string("Failed to transfer data. Error code: ")
                             + std::string(err_code_buff) + std::string(" `")
                             + std::string(curl_error_buffer_) + std::string("'.");

        throw downloader_transfer_error(err_msg);
    }

    //
    // Check HTTP response code, if fail throw exception too.
    //

    long http_code = 0;

    curl_easy_getinfo(curl_ctx_, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code != 200)
    {
        char err_code_buff[16] = {'\0'};
        sprintf(err_code_buff, "%d", (int)(http_code));
        std::string err_msg = std::string("Transfer error: Got HTTP error ")
                              + std::string(err_code_buff);

        throw downloader_transfer_error(err_msg);

    }

    //
    // If file name is not specified,
    // data will not be stored on the disk.
    //

    if (file_name.length() == 0)
    {
        return;
    }

    //
    // If downloaded successfully,
    // write data to the file.
    //

    FILE* pFile;
    pFile = fopen(file_name.c_str(), "wb");
    int e = errno;

    if (pFile == NULL)
    {
        char err_code_buff[16] = {'\0'};
        sprintf(err_code_buff, "%d", e);

        std::string err_msg = std::string("Unable to write file: `")
                             + file_name + std::string("' errno is ")
                             + std::string(err_code_buff);

        throw downloader_transfer_error(err_msg);
    }

    int chunks = transfer_buffer_.size() / 1024;
    size_t s;

    for (int i = 0; i < chunks; i++)
    {
        s = fwrite(transfer_buffer_.c_str() + i*1024, 1, 1024, pFile);

        if (s != 1024)
        {
            e = errno;
            char err_code_buff[16] = {'\0'};
            sprintf(err_code_buff, "%d", e);

            fclose(pFile);

            std::string err_msg = std::string("downloader I/O error (")
                                  + std::string(err_code_buff) + std::string(")");

            throw downloader_transfer_error(err_msg);
        }
    }

    int remainder_size = transfer_buffer_.size() % 1024;

    if (remainder_size > 0)
    {
        s = fwrite(transfer_buffer_.c_str()+chunks*1024, 1, remainder_size, pFile);

        if (s != remainder_size)
        {
            e = errno;
            char err_code_buff[16] = {'\0'};
            sprintf(err_code_buff, "%d", e);

            fclose(pFile);

            std::string err_msg = std::string("downloader I/O error (")
                                  + std::string(err_code_buff) + std::string(")");

            throw downloader_transfer_error(err_msg);
        }
    }

    fflush(pFile);
    fclose(pFile);
}

void downloader::load_from_local_filesystem(const std::string &path)
{
    off_t file_size;
    struct stat stbuf;
    int fd;

    fd = ::open(path.c_str(), O_RDONLY);

    if (fd == -1)
    {
        int err = errno;
        char err_code_buff[16] = {'\0'};
        sprintf(err_code_buff, "%d", (int)(err));

        std::string err_msg = std::string("Unable to open file: ") + path
                              + std::string(". Error is (")
                              + std::string(err_code_buff) + std::string(")");
        throw downloader_transfer_error(err_msg);
    }

    if ((::fstat(fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode)))
    {
        int err = errno;
        char err_code_buff[16] = {'\0'};
        sprintf(err_code_buff, "%d", (int)(err));

        std::string err_msg = std::string("fstat, S_ISREG: ") + path
                              + std::string(". Error is (")
                              + std::string(err_code_buff) + std::string(")");

        close(fd);
        throw downloader_transfer_error(err_msg);
    }

    file_size = stbuf.st_size;
    transfer_buffer_.resize(file_size, ' ');

    if (::read(fd, &transfer_buffer_.at(0), file_size) != file_size)
    {
        int err = errno;
        char err_code_buff[16] = {'\0'};
        sprintf(err_code_buff, "%d", (int)(err));

        std::string err_msg = std::string("read failed: ") + path
                              + std::string(". Error is (")
                              + std::string(err_code_buff) + std::string(")");

        close(fd);
        throw downloader_transfer_error(err_msg);
    }
}

const std::string &downloader::get_buffer(void) const
{
    return transfer_buffer_;
}

void downloader::clear_buffer(void)
{
    transfer_buffer_.clear();
}

void downloader::destroy_curl(void)
{
    //
    // Destroy curl context.
    //

    if (curl_ctx_ != NULL)
    {
        curl_easy_cleanup(curl_ctx_);
    }

    //
    // Uninit curl library.
    //
/*
XXX Nie wiemy, który wątek ma to wykonać.
    Pomysł taki, aby wykonywał to destruktor jakiegos
    statycznego obiektu globalnego.

    if (curl_initialized_)
    {
        curl_global_cleanup();

        curl_global_initialized_ = false;
    }
*/
}

void downloader::curl_easy_setopt_chk(CURLcode curl_code, const char *message)
{
    if (curl_code == CURLE_OK)
    {
        return;
    }

    char err_code_buff[16] = {'\0'};
    sprintf(err_code_buff, "%d", (int)(curl_code));

    std::string err_msg = std::string("Call `curl_easy_setopt' with option `")
                          + std::string(message) + std::string("'. Error code: ")
                          + std::string(err_code_buff) + std::string(" `")
                          + std::string(curl_error_buffer_) + std::string("'.");

    throw downloader_setup_error(err_msg);
}

size_t downloader::curl_write_data_callback(char *ptr, size_t size,
                                               size_t nmemb, void *dev_private)
{
    //
    // Regain reference to the object;
    //

    downloader *self = reinterpret_cast<downloader *>(dev_private);

    self -> transfer_buffer_ += std::string(ptr, size*nmemb);

    return size*nmemb;
}

} /* namespace bdm */
