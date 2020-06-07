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

#include <iostream>
#include <easylogging++.h>

#include <exception>

INITIALIZE_EASYLOGGINGPP

typedef int (*program)(int, char **);

//
// HFT tools entry points.
//

extern int hftr_generator_main(int argc, char *argv[]);
extern int hft_trainer_main(int argc, char *argv[]);
extern int hftr_mixer_main(int argc, char *argv[]);
extern int hft_instrument_variability_distribution_main(int argc, char *argv[]);
extern int hft_distribution_approximation_generator_main(int argc, char *argv[]);
extern int hft_fxemulator_main(int argc, char *argv[]);
extern int hft_serial_analyzer_main(int argc, char *argv[]);
extern int hft_server_main(int argc, char *argv[]);
extern int hft_bcalc_main(int argc, char *argv[]);
extern int hft_hci_tuner_main(int argc, char *argv[]);

static struct
{
    const char *tool_name;
    program start_program;

} hft_programs[] = {
    { .tool_name = "hftr-generator",           .start_program = &hftr_generator_main },
    { .tool_name = "hftr-mixer",               .start_program = &hftr_mixer_main },
    { .tool_name = "ai-trainer",               .start_program = &hft_trainer_main },
    { .tool_name = "instrument-distrib",       .start_program = &hft_instrument_variability_distribution_main },
    { .tool_name = "distrib-approx-generator", .start_program = &hft_distribution_approximation_generator_main },
    { .tool_name = "serial-analyzer",          .start_program = &hft_serial_analyzer_main },
    { .tool_name = "fxemulator",               .start_program = &hft_fxemulator_main },
    { .tool_name = "server",                   .start_program = &hft_server_main },
    { .tool_name = "bcalc",                    .start_program = &hft_bcalc_main },
    { .tool_name = "hci-tuner",                .start_program = &hft_hci_tuner_main }
};

int main(int argc, char *argv[])
{
    try
    {
        std::cout << "High Frequency Trading System - Professional Expert Advisor\n";
        std::cout << " Copyright   2017 - 2020 by LLG Ryszard Gradowski, All Rights Reserved.\n\n";

        if (argc == 1 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
        {
            std::cout << "Usage:\n"
                      << "  hft <tool> [tool options]\n\n"
                      << "Available tools:\n"
                      << "  hftr-generator            generate hftr record file based on Dukascopy\n"
                      << "                            .csv history data\n\n"
                      << "  hftr-mixer                generates new HFTR file based on original, by\n"
                      << "                            randomly mixing records order\n\n"
                      << "  hci-tuner                 finds optimal settings for HCI regulator\n"
                      << "                            optimizing specified indicator\n\n"
                      << "  ai-trainer                proceed train AI predictor's neural networks\n\n"
                      << "  serial-analyzer           serial distribution analyzer for instrument\n\n"
                      << "  bcalc                     Bernouli Calculator - calculates correct\n"
                      << "                            prediction probability of set of defined amount\n"
                      << "                            predictors\n\n"
                      << "  instrument-distrib        obtain instrument variability distribution\n\n"
                      << "  distrib-approx-generator  generate binomial distribution approximation\n"
                      << "                            based on instrument variability distribution\n\n"
                      << "  fxemulator                HFT TCP Client emulates Dukascopy forex trading\n"
                      << "                            platform using dukascopy historical CSV data\n\n"
                      << "  server                    HFT Trading TCP Server. Expert Advisor for\n"
                      << "                            production and testing purposes\n\n";

            return 0;
        }

//        prog_opts::variables_map vm;
//        prog_opts::store(prog_opts::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
//        prog_opts::notify(vm);

        for (auto &p : hft_programs)
        {
            if (strcmp(p.tool_name, argv[1]) == 0)
            {
                return p.start_program(argc - 1, argv + 1);
            }
        }

        std::cerr << "Invalid tool. Type `hft --help´ for more informations\n\n";

        return 1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "fatal error: " << e.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "fatal error: Exception of unknown type!\n";
    }

    /* Fatal error exit code */

    return 255;
}
