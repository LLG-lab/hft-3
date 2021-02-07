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

#ifndef __DAEMON_PROCESS_HPP__
#define __DAEMON_PROCESS_HPP__

#include <stdexcept>

#include <boost/noncopyable.hpp>

#include <custom_except.hpp>

class daemon_process : private boost::noncopyable
{
public:

    DEFINE_CUSTOM_EXCEPTION_CLASS(exception, std::runtime_error)

    daemon_process(const std::string &pid_file_name = std::string(""));

    ~daemon_process(void);

    void notify_success(void);

private:

    void process_daemonize(void);

    const std::string pid_file_name_;
    static bool daemonized_;
    int   fd_[2];
    bool notified_;
};

#endif /* __DAEMON_PROCESS_HPP__ */
