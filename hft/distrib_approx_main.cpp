/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System  ≣≡=-              **
**                              b                                     **
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

#include <cmath>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>

#include <easylogging++.h>
#include <cJSON/cJSON.h>

namespace prog_opts = boost::program_options;

//
// Global tool config structure.
//

static struct hft_distribution_approximation_config_type
{
    std::string  distribution_file_name;
    unsigned int startn;
    unsigned int increment;
    double       precision;
    std::string  output_fmt;
    std::string  output_file;

} hft_distribution_approximation_config;

struct preapprox_record
{
    preapprox_record(unsigned int _x, unsigned int _start_index, unsigned int _end_index)
        : x(_x), start_index(_start_index), end_index(_end_index) {}

    unsigned int x;
    unsigned int start_index;
    unsigned int end_index;
};

#define hft_log(__X__) \
    CLOG(__X__, "binomial_distrib_approx")

#define hftOption(__X__) \
    hft_distribution_approximation_config.__X__

static std::map<unsigned int, double> distribution;
static std::vector<preapprox_record>  preaprox;

static double binomial(unsigned int N, unsigned int k)
{
    double S1, S2, S3;
    S1 = S2 = S3 = 0.0;
    for (unsigned int i = 1; i <= k; i++) {
        S1 += log(i);
    }

    for (unsigned int i = 1; i <= (N-k); i++) {
        S2 += log(i);
    }

    for (unsigned int i = 1; i <= N; i++) {
        S3 += log(i);
    }

    return exp(S3 - S2 - S1 - N*log(2));
}

static void load_distribution_from_json(const std::string &file_name)
{
    std::string line, json_str;
    std::ifstream json_file(file_name.c_str());
    std::map<unsigned int, double> tmp_distribution;
    const std::string eol = "\n";

    //
    // Load JSON data from file.
    //

    if (! json_file.is_open())
    {
        throw std::runtime_error(std::string("Unable to open file for read : ") + file_name);
    }

    while (std::getline(json_file, line))
    {
        json_str += (line + eol);
    }

    //
    // Parse JSON.
    //

    cJSON *json = cJSON_Parse(json_str.c_str());

    if (json == NULL)
    {
        std::string err_msg("JSON parse error ");

        const char *json_emsg = cJSON_GetErrorPtr();

        if (json_emsg != NULL)
        {
            err_msg += std::string(json_emsg);
        }

        cJSON_Delete(json);

        throw std::runtime_error(err_msg);
    }

    cJSON *element = NULL;
    unsigned int index = 0;

    for (int i = 0; i < cJSON_GetArraySize(json); i++)
    {
        element = cJSON_GetArrayItem(json, i);

        index = boost::lexical_cast<unsigned int>(element -> string);

        if (index == 0)
        {
            throw std::runtime_error("Index in distribution cannot be zero.");
        }

        tmp_distribution[index] = element -> valuedouble;
    }

    cJSON_Delete(json);

    //
    // Check distribution consistency.
    //

    std::map<unsigned int, double>::iterator it1, it2;

    unsigned int prev_key = 0;
    for (it1 = tmp_distribution.begin(); it1 != tmp_distribution.end(); it1++)
    {
        if (it1 -> first != prev_key + 1)
        {
            break;
        }

        prev_key = it1 -> first;
    }

    double prev_value = 1.0;
    for (it2 = tmp_distribution.begin(); it2 != it1; it2++)
    {
        if (it2 -> second > prev_value)
        {
            break;
        }

        prev_value = it2 -> second;
    }

    distribution = std::map<unsigned int, double>(tmp_distribution.begin(), it2);
}

static bool approx(unsigned int n, unsigned int start_index,
                       unsigned int &end_index, double goal)
{
    double s = 0.0;

    for (unsigned int i = start_index; i <= n; i++)
    {
        s += binomial(n, i);

        if (fabs(s - goal) < hftOption(precision))
        {
            end_index = i;
            std::cout << "Success: indexes from [" << start_index << ".." << end_index << "], result is ";
            printf("%0.6f goal was %0.6f\n", s, goal);

            return true;
        }

        if (s > goal)
        {
            return false;
        }
    }

    return false;
}

static unsigned int logarithmic_incrementation(unsigned int n)
{
    //
    // FIXME: Not implemented.
    //

    return 2;
}

static std::string compute_average_amount(unsigned int n,
                                              unsigned int minimum,
                                                  unsigned int maximum)
{
    char buffer[40];
    double sw = 0.0;
    double w = 0.0;
    double numerator = 0.0;

    if (minimum == maximum)
    {
        return boost::lexical_cast<std::string>(minimum);
    }

    for (unsigned int i = minimum; i <= maximum; i++)
    {
        w = binomial(n, i);
        sw += w;
        numerator += i*w;
    }

    sprintf(buffer, "%0.6f", (numerator/sw));

    return std::string(buffer);
}

