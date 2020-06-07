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

#include <text_file_reader.hpp>

text_file_reader::text_file_reader(const std::string &file_name)
{
    text_file_.open(file_name, std::fstream::in);

    if (text_file_.fail())
    {
        std::string msg = std::string("Unable to open file: ")
                          + file_name;

        throw text_file_reader_exception(msg);
    }
}

void text_file_reader::set_position(unsigned int p)
{
    text_file_.clear();
    text_file_.seekg(p);

    if (text_file_.fail())
    {
        throw text_file_reader_exception("set_position failed");
    }
}

unsigned int text_file_reader::get_position(void)
{
    return text_file_.tellg();
}

bool text_file_reader::read_line(std::string &line)
{
    std::getline(text_file_, line);

    if (text_file_.eof())
    {
        return false;
    }

    return true;
}
