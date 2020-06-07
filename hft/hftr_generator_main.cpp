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

#include <fstream>
#include <boost/program_options.hpp>
#include <easylogging++.h>

#include <csv_loader.hpp>
#include <basic_artifical_inteligence.hpp>
#include <hft_utils.hpp>

namespace prog_opts = boost::program_options;

static struct hftr_generator_options_type
{
    std::string csv_file_name;
    std::string hftr_file_name;
    std::string approximator_file;
    int offset;
    unsigned int granularity;
    double model_allowance_frac;
    unsigned int up_pips_limit;
    unsigned int down_pips_limit;
    size_t collector_size;

} hftr_generator_options;

static struct hftr_resolved_stats
{
    hftr_resolved_stats(void)
        : n(0), sum(0) {}

    unsigned n;
    unsigned sum;
} hrs;

#define hftOption(__X__) \
    hftr_generator_options.__X__

#define hft_log(__X__) \
    CLOG(__X__, "hftr_generator")

//
// Return value: -1 - drop of pips_limit
//                0 - unknown (end of file reached)
//                1 - increase of pips_limit
//

static hftr::output_type hftr_lookup_settelment(unsigned int ask, std::vector<csv_loader::csv_record> &buffer, int j)
{
    hftr::output_type rc = hftr::UNDEFINED;
    int ticks_num = 0;

    for (int i = j + 1; i < buffer.size(); i++)
    {
        ++ticks_num;

        int current_ask = hft::floating2dpips(boost::lexical_cast<std::string>(buffer.at(i).ask));

        if (current_ask  >= ask + 10*hftOption(up_pips_limit))
        {
            rc = hftr::INCREASE;
            break;
        }
        else if (current_ask <= ask - 10*hftOption(down_pips_limit))
        {
            rc = hftr::DECREASE;
            break;
        }
    }

    if (rc != hftr::UNDEFINED)
    {
        if (hftOption(model_allowance_frac) > 0.0 &&
                static_cast<double>(ticks_num) > hftOption(model_allowance_frac)*hftOption(granularity)*4030.0)
        {
            hft_log(INFO) << "Skipping hft record - resolution after ticks "
                          << ticks_num << " is beyond model.";

            return hftr::UNDEFINED;
        }

        hft_log(INFO) << "Resolved after ticks | " << ticks_num;

        hrs.n++;
        hrs.sum += ticks_num;
    }
    else
    {
        hft_log(INFO) << "Unknown settelment because of end of file";
    }

    return rc;
}

