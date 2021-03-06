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
#include <sstream>
#include <vector>
#include <cmath>
#include <thread>

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

#include <easylogging++.h>

#include <hci.hpp>

namespace prog_opts = boost::program_options;

typedef std::vector<double> container;

static struct hft_hci_tuner_options_type
{
    std::string data_file_name;
    std::string csv_file_name;
    std::string extremize;
    unsigned int min_csize;
    unsigned int max_csize;
    int threshold_min;
    int threshold_max;
    bool reversed_hci;
    bool json_out;
    int ncores;

} hft_hci_tuner_options;

struct worker_data
{
    worker_data(int a, int b, const container &d)
        : min(a), max(b), best_csize(0), best_tsf(10e10),
          best_st(0.0), best_it(0.0), max_profit(-10e10),
          data(d) {}

    int min;
    int max;

    int    best_csize;
    double best_tsf; // The smaller, the better.
    double best_st;
    double best_it;

    double max_profit;
    const container &data;

    std::shared_ptr<std::thread> thread;
};

typedef std::vector<worker_data> workers;
typedef void (*thread_function_type)(void *);

//
// Forward declarations.
//

static double get_overall_profit(const container &data, size_t hci_capacity,
                                     double hci_st, double hci_it);

static double get_tsf(const container &data, double a,
                          size_t hci_capacity, double hci_st, double hci_it);



#define hftOption(__X__) \
    hft_hci_tuner_options.__X__

#define hft_log(__X__) \
    CLOG(__X__, "hci_tuner")

#define reversed_hci_coeff (hft_hci_tuner_options.reversed_hci ? -1.0 : 1.0)

static void thread_funct_best_profit(void *worker_data_info)
{
    worker_data *self = (worker_data *)(worker_data_info);

    double profit;

    for (unsigned int csize = self -> min; csize <= self -> max; csize++)
    {
        for (int st = hftOption(threshold_min); st < hftOption(threshold_max) - 1; st++)
        {
            for (int it = st + 1; it < hftOption(threshold_max); it++)
            {
                profit = get_overall_profit(self -> data, csize, st, it);

                if (profit > (self -> max_profit))
                {
                    self -> max_profit = profit;

                    self -> best_csize = csize;
                    self -> best_st = st;
                    self -> best_it = it;
                }
            }
        }
    }
}

static void thread_funct_least_profit(void *worker_data_info)
{
    worker_data *self = (worker_data *)(worker_data_info);

    double profit;
    self -> max_profit *= -1.0; // Big positive number now.

    for (unsigned int csize = self -> min; csize <= self -> max; csize++)
    {
        for (int st = hftOption(threshold_min); st < hftOption(threshold_max) - 1; st++)
        {
            for (int it = st + 1; it < hftOption(threshold_max); it++)
            {
                profit = get_overall_profit(self -> data, csize, st, it);

                if (profit < (self -> max_profit))
                {
                    self -> max_profit = profit;

                    self -> best_csize = csize;
                    self -> best_st = st;
                    self -> best_it = it;
                }
            }
        }
    }
}

static void thread_funct_tsf(void *worker_data_info)
{
    worker_data *self = (worker_data *)(worker_data_info);

    double tsf;
    double a, profit;

    for (unsigned int csize = self -> min; csize <= self -> max; csize++)
    {
        for (int st = hftOption(threshold_min); st < hftOption(threshold_max) - 1; st++)
        {
            for (int it = st + 1; it < hftOption(threshold_max); it++)
            {
                profit = get_overall_profit(self -> data, csize, st, it);

                if (profit > 0.0)
                {
                    a = profit / static_cast<double>(self -> data.size());
                    tsf = get_tsf(self -> data, a, csize, st, it);

                    if (tsf < (self -> best_tsf))
                    {
                        self -> max_profit = profit;
                        self -> best_tsf = tsf;

                        self -> best_csize = csize;
                        self -> best_st = st;
                        self -> best_it = it;
                    }
                }
            }
        }
    }
}

