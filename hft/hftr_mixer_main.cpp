/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System  ≣≡=-              **
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
#include <fstream>
#include <vector>
#include <random>

#include <boost/program_options.hpp>

#include <easylogging++.h>

namespace prog_opts = boost::program_options;

struct info_rec
{
    info_rec(int i, long int fp)
       : id(i), file_pos(fp) {}
    int id;
    long int file_pos;
};

static std::vector<info_rec> records;

static std::random_device rd;
static std::mt19937 rng(rd());

static struct _hftr_mixer_config
{
    std::string hftr_file_name;
} hftr_mixer_config;

#define hftOption(__X__) \
    hftr_mixer_config.__X__

#define hft_log(__X__) \
    CLOG(__X__, "hftr_mixer")

int hftr_mixer_main(int argc, char *argv[])
{
    //
    // Define default logger configuration.
    //

    el::Configurations logger_cfg;
    logger_cfg.setToDefault();
    logger_cfg.parseFromText("* GLOBAL:\n"
                             " FORMAT               =  \"%datetime %level [%logger] %msg\"\n"
                             " FILENAME             =  \"/dev/null\"\n"
                             " ENABLED              =  true\n"
                             " TO_FILE              =  false\n"
                             " TO_STANDARD_OUTPUT   =  true\n"
                             " SUBSECOND_PRECISION  =  1\n"
                             " PERFORMANCE_TRACKING =  true\n"
                             " MAX_LOG_FILE_SIZE    =  10485760 ## 10MiB\n"
                             " LOG_FLUSH_THRESHOLD  =  1 ## Flush after every single log\n"
                            );
    el::Loggers::setDefaultConfigurations(logger_cfg);

    START_EASYLOGGINGPP(argc, argv);

    prog_opts::options_description hidden("Hidden options");
    hidden.add_options()
        ("hftr-mixer", "")
    ;

    prog_opts::options_description desc("Options for hftr mixer");
    desc.add_options()
        ("help,h", "produce help message")
        ("input-file,i",  prog_opts::value<std::string>(&hftOption(hftr_file_name) ), "input HFTR file name for mixing")
    ;

    prog_opts::options_description cmdline_options;
    cmdline_options.add(desc).add(hidden);

    prog_opts::variables_map vm;
    prog_opts::store(prog_opts::command_line_parser(argc, argv).options(cmdline_options).run(), vm);
    prog_opts::notify(vm);

    //
    // If user requested help, show help and quit
    // ignoring other options, if any.
    //

    if (vm.count("help"))
    {
        std::cout << desc << "\n";

        return 0;
    }

    el::Logger *logger = el::Loggers::getLogger("hftr_mixer", true);

    std::fstream hftr_in;
    std::fstream hftr_out;
    hftr_in.open(hftOption(hftr_file_name), std::fstream::in);
    hftr_out.open(hftOption(hftr_file_name)+std::string(".mixed"), std::fstream::out | std::fstream::app);

    if (hftr_in.fail())
    {
        throw std::runtime_error("Unable to open input file: " + hftOption(hftr_file_name));
    }
    else if (hftr_out.fail())
    {
        throw std::runtime_error("Unable to open output file: " + hftOption(hftr_file_name) + std::string(".mixed"));
    }

    hft_log(INFO) << "Scanning...";
    int id = 1;

    while (true)
    {
        std::cout << "Scanned record #" << id << "\r";
        records.push_back(info_rec(id++, hftr_in.tellg()));

        std::string line;
        std::getline(hftr_in, line);

        if (hftr_in.eof())
        {
            std::cout << "\n";
            break;
        }

        if (hftr_in.fail())
        {
            std::cout << "\n";
            throw std::runtime_error("Error while reading input file");
        }
    }

    hft_log(INFO) << "Scanning completed.";

    std::uniform_int_distribution<int> uni(0, records.size()-1);
    int index;

    while (records.size() > 0)
    {
        std::string line;

        if (records.size() == 1)
        {
            index = 0;
        }
        else
        {
            index = uni(rng) % (records.size() - 1);
        }

        hftr_in.clear();
        hftr_in.seekg(records.at(index).file_pos);
        std::getline(hftr_in, line);

        if (line.length() > 0)
        {
            hftr_out << line << "\n";
        }

        records.erase(records.begin()+index);
        std::cout << "Remain " << records.size() << "      " << "\r";
    }

    hft_log(INFO) << "Completed.";

    return 0;
}
