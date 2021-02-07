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

#include <random>
#include <cmath>
#include <limits>
#include <sstream>

#include <easylogging++.h>
#include <neuron.hpp>

#undef HFT_TRACE

static el::Logger *logger = nullptr;

#define hft_log(__X__) \
    CLOG(__X__, "neuron")

neuron::neuron(size_t inputs, double lc)
    : pins_(inputs), weights_(inputs), learn_coefficient_(lc),
      function_type_(SIGMOID), argument_(0.0), output_(0.0)
{
    //
    // Randimize weight.
    //

    static std::random_device rd;

    std::mt19937 rng(rd());    // Random-number engine used (Mersenne-Twister in this case).
    std::uniform_int_distribution<int> uni(100, 10000); // Guaranteed unbiased.

    int random_integer;

    for (int i = 0; i < weights_.size(); i++)
    {
        weights_.at(i) = double(uni(rng)) / 10000.0;
        weights_.at(i) /= weights_.size();
    }

    activate_function_init_params();

    //
    // Initialize logger.
    //

    if (logger == nullptr)
    {
        logger = el::Loggers::getLogger("neuron", true);
    }
}

std::string neuron::export_to_json(void) const
{
    std::ostringstream json;
    json.precision(std::numeric_limits<double>::max_digits10);

    json << "{\"id\":\"" << id_ << "\",\"weights\":";

    for (int i = 0; i < weights_.size(); i++)
    {
        if (i == 0)
        {
            json << "[";
        }
        else
        {
            json << ",";
        }

        json << weights_.at(i);
    }

    json << "]}";

    return json.str();
}

#ifdef USE_CJSON
void neuron::import_from_json(cJSON *json)
{
    cJSON *it = json -> child;

    while (it)
    {
        cJSON *id_object = cJSON_GetObjectItem(it, "id");

        if (id_object != NULL && std::string(id_object -> valuestring) == id_)
        {
            //
            // Found data.
            //

            cJSON *weights_array = cJSON_GetObjectItem(it, "weights");

            if (weights_array == NULL)
            {
                throw data_error("JSON error: „weights” array missing");
            }

            size_t n = cJSON_GetArraySize(weights_array);

            if (n != weights_.size())
            {
                std::ostringstream msg;

                msg << "JSON error: Number of input mismach. Required "
                    << weights_.size() << ", found " << n;

                throw data_error(msg.str());
            }

            for (int i = 0; i < n; i++)
            {
                weights_.at(i) = cJSON_GetArrayItem(weights_array, i) -> valuedouble;
            }

            return;
        }

        it = it -> next;
    }

    //
    // Warning unable to import.
    //

    hft_log(WARNING) << "[" << id_ << "] Unable to import data "
                     << "from json. Object stay unchanged.";
}
#endif /* USE_CJSON */

void neuron::display_weights(void) const
{
    int i = 0;

    for (auto it = weights_.begin(); it != weights_.end(); ++it)
    {
        #ifdef HFT_TRACE
        hft_log(TRACE) << id_ << " weight#" << i << ": "
                       << weights_.at(i) << "\n";
        #endif

        i++;
    }
}

void neuron::copy_inputs(const neuron &src)
{
    if (pins_.size() != src.pins_.size())
    {
        throw data_error("copy inputs: input bus dimension missmatch");
    }

    for (int i = 0; i < pins_.size(); i++)
    {
        pins_.at(i).setup(src.pins_.at(i).get());
    }
}

void neuron::fire(void)
{
    argument_ = 0.0;

    for (int i = 0; i < weights_.size(); i++)
    {
        #ifdef HFT_TRACE
        hft_log(TRACE) << id_ << " #" << i
                       << " weight: " << weights_.at(i)
                       << " pin: " << pins_.at(i).get()
                       << " mul_result: "
                       << weights_.at(i)*pins_.at(i).get();
        #endif

        argument_ += weights_.at(i)*pins_.at(i).get();
    }

    #ifdef HFT_TRACE
    hft_log(TRACE) << id_ << " TOTAL SUM BEFORE ACTIVATION : "
                   << argument_;
    #endif

    //
    // Activation.
    //

    output_ = activate_function();

    #ifdef HFT_TRACE
    hft_log(TRACE) << id_ << " OUTPUT (after activation) is: "
                   << output_;
    #endif

    //
    // Apply result to the connected
    // neurons (if any).
    //

    for (auto it = connections_.begin(); it != connections_.end(); it++)
    {
        it -> outer_neuron -> set_pin(it -> pin, output_);
    }
}

void neuron::feedback(double d)
{
    for (int i = 0; i < weights_.size(); i++)
    {
        #ifdef HFT_TRACE
        hft_log(TRACE) << id_ << " delta is: " << get_delta(d);
        #endif

        weights_.at(i) += learn_coefficient_ * get_delta(d) * pins_.at(i).get();
    }
}

double neuron::get_delta(double d) const
{
    if (connections_.size() == 0)
    {
        //
        // Output layer.
        //

        return (d - output_) * activate_derived_function();
    }
    else
    {
        //
        // Hidden layer.
        //

        double S = 0.0;

        for (auto it = connections_.begin(); it != connections_.end(); it++)
        {
            unsigned int pin = it -> pin;
            double a = it -> outer_neuron -> get_delta(d);
            double w = it -> outer_neuron -> weights_.at(pin);
            S += a*w;
        }

        return S * activate_derived_function();
    }
}

void neuron::activate_function_init_params(void)
{
    //TODO: Na razie pusto, jak bedziemy parametryzowac funkcje
    //      to dorzucimy.
    return;
}

double neuron::activate_function(void) const
{
    double x = argument_;

    switch (function_type_)
    {
        case SIGMOID:
            return 1.0 / (1.0 + exp(-1.0*x));
        case SIGMOID_BIPOLAR:
            return 2.0 / (1.0 + exp(-1.0*x)) - 1.0;
        case LINE:
            return x;
        default:
            throw invalid_objec_state("Unknown activate function");
    }

    return 0.0;
}

double neuron::activate_derived_function(void) const
{
    double x = argument_;

    switch (function_type_)
    {
        case SIGMOID:
            return exp(-1.0*x) / pow((1.0 + exp(-1.0*x)), 2.0);
        case SIGMOID_BIPOLAR:
            return 2.0*(exp(-1.0*x) / pow((1.0 + exp(-1.0*x)), 2.0));
        case LINE:
            return 1.0;
        default:
            throw invalid_objec_state("Unknown activate function");
    }

    return 0.0;
}

void neuron::add_const_input(void)
{
    input inp(true);
  //  inp.setup(1.0);

    pins_.push_back(inp);
    weights_.push_back(0.5);
}