static void load_data(container &data)
{
    data.clear();

    std::ifstream is(hftOption(data_file_name), std::ifstream::binary);
    std::string file_content;

    if (is)
    {
        is.seekg(0, is.end);
        int length = is.tellg();
        is.seekg(0, is.beg);

        file_content.resize(length, '\0');

        is.read(&file_content.front(), length);

        if (! is)
        {
            is.close();

            std::string msg = std::string("Error while reading file: ")
                              + hftOption(data_file_name);

            throw std::runtime_error(msg.c_str());
        }

        is.close();
    }
    else
    {
        std::string msg = std::string("Unable to open file for read: ")
                          + hftOption(data_file_name);

        throw std::runtime_error(msg.c_str()); 
    }

    boost::char_separator<char> sep(" \t\n\r");
    boost::tokenizer<boost::char_separator<char> > tokens(file_content, sep);

    for (auto token : tokens)
    {
        try
        {
            data.push_back(boost::lexical_cast<double>(token));
        }
        catch (const boost::bad_lexical_cast &e)
        {
            std::cout << token.length() << "\nDlugosc liczona w C: " << strlen(token.c_str()) << "\n";
            throw std::runtime_error(std::string("Given token is not a number: [")
                                     + token + std::string("]\n\n"));
        }
    }
}

static double get_overall_profit(const container &data, size_t hci_capacity,
                                     double hci_st, double hci_it)
{
    double profit = 0.0;
    double pips_yield;
    const decision_type network_virtual_decision = DECISION_LONG;

    hci regulator(hci_capacity, hci_st, hci_it);

    for (auto d : data)
    {
        if (regulator.decision(network_virtual_decision) == network_virtual_decision)
        {
            pips_yield = d;
        }
        else
        {
            pips_yield = -1.0 * d;
        }

        regulator.insert_pips_yield(pips_yield);

        profit += reversed_hci_coeff * pips_yield;
    }

    return profit;
}

static double get_tsf(const container &data, double a,
                          size_t hci_capacity, double hci_st, double hci_it)
{
    int    xi = 0;
    double yi = 0.0;
    double pips_yield;
    const  decision_type network_virtual_decision = DECISION_LONG;

    double b1 = 0;
    double b2 = 0;

    hci regulator(hci_capacity, hci_st, hci_it);

    for (auto d : data)
    {
        if (regulator.decision(network_virtual_decision) == network_virtual_decision)
        {
            pips_yield = d;
        }
        else
        {
            pips_yield = -1.0 * d;
        }

        regulator.insert_pips_yield(pips_yield);

        xi++;
        yi += reversed_hci_coeff * pips_yield;

        if (yi - a*xi > b1)
        {
            b1 = yi - a*xi;
        }

        if (yi - a*xi < b2)
        {
            b2 = yi - a*xi;
        }
    }

    return (b1-b2);
}

static std::string get_csv(const container &data, size_t hci_capacity,
                               double hci_st, double hci_it)
{
    std::ostringstream csv;
    double profit = 0.0;
    double non_hci_profit = 0.0;
    double pips_yield;
    int n = 0;
    const decision_type network_virtual_decision = DECISION_LONG;

    hci regulator(hci_capacity, hci_st, hci_it);
    //regulator.debug(true);

    for (auto d : data)
    {
        if (regulator.decision(network_virtual_decision) == network_virtual_decision)
        {
            pips_yield = d;
        }
        else
        {
            pips_yield = -1.0 * d;
        }

        regulator.insert_pips_yield(pips_yield);

        profit += reversed_hci_coeff * pips_yield;
        non_hci_profit += d;

        csv << (++n) << ';' << non_hci_profit << ';' << profit << "\n";
    }

    return csv.str();
}

