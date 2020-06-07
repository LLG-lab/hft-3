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

#include <basic_artifical_inteligence.hpp>

el::Logger *basic_artifical_inteligence::logger_ = nullptr;

#define hft_log(__X__) \
    CLOG(__X__, "basic_ai")

//
// Creates logger, standard collector and
// set „network_input_bus_loaded_” to false;
// Does not create neural network and does not
// initialize binomial approximator for collector.
// It is up to the user to call appropriate methods
// to achieve above.
// If this is a first instance, creates also
// thread workers for „fireup_neural_networks()”
// purposes.
//

basic_artifical_inteligence::basic_artifical_inteligence(void)
    : network_input_bus_loaded_(false),
      c2ib_gain_(0),
      granularity_(1),
      gc_(granularity_),
      option_never_hftr_export_(false),
      option_hft_multicore_(false),
      option_verbose_(true)
{
    //
    // Initialize logger.
    //

    if (basic_artifical_inteligence::logger_ == nullptr)
    {
        basic_artifical_inteligence::logger_ = el::Loggers::getLogger("basic_ai", true);
    }
}

basic_artifical_inteligence::~basic_artifical_inteligence(void)
{
    // Nothing to do so far.
}

//
// Extra options.
//

void basic_artifical_inteligence::set_opt(option_type opt_name, bool value)
{
    switch (opt_name)
    {
        case AI_OPTION_NEVER_HFTR_EXPORT:
            option_never_hftr_export_ = value;
            break;
        case AI_OPTION_HFT_MULTICORE:
            option_hft_multicore_ = value;
            break;
        case AI_OPTION_VERBOSE:
            option_verbose_ = value;
            break;
        default:
            throw exception("basic_artifical_inteligence: Invalid option");
    }
}

//
// Collector's approximation initialization.
//

void basic_artifical_inteligence::initialize_approximator_from_file(const std::string &file_name)
{
    collector().set_btf_approximator_from_file(file_name);
}

void basic_artifical_inteligence::initialize_approximator_from_buffer(const std::string &buffer)
{
    collector().set_btf_approximator_from_buffer(buffer);
}

//
// Dodanie sieci neuronowej do układu.
// Podawana jest architektura warstw ukrytych.
// Metoda zwraca numer ID dla nowododanej
// sieci neuronowej.
//

unsigned int basic_artifical_inteligence::add_neural_network(const neural_network::layers_architecture &la)
{
    std::shared_ptr<neural_network> net;

    neural_network::layers_architecture arch;
    arch.push_back(20);

    if (la.empty())
    {
        hft_log(WARNING) << "Requested to create neural network with no hidden layers.";
    }
    else for (auto x : la)
    {
        arch.push_back(x);
    }

    arch.push_back(1);

    net.reset(new neural_network(11*20, arch));

    //
    // Setup bipolar activate function
    // for all network layers.
    //

    for (int i = 0; i < arch.size(); i++)
    {
        net -> set_activate_function(i, neuron::activate_function_class::SIGMOID_BIPOLAR);
    }

    neural_networks_.push_back(net);

    return neural_networks_.size() - 1;
}

//
// Zwraca liczbę sieci neuronowych zaaplikowanych do systemu.
//

unsigned int basic_artifical_inteligence::get_neural_networks_number(void) const
{
    return neural_networks_.size();
}

void basic_artifical_inteligence::set_learn_coefficient(double lc)
{
    for (auto &net : neural_networks_)
    {
        net -> set_learn_coefficient(lc);
    }
}

void basic_artifical_inteligence::load_network(unsigned int nid, const std::string &file_name)
{
    neural_networks_.at(nid) -> load_network(file_name);
}

void basic_artifical_inteligence::save_network(unsigned int nid, const std::string &file_name)
{
    neural_networks_.at(nid) -> save_network(file_name);
}

double basic_artifical_inteligence::get_network_output(unsigned int nid)
{
    return neural_networks_.at(nid) -> get_output(0);
}

//
// Setup granularity.
//

void basic_artifical_inteligence::set_granularity(unsigned int granularity)
{
    granularity_ = granularity;
}

