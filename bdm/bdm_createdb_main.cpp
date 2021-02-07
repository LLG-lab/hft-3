/**********************************************************************\
**                                                                    **
**                 -=≡≣ BitMex Data Manager  ≣≡=-                    **
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

#include <iostream>
#include <clocale>
#include <newt.h>
#include <dialogs.hpp>
#include <bdmdb.hpp>

#include <boost/filesystem.hpp>

extern bdm::appconfig config;
extern std::string db_file_name;

int bdm_createdb_main(int argc, char *argv[])
{
    //
    // Create database & setup mode.
    //

    if (boost::filesystem::exists(db_file_name))
    {
        std::cerr << "bdm: error! database " << db_file_name
                  << " already exists\n";

        return 1;
    }

    try
    {
        bdm::db::initialize_database(db_file_name);
        bdm::db::create_database();
    }
    catch (const std::exception &e)
    {
        std::cerr << "bdm: Database error: " << e.what() << "\n";

        return 1;
    }

    newtInit();
    newtCls();

    setlocale(LC_ALL, "C");

    newtDrawRootText(0, 0, "Bitmex Data Manager® by LLG Ryszard Gradowski - CONFIG MODE");

    newtPushHelpLine(NULL);
    newtRefresh();

    bdm::dialogs::settings(config);

    newtFinished();

    try
    {
        bdm::db::save_config(config);
    }
    catch (const std::exception &e)
    {
        std::cerr << "bdm: Database error: " << e.what() << "\n";

        return 1;
    }

    return 0;
}
