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

#ifndef __BASIC_ARTIFICAL_INTELIGENCE_HPP__
#define __BASIC_ARTIFICAL_INTELIGENCE_HPP__

#include <memory>

#include <mdc.hpp>
#include <granularity_counter.hpp>
#include <neural_network.hpp>
#include <hftr.hpp>
#include <easylogging++.h>

//
// Following class implements very basic functionality
// integrating market collector of specified size with
// set of neural networks and import/export input bus
// to/from HFTR, and provides neural networks train
// functionality interface.
//

class basic_artifical_inteligence
{
public:

    DEFINE_CUSTOM_EXCEPTION_CLASS(exception, std::runtime_error)

    typedef enum
    {
        AI_OPTION_NEVER_HFTR_EXPORT = 1,
        AI_OPTION_HFT_MULTICORE,
        AI_OPTION_VERBOSE
    } option_type;

    //
    // Creates logger and sets ‘network_input_bus_loaded_’ to false;
    // Does not create market collector, neural networks and not
    // initialize aproximator for market collector.
    //

    basic_artifical_inteligence(void);

    virtual ~basic_artifical_inteligence(void);

    //
    // Extra options.
    //

    void set_opt(option_type opt_name, bool value);

    //
    // Inicjalizacja aproksymatora dla kolektora.
    //

    void initialize_approximator_from_file(const std::string &file_name);
    void initialize_approximator_from_buffer(const std::string &buffer);

    //
    // Creates market collector of specified size.
    //

    void create_collector(size_t csize);

    //
    // Dodanie sieci neuronowej do układu.
    // Podawana jest architektura warstw ukrytych.
    // Metoda zwraca numer ID dla nowododanej
    // sieci neuronowej.
    //

    unsigned int add_neural_network(const neural_network::layers_architecture &la);

    //
    // Zwraca liczbę sieci neuronowych zaaplikowanych do systemu.
    //

    unsigned int get_neural_networks_number(void) const;

    void set_learn_coefficient(double lc);

    void load_network(unsigned int nid, const std::string &file_name);

    void save_network(unsigned int nid, const std::string &file_name);

    double get_network_output(unsigned int nid);

    //
    // Setup granularity.
    //

    void set_granularity(unsigned int granularity);

    bool is_collector_ready(void) const;

    //
    // Do sprawdzania czy dodanie danych spowodowało
    // zmianę w kolektorze czy nie. Jesli magistrala
    // wejściowa jest ważna mimo dodania danych
    // to dane zostały dropnięte ze wzgledu na
    // granularity.
    //

    bool is_valid_bus(void) const { return network_input_bus_loaded_; }

    //
    // Ustawia wejścia wszystkich sieci neuronowych
    // wartościami obliczonymi na podstawie kolektora.
    // Metoda musi być publiczna z uwagi na fakt, iż
    // Chcemy ją jawnie wywołać na potrzeby genera-
    // tora hftr (nie interesuje nas wynik sieci
    // a jedynie jego wejscia).
    //

    void load_input_bus(void);

    void feed_data(unsigned int data);

    void fireup_networks(void);

    void feedback(double expected);

    void import_input_bus_from_hftr(const hftr &h);

    hftr export_input_bus_to_hftr(void) const;

protected:

    void enable_market_collector_cache(const std::string &cache_path);

    void invalidate_bus(void) { network_input_bus_loaded_ = false; }

private:

    typedef std::vector<std::shared_ptr<neural_network> > neural_network_container;

    //
    // Whether neural network input bus is loaded or not.
    // importing input entry from HFTR or call load_input_buses()
    // makes this flag true, while applying data to the collector
    // invalidates it.
    //

    bool network_input_bus_loaded_;

    //
    // Collector to input bus function gain parameter.
    //

    double c2ib_gain_;

    unsigned int i2q(unsigned int index) { return c2ib_gain_*index + 1; }

    market_data_collector &collector(void) const;

    //
    // Collector for market data with btf - approximation.
    //

    std::shared_ptr<market_data_collector> pcollector_;

    neural_network_container neural_networks_;

    unsigned int granularity_;
    granularity_counter gc_;
    hftr input_bus_copy_;

    //
    // Additional parameters (flags).
    //

    bool option_never_hftr_export_;
    bool option_hft_multicore_;
    bool option_verbose_;

    //
    // Logger for all object of this class.
    //

    static el::Logger *logger_;
};

#endif /* __BASIC_ARTIFICAL_INTELIGENCE_HPP__ */