static std::string hci_state(const container &data, size_t hci_capacity,
                                 double hci_st, double hci_it)
{
    double pips_yield;
    const decision_type network_virtual_decision = DECISION_LONG;

    hci regulator(hci_capacity, hci_st, hci_it);

    for (auto d : data)
    {
        if (regulator.decision(network_virtual_decision) == network_virtual_decision)
        {
            pips_yield = d;
        }
        else
        {
            pips_yield = -1.0 * d;
        }

        regulator.insert_pips_yield(pips_yield);
    }

    return regulator.get_state_str();
}

int hft_hci_tuner_main(int argc, char *argv[])
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

    prog_opts::options_description desc("Options for HCI tuner");
    desc.add_options()
        ("help,h", "produce help message")
        ("input-file,i", prog_opts::value<std::string>(&hftOption(data_file_name)), "data input file name with trade results in pips; each data must be white space delimited")
        ("output-csv,c", prog_opts::value<std::string>(&hftOption(csv_file_name)) -> default_value("none"), "CSV output file name with trade simulation")
        ("min-csize,m", prog_opts::value<unsigned int>(&hftOption(min_csize)) -> default_value(3), "minimal regulator capacity")
        ("max-csize,M", prog_opts::value<unsigned int>(&hftOption(max_csize)) -> default_value(23), "maximal regulator capacity")
        ("threshold-b-range,t", prog_opts::value<int>(&hftOption(threshold_min)) -> default_value(-100), "regulator threshold bottom range")
        ("threshold-u-range,T", prog_opts::value<int>(&hftOption(threshold_max)) -> default_value(+100), "regulator threshold upper range")
        ("extremize,e", prog_opts::value<std::string>(&hftOption(extremize)) -> default_value("best-profit"), "Indicator to optimize; available are ‘best-profit’, ‘least-profit’, ‘tsf’")
        ("reversed-hci,r", prog_opts::value<bool>(&hftOption(reversed_hci)) -> default_value(false), "Use reversed decision from HCI regulator")
        ("json-out,j", prog_opts::value<bool>(&hftOption(json_out)) -> default_value(false), "Output results in JSON format. Useful for parsing purposes by extermal program")
        ("threads,u", prog_opts::value<int>(&hftOption(ncores)) -> default_value(std::thread::hardware_concurrency()), "Define number of threads")
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

    //
    // Setup logging.
    //

    el::Logger *logger = el::Loggers::getLogger("hci_tuner", true);

    if (hftOption(threshold_min) >= hftOption(threshold_max))
    {
        hft_log(ERROR) << "HCI threshold bottom range (defined as "
                       << hftOption(threshold_min) << ") cannot be equal "
                       << "or exceed threshold upper range (defined as "
                       << hftOption(threshold_max) << ").";

        return 1;
    }

    container data;
    load_data(data);

    thread_function_type thread_function = nullptr;
    int best_worker_result_index = 0;

    int    best_csize = 0;
    double best_st = 0.0;
    double best_it = 0.0;

    double max_profit = -10e10;

    //
    // Compute range for operations per each thread.
    //

    workers thread_workers;

    double offset = static_cast<double>(hftOption(max_csize) - hftOption(min_csize)) / (hftOption(ncores) ? hftOption(ncores) : 1);
    double upper_limit = static_cast<double>(hftOption(min_csize)) + offset > hftOption(max_csize) ? hftOption(max_csize) : static_cast<double>(hftOption(min_csize)) + offset;
    worker_data r(hftOption(min_csize), static_cast<int>(upper_limit), data);
    thread_workers.push_back(r);

    while (upper_limit < hftOption(max_csize))
    {
        upper_limit += offset;

        if (upper_limit > hftOption(max_csize))
        {
            upper_limit = hftOption(max_csize);
        }

        r.max = round(upper_limit);
        r.min = thread_workers.at(thread_workers.size() - 1).max + 1;

        if (r.max >= r.min)
        {
            thread_workers.push_back(r);
        }
    }

    //
    // Select thread routine.
    //

    if (hftOption(extremize) == "best-profit")
    {
        thread_function = thread_funct_best_profit;
    }
    else if (hftOption(extremize) == "least-profit")
    {
        thread_function = thread_funct_least_profit;
    }
    else if (hftOption(extremize) == "tsf")
    {
        thread_function = thread_funct_tsf;
    }
    else
    {
        hft_log(ERROR) << "Unrecognized indicator ‘"
                       << hftOption(extremize) << "’";

        return 1;
    }

    //
    // Start threads and wait until complete.
    //

    for (auto &x : thread_workers)
    {
        x.thread.reset(new std::thread(thread_function, &x));
    }

    if (! hftOption(json_out))
    {
        hft_log(INFO) << "Proceeding HCI tuning, patience...";
    }

    for (auto &x : thread_workers)
    {
        x.thread -> join();
    }

    //
    // Select thread that has the best result.
    //

    if (hftOption(extremize) == "best-profit")
    {
        for (int i = 0; i < thread_workers.size(); i++)
        {
            if (thread_workers.at(i).max_profit > thread_workers.at(best_worker_result_index).max_profit)
            {
                best_worker_result_index = i;
            }
        }
    }
    else if (hftOption(extremize) == "least-profit")
    {
        for (int i = 0; i < thread_workers.size(); i++)
        {
            if (thread_workers.at(i).max_profit < thread_workers.at(best_worker_result_index).max_profit)
            {
                best_worker_result_index = i;
            }
        }
    }
    else if (hftOption(extremize) == "tsf")
    {
        for (int i = 0; i < thread_workers.size(); i++)
        {
            if (thread_workers.at(i).best_tsf < thread_workers.at(best_worker_result_index).best_tsf)
            {
                best_worker_result_index = i;
            }
        }
    }
    else
    {
        hft_log(ERROR) << "Unrecognized indicator ‘"
                       << hftOption(extremize) << "’";

        return 1;
    }

    best_csize = thread_workers.at(best_worker_result_index).best_csize;
    best_st    = thread_workers.at(best_worker_result_index).best_st;
    best_it    = thread_workers.at(best_worker_result_index).best_it;
    max_profit = thread_workers.at(best_worker_result_index).max_profit;

    if (hftOption(csv_file_name) != "none")
    {
        if (! hftOption(json_out))
        {
            hft_log(INFO) << "Saving trade simulation based on tuned HCI into file ["
                          << hftOption(csv_file_name) << "].";
        }

        std::fstream csv;
        csv.open(hftOption(csv_file_name), std::fstream::out);

        if (csv.fail())
        {
            hft_log(ERROR) << "Failed to create file ["
                           << hftOption(csv_file_name)
                           << "].";
        }
        else
        {
            csv << get_csv(data, best_csize, best_st, best_it);
        }
    }

    if (hftOption(json_out))
    {
        std::cout << "{"
                  << "\"probed\" : " << data.size() << ", "
                  << "\"profit\" : " << max_profit << ", "
                  << "\"pips_per_trade\" : " << (max_profit / data.size()) << ", "
                  << "\"hci_state\" : \"" << hci_state(data, best_csize, best_st, best_it) << "\","
                  << "\"hysteresis_compensate_inverter\" : {"
                  << "\"capacity\" : " << best_csize << ", "
                  << "\"s_threshold\" : " << best_st << ", "
                  << "\"i_threshold\" : " << best_it << ""
                  << "}"
                  << "}\n";
    }
    else
    {
        hft_log(INFO) << "Completed.";
        hft_log(INFO) << "Probed: " << data.size();
        hft_log(INFO) << "Profit: " << max_profit << " pips, ie. "
                      << (max_profit / data.size()) << " pips per trade.";
        hft_log(INFO) << "HCI state: \"" << hci_state(data, best_csize, best_st, best_it) << "\"\n";
        hft_log(INFO) << "Tuned HCI settings:\n\n"
                      << "    \"hysteresis_compensate_inverter\" : {\n"
                      << "        \"capacity\" : " << best_csize << ",\n"
                      << "        \"s_threshold\" : " << best_st << ",\n"
                      << "        \"i_threshold\" : " << best_it << "\n"
                      << "    }";
    }

    return 0;
}
