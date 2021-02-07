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

#include <fstream>
#include <neural_network.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp> 
#include <boost/tokenizer.hpp>

void neural_network::reset_input_bus(void)
{
    if (internal_network_.size() == 0)
    {
        //
        // Nothing to reset.
        //

        return;
    }

    for (auto &n : internal_network_.at(0))
    {
        n.reset();
    }
}

void neural_network::copy_input_bus(const neural_network &src)
{
    if (internal_network_.size() == 0 && src.internal_network_.size() == 0)
    {
        //
        // Nothing to copy.
        //

        return;
    }
    else if (internal_network_.at(0).size() != src.internal_network_.at(0).size())
    {
        //
        // Cannot copy.
        //

        throw general_exception("copy_input_bus: Input bus dimension missmatch");
    }

    for (int i = 0; i < internal_network_.at(0).size(); i++)
    {
        internal_network_.at(0).at(i).copy_inputs(src.internal_network_.at(0).at(i));
    }
}

void neural_network::set_pin(unsigned int pin, double value)
{
    if (internal_network_.size() == 0)
    {
        throw general_exception("Attempt to set pin for empty network");
    }

    unsigned int pin_index    = pin % (inputs_ / internal_network_.at(0).size());
    unsigned int neuron_index = pin / (inputs_ / internal_network_.at(0).size());

    if (neuron_index >= internal_network_.at(0).size())
    {
        throw general_exception("Pin number too big");
    }

    internal_network_.at(0).at(neuron_index).set_pin(pin_index, value);
}

neuron &neural_network::neuron_at(unsigned int num_layer, unsigned int num)
{
    if (num_layer >= internal_network_.size())
    {
        throw general_exception("Bad layer number");
    }

    layer &requested_layer = internal_network_.at(num_layer);

    if (num >= requested_layer.size())
    {
        throw  general_exception("Attempt to access of non existent neuron in layer");
    }

    return internal_network_.at(num_layer).at(num);
}

void neural_network::fire(void)
{
    for (auto it = internal_network_.begin(); it != internal_network_.end(); it++)
    {
        layer &l = (*it);

        for (auto jt = l.begin(); jt != l.end(); jt++)
        {
            jt -> fire();
        }
    }
}

void neural_network::feedback(double d, bool autosave)
{


    for (auto it = internal_network_.begin(); it != internal_network_.end(); it++)
    {
        layer &l = (*it);

        for (auto jt = l.begin(); jt != l.end(); jt++)
        {
            jt -> feedback(d);
        }
    }

    if (file_name_.length() != 0 && autosave)
    {
        //
        // Save neuron weights.
        //

        save_network();
    }
}

void neural_network::save_network(const std::string &file_name)
{
    file_name_ = file_name;

    std::string json_data = "[";

    for (auto it = internal_network_.begin(); it != internal_network_.end(); it++)
    {
        layer &l = (*it);

        for (auto jt = l.begin(); jt != l.end(); jt++)
        {
            json_data += (jt -> export_to_json()) + std::string(",");
        }
    }

    if (json_data.length() == 1)
    {
        json_data += std::string("]");
    }
    else
    {
        json_data.at(json_data.length() - 1) = ']';
    }

    std::ofstream os(file_name_, std::ifstream::binary);

    if (os)
    {
        os.write(json_data.c_str(), json_data.length());
        os.close();
    }
    else
    {
        std::string msg = std::string("Unable to open file for write: ") + file_name_;
        throw ioerror(msg.c_str());
    }
}

void neural_network::factory_network(const layers_architecture &la)
{
    if (la.size() == 0)
    {
        return;
    }

    if (inputs_ % la.at(0) != 0)
    {
        throw general_exception("Number of network inputs must be divisible by number of neurons in first layer");
    }

    //
    // First create internal_network_ with neurons
    // and assign identifiers them.
    //

    unsigned int inputs = inputs_ / la.at(0);

    for (int i = 0; i < la.size(); i++)
    {
        internal_network_.push_back(layer());

        //
        // internal_network_.at(i) indicates current layer.
        //

        for (int j = 0; j < la.at(i); j++)
        {
            std::string id = std::string("L")
                           + boost::lexical_cast<std::string>(i)
                           + std::string("N")
                           + boost::lexical_cast<std::string>(j);

            internal_network_.at(i).push_back(neuron(inputs /*+ 1*/, lc_));
            internal_network_.at(i).at(j).set_id(id);
            //internal_network_.at(i).at(j).set_pin(inputs, 1.0);
            internal_network_.at(i).at(j).add_const_input();
        }

        //
        // Number of input for each neuron in next layer.
        //

        inputs = la.at(i);
    }

    //
    // Perform connections between layers.
    //

    for (int i = 0; i < internal_network_.size() - 1; i++)
    {
        layer &current_layer = internal_network_.at(i);
        layer &next_layer = internal_network_.at(i+1);

        for (int j = 0; j < current_layer.size(); j++)
        {
            for (int k = 0; k < next_layer.size(); k++)
            {
                current_layer.at(j).connect(&next_layer.at(k), j);
            }
        }
    }
}

