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


#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <poll.h>

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <map>

#include <easylogging++.h>
#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <overnight_swaps.hpp>
#include <custom_except.hpp>
#include <text_file_reader.hpp>

#define hft_log(__X__) \
    CLOG(__X__, "overnight_swaps")

//
// Auxiliary class for monitoring change of
// file with overnight swaps table.
//

#define DIR_MONITOR_EVENT_SIZE    (sizeof(struct inotify_event))
#define DIR_MONITOR_EVENT_BUF_LEN (1024 * (DIR_MONITOR_EVENT_SIZE + 16))

class dir_monitor
{
public:

    DEFINE_CUSTOM_EXCEPTION_CLASS(exception, std::runtime_error)

    dir_monitor(const std::string &d);
    dir_monitor(void) = delete;
    ~dir_monitor(void);

    bool has_changed(const std::string &f);

private:

    int fd_;
    int wd_;
    char buffer_[DIR_MONITOR_EVENT_BUF_LEN];
    std::string dir_;
};

class swaps_table
{
public:

    swaps_table(const std::string &swaps_table_full_path);
    swaps_table(void) = delete;
    ~swaps_table(void);

    bool is_positive_swap_long(const std::string &instrument_str) const;
    bool is_positive_swap_short(const std::string &instrument_str) const;

private:

    void process_swaps_table_line(std::string &line);

    std::map<std::string, std::pair<bool, bool> > swaps_;
};

dir_monitor::dir_monitor(const std::string &d)
    : dir_(d)
{
    hft_log(INFO) << "Creating directory monitor instance for ["
                  << d << "]";

    //
    // Creating the INOTIFY instance.
    //

    fd_ = ::inotify_init();

    if (fd_ == -1)
    {
        std::ostringstream err_msg;

        err_msg << "Failed to initialize dir_monitor for directory ["
                << dir_ << "]: inotify_init failed with system error [";

        switch (errno)
        {
            case EMFILE:
                err_msg << "EMFILE] - The user limit on the total number of inotify instances has been reached.";
                break;
            case ENFILE:
                err_msg << "ENFILE] - The system limit on the total number of file descriptors has been reached.";
                break;
            case ENOMEM:
                err_msg << "ENOMEM] - Insufficient kernel memory is available.";
                break;
            default:
                /* Should never happen. */
                err_msg << "?] - Unknown";
        }

        throw exception(err_msg.str());
    }

    wd_ = ::inotify_add_watch(fd_, d.c_str(), IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO);

    if (wd_ == -1)
    {
        std::ostringstream err_msg;

        err_msg << "Failed to initialize dir_monitor for directory ["
                << dir_ << "]: inotify_add_watch failed with system error [";

        switch (errno)
        {
            case EACCES:
                err_msg << "EACCES] - Read access to the given file is not permitted.";
                break;
            case EBADF:
                err_msg << "EBADF] - The given file descriptor is not valid.";
                break;
            case EFAULT:
                err_msg << "EFAULT] - Pathname points outside of the process's accessible address space.";
                break;
            case EINVAL:
                err_msg << "EINVAL] - The given event mask contains no valid events; or fd is not an inotify file descriptor.";
                break;
            case ENOENT:
                err_msg << "ENOENT] - A directory component in pathname does not exist or is a dangling symbolic link.";
                break;
            case ENOMEM:
                err_msg << "ENOMEM] - Insufficient kernel memory was available.";
                break;
            case ENOSPC:
                err_msg << "ENOSPC] - The user limit on the total number of inotify watches was "
                        << "reached or the kernel failed to allocate a needed resource.";
                break;
            default:
                /* Should never happen. */
                err_msg << "?] - Unknown";
        }

        throw exception(err_msg.str());
    }
}

dir_monitor::~dir_monitor(void)
{
    hft_log(INFO) << "Destroying directory monitor instance for ["
                  << dir_ << "]";

    //
    // Removing directory from the watch list.
    //

    ::inotify_rm_watch(fd_, wd_);

    //
    // Closing the INOTIFY instance.
    //

    ::close(fd_);
}

bool dir_monitor::has_changed(const std::string &f)
{
    int length, i = 0;
    struct pollfd pfd = { .fd = fd_, .events = POLLIN };

    if (::poll(&pfd, 1, 0) != 1)
    {
        //
        // Nothing has changed.
        //

        return false;
    }

    length = ::read(fd_, buffer_, DIR_MONITOR_EVENT_BUF_LEN);

    //
    // Actually read return the list of change
    // events happens. Here, read the change
    // event one by one and process it
    // accordingly.
    //

    while (i < length)
    {
        struct inotify_event *event = (struct inotify_event *) &buffer_[i];

        if (event -> len && std::string(event -> name) == f)
        {
            return true;
        }

        i += DIR_MONITOR_EVENT_SIZE + event -> len;
    }

    return false;
}

