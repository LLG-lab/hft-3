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
#include <memory>
#include <random>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>

#include <easylogging++.h>

#include <basic_artifical_inteligence.hpp>
#include <hft_utils.hpp>
#include <text_file_reader.hpp>
#include <train_stat.hpp>

namespace prog_opts = boost::program_options;
namespace fs = boost::filesystem;

static struct hft_ai_trainer_options_type
{
    std::string train_hftr_file_name;
    std::string validate_hftr_file_name;
    std::string neural_network_file_name;
    neural_network::layers_architecture arch;
    std::vector<std::pair<double, unsigned int>> train_suite;
    double train_break_condition;
    size_t collector_size;
    bool json_report;

} hft_ai_trainer_options;


#define hftOption(__X__) \
    hft_ai_trainer_options.__X__

#define hft_log(__X__) \
    CLOG(__X__, "ai_trainer")

int hft_trainer_main(int argc, char *argv[])
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
        ("ai-trainer", "")
    ;

    prog_opts::options_description desc("Options for hftr AI trainer");
    desc.add_options()
        ("help,h", "produce help message")
        ("train-hftr-file,t",  prog_opts::value<std::string>(&hftOption(train_hftr_file_name) ), "input HFTR file name for training")
        ("validate-hftr-file,u", prog_opts::value<std::string>(&hftOption(validate_hftr_file_name)) -> default_value("none"), "input HFTR file name for train validation. If not specified, no validation will be performed")
        ("network-file,f",     prog_opts::value<std::string>(&hftOption(neural_network_file_name )), "file for neural network, if not exist, will be created")
        ("train-break-if,b", prog_opts::value<double>(&hftOption(train_break_condition) ) -> default_value(-1.0), "Brak learning if absolute difference of pessimistic ratio indicators between validate sample and train is less than specified" )
        ("network-arch,a", prog_opts::value<std::string>() -> default_value("[7]"), "Network hidden layers architecture")
        ("collector-size,C", prog_opts::value<size_t>(&hftOption(collector_size) ) -> default_value(220), "Market collector size")
        ("train-suite,s",  prog_opts::value< std::vector<std::string> >(), "list of train settings in format <lc>:<en>, where <lc> is learn coefficient, <en> is epoch number")
        ("json-report,j", prog_opts::value<bool>(&hftOption(json_report)) -> default_value(false), "Wheather train statistics should be displayed in human readable format or JSON")
    ;

    prog_opts::options_description cmdline_options;
    cmdline_options.add(desc).add(hidden);

    prog_opts::positional_options_description p;
    p.add("train-suite", -1);

    prog_opts::variables_map vm;
    prog_opts::store(prog_opts::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
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

    el::Logger *logger = el::Loggers::getLogger("ai_trainer", true);

    //
    // Parse train suite informations.
    //

    if (! vm.count("train-suite"))
    {
        throw std::runtime_error("No train suite information specified");
    }

    boost::char_separator<char> sep(":");
    const std::vector<std::string> &ustp = vm["train-suite"].as<std::vector<std::string>>();

    for (auto &item : ustp)
    {
        std::pair<double, unsigned int> suite;
        boost::tokenizer<boost::char_separator<char> > tokens(item, sep);

        auto iterator = tokens.begin();

        if (iterator == tokens.end())
        {
            std::string err_msg = std::string("No learn coefficient specified in „")
                                  + item + std::string("”");

            throw std::runtime_error(err_msg);
        }

        suite.first = boost::lexical_cast<double>(*iterator);
        iterator++;

        if (iterator == tokens.end())
        {
            std::string err_msg = std::string("No epochs number specified in „")
                                  + item + std::string("”");

            throw std::runtime_error(err_msg);
        }

        suite.second = boost::lexical_cast<unsigned int>(*iterator);

        hft_log(INFO) << "Network(s) will be train [" << suite.second
                      << "] times with learn coefficient ["
                      << suite.first << "].";

        hftOption(train_suite).push_back(suite);
    }

    //
    // Stuff configuration and setup.
    //

    train_stat train_statistics;
    train_stat validate_statistics;
    std::shared_ptr<text_file_reader> train_hftr;
    std::shared_ptr<text_file_reader> validate_hftr;
    std::shared_ptr<basic_artifical_inteligence> bai;

    hft_log(INFO) << "Loading HFTR for training ["
                  << hftOption(train_hftr_file_name)
                  << "].";

    train_hftr.reset(new text_file_reader(hftOption(train_hftr_file_name)));

    if (hftOption(validate_hftr_file_name) != "none")
    {
        hft_log(INFO) << "Loading HFTR for validation ["
                      << hftOption(validate_hftr_file_name)
                      << "].";

        validate_hftr.reset(new text_file_reader(hftOption(validate_hftr_file_name)));
    }

    std::string line;
    double net_response, expected;
    hftr h;
    hftOption(arch) = neural_network::parse_layers_architecture_from_string(vm["network-arch"].as<std::string>());
    bai.reset(new basic_artifical_inteligence());
    bai -> create_collector(hftOption(collector_size));
    bai -> set_opt(basic_artifical_inteligence::AI_OPTION_NEVER_HFTR_EXPORT, true);
    bai -> set_opt(basic_artifical_inteligence::AI_OPTION_HFT_MULTICORE, false);
    bai -> set_opt(basic_artifical_inteligence::AI_OPTION_VERBOSE, false);
    unsigned int nid = bai -> add_neural_network(hftOption(arch));

    if (! fs::exists(hftOption(neural_network_file_name)))
    {
        hft_log(INFO) << "File name: [" << hftOption(neural_network_file_name)
                      << "] represents new network.";
    }
    else
    {
        hft_log(INFO) << "Loading neural network ["
                      << hftOption(neural_network_file_name)
                      << "].";

        bai -> load_network(nid, hftOption(neural_network_file_name ));
    }

    //
    // Proceed training.
    //

    for (auto &suite : hftOption(train_suite))
    {
        unsigned int epochs = suite.second;
        double learn_coefficient = suite.first;

        hft_log(INFO) << "Starting suite: epochs [" << epochs
                      << "], learn coefficient [" << learn_coefficient
                      << "].";

        bai -> set_learn_coefficient(learn_coefficient);

        for (unsigned int i = 0; i < epochs; i++)
        {
            hft_log(INFO) << "Starting epoch [" << i+1 << "] of [" << epochs
                          << "], learn coefficient [" << learn_coefficient
                          << "].";

            train_hftr -> rewind();

            hft_log(INFO) << "Now training...";

            while (train_hftr -> read_line(line))
            {
                h.clear();
                h.import_from_text_line(line);
                bai -> import_input_bus_from_hftr(h);
                bai -> fireup_networks();

                net_response = bai -> get_network_output(nid);

                switch (h.get_output())
                {
                    case hftr::INCREASE:
                        expected = 1.0;
                        break;
                    case hftr::DECREASE:
                        expected = -1.0;
                        break;
                    default:
                        throw std::runtime_error("Bad HFTR, unrecognized output");
                }

                bai -> feedback(expected);
                train_statistics.store(net_response, expected);
            }

            //
            // If validation available, proceed one.
            //

            if (hftOption(validate_hftr_file_name) != "none")
            {
                hft_log(INFO) << "Now verification...";

                validate_hftr-> rewind();

                while (validate_hftr -> read_line(line))
                {
                    h.clear();
                    h.import_from_text_line(line);
                    bai -> import_input_bus_from_hftr(h);
                    bai -> fireup_networks();

                    net_response = bai -> get_network_output(nid);

                    switch (h.get_output())
                    {
                        case hftr::INCREASE:
                            expected = 1.0;
                            break;
                        case hftr::DECREASE:
                            expected = -1.0;
                            break;
                        default:
                            throw std::runtime_error("Bad HFTR, unrecognized output");
                    }

                    validate_statistics.store(net_response, expected);
                }
            }
            else
            {
                hft_log(INFO) << "(verification skipped)";
            }

            //
            // After each epoch, we have to save neurons to the file
            // and display stats.
            //

            bai -> save_network(nid, hftOption(neural_network_file_name ));

            hft_log(INFO) << "Training statistics:\n"
                          << train_statistics.generate_report(hftOption(json_report) ? train_stat::report_type::JSON : train_stat::report_type::HUMAN_READABLE);


            if (hftOption(validate_hftr_file_name) != "none")
            {
                hft_log(INFO) << "Validation statistics:\n"
                              << validate_statistics.generate_report(hftOption(json_report) ? train_stat::report_type::JSON : train_stat::report_type::HUMAN_READABLE);

                double vpri = validate_statistics.get_pessimistic_ratio_indicator();
                double tpri = train_statistics.get_pessimistic_ratio_indicator();

                if (fabs(vpri - tpri) <= hftOption(train_break_condition))
                {
                    hft_log(WARNING) << "Breaking training because of option --train-break-if : "
                                     << fabs(vpri - tpri) << "≤" << hftOption(train_break_condition);

                    return 0;
                }

                validate_statistics.clear();
            }

            train_statistics.clear();
        }
    }

    return 0;
}
