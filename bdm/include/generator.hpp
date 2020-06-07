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

#ifndef __BDM__GENERATOR_HPP__
#define __BDM__GENERATOR_HPP__

#include <appconfig.hpp>

namespace bdm {
namespace generator {

void proceed(mode_operation mode, const appconfig &config, bool guimode = true);

} /* namespace generator */
} /* namespace bdm */

#endif /* __BDM__GENERATOR_HPP__ */
