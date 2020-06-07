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

#include <files_change_tracker.hpp>

#include <stdexcept>

void files_change_tracker::register_file(unsigned int id, const std::string &file_name)
{
    file_info info;
    struct stat buf;

    if (::stat(file_name.c_str(), &buf) != 0)
    {
        std::string err_msg = std::string("Failed to stat file: ")
                              + file_name + std::string(" system error(")
                              + std::to_string(errno) + std::string(")");

        throw std::runtime_error(err_msg);
    }

    info.path = file_name;
    info.last_mod = buf.st_mtime;

    info_storage_[id] = info;
}

bool files_change_tracker::has_changed(unsigned int id)
{
    struct stat buf;

    if (::stat(get_name(id).c_str(), &buf) != 0)
    {
        std::string err_msg = std::string("Failed to stat file: ")
                              + get_name(id) + std::string(" system error(")
                              + std::to_string(errno) + std::string(")");

        throw std::runtime_error(err_msg);
    }

    return (info_storage_[id].last_mod != buf.st_mtime);
}

void files_change_tracker::notify_change(unsigned int id)
{
    register_file(id, get_name(id));
}

std::string files_change_tracker::get_name(unsigned int id)
{
    return info_storage_[id].path;
}
