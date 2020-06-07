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

#include <memory>
#include <iostream>
#include <exception>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>

#include <easylogging++.h>

#include "caans-config.h"
#include <daemon_process.hpp>
#include <ipc_server.hpp>
#include <caans_client.hpp>

INITIALIZE_EASYLOGGINGPP

namespace prog_opts = boost::program_options;

static struct caans_server_options_type
{
    bool start_as_daemon;
    std::string client_message;

} caans_server_options;

#define caansOption(__X__) \
    caans_server_options.__X__

#define caans_log(__X__) \
    CLOG(__X__, "server")

static std::unique_ptr<daemon_process> server_daemon;

static bool caans_check_instance(void)
{
    // FIXME: Not implemented.

    return false;
}

int main(int argc, char *argv[])
{
    try
    {
        std::cout << "Central Alerting And Notification Service , version "
                  << CAANS_VERSION_MAJOR << '.' << CAANS_VERSION_MINOR << std::endl
                  << "Copyright  2017 - 2020 by LLG Ryszard Gradowski, All Rights Reserved\n\n";

        //
        // Handle simple program options.
        //

        prog_opts::options_description desc("Options for CAANS server");
        desc.add_options()
            ("help,h", "Produce help message")
            ("daemon,D", "Start server as a daemon")
            ("client-message,m", prog_opts::value<std::string>(&caansOption(client_message)), "SMS message to send")
        ;

        prog_opts::options_description cmdline_options;
        cmdline_options.add(desc);

        prog_opts::variables_map vm;
        prog_opts::store(prog_opts::command_line_parser(argc, argv).options(cmdline_options).run(), vm);
        prog_opts::notify(vm);

        //
        // If user requested help, show help and quit
        // ignoring other options, if any.
        //

        if (vm.count("help"))
        {
            std::cout << desc << "\n";

            return 0;
        }

        //
        // Client mode, connect to CAANS server, then send sms
        // and instatnly quit.
        //

        if (caansOption(client_message).length() > 0)
        {
            try
            {
                std::cout << "Sending message\n";

                caans::notify("shell", caansOption(client_message));
            }
            catch (const std::runtime_error &err)
            {
                std::cerr << "ERROR! " << err.what() << std::endl;
            }

            return 0;
        }

        if (caans_check_instance())
        {
            std::cerr << "ERROR! Service CAANS is already running.\n";

            return 1;
        }

        if (vm.count("daemon"))
        {
            caansOption(start_as_daemon) = true;
        }
        else
        {
            caansOption(start_as_daemon) = false;
        }

        //
        // Define default configuration for all future server loggers.
        //

        el::Configurations logger_cfg;
        logger_cfg.setToDefault();
        logger_cfg.parseFromText("* GLOBAL:\n"
                             " FORMAT               =  \"%datetime %level [%logger] %msg\"\n"
                             " FILENAME             =  \"/var/log/caans.log\"\n"
                             " ENABLED              =  true\n"
                             " TO_FILE              =  true\n"
                             " TO_STANDARD_OUTPUT   =  false\n"
                             " SUBSECOND_PRECISION  =  1\n"
                             " PERFORMANCE_TRACKING =  true\n"
                             " MAX_LOG_FILE_SIZE    =  10485760 ## 10MiB\n"
                             " LOG_FLUSH_THRESHOLD  =  1 ## Flush after every single log\n"
                            );

        el::Loggers::setDefaultConfigurations(logger_cfg);

        START_EASYLOGGINGPP(argc, argv);

        //
        // Setup logging.
        //

        el::Logger *logger = el::Loggers::getLogger("server", true);

        //
        // Initialize server config.
        //

        prog_opts::variables_map server_config;
        prog_opts::options_description server_config_desc;
        server_config_desc.add_options()
            ("enabled", prog_opts::value<bool>() -> default_value(true))
            ("sandbox", prog_opts::value<bool>() -> default_value(false))
            ("user", prog_opts::value<std::string>())
            ("password", prog_opts::value<std::string>())
            ("recipient", prog_opts::value<std::string>())
            ("sender", prog_opts::value<std::string>() -> default_value(""))
        ;
        prog_opts::store(prog_opts::parse_config_file<char>("/etc/caans/caansd.conf", server_config_desc), server_config);
        prog_opts::notify(server_config);

        if (caansOption(start_as_daemon))
        {
            try
            {
                server_daemon.reset(new daemon_process("/var/run/caans.pid"));
            }
            catch (const daemon_process::exception &e)
            {
                std::cerr << "**** FATAL ERROR: "
                          << e.what() << std::endl;

                caans_log(ERROR) << "**** FATAL ERROR: " << e.what();

                return 1; /* Return with fatal error exit code. */
            }
        }

        boost::asio::io_context ioctx;

        //
        // Register signal handlers so that the daemon may be shut down.
        //

        boost::asio::signal_set signals(ioctx, SIGINT, SIGTERM);
        signals.async_wait(boost::bind(&boost::asio::io_context::stop, &ioctx));

        try
        {
            caans::sms_messenger messenger(server_config);
            caans::ipc_server server(ioctx, messenger);

            if (caansOption(start_as_daemon))
            {
                //
                // Notify parent process that daemon is ready to go.
                //

                server_daemon -> notify_success();
            }

            ioctx.run();
        }
        catch (const std::exception &e)
        {
            std::cerr << "**** FATAL ERROR: " << e.what() << std::endl;

            caans_log(ERROR) << "**** FATAL ERROR: " << e.what();

            return 1; /* Return with fatal error exit code. */
        }
	
        return 0;
    }
    catch (const std::exception &e)
    {
		std::cerr << "*** Fatal Error: " << e.what() << "\n";
	}
	catch (...)
	{
		std::cerr << "*** Fatal Error of unknown exception type\n";
	}

    /* Error exit code */
    return 1;
}