bool basic_artifical_inteligence::is_collector_ready(void) const
{
    int remain = collector().get_data_remain();

    if (remain == 0)
    {
        return true;
    }

    if (option_verbose_)
    {
        hft_log(INFO) << "Collector not ready, reading market mode. Remain ["
                      << remain << "].";
    }

    return false;
}

void basic_artifical_inteligence::load_input_bus(void)
{
    if (! is_collector_ready())
    {
        hft_log(ERROR) << "Unable to load input bus: insufficient data "
                          "in market collector.";

        throw exception("Bus error. Insufficient data in market collector");
    }

    int quantity, offset;
    double value;

    /*
    for (int i = 0; i < 20; i++)
    {
        quantity = basic_artifical_inteligence::index2quantity(i);

        for (int j = 0; j < 11; j++)
        {
            offset = basic_artifical_inteligence::index2offset(j);
            value  = collector().get_btfa(quantity, offset);

            for (auto &net : neural_networks_)
            {
                net -> set_pin(11*i+j, value);
            }

            if (! option_never_hftr_export_)
            {
                input_bus_copy_.set_pin(11*i+j, value);
            }
        }
    }
    */

    for (int i = 0; i < 220; i++)
    {
        quantity = i2q(i);

        value  = collector().get_btfa(quantity, 0);

        for (auto &net : neural_networks_)
        {
            net -> set_pin(i, value);
        }

        if (! option_never_hftr_export_)
        {
            input_bus_copy_.set_pin(i, value);
        }
    }

    network_input_bus_loaded_ = true;
}

void basic_artifical_inteligence::feed_data(unsigned int data)
{
    if (gc_.is_important_tick(data))
    {
        collector().feed_data(data);
        network_input_bus_loaded_ = false;
    }
    else
    {
        if (option_verbose_)
        {
            hft_log(INFO) << "Tick skipped due to granularity";
        }
    }
}

void basic_artifical_inteligence::fireup_networks(void)
{
    if (! network_input_bus_loaded_)
    {
        load_input_bus();
    }

    for (auto &net : neural_networks_)
    {
        net -> fire();
    }
}

void basic_artifical_inteligence::feedback(double expected)
{
    for (auto &net : neural_networks_)
    {
        net -> feedback(expected);
    }
}

void basic_artifical_inteligence::import_input_bus_from_hftr(const hftr &h)
{
    if (neural_networks_.empty())
    {
        hft_log(ERROR) << "Unable to load input bus: no neural network "
                          "applied to the system.";

        throw exception("Bus error. No neural network applied to the system");
    }

    for (auto &net : neural_networks_)
    {
        if (net -> get_number_of_inputs() != h.get_size())
        {
            hft_log(ERROR) << "Incompatibile HFTR. Size of HFTR (="
                           << h.get_size()
                           << ") not match with with size of neural "
                           << "network input bus (="
                           << net -> get_number_of_inputs()
                           << ").";

            throw exception("Bus incompatibility error");
        }

        net -> reset_input_bus();

        for (int i = 0; i < h.get_size(); i++)
        {
            net -> set_pin(i, h.get_pin(i));

            if (! option_never_hftr_export_)
            {
                input_bus_copy_.set_pin(i, h.get_pin(i));
            }
        }
    }

    network_input_bus_loaded_ = true;
}

hftr basic_artifical_inteligence::export_input_bus_to_hftr(void) const
{
    if (option_never_hftr_export_)
    {
        hft_log(ERROR) << "Exporting HFTR id forbidden because of option";

        throw exception("HFTR export not allowed");
    }

    return input_bus_copy_;
}

void basic_artifical_inteligence::create_collector(size_t csize)
{
    hft_log(INFO) << "Creating market collector of size ["
                  << csize << "].";

    pcollector_ = std::make_shared<market_data_collector>(csize+1);

    //                              1
    // c2ib_gain_ = (c_size - 1) • ———
    //                             219

    c2ib_gain_ = 0.00456621*(csize-1);
}

market_data_collector &basic_artifical_inteligence::collector(void) const
{
    if (pcollector_.use_count() == 0)
    {
        hft_log(ERROR) << "Market collector uninitialized.";

        throw exception("Market collector uninitialized");
    }

    return *pcollector_;
}

void basic_artifical_inteligence::enable_market_collector_cache(const std::string &cache_path)
{
    collector().enable_collector_cache(cache_path);
}
