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

#ifndef __BDM_DIALOGS_HPP__
#define __BDM_DIALOGS_HPP__

#include <appconfig.hpp>
#include <bdmdb.hpp>

namespace bdm {
namespace dialogs {

enum main_menu_selections
{
    MMS_GENERATE_PROD_NETWORK,
    MMS_GENERATE_TESTNET_NETWORK,
    MMS_BROWSE_PROD_NETWORKS,
    MMS_BROWSE_TESTNET_NETWORKS,
    MMS_SETTINGS,
    MMS_EXIT
};

main_menu_selections main_menu(void);

bool settings(appconfig &cfg);

std::string file_browse(void);

void announcement(const std::string &title, const std::string &msg);

void browse_networks(mode_operation mode, db::networks_container &data);

} /* namespace dialogs */
} /* namespace bdm */

#endif /* __BDM_DIALOGS_HPP__ */
