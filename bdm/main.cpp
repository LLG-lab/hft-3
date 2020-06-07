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

#include <iostream>
#include <clocale>
#include <newt.h>
#include <dialogs.hpp>
#include <bdmdb.hpp>
#include <generator.hpp>

#include <boost/filesystem.hpp>

#include "bdm-config.h"

typedef int (*program)(int, char **);

extern int bdm_createdb_main(int argc, char *argv[]);
extern int bdm_network_autogen_main(int argc, char *argv[]);

static int help(int argc, char *argv[])
{
    std::cout << "Usage:\n"
              << "   bdm -h|--help|help\n"
              << "   bdm [mode [mode options] ]\n\n"
              << "Modes:\n"
              << "  create-db       - creates `bdm.db' database in home\n"
              << "                    directory; database MUST NOT exist\n"
              << "  network-autogen - download data from BitMex and\n"
              << "                    generate neural network\n\n"
              << "Run bdm without parameters to start bdm in interactive\n"
              << "mode. Run `bdm <mode> --help' to get further options\n"
              << "for particular mode.\n\n";

    return 0;
}

static struct
{
    const char *tool_name;
    program start_program;

} bdm_programs[] = {
    { .tool_name = "-h",              .start_program = &help },
    { .tool_name = "--help",          .start_program = &help },
    { .tool_name = "help",            .start_program = &help },
    { .tool_name = "create-db",       .start_program = &bdm_createdb_main },
    { .tool_name = "network-autogen", .start_program = &bdm_network_autogen_main }
};

bdm::appconfig config;
std::string db_file_name = getenv("HOME") + std::string("/bdm.db");

int main(int argc, char *argv[])
{
    std::cout << "Bitmex Data Manager®, version " << BDM_VERSION_MAJOR << '.' << BDM_VERSION_MINOR << std::endl
              << " 2017 - 2020 by LLG Ryszard Gradowski, All Rights Reserved" << std::endl << std::endl;

    if (argc > 1)
    {
        for (auto &p : bdm_programs)
        {
            if (strcmp(p.tool_name, argv[1]) == 0)
            {
                return p.start_program(argc - 1, argv + 1);
            }
        }

        std::cerr << "WARNING! Invalid tool specified: "
                  << argv[1] << std::endl;
    }

    //
    // Normal mode.
    //

    if (! boost::filesystem::exists(db_file_name))
    {
        std::cerr << "bdm: error! database " << db_file_name
                  << " does not exist. Run program with option `create-db'"
                  << std::endl;

        return 1;
    }

    try
    {
        bdm::db::initialize_database(db_file_name);
        bdm::db::load_config(config);
    }
    catch (const std::exception &e)
    {
        std::cerr << "bdm: Database error: " << e.what() << "\n";

        return 1;
    }

    newtInit();
    newtCls();

    setlocale(LC_ALL, "C");

    newtDrawRootText(0, 0, "Bitmex Data Manager® by LLG Ryszard Gradowski");

    newtPushHelpLine(NULL);
    newtRefresh();

    while (true)
    {
        bdm::dialogs::main_menu_selections user_selected = bdm::dialogs::main_menu();

        switch (user_selected)
        {
            case bdm::dialogs::MMS_GENERATE_PROD_NETWORK:
                try
                {
                    bdm::generator::proceed(bdm::mode_operation::PRODUCTION, config);
                }
                catch (const std::exception &e)
                {
                    bdm::dialogs::announcement("ERROR", e.what());
                }
                break;
            case bdm::dialogs::MMS_GENERATE_TESTNET_NETWORK:
                try
                {
                    bdm::generator::proceed(bdm::mode_operation::TESTNET, config);
                }
                catch (const std::exception &e)
                {
                    bdm::dialogs::announcement("ERROR", e.what());
                }
                break;
            case bdm::dialogs::MMS_BROWSE_PROD_NETWORKS:
                 try
                 {
                     bdm::db::networks_container data;
                     bdm::db::get_networks(bdm::mode_operation::PRODUCTION, data);

                     if (data.size() > 0)
                     {
                         bdm::dialogs::browse_networks(bdm::mode_operation::PRODUCTION, data);
                     }
                     else
                     {
                         bdm::dialogs::announcement("Info", "No production networks in database");
                     }
                 }
                 catch (const std::exception &e)
                 {
                     bdm::dialogs::announcement("ERROR", e.what());
                 }
                 break;
            case bdm::dialogs::MMS_BROWSE_TESTNET_NETWORKS:
                 try
                 {
                     bdm::db::networks_container data;
                     bdm::db::get_networks(bdm::mode_operation::TESTNET, data);

                     if (data.size() > 0)
                     {
                         bdm::dialogs::browse_networks(bdm::mode_operation::TESTNET, data);
                     }
                     else
                     {
                         bdm::dialogs::announcement("Info", "No testnet networks in database");
                     }
                 }
                 catch (const std::exception &e)
                 {
                     bdm::dialogs::announcement("ERROR", e.what());
                 }
                 break;
            case bdm::dialogs::MMS_SETTINGS:
                 if (bdm::dialogs::settings(config))
                 {
                     try
                     {
                         bdm::db::save_config(config);
                     }
                     catch (const std::exception &e)
                     {
                         bdm::dialogs::announcement("ERROR", e.what());
                     }
                 }
                 break;
            case bdm::dialogs::MMS_EXIT:
                 newtFinished();

                 std::cout << "bdm: See you later.\n";

                 return 0;
        }
    }
}
