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
#include <hft_utils.hpp>
#include <csv_loader.hpp>

#include <vector>
#include <map>
#include <random>

#include <boost/program_options.hpp>

#include <easylogging++.h>

namespace prog_opts = boost::program_options;

typedef std::vector<int> serial_container;
typedef std::map<int, int> series_distrib;

static struct hft_serial_analyzer_options_type
{
    std::string list_filename;
    std::string serial_filename;
    unsigned int pips_limit;
} hft_serial_analyzer_options;

#define hftOption(__X__) \
    hft_serial_analyzer_options.__X__

#define hft_log(__X__) \
    CLOG(__X__, "serial_analyzer")

static serial_container experiment_series;
static unsigned int start_price = 0;

static void proceed_file(const std::string &csv_file_name)
{
    csv_loader csv(csv_file_name);
    csv_loader::csv_record rec;

    unsigned int current_price;

    while (csv.get_record(rec))
    {
        current_price = hft::floating2dpips(boost::lexical_cast<std::string>(rec.ask));

        if (start_price == 0)
        {
            start_price = current_price;
        }
        else
        {
            if (current_price  >= start_price + 10*hftOption(pips_limit))
            {
                experiment_series.push_back(1);
                start_price = current_price;
            }
            else if (current_price <= start_price - 10*hftOption(pips_limit))
            {
                experiment_series.push_back(0);
                start_price = current_price;
            }
        }
    }
}

static int serial(serial_container::const_iterator begin,
                      serial_container::const_iterator end)
{
    if (begin == end)
    {
        return 0;
    }

    int first_element = *begin;
    int n = 1;

    for (auto it = begin + 1; it != end; it++)
    {
        if (*it == first_element)
        {
            n++;
        }
        else
        {
            break;
        }
    }

    return n;
}

static series_distrib generate_series_distrib(const serial_container &data)
{
    series_distrib sd;
    int offset = 0;
    int n = 0;

    while (true)
    {
        n = serial(data.begin() + offset, data.end());

        if (n == 0)
        {
            break;
        }

        if (sd.find(n) == sd.end())
        {
            sd[n] = 1;
        }
        else
        {
            sd[n]++;
        }

        offset += n;
    }

    return sd;
}

static void display_series_distrib(const series_distrib &sd)
{
    int s = 0;

    for (auto it = sd.rbegin(); it != sd.rend(); it++)
    {
        s += it -> second;
        hft_log(INFO) << (it -> first) << ":\t" << (it -> second) << "\t" << (double(it -> second)/s);
    }
}

static serial_container generate_random_serial(int n)
{
    serial_container sc;

    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, 1); // distribution in range [0, 1]

    for (int i = 0; i < n; i++)
    {
        sc.push_back(dist(rng));
    }

    return sc;
}

int hft_serial_analyzer_main(int argc, char *argv[])
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
    // Parsing options for serial analyzer.
    //

    prog_opts::options_description desc("Options for serial analyzer");
    desc.add_options()
        ("help,h", "produce help message")
        ("list-file,l", prog_opts::value<std::string>(&hftOption(list_filename)), "File with list of Dukascoopy historical CSV file names")
        ("serial-file,s", prog_opts::value<std::string>(&hftOption(serial_filename)), "File name with serial data, like \"00101011\"")
        ("pips-limit,p", prog_opts::value<unsigned int>(&hftOption(pips_limit)) -> default_value(0), "Pips limit")
    ;

    prog_opts::options_description cmdline_options;
    cmdline_options.add(desc);

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

    el::Logger *logger = el::Loggers::getLogger("serial_analyzer", true);

    //
    // Validate options.
    //

    if (hftOption(list_filename).length() == 0 && hftOption(serial_filename).length() == 0)
    {
        hft_log(ERROR) << "Unspecified csv files list file name or serial file name";

        return 1;
    }

    if (hftOption(list_filename).length())
    {
        if (hftOption(pips_limit) == 0)
        {
            hft_log(ERROR) << "Unspecified pips limit";

            return 1;
        }

        hft_log(INFO) << "Pips limit [" << hftOption(pips_limit) << "]";
        hft_log(INFO) << "File name [" << hftOption(list_filename) << "]";

        std::string csv_file_name;
        text_file_reader csv_files_list(hftOption(list_filename));

        while (csv_files_list.read_line(csv_file_name))
        {
            if (csv_file_name.length() == 0)
            {
                continue;
            }

            hft_log(INFO) << "Now processing [" << csv_file_name << "]";

            proceed_file(csv_file_name);
        }
    }
    else
    {
        std::string data;
        text_file_reader series_file(hftOption(serial_filename));

        bool eof;

        do
        {
            eof = series_file.read_line(data);

            if (data.length() == 0)
            {
                continue;
            }

            for (auto b : data)
            {
                if (b == '1')
                {
                    experiment_series.push_back(1);
                }
                else if (b == '0')
                {
                    experiment_series.push_back(0);
                }
                else
                {
                    hft_log(WARNING) << "Skipping '" << b << "'";
                }
            }
        } while (eof);
    }

    hft_log(INFO) << "Experimental instrument serial distribution:";

    hft_log(INFO) << "Items number : " << (experiment_series.size());

    series_distrib experiment_distrib = generate_series_distrib(experiment_series);

    display_series_distrib(experiment_distrib);

    hft_log(INFO) << "Randomed serial distribution";

    display_series_distrib(generate_series_distrib(generate_random_serial(experiment_series.size())));

    return 0;
}
