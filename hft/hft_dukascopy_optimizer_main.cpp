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

#include <vector>

#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#include <easylogging++.h>

namespace prog_opts = boost::program_options;

typedef std::vector<double> trade_yield_container;

static struct hft_dukascopy_optimizer_options_type
{
    std::string filename;
    double bankroll;
    double value_commission_1000;
    double pip_value_1000;
    double alpha_function;

} hft_dukascopy_optimizer_options;

#define hftOption(__X__) \
    hft_dukascopy_optimizer_options.__X__

#define hft_log(__X__) \
    CLOG(__X__, "dukascopy_optimizer")

static void process_line(trade_yield_container &ret, std::string &line)
{
    static boost::char_separator<char> sep(" \t");
    boost::tokenizer<boost::char_separator<char> > tokens(line, sep);

    for (auto token : tokens)
    {
        try
        {
            ret.push_back(boost::lexical_cast<double>(token));
        }
        catch (boost::bad_lexical_cast &e)
        {
            hft_log(ERROR) << "File item [" << token
                           << "] is not a valid floating point number in line ["
                           << line << "].";

            exit(1);
        }
    }
}

static void load_trade_yield_data(trade_yield_container &ret)
{
    ret.clear();

    std::string line;
    text_file_reader trade_yield_file(hftOption(filename));

    bool eof;

    do
    {
        eof = trade_yield_file.read_line(line);

        if (line.length() == 0)
        {
            continue;
        }

        process_line(ret, line);

    } while (eof);
}

static double overall_profit(const trade_yield_container &tyc, double alpha)
{
    double k = hftOption(bankroll);

    for (auto y : tyc)
    {
        k = k + y*alpha*k*hftOption(pip_value_1000) - 2.0*hftOption(value_commission_1000)*alpha*k;

        if (k < 0.0)
        {
            hft_log(ERROR) << "Bankruptcy";
            return k;
        }
    }

    return k;
}

int hft_dukascopy_optimizer_main(int argc, char *argv[])
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
    // Parsing options for dukascopy optimizer.
    //

    prog_opts::options_description desc("Options for dukascopy optimizer");
    desc.add_options()
        ("help,h", "produce help message")
        ("trade-yield-file,f", prog_opts::value<std::string>(&hftOption(filename)), "File with white space delimited of Dukascoopy trade pips yield")
        ("initial-bankroll,b", prog_opts::value<double>(&hftOption(bankroll)) -> default_value(10000.0), "Initial bankroll")
        ("value-commission-1000,c", prog_opts::value<double>(&hftOption(value_commission_1000)), "Value commision for amount 1000 currency units")
        ("pip-value-1000,p", prog_opts::value<double>(&hftOption(pip_value_1000)), "Pips limit for amount 1000 currency units")
        ("alpha-function,a", prog_opts::value<double>(&hftOption(alpha_function)) -> default_value(-1.0), "Alpha function parameter. If negative, parameter will be optimized")
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

    el::Logger *logger = el::Loggers::getLogger("dukascopy_optimizer", true);

    trade_yield_container trade_yields;
    load_trade_yield_data(trade_yields);

    if (hftOption(alpha_function) > 0.0)
    {
        //
        // Simple calculation.
        //

        hft_log(INFO) << "Bankroll after investition: " << overall_profit(trade_yields, hftOption(alpha_function)/1000.0);
    }
    else
    {
        //
        // Optimization.
        //

        hft_log(ERROR) << "FEATURE NOT IMPLEMENTED";
    }

    double s = 0;
    for (auto yield : trade_yields)
    {
       s += yield;
    }

    hft_log(INFO) << "Total pips yield: " << s << "\n";

    return 0;
}
