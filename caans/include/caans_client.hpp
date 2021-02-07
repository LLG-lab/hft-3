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

#ifndef __CAANS_CLIENT_HPP__
#define __CAANS_CLIENT_HPP__

#include <stdexcept>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cJSON/cJSON.h>

namespace caans {

void notify(const std::string &process, const std::string &message)
{
    static const char *socket_path = "/var/run/caans.socket";

    struct sockaddr_un addr;
    char buf[1024];
    int fd, rc;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        // obsługa errno.
        throw std::runtime_error("CAANS: Failed to create socket");
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr*) &addr, sizeof(addr)) == -1)
    {
        // obsługa errno.
        close(fd);
        throw std::runtime_error("CAANS: Failed to connect to caans service");
    }

    //
    // Example: {"process":"NAME","message":"SMSDATA"}
    //

    cJSON *req_json = cJSON_CreateObject();
    cJSON_AddItemToObject(req_json, "process", cJSON_CreateString(process.c_str()));
    cJSON_AddItemToObject(req_json, "message", cJSON_CreateString(message.c_str()));
    std::string msg = cJSON_PrintUnformatted(req_json);
    msg += std::string("\n");
    cJSON_Delete(req_json);

    //
    // Write message.
    //

    if (write(fd, msg.c_str(), msg.length()) != msg.length())
    {
        close(fd);
        throw std::runtime_error("CAANS: Failed to write message to socket");
    }

    //
    // Read reply.
    //

    std::string rep;

    while ((rc = read(fd, buf, sizeof(buf)-1)) > 0)
    {
        buf[rc] = '\0';
        rep += std::string(buf);

        if (buf[rc-1] == '\n')
        {
            break;
        }
    }

    close(fd);

    cJSON *json = cJSON_Parse(rep.c_str());

    if (json == nullptr)
    {
        throw std::runtime_error("CAANS: Received invalid reply from service");
    }

    cJSON *status_json = cJSON_GetObjectItem(json, "status");

    if (status_json == nullptr)
    {
        cJSON_Delete(json);

        throw std::runtime_error("CAANS: Received invalid reply from service");
    }
    else if (status_json -> type != cJSON_String)
    {
        cJSON_Delete(json);

        throw std::runtime_error("CAANS: Received invalid reply from service");
    }

    std::string status = status_json -> valuestring;

    if (status == "OK")
    {
        cJSON_Delete(json);

        return;
    }

    cJSON *message_json = cJSON_GetObjectItem(json, "message");

    if (message_json == nullptr)
    {
        cJSON_Delete(json);

        throw std::runtime_error("CAANS: Received invalid reply from service");
    }
    else if (message_json -> type != cJSON_String)
    {
        cJSON_Delete(json);

        throw std::runtime_error("CAANS: Received invalid reply from service");
    }

    std::string err_msg = std::string("CAANS: ") + std::string(message_json -> valuestring);
    cJSON_Delete(json);
    throw std::runtime_error(err_msg);
}

} /* namespace caans */

#endif /* __CAANS_CLIENT_HPP__ */
