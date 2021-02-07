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

#ifndef __NEURAL_NETWORK_HPP__
#define __NEURAL_NETWORK_HPP__

#include <neuron.hpp>

class neural_network
{
public:

    DEFINE_CUSTOM_EXCEPTION_CLASS(ioerror, std::runtime_error)
    DEFINE_CUSTOM_EXCEPTION_CLASS(general_exception, std::runtime_error)

    typedef std::vector<unsigned int> layers_architecture;

    //
    // Arguments:
    //   inputs - network input number (must be divisible by
    //            number of neurons in input layer),
    //   la     - defines architecture of neural network:
    //            first element in container defines number of
    //            neurons in first layer, second defines number
    //            of neurons in second layer, and so on.
    //   lc     - learning coefficient (default 0). Set 0 if
    //            should not be used for training. Parameter
    //            can be changd also by „set_learn_coefficient”
    //            method.
    //

    neural_network(unsigned int inputs,
                   const layers_architecture &la,
                   double lc = 0.0)
        : inputs_(inputs), lc_(lc)
    {
        factory_network(la);
    }

    //
    // Network interface.
    //

    void reset_input_bus(void);

    void copy_input_bus(const neural_network &src);

    void set_pin(unsigned int pin, double value);

    neuron &neuron_at(unsigned int num_layer, unsigned int num);

    void fire(void);

    void feedback(double d, bool autosave = false);

    void save_network(void) { save_network(file_name_); }

    void save_network(const std::string &file_name);

    void set_learn_coefficient(double lc);

    void set_activate_function(unsigned int layer_number,
                                   neuron::activate_function_class af);

    double get_output(unsigned int pin) const;

    unsigned int get_number_of_inputs(void) const
    {
        return inputs_;
    }

    #ifdef USE_CJSON
    void load_network(const std::string &file_name);

    void load_network_from_buffer(const char *buffer);
    #endif

    static layers_architecture parse_layers_architecture_from_string(std::string arch);

private:

    typedef std::vector<neuron> layer;
    typedef std::vector<layer> network;

    void factory_network(const layers_architecture &la);

    //
    // Network architecture.
    //

    const unsigned int inputs_;
    network internal_network_;

    //
    // File name of neurons data.
    //

    std::string file_name_;

    //
    // Learning coefficient.
    //

    double lc_;
};

#endif /* __NEURAL_NETWORK_HPP__ */
