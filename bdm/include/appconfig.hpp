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

#ifndef __BDM_APPCONFIG_HPP__
#define __BDM_APPCONFIG_HPP__

#include <string>

namespace bdm {

typedef struct _appconfig
{
    _appconfig(void)
        : trade_setup_target(10),
          market_collector_size(220),
          hftr_offset(100),
          hft_program_path(""),
          records_per_hftr(765750),
          network_arch("[7]"),
          train_goal(0.89),
          learn_coeeficient(0.01) {}

    unsigned int trade_setup_target;
    unsigned int market_collector_size;
    unsigned int hftr_offset;

    std::string hft_program_path;
    unsigned int records_per_hftr;
    std::string network_arch;
    double train_goal;
    double learn_coeeficient;

} appconfig;

typedef enum _mode_operation
{
    PRODUCTION,
    TESTNET

} mode_operation;

} /* namespace bdm */

#endif /* __BDM_APPCONFIG_HPP__ */