static void print_approximation(std::ostream &file, unsigned int n)
{
    if (hftOption(output_fmt) == "simple")
    {
        file << "[n] => " << n << "\n";

        for (auto it = preaprox.begin(); it != preaprox.end(); it++)
        {
            file << "[" << (it -> x) << "] => "
                 << compute_average_amount(n, it -> start_index, it -> end_index)
                 << "\n";
        }
    }
    else if (hftOption(output_fmt) == "json")
    {
        //
        // Example produced json:
        //
        // {
        //     "n" : 1010,
        //     "distribution" :
        //     {
        //         "1" : 501.3,
        //         "2" : 476.4,
        //         "3" : 321.1
        //     }
        // }

        file << "{\n";
        file << "\t\"n\" : " << n << ",\n";
        file << "\t\"distribution\" :\n";
        file << "\t{\n";

        for (auto it = preaprox.begin(); it != preaprox.end(); it++)
        {
            if (it != preaprox.begin())
            {
                file << ",\n";
            }

            file << "\t\t\"" << (it -> x) << "\" : "
                 << compute_average_amount(n, it -> start_index, it -> end_index);
        }

        file << "\n\t}\n}\n";
    }
}

int hft_distribution_approximation_generator_main(int argc, char *argv[])
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

    el::Logger *logger = el::Loggers::getLogger("binomial_distrib_approx", true);

    //
    // Parsing options for hftr generator.
    //

    prog_opts::options_description hidden("Hidden options");
    hidden.add_options()
        ("distrib-approx-generator", "")
    ;

    prog_opts::options_description desc("Options for binomial distribution approximation generator");
    desc.add_options()
        ("help,h", "produce help message")
        ("distribution-file,d", prog_opts::value<std::string>(&hftOption(distribution_file_name) ), "distribution input file name in JSON format")
        ("start-n,n", prog_opts::value<unsigned int>(&hftOption(startn)) -> default_value(1000), "Begin N value, default is 1000")
        ("increment,i", prog_opts::value<unsigned int>(&hftOption(increment)) -> default_value(0), "Delta-increment, default is logarithmic autoincrement")
        ("precision,p", prog_opts::value<double>(&hftOption(precision)) -> default_value(0.0003), "Precision of the approximation. Default is 0.0003")
        ("output-fmt,o", prog_opts::value<std::string>(&hftOption(output_fmt)) -> default_value("simple"), "Output approximation format. "
                                                                              "Available formats: „simple” or „json”. Default is „simple”")
        ("approx-file-name,a", prog_opts::value<std::string>(&hftOption(output_file)) -> default_value(""), "File name for save generated distribution approximation."
                                                                              " If unspecified, distribution approximation is printed to STDOUT.")
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

    if (hftOption(output_fmt) != "simple" &&
            hftOption(output_fmt) != "json")
    {
        hft_log(ERROR) << "Unsupported output format „"
                       << hftOption(output_fmt)
                       << "”";

        return 1;
    }

    if (hftOption(distribution_file_name) == "")
    {
        hft_log(ERROR) << "No distribution input file specified";

        return 1;
    }

    if (hftOption(increment) % 2 == 1)
    {
        hftOption(increment)++;
    }

    //
    // Start procedure.
    //

    load_distribution_from_json(hftOption(distribution_file_name));

    //
    // Generating approximation.
    //

    unsigned int n = ((hftOption(startn) % 2) == 0 ? hftOption(startn) : hftOption(startn) + 1);
    unsigned int start_index = n / 2;
    unsigned int end_index;
    int maximum = distribution.size();
    bool completed = false;

    while (true)
    {
        hft_log(INFO) << "-------- New trial for N = "
                      << n << " --------------";

        for (int i = 1; i <= maximum; i++)
        {
            std::cout << "[" << i << "]: ";
            if (approx(n, start_index, end_index, distribution[i]) == false)
            {
                std::cout << "\n";
                break;
            }

            preaprox.push_back(preapprox_record(i, start_index, end_index));

            start_index = end_index + 1;

            if (i == maximum)
            {
                completed = true;
                break;
            }
        }

        if (completed)
        {
            break;
        }

        n += hftOption(increment) == 0 ? logarithmic_incrementation(n) : hftOption(increment);
        start_index = n / 2;
        preaprox.clear();
    }

    //
    // Generating approximation.
    //

    if (hftOption(output_file) == "")
    {
        // File name unspecified, flush result to stdout;
        print_approximation(std::cout, n);
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

        print_approximation(fs, n);
    }

    hft_log(INFO) << "*** Done";

    return 0;
}
