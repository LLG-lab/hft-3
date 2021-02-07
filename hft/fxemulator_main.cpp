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
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <fx_account.hpp>
#include <boost/program_options.hpp>
#include <easylogging++.h>

namespace prog_opts = boost::program_options;

using boost::asio::ip::tcp;

enum { max_length = 1024 };

static struct fxemulator_options_type
{
    std::string host;
    std::string port;
    std::string instrument;
    bool invert_hft_decision;

} fxemulator_options;

#define hftOption(__X__) \
    fxemulator_options.__X__

#define hft_log(__X__) \
    CLOG(__X__, "fxemulator")

static bool subscribe_instrument(tcp::socket &server_connection)
{
    std::string reply;
    std::string data_to_server = std::string("SUBSCRIBE;")
                                 + hftOption(instrument) + std::string("\n");

    hft_log(INFO) << "Subscribe instrument [" << hftOption(instrument) << "]";

    boost::asio::write(server_connection, boost::asio::buffer(data_to_server.c_str(),data_to_server.length()));

    boost::asio::streambuf data_from_server;
    size_t reply_length = boost::asio::read_until(server_connection, data_from_server, '\n');
    std::istream str(&data_from_server); 
    std::getline(str, reply);

    if (reply != "OK")
    {
        hft_log(ERROR) << "Unable to subscribe instrument ["
                       << hftOption(instrument) << "]. Server response error ["
                       << reply << "]";

        return false;
    }
    else
    {
        hft_log(INFO) << "Subscription successful";
    }

    return true;
}

static void emulatefx(tcp::socket &server_connection, const std::string &csv_file, fx_account &account)
{
    csv_loader::csv_record tick_record;
    csv_loader csv_data(csv_file);

    std::string data_to_server;
    std::string reply;

    while (csv_data.get_record(tick_record))
    {
        data_to_server = std::string("TICK;") + hftOption(instrument)
                         + std::string(";")
                         + tick_record.request_time
                         + std::string(";")
                         + (boost::lexical_cast<std::string>(tick_record.ask))
                         + std::string(";0;")
                         + account.get_position_status(tick_record)
                         + std::string("\n");

        boost::asio::write(server_connection, boost::asio::buffer(data_to_server.c_str(),data_to_server.length()));

        boost::asio::streambuf data_from_server;
        size_t reply_length = boost::asio::read_until(server_connection, data_from_server, '\n');
        std::istream str(&data_from_server); 
        std::getline(str, reply);

        #ifdef TEST
        std::cout << "Reply is: " << reply << "\n";
        #endif

        account.proceed_operation(reply, tick_record);
    }

    account.forcibly_close_position(tick_record);
}

int hft_fxemulator_main(int argc, char *argv[])
{
    //
    // Define default logger configuration.
    //

    el::Configurations logger_cfg;
    logger_cfg.setToDefault();
    logger_cfg.parseFromText("* GLOBAL:\n"
                             " FORMAT               =  \"%datetime %level [%logger] %msg\"\n"
                             " FILENAME             =  \"/dev/null\"\n"
                             " ENABLED              =  true\n"
                             " TO_FILE              =  false\n"
                             " TO_STANDARD_OUTPUT   =  true\n"
                             " SUBSECOND_PRECISION  =  1\n"
                             " PERFORMANCE_TRACKING =  true\n"
                             " MAX_LOG_FILE_SIZE    =  10485760 ## 10MiB\n"
                             " LOG_FLUSH_THRESHOLD  =  1 ## Flush after every single log\n"
                            );
    el::Loggers::setDefaultConfigurations(logger_cfg);

    START_EASYLOGGINGPP(argc, argv);

    //
    // Parsing options for fxemulator.
    //

    prog_opts::options_description hidden("Hidden options");
    hidden.add_options()
        ("fxemulator", "")
    ;

    prog_opts::options_description desc("Options for fxemulator");
    desc.add_options()
        ("help,h", "produce help message")
        ("host,H", prog_opts::value<std::string>(&hftOption(host)) -> default_value("localhost"), "HFT server hostname or ip address. Defaut is localhost.")
        ("port,P", prog_opts::value<std::string>(&hftOption(port)) -> default_value("8137"), "HFT server listen port. Default is 8137.")
        ("invert-hft-decision,I", prog_opts::value<bool>(&hftOption(invert_hft_decision)) -> default_value(false), "Trade based on inverted HFT decision.")
        ("instrument,i", prog_opts::value<std::string>(&hftOption(instrument)))
        ("csv-files,f", prog_opts::value< std::vector<std::string> >(), "CSV file name(s) with dukascopy history data.")
    ;

    prog_opts::options_description cmdline_options;
    cmdline_options.add(desc).add(hidden);

    prog_opts::positional_options_description p;
    p.add("csv-files", -1);

    prog_opts::variables_map vm;
    prog_opts::store(prog_opts::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
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
    // Setup logging.
    //

    el::Logger *logger = el::Loggers::getLogger("fxemulator", true);

    hft::instrument_type itype = hft::instrument2type(hftOption(instrument));

    if (itype == hft::UNRECOGNIZED_INSTRUMENT)
    {
        hft_log(ERROR) << "Unsupported instrument ‘" << hftOption(instrument) << "’";

        return 1;
    }

    const std::vector<std::string> &files = vm["csv-files"].as<std::vector<std::string>>();

    boost::asio::io_context ioctx;

    tcp::socket socket(ioctx);
    tcp::resolver resolver(ioctx);
    boost::asio::connect(socket, resolver.resolve({hftOption(host), hftOption(port)}));

    if (! subscribe_instrument(socket))
    {
        return 1;
    }

    fx_account account(itype);

    if (hftOption(invert_hft_decision))
    {
        hft_log(INFO) << "NOTICE: Trading based on INVERTED hft decision.";

        account.invert_hft_decision();
    }

    for (auto file : files)
    {
        hft_log(INFO) << "Now file: [" << file << "]";
        emulatefx(socket, file, account);
    }

    //
    // TODO: In future display statistics of account.
    //

    account.dispaly_statistics();

    hft_log(INFO) << "** Done.";

    return 0;
}
