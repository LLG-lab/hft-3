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

#ifndef __TEXT_FILE_READER_HPP__
#define __TEXT_FILE_READER_HPP__

#include <fstream>
#include <stdexcept>
#include <custom_except.hpp>

class text_file_reader
{
public:

    DEFINE_CUSTOM_EXCEPTION_CLASS(text_file_reader_exception, std::runtime_error)

    text_file_reader(const std::string &file_name);

    void set_position(unsigned int p);
    unsigned int get_position(void);

    void rewind(void)
    {
        set_position(0);
    }

    bool read_line(std::string &line);

private:

    text_file_reader(void)
    {
        throw text_file_reader_exception("Invalid implementation");
    }

    std::fstream text_file_;
};

#endif /* __TEXT_FILE_READER_HPP__ */
