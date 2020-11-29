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

#ifndef __HFT_UTILS_HPP__
#define __HFT_UTILS_HPP__

#include <string>

namespace hft {

typedef unsigned int instrument_type;

const instrument_type UNRECOGNIZED_INSTRUMENT=0xFFFF;

unsigned int floating2dpips(const std::string &data, instrument_type t);
const char *get_instrument_description(instrument_type t);
instrument_type instrument2type(const std::string &instrstr);

std::string timestamp_to_datetime_str(long timestamp);

} /* namespace hft */

#endif /* __HFT_UTILS_HPP__ */
