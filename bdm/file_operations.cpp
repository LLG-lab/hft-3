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

#include <file_operations.hpp>

#include <stdexcept>
#include <fstream>

namespace bdm {

void file_put_contents(const std::string &filename, const std::string &content)
{
    std::ofstream out_stream;
    out_stream.open(filename.c_str(), std::ofstream::out);

    if (out_stream.fail())
    {
        std::string msg = std::string("Unable to create file: ")
                          + filename;

        throw std::runtime_error(msg);
    }

    out_stream << content;
    out_stream.close();
}

std::string file_get_contents(const std::string &filename)
{
    std::fstream in_stream;
    std::string line, ret;

    in_stream.open(filename, std::fstream::in);

    if (in_stream.fail())
    {
        std::string msg = std::string("Unable to open file: ")
                          + filename;

        throw std::runtime_error(msg);
    }

    while (! in_stream.eof())
    {
        std::getline(in_stream, line);
        ret += (line + std::string("\n"));
    }

    in_stream.close();

    return ret;
}

} /* namespace bdm */
