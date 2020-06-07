/**********************************************************************\
**                                                                    **
**                 -=≡≣ BitMex Data Manager  ≣≡=-                    **
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

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <bdmdb.hpp>
#include <generator.hpp>
#include <file_operations.hpp>
#include "../caans/include/caans_client.hpp"

namespace prog_opts = boost::program_options;

extern bdm::appconfig config;
extern std::string db_file_name;

static struct bdm_net_autogen_options_type
{
    std::string file_name;
    std::string type;
    bool with_caans;

} bdm_net_autogen_options;

#define bdmOption(__X__) \
    bdm_net_autogen_options.__X__

#define CAANS_NOTIFY(__X__) \
        if (bdmOption(with_caans)) {\
            try { \
                caans::notify("bdm", __X__); \
            } catch (const std::runtime_error &e) { \
                std::cerr << "bdm: ERROR! Fail to send notify to CAANS: " \
                          << e.what() << std::endl; \
            } \
        }

int bdm_network_autogen_main(int argc, char *argv[])
{
    prog_opts::options_description desc("Options for network autogen mode");
    desc.add_options()
        ("help,h", "Produce help message")
        ("file,f", prog_opts::value<std::string>(&bdmOption(file_name)), "Neural network output file")
        ("type,t", prog_opts::value<std::string>(&bdmOption(type)), "Neural network type: PROD or TESTNET")
        ("with-caans,c", prog_opts::value<bool>(&bdmOption(with_caans)) -> default_value(false), "Enable integration with CAANS")
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

    bdm::mode_operation mode;

    if (bdmOption(type) == "PROD")
    {
        mode = bdm::mode_operation::PRODUCTION;
    }
    else if (bdmOption(type) == "TESTNET")
    {
        mode = bdm::mode_operation::TESTNET;
    }
    else
    {
        std::string errmsg = std::string("ERROR! Invalid network type `")
                             + bdmOption(type) + std::string("'");

        std::cerr << "bdm: " << errmsg << std::endl;

        CAANS_NOTIFY(errmsg)

        return 1;
    }

    if (! boost::filesystem::exists(db_file_name))
    {
        std::string errmsg = std::string("ERROR! database ") + db_file_name
                             + std::string(" does not exist. Run program with parameter `create-db'");

        std::cerr << "bdm: " << errmsg << std::endl;

        CAANS_NOTIFY(errmsg)

        return 1;
    }

    try
    {
        bdm::db::initialize_database(db_file_name);
        bdm::db::load_config(config);
    }
    catch (const std::exception &e)
    {
        std::string errmsg = std::string("Database error: ")
                             + std::string(e.what());

        std::cerr << "bdm: " << errmsg << std::endl;

        CAANS_NOTIFY(errmsg)

        return 1;
    }

    try
    {
        bdm::generator::proceed(mode, config, false);
        bdm::db::network_record network = bdm::db::get_last_network(mode);
        bdm::file_put_contents(bdmOption(file_name), network.network);
    }
    catch (const std::exception &e)
    {
        std::cerr << "ERROR! " << e.what() << std::endl;

        CAANS_NOTIFY(e.what())

        return 1;
    }

    return 0;
}