swaps_table::swaps_table(const std::string &swaps_table_full_path)
{
    hft_log(INFO) << "Creating swaps table instance for data ["
                  << swaps_table_full_path << "]";

    if (! boost::filesystem::exists(swaps_table_full_path))
    {
        hft_log(WARNING) << "Swap table file [" << swaps_table_full_path
                         << "] does not exists!";

        return;
    }

    //
    // Load data.
    //

    text_file_reader swaps_file(swaps_table_full_path);
    std::string line;
    bool is_eof = false;

    while (! is_eof)
    {
        is_eof = swaps_file.read_line(line);

        boost::trim(line);

        process_swaps_table_line(line);
    }

    //
    // Log swap table.
    //

    hft_log(INFO) << "+------------+------+-------+";
    hft_log(INFO) << "| Instrument | LONG | SHORT |";
    hft_log(INFO) << "+------------+------+-------+";

    for (auto &item : swaps_)
    {
        hft_log(INFO) << "|   " << item.first << "   |   "
                      << (item.second.first ? '+' : '-')
                      << "  |   "
                      << (item.second.second ? '+' : '-')
                      << "   |";
    }

    hft_log(INFO) << "+------------+------+-------+";
}

swaps_table::~swaps_table(void)
{
    hft_log(INFO) << "Destroying swaps table";
}

bool swaps_table::is_positive_swap_long(const std::string &instrument_str) const
{
    std::map<std::string, std::pair<bool, bool> >::const_iterator it;

    it = swaps_.find(instrument_str);

    if (it == swaps_.end())
    {
        CLOG_EVERY_N(1000, ERROR, "overnight_swaps") << "No instrument ["
                                                     << instrument_str
                                                     << "] in swap table!";

        return false;
    }

    return it -> second.first;
}

bool swaps_table::is_positive_swap_short(const std::string &instrument_str) const
{
    std::map<std::string, std::pair<bool, bool> >::const_iterator it;

    it = swaps_.find(instrument_str);

    if (it == swaps_.end())
    {
        CLOG_EVERY_N(1000, ERROR, "overnight_swaps") << "No instrument ["
                                                     << instrument_str
                                                     << "] in swap table!";

        return false;
    }

    return it -> second.second;
}

void swaps_table::process_swaps_table_line(std::string &line)
{
    if (line.length() == 0)
    {
        return;
    }

    std::string instrument;
    bool long_swap_positive;
    bool short_swap_positive;

    static boost::char_separator<char> sep(" \t");
    boost::tokenizer<boost::char_separator<char> > tokens(line, sep);

    int n = 0;
    for (auto token : tokens)
    {
        if (n == 0)
        {
            instrument = boost::erase_all_copy(token, "/");

            if (instrument.length() != 6)
            {
                hft_log(ERROR) << "Bad instrument [" << instrument
                               << "] in line [" << line << "], skipping record.";
                return;
            }
        }
        else if (n == 1)
        {
            try
            {
                if (boost::lexical_cast<double>(token) >= 0.0)
                {
                    long_swap_positive = true;
                }
                else
                {
                    long_swap_positive = false;
                }
            }
            catch (boost::bad_lexical_cast &e)
            {
                hft_log(ERROR) << "Long swap point [" << token
                               << "] is not a floating point number in line ["
                               << line << "], skipping record.";

                return;
            }
        }
        else if (n == 2)
        {
            try
            {
                if (boost::lexical_cast<double>(token) >= 0.0)
                {
                    short_swap_positive = true;
                }
                else
                {
                    short_swap_positive = false;
                }
            }
            catch (boost::bad_lexical_cast &e)
            {
                hft_log(ERROR) << "Short swap point [" << token
                               << "] is not a floating point number in line ["
                               << line << "], skipping record.";

                return;
            }

        }
        else
        {
            break;
        }

        ++n;
    }

    if (n != 3)
    {
        hft_log(ERROR) << "Bad record [" << line << "]";

        return;
    }

    swaps_[instrument] = std::make_pair(long_swap_positive, short_swap_positive);
}

static const std::string overnight_swaps_dir  = "/var/lib/hft/overnight_swaps";
static const std::string overnight_swaps_file = "swap_table";

static el::Logger *logger = nullptr;
static std::shared_ptr<dir_monitor> monitor;
static std::shared_ptr<swaps_table> table;

static void oswps_check_initialized(void)
{
    //
    // Initialize logger.
    //

    if (logger == nullptr)
    {
        logger = el::Loggers::getLogger("overnight_swaps", true);
    }

    //
    // Initialize directory monitor to watch
    // directory with overnight swap table
    // file.
    //

    if (monitor.use_count() == 0)
    {
        monitor.reset(new dir_monitor(overnight_swaps_dir));
    }

    //
    // Initialize swaps_table object.
    //

    if (table.use_count() == 0)
    {
        table.reset(new swaps_table(overnight_swaps_dir + std::string("/") + overnight_swaps_file));
    }

    //
    // Check if swaps table file has changed, if so, reinit.
    //

    if (monitor -> has_changed(overnight_swaps_file))
    {
        hft_log(WARNING) << "Swaps table file ["
                         << overnight_swaps_file
                         << "] has changed. "
                         << "Reinitialization...";

        table.reset(new swaps_table(overnight_swaps_dir + std::string("/") + overnight_swaps_file));
    }
}

//
// Public interface.
//

namespace hft {

bool is_positive_swap_long(const std::string &instrument_str)
{
    oswps_check_initialized();

    return table -> is_positive_swap_long(instrument_str);
}

bool is_positive_swap_short(const std::string &instrument_str)
{
    oswps_check_initialized();

    return table -> is_positive_swap_short(instrument_str);
}

} /* namespace hft */
