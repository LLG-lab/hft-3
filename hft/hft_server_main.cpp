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

#include <memory>

#include <daemon_process.hpp>
#include <marketplace_gateway_process.hpp>
#include <hft_session.hpp>
#include <basic_tcp_server.hpp>
#include <auto_deallocator.hpp>
#include "../caans/include/caans_client.hpp"

#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>

#include <easylogging++.h>

namespace prog_opts = boost::program_options;

class hft_server : public basic_tcp_server<hft_session>
{
public:

    using basic_tcp_server<hft_session>::basic_tcp_server;

    ~hft_server(void)
    {
        //
        // Destroy pending sessions, if any.
        //

        auto_deallocator::cleanup();

    }
};

static struct hft_server_options_type
{
    std::string config_file_name;
    bool start_as_daemon;
} hft_server_options;

#define hftOption(__X__) \
    hft_server_options.__X__

#define hft_log(__X__) \
    CLOG(__X__, "server")

static std::unique_ptr<daemon_process> server_daemon;

int hft_server_main(int argc, char *argv[])
{
    //
    // Define default configuration for all future server loggers.
    //

    el::Configurations logger_cfg;
    logger_cfg.setToDefault();
    logger_cfg.parseFromText("* GLOBAL:\n"
                             " FORMAT               =  \"%datetime %level [%logger] %msg\"\n"
                             " FILENAME             =  \"/var/log/hft/server.log\"\n"
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
    // Parsing options for hft server.
    //

    prog_opts::options_description hidden("Hidden options");
    hidden.add_options()
        ("server", "")
    ;

    prog_opts::options_description desc("Options for hft server");
    desc.add_options()
        ("help,h", "Produce help message")
        ("daemon,D", "Start server as a daemon")
        ("config,c", prog_opts::value<std::string>(&hftOption(config_file_name) ) -> default_value("/etc/hft/hftd.conf"), "Server configuration file name")
    ;

    prog_opts::options_description cmdline_options;
    cmdline_options.add(desc).add(hidden);

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

    if (vm.count("daemon"))
    {
        hftOption(start_as_daemon) = true;
    }
    else
    {
        hftOption(start_as_daemon) = false;
    }

    //
    // Initialize server config.
    //

    prog_opts::variables_map server_config;
    prog_opts::options_description server_config_desc;
    server_config_desc.add_options()
        ("general.caans_integration_enabled", prog_opts::value<bool>() -> default_value(false))
        ("networking.listen_port", prog_opts::value<short>() -> default_value(8137))
        ("handlers.enable_cache", prog_opts::value<bool>() -> default_value(false))
        ("handlers.prolong_position_to_next_setup", prog_opts::value<bool>() -> default_value(false))
        ("handlers.trade_positive_swaps_only", prog_opts::value<bool>() -> default_value(false))
        ("marketplace.enabled", prog_opts::value<bool>() -> default_value(false))
        ("marketplace.bridges_config", prog_opts::value<std::string>() -> default_value(""))
        ("trade_time_frame.enabled", prog_opts::value<bool>() -> default_value(false))
        ("trade_time_frame.Mon", prog_opts::value<std::string>() -> default_value("0:00:00,00-23:59:59,99"))
        ("trade_time_frame.Tue", prog_opts::value<std::string>() -> default_value("0:00:00,00-23:59:59,99"))
        ("trade_time_frame.Wed", prog_opts::value<std::string>() -> default_value("0:00:00,00-23:59:59,99"))
        ("trade_time_frame.Thu", prog_opts::value<std::string>() -> default_value("0:00:00,00-23:59:59,99"))
        ("trade_time_frame.Fri", prog_opts::value<std::string>() -> default_value("0:00:00,00-23:59:59,99"))
    ;
    prog_opts::store(prog_opts::parse_config_file<char>(hftOption(config_file_name).c_str(), server_config_desc), server_config);
    prog_opts::notify(server_config);

    if (hftOption(start_as_daemon))
    {
        try
        {
            server_daemon.reset(new daemon_process("/var/run/hftd.pid"));
        }
        catch (const daemon_process::exception &e)
        {
            std::cerr << "**** FATAL ERROR: "
                      << e.what() << std::endl;

            hft_log(ERROR) << "**** FATAL ERROR: " << e.what();

            return 255; /* Return with fatal error exit code. */
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
        if (server_config["marketplace.enabled"].as<bool>())
        {
            //
            // Start HFT server with marketplace bridge process to.
            // Bridge process starts before server, since we don't
            // that child process (bridge) inherit accepting
            // connection socket.
            //

            marketplace_gateway_process gateway_process(ioctx, server_config);
            hft_server server(ioctx, server_config);

            if (hftOption(start_as_daemon))
            {
                //
                // Notify parent process that daemon is ready to go.
                //

                server_daemon -> notify_success();
            }

            ioctx.run();
        }
        else
        {
            //
            // Start HFT server standalone.
            //

            hft_server server(ioctx, server_config);

            if (hftOption(start_as_daemon))
            {
                //
                // Notify parent process that daemon is ready to go.
                //

                server_daemon -> notify_success();
            }

            ioctx.run();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "**** FATAL ERROR: "
                  << e.what() << std::endl;

        hft_log(ERROR) << "**** FATAL ERROR: " << e.what();

        //
        // SMS notification, if option enabled.
        //

        if (server_config["general.caans_integration_enabled"].as<bool>())
        {
            hft_log(INFO) << "Sending error notification via CAANS";

            try
            {
                caans::notify("hft", e.what());
            }
            catch (const std::runtime_error &f)
            {
                hft_log(ERROR) << f.what();
            }
        }

        return 255; /* Return with fatal error exit code. */
    }

    hft_log(INFO) << "*** Server terminated.";

    return 0;
}
