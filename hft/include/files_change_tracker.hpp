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

#ifndef __FILES_CHANGE_TRACKER__
#define __FILES_CHANGE_TRACKER__

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h> 

#include <string>
#include <map>

class files_change_tracker
{
public:

    files_change_tracker(void) = default;
    ~files_change_tracker(void) = default;

    void register_file(unsigned int id, const std::string &file_name);
    bool has_changed(unsigned int id);
    void notify_change(unsigned int id);
    std::string get_name(unsigned int id);

private:

    typedef struct _file_info
    {
        std::string path;
        time_t last_mod;

    } file_info;

    std::map<unsigned int, file_info> info_storage_;
};

#endif /* __FILES_CHANGE_TRACKER__ */
