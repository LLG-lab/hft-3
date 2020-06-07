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

#ifndef __NEURON_HPP__
#define __NEURON_HPP__

#include <iostream>
#include <stdexcept>
#include <vector>


//
// Define if application is compiled with cJSON.c
//

#define USE_CJSON

#include <custom_except.hpp>

#ifdef USE_CJSON
#include <cJSON/cJSON.h>
#endif

class neuron
{
public:

    DEFINE_CUSTOM_EXCEPTION_CLASS(data_error, std::runtime_error)
    DEFINE_CUSTOM_EXCEPTION_CLASS(invalid_objec_state, std::runtime_error)

    enum activate_function_class
    {
        SIGMOID,
        SIGMOID_BIPOLAR,
        LINE
    };

    neuron(size_t inputs, double lc);

    ~neuron(void)
    {
        #ifdef DUMP
        display_weights();
        #endif
    }

    void set_id(const std::string &id)
    {
        id_ = id;
    }

    std::string export_to_json(void) const;

    #ifdef USE_CJSON
    void import_from_json(cJSON *json);
    #endif

    void display_weights(void) const;

    void reset(void)
    {
        for (auto it = pins_.begin(); it != pins_.end(); ++it)
        {
            it -> invalidate();
        }
    }

    void set_pin(unsigned int n, double value)
    {
        pins_.at(n).setup(value);
    }

    void set_pin_weight(unsigned int n, double value)
    {
        weights_.at(n) = value;
    }

    double output_pin(void) const
    {
        return output_;
    }

    void connect(neuron *p, unsigned int pin)
    {
        connections_.push_back(neuron_connection(p, pin));
    }

    void copy_inputs(const neuron &src);

    void fire(void);

    void feedback(double d);

    void set_activate_function(activate_function_class af)
    {
        function_type_ = af;
        activate_function_init_params();
    }

    void set_learn_coefficient(double lc)
    {
        learn_coefficient_ = lc;
    }

    void add_const_input(void);

private:

    double get_delta(double d) const;

    double activate_function(void) const;
    double activate_derived_function(void) const;
    void activate_function_init_params(void);

    struct neuron_connection
    {
        neuron_connection(neuron *op, unsigned int p)
            : outer_neuron(op), pin(p) {}

        neuron *outer_neuron;
        unsigned int pin;
    };

    struct input
    {
        DEFINE_CUSTOM_EXCEPTION_CLASS(invalid_input, std::runtime_error)

        input(bool is_const_pin = false)
            : valid_(false), value_(0.0), const_pin_(is_const_pin)
        {
            if (is_const_pin)
            {
                value_ = 1.0;
                valid_ = true;
            }
        }

        void setup(double value)
        {
            if (const_pin_) return;

            value_ = value;
            valid_ = true;
        }

        void invalidate(void)
        {
            if (const_pin_) return;

            valid_ = false;
        }

        bool valid(void) const
        {
            return valid_;
        }

        double get(void) const
        {
            if (valid_)
            {
                return value_;
            }

            throw invalid_input("Unset input");
        }

    private:

        bool valid_;
        double value_;
        bool const_pin_;
    };

    typedef std::vector<input> input_pins;

    //
    // Private constructor (not accessible by user code).
    //

    neuron(void)
    {
        throw std::runtime_error("Bad implementation");
    }

    //
    // Data members.
    //

    std::string id_;
    input_pins pins_;
    std::vector<double> weights_;
    double learn_coefficient_;
    activate_function_class function_type_;
    double argument_;

    double output_;

    std::vector<neuron_connection> connections_;
};

#endif /* __NEURON_HPP__ */