void neural_network::set_learn_coefficient(double lc)
{
    lc_ = lc;

    for (auto it = internal_network_.begin(); it != internal_network_.end(); it++)
    {
        layer &l = (*it);

        for (auto jt = l.begin(); jt != l.end(); jt++)
        {
            jt -> set_learn_coefficient(lc_);
        }
    }
}

void neural_network::set_activate_function(unsigned int layer_number,
                                               neuron::activate_function_class af)
{
    int size = internal_network_.size();

    if (layer_number >= size)
    {
        throw general_exception("Layer number too big");
    }

    layer &requested_layer = internal_network_.at(layer_number);

    for (auto it = requested_layer.begin(); it != requested_layer.end(); it++)
    {
        it -> set_activate_function(af);
    }
}

double neural_network::get_output(unsigned int pin) const
{
    //
    // Obtain last layer.
    //

    int size = internal_network_.size();

    if (size == 0)
    {
        throw general_exception("Attempt to get output from empty network");
    }

    const layer &last_layer = internal_network_.at(size - 1);

    if (pin >= last_layer.size())
    {
        throw general_exception("Pin number too big");
    }

    return last_layer.at(pin).output_pin();
}

#ifdef USE_CJSON
void neural_network::load_network(const std::string &file_name)
{
    file_name_ = file_name;

    std::ifstream is(file_name_, std::ifstream::binary);
    std::vector<char> buffer;

    if (is)
    {
        is.seekg(0, is.end);
        int length = is.tellg();
        is.seekg(0, is.beg);

        buffer.resize(length+1);
        buffer.at(length) = '\0';

        is.read(&buffer.front(), length);

        if (! is)
        {
            is.close();

            std::string msg = std::string("Error while reading file: ") + file_name_;

            throw ioerror(msg.c_str());
        }

        is.close();
    }
    else
    {
        std::string msg = std::string("Unable to open file for read: ") + file_name_;

        throw ioerror(msg.c_str());
    }

    load_network_from_buffer(&buffer.front());
}

void neural_network::load_network_from_buffer(const char *buffer)
{
    cJSON *json = cJSON_Parse(buffer);

    if (json == NULL)
    {
        throw general_exception("Invalid json data");
    }

    for (auto it = internal_network_.begin(); it != internal_network_.end(); it++)
    {
        layer &l = (*it);

        for (auto jt = l.begin(); jt != l.end(); jt++)
        {
            jt -> import_from_json(json);
        }
    }

    cJSON_Delete(json);
}
#endif /* USE_CJSON */

neural_network::layers_architecture neural_network::parse_layers_architecture_from_string(std::string arch)
{
    boost::algorithm::trim(arch);
    neural_network::layers_architecture ret;

    unsigned int len = arch.length();

    if (arch.length() < 2)
    {
        std::ostringstream message;

        message << "Invalid architecture format : „"
                << arch << "”";

        throw general_exception(message.str());
    }

    if (arch.at(0) != '[' || arch.at(len-1) != ']')
    {
        std::ostringstream message;

        message << "Error : „" << arch << "” Requires „[” at the beginning and „]” at the end.";

        throw general_exception(message.str());
    }

    arch = arch.substr(1, len-2);

    boost::char_separator<char> sep(" \t");
    boost::tokenizer<boost::char_separator<char> > tokens(arch, sep);
    boost::tokenizer<boost::char_separator<char> >::iterator it = tokens.begin();

    for (; it != tokens.end(); it++)
    {
        ret.push_back(boost::lexical_cast<unsigned int>(*it));
    }

    return ret;
}
