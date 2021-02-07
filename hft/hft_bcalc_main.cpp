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
#include <cmath>

#include <boost/program_options.hpp>

#include <easylogging++.h>

namespace prog_opts = boost::program_options;

static struct _hft_bcalc_config
{
    double p;
    unsigned int n;
    unsigned int k;
    bool json_report;
} hft_bcalc_config;

#define hftOption(__X__) \
    hft_bcalc_config.__X__

#define hft_log(__X__) \
    CLOG(__X__, "hft_bcalc")

static double bernoulli(double p, unsigned int n, unsigned int k)
{
    if (k > n)
    {
        throw std::logic_error("k must be less than n");
    }

    double d = log(p)*k + log(1.0-p)*(n-k);
    double s1 = 0.0, s2 = 0.0;

    if (k > n - k)
    {
        for (unsigned int i = k + 1; i <= n; i++)
        {
            s1 += log(i);
        }

        for (unsigned int i = 1; i <= n - k; i++)
        {
            s2 += log(i);
        }
    }
    else
    {
        for (unsigned int i = n - k + 1; i <= n; i++)
        {
            s1 += log(i);
        }

        for (unsigned int i = 1; i <= k; i++)
        {
            s2 += log(i);
        }

    }

    return exp(s1-s2+d);
}

int hft_bcalc_main(int argc, char *argv[])
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
        ("hft-bcalc", "")
    ;

    prog_opts::options_description desc("Options for hft bcalc");
    desc.add_options()
        ("help,h", "produce help message")
        ("probability,p", prog_opts::value<double>(&hftOption(p) ), "correct probability predictiion for single predictor")
        ("predictors,n", prog_opts::value<unsigned int>(&hftOption(n) ), "total amount of predictors")
        ("amount,k", prog_opts::value<unsigned int>(&hftOption(k) ), "amount of predictors with equal predictions")
        ("json-report,j", prog_opts::value<bool>(&hftOption(json_report)) -> default_value(false), "Wheather report should be displayed in human readable format or JSON")
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

    el::Logger *logger = el::Loggers::getLogger("hft_bcalc", true);

    //
    // Config validation.
    //

    if (hftOption(p) < 0.0 || hftOption(p) > 1.0)
    {
        hft_log(ERROR) << "Invalid probability value";

        return 1;
    }

    if (hftOption(k) > hftOption(n))
    {
        hft_log(ERROR) << "Amount must be less or equal than predictors";

        return 1;
    }

    if (hftOption(k) < 0.5*hftOption(n))
    {
        hft_log(ERROR) << "Amount must be greater or equal than 0.5 * predictors";

        return 1;
    }

    double gp = 0.0;

    for (unsigned int i = hftOption(k); i <= hftOption(n); i++)
    {
        gp += bernoulli(hftOption(p), hftOption(n), i);
    }

    double pp = gp;

    for (unsigned int i = 0; i <= hftOption(n) - hftOption(k); i++)
    {
        gp += bernoulli(hftOption(p), hftOption(n), i);
    }

    std::cout << "Game probability: " << gp << "\n";
    std::cout << "Prediction probability: " << (pp/gp) << "\n";

    return 0;
}
