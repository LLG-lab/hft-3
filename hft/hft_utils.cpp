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

#include <hft_utils.hpp>

#include <cstdio>
#include <ctime>
#include <stdexcept>

#include <boost/lexical_cast.hpp>

namespace hft {

unsigned int floating2dpips(const std::string &data)
{
    double numeric = boost::lexical_cast<double>(data);
    char buffer[15];
    int i = 0, j = 0;

    int status = snprintf(buffer, 15, "%0.5f", numeric);

    if (status <= 0 || status >= 15)
    {
        std::string msg = std::string("Floating point operation error: ")
                          + data;

        throw std::runtime_error(msg);
    }

    //
    // Traverse through all string including null character.
    //

    while (i <= status)
    {
        if (buffer[i] != '.')
        {
            buffer[j++] = buffer[i];
        }

        ++i;
    }

    return boost::lexical_cast<unsigned int>(buffer);
}

std::string timestamp_to_datetime_str(long timestamp)
{
    const time_t rawtime = (const time_t) timestamp;

    struct tm *dt;
    char timestr[40];
    char buffer [40];

    dt = localtime(&rawtime);

    strftime(timestr, sizeof(timestr), "%c", dt);
    sprintf(buffer,"%s", timestr);

    return std::string(buffer);
}


} /* namespace hft */