int hftr_generator_main(int argc, char *argv[])
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
    // Parsing options for hftr generator.
    //

    prog_opts::options_description hidden("Hidden options");
    hidden.add_options()
        ("hftr-generator", "")
    ;

    prog_opts::options_description desc("Options for hftr generator");
    desc.add_options()
        ("help,h", "produce help message")
        ("input-file,i",  prog_opts::value<std::string>(&hftOption(csv_file_name) ), "input CSV file of Dukascopy .csv history data")
        ("output-file,o", prog_opts::value<std::string>(&hftOption(hftr_file_name)), "output HFTR (LLG-standarized HFT record) file. If file exists, data will be appended")
        ("approximator,a", prog_opts::value<std::string>(&hftOption(approximator_file)), "Binomial approximator of distribution")
        ("up-pips-limit,u", prog_opts::value<unsigned int>(&hftOption(up_pips_limit)) -> default_value(10), "Pips number of rise, after position is closed, default 10")
        ("down-pips-limit,d", prog_opts::value<unsigned int>(&hftOption(down_pips_limit)) -> default_value(10), "Pips number of fall after position is closed, default 10")
        ("offset,O", prog_opts::value<int>(&hftOption(offset)) -> default_value(100), "HFTR sample offset (in ticks). Default value is 100")
        ("granularity,g",  prog_opts::value<unsigned int>(&hftOption(granularity)) -> default_value(1), "Ticks acquisition resolution. Default value is 1")
        ("collector-size,C", prog_opts::value<size_t>(&hftOption(collector_size) ) -> default_value(220), "Market collector size")
        ("model-frac,m", prog_opts::value<double>(&hftOption(model_allowance_frac)) -> default_value(-1), "Fraction of absolute size of market collector (collector_size*granularity). If settelment requires more ticks that this amount, hft record is dropped. Negative value of this parameter disables this feature")
    ;

    prog_opts::options_description cmdline_options;
    cmdline_options.add(desc).add(hidden);

    prog_opts::variables_map vm;
    prog_opts::store(prog_opts::parse_command_line(argc, argv, cmdline_options), vm);
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

    //
    // Setup logging.
    //

    el::Logger *logger = el::Loggers::getLogger("hftr_generator", true);

    hft_log(INFO) << "Input file CSV: [" << hftOption(csv_file_name) << "]";
    hft_log(INFO) << "HFTR output file: [" << hftOption(hftr_file_name) << "]";

    basic_artifical_inteligence bai;
    bai.create_collector(hftOption(collector_size));
    bai.set_opt(basic_artifical_inteligence::AI_OPTION_NEVER_HFTR_EXPORT, false);
    bai.set_opt(basic_artifical_inteligence::AI_OPTION_HFT_MULTICORE, false);
    bai.set_opt(basic_artifical_inteligence::AI_OPTION_VERBOSE, false);
    bai.initialize_approximator_from_file(hftOption(approximator_file));
    bai.set_granularity(hftOption(granularity));

    csv_loader csv(hftOption(csv_file_name));
    csv_loader::csv_record rec;
    std::vector<csv_loader::csv_record> rec_buffer;

    unsigned int last_ask = 0;
    unsigned int ask;
    unsigned int sample_offset = 0;
    unsigned int offset = 0;

    //
    // Opening output file.
    //

    std::fstream hftr_file;
    hftr_file.open(hftOption(hftr_file_name), std::fstream::out | std::fstream::app);

    if (hftr_file.fail())
    {
        std::string err_msg = std::string("Unable to open file ") + hftOption(hftr_file_name);

        throw std::runtime_error(err_msg);
    }

    hft_log(INFO) << "Loading CSV...";

    while (csv.get_record(rec))
    {
        rec_buffer.push_back(rec);
    }

    hft_log(INFO) << "Loading CSV completed.";

    for (int i = 0; i < rec_buffer.size(); ++i)
    {
        ask = hft::floating2dpips(boost::lexical_cast<std::string>(rec_buffer.at(i).ask));

        if (ask == last_ask)
        {
            //
            // No significant changed on the market (same ask price),
            // skipping tick.
            //

            continue;
        }

        last_ask = ask;
        bai.feed_data(ask);

        if (bai.is_collector_ready() && ! bai.is_valid_bus())
        {
            if (sample_offset++ == offset)
            {
                sample_offset = 0;
                unsigned int before_offset = hrs.sum;
                hftr::output_type settelment = hftr_lookup_settelment(ask, rec_buffer, i);
                unsigned int after_offset = hrs.sum;

                if (hftOption(offset) > 0)
                {
                    offset = hftOption(offset);
                }
                else
                {
                    offset = (after_offset - before_offset);
                }

                if (settelment != hftr::UNDEFINED)
                {
                    bai.load_input_bus();
                    hftr h = bai.export_input_bus_to_hftr();
                    h.set_output(settelment);

                    hftr_file << h.export_to_text_line() << "\n";
                }
            }
        }
    }

    //
    // Display statistics about resolved records.
    //

    if (hrs.n != 0)
    {
        hft_log(INFO) << "Total records ............ " << hrs.n;
        hft_log(INFO) << "Average resolved after ... "
                      << (hrs.sum / hrs.n) << " ticks";
    }

    //
    // Important to return value (exit code).
    //

    return 0;
}
