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

#include <cstdlib>
#include <sstream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <daemon_process.hpp>

bool daemon_process::daemonized_ = false;

daemon_process::daemon_process(const std::string &pid_file_name)
    : pid_file_name_(pid_file_name), notified_(false)
{
    process_daemonize();
}

daemon_process::~daemon_process(void)
{
    if (! notified_)
    {
        ::write(fd_[1], "1", 1);
        ::close(fd_[1]);
    }

    if (pid_file_name_.length() > 0)
    {
        ::unlink(pid_file_name_.c_str());
    }
}

void daemon_process::notify_success(void)
{
    ::write(fd_[1], "0", 1);
    ::close(fd_[1]);
    notified_ = true;
}

void daemon_process::process_daemonize(void)
{
    if (daemon_process::daemonized_)
    {
        //
        // Assume, the process can be daemonized only once.
        //

        notified_ = true;
        return;
    }

    pid_t process_id = 0;
    pid_t sid = 0;

    pipe(fd_);

    //
    // Create child process.
    //

    process_id = ::fork();

    if (process_id < 0)
    {
        int error_code = errno;
        std::ostringstream err_msg;
        err_msg << "fork() failed with system error code ("
                << error_code << ')';

        throw exception(err_msg.str());
    }

    if (process_id > 0)
    {
        //
        // Parent process closes up output side of pipe
        // and wait for child notify.
        //

        ::close(fd_[1]);
        char readbuffer[1];

        ::read(fd_[0], readbuffer, 1);

        if (readbuffer[0] == '0')
        {
            //
            // Parent process goes to die gracefully.
            //

            exit(0);
        }
        else
        {
            //
            // Something bad has happen with child process.
            //

            throw exception("Process failed to start");
        }
    }
    else
    {
        //
        // Child process closes up input side of pipe.
        //

        ::close(fd_[0]);
    }

    //
    // Unmask the file mode.
    //

    ::umask(0);

    //
    // Set new session.
    //

    sid = ::setsid();

    if (sid < 0)
    {
        int error_code = errno;
        std::ostringstream err_msg;
        err_msg << "setsid() failed with system error code ("
                << error_code << ')';

        throw exception(err_msg.str());
    }

    //
    // Change the current working directory to root.
    //

    ::chdir("/");

    //
    // Close stdin, stdout and stderr.
    //

    ::close(STDIN_FILENO);
    ::close(STDOUT_FILENO);
    ::close(STDERR_FILENO);

    //
    // If PID file name specified, create file
    // and write pid of the daemon process
    // into.
    //

    if (pid_file_name_.length() > 0)
    {
        std::fstream fs;
        fs.open(pid_file_name_, std::fstream::out);
        if (! fs.is_open())
        {
            int err = errno;
            std::ostringstream err_msg;
            err_msg << "Unable to create PID file: "
                    << pid_file_name_ << " ("
                    << err << ')';

            throw exception(err_msg.str());
        }

        fs << ::getpid() << std::endl;
    }

    daemon_process::daemonized_ = true;
}
