/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System  ≣≡=-              **
**                              b                                     **
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

#include <cstdio>
#include <cmath>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include <easylogging++.h>

#include <csv_loader.hpp>
#include <hft_utils.hpp>

namespace prog_opts = boost::program_options;
namespace fs = boost::filesystem;

//
// Globals.
//

static std::map<unsigned int, unsigned int> predistr;
static unsigned int total = 0;

//
// Global tool config structure.
//

static struct hft_instrument_variability_distribution_config_type
{
    std::string source;
    std::string instrument;
    hft::instrument_type itype;
    int granularity;
    std::string output_fmt;
    std::string output_file;

} hft_instrument_variability_distribution_config;

#define hft_log(__X__) \
    CLOG(__X__, "instr_distrib")

#define hftOption(__X__) \
    hft_instrument_variability_distribution_config.__X__

static void proceed_file(const std::string &csv_file_name, int granularity)
{
    csv_loader::csv_record tick_record;
    csv_loader csv_data(csv_file_name);

    int prev = 0;
    int dpips = 0;
    int skip_num = 0;

    std::map<unsigned int, unsigned int>::iterator it;
    std::vector<int> filtered_data;

    if (granularity <= 0)
    {
        throw std::runtime_error("Granularity must be greater than 0");
    }

    //
    // Load data and filtering.
    //

    while (csv_data.get_record(tick_record))
    {
        dpips = hft::floating2dpips(boost::lexical_cast<std::string>(tick_record.ask), hftOption(itype));

        if (prev == 0)
        {
            prev = dpips;
            filtered_data.push_back(dpips);
            continue;
        }

        int d = abs(dpips - prev);

        if (d == 0)
        {
            continue;
        }

        prev = dpips;
        filtered_data.push_back(dpips);
    }

    //
    // Compute distribution.
    //

    prev = 0;

    for (auto p : filtered_data)
    {
        if (prev == 0)
        {
            prev = p;
            continue;
        }

        if (skip_num < granularity)
        {
            skip_num++;
            continue;
        }

        int d = abs(p - prev);

        if (d == 0)
        {
            continue;
        }

        skip_num = 0;

        it = predistr.find(d);

        if (it == predistr.end())
        {
            predistr[d] = 1;
        }
        else
        {
            predistr[d]++;
        }

        prev = p;
        total++;
    }
}

static double probab(unsigned int c)
{
    return double(c) / double(2*total);
}

static void print_distribution(std::ostream &file)
{
    char buffer[25];

    if (hftOption(output_fmt) == "simple")
    {
        for (auto it = predistr.begin(); it != predistr.end(); ++it)
        {
            sprintf(buffer, "%0.6f]\n", probab(it -> second));
            file << "[" << (it -> first) << "] => [" << buffer;
        }
    }
    else if (hftOption(output_fmt) == "json")
    {
        file << "{\n";
        for (auto it = predistr.begin(); it != predistr.end(); ++it)
        {
            if (it != predistr.begin())
            {
                file << ",\n";
            }

            sprintf(buffer, "%0.6f", probab(it -> second));
            file << "\t\"" << (it -> first) << "\" : " << buffer;
        }
        file << "\n}\n";
    }
}

int hft_instrument_variability_distribution_main(int argc, char *argv[])
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

    //
    // Setup logging.
    //

    el::Logger *logger = el::Loggers::getLogger("instr_distrib", true);

    //
    // Parsing options for hftr generator.
    //

    prog_opts::options_description hidden("Hidden options");
    hidden.add_options()
        ("instrument-distrib", "")
    ;

    prog_opts::options_description desc("Options for instrument variability distribution generator");
    desc.add_options()
        ("help,h", "produce help message")
        ("source,s", prog_opts::value<std::string>(&hftOption(source) ), "directory with ducascopy's .csv files or single .csv file name")
        ("instrument,i", prog_opts::value<std::string>(&hftOption(instrument)), "Instrument associated to CSV file")
        ("granularity,g", prog_opts::value<int>(&hftOption(granularity)) -> default_value(1), "ticks distance, default value is 1")
        ("output-fmt,o", prog_opts::value<std::string>(&hftOption(output_fmt)) -> default_value("simple"), "Output distribution format. "
                                                                              "Available formats: „simple” or „json”. Default is „simple”")
        ("output-file-name,f", prog_opts::value<std::string>(&hftOption(output_file)) -> default_value(""), "File name for save generated distribution."
                                                                              " If unspecified, distribution is printed to STDOUT.")
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

    hftOption(itype) = hft::instrument2type(hftOption(instrument));

    if (hftOption(itype) == hft::UNRECOGNIZED_INSTRUMENT)
    {
        hft_log(ERROR) << "Unsupported instrument ‘" << hftOption(instrument) << "’";

        return 1;
    }

    if (hftOption(output_fmt) != "simple" &&
            hftOption(output_fmt) != "json")
    {
        hft_log(ERROR) << "Unsupported output format „"
                       << hftOption(output_fmt)
                       << "”";

        return 1;
    }

    fs::path path(hftOption(source));

    try
    {
        if (fs::exists(path))
        {
            if (fs::is_regular_file(path))
            {
                if (path.extension().generic_string() == ".csv")
                {
                    hft_log(INFO) << "Proceeding file " << path;
                    proceed_file(path.generic_string(), hftOption(granularity));
                }
                else
                {
                    hft_log(INFO) << "Skipping " << path << " (not a .csv file)";
                }
            }
            else if (fs::is_directory(path))
            {
                hft_log(INFO) << path << " is a directory containing:\n";

                int fno = 1;

                for (fs::directory_entry &x : fs::directory_iterator(path))
                {
                    if (x.path().extension().generic_string() == ".csv")
                    {
                        std::string file_name = x.path().generic_string();
                        hft_log(INFO) << "Proceedeing (file #" << fno++
                                      << ") " << file_name;
                        proceed_file(file_name, hftOption(granularity));
                      }
                     else
                     {
                        hft_log(INFO) << "Skipping " << x.path() << " (not a .csv file)";
                     }
                }
            }
            else
            {
                hft_log(ERROR) << path << " exists, but is not a regular file or directory";
                return 1;
            }
        }
        else
        {
            hft_log(ERROR) << path << " No such file or directory";
            return 1;
        }
    }
    catch (const fs::filesystem_error &ex)
    {
        hft_log(ERROR) << ex.what();
        return 255;
    }

    //
    // Generating distribution.
    //

    if (hftOption(output_file) == "")
    {
        // File name unspecified, flush result to stdout;
        print_distribution(std::cout);
    }
    else
    {
        // File specifed, open for writing.
        std::fstream fs;

        fs.open(hftOption(output_file), std::fstream::out);
        if (! fs.is_open())
        {
            hft_log(ERROR) << "Unable to create file „"
                           << hftOption(output_file) << "”";

            return 1;
        }

        print_distribution(fs);
    }

    hft_log(INFO) << "*** Done";

    return 0;
}
