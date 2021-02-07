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

#include <ai_predictor.hpp>
#include <easylogging++.h>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

static el::Logger *logger = nullptr;

static boost::char_separator<char> sep1("\t");
static boost::char_separator<char> sep2("=" );

#undef HFT_TRACE

#define hft_log(__X__) \
    CLOG(__X__, "ai_predictor")

#define INVALIDATE_BUS \
    valid_bus_ = false;

#define VALIDATE_BUS    \
    if (! valid_bus_) {  \
        load_input_bus(); \
        valid_bus_ = true; \
    }

ai_predictor::ai_predictor(const std::string &directory)
    : ticks_collector_(4031), valid_bus_(false),
      last_decision_(NO_DECISION),
      resolution_(1), skipped_ticks_(0), prev_ask_(0)
{
    //
    // Initialize logger.
    //

    if (logger == nullptr)
    {
        logger = el::Loggers::getLogger("ai_predictor", true);
    }

    //
    // Create neural networks cascade structures
    // dichotomizer, discriminators.
    //

    factory_networks(directory);

    //
    // Initialize tracer.
    //

    hft_log(INFO) << "Initializing ranges for tracer.";

    tracer_.add_range(range<unsigned int>(0, 99999));

    for (unsigned i = 100000; i <= 150000; i += 10)
    {
        tracer_.add_range(range<unsigned int>(i, i+9));
    }

    tracer_.add_range(range<unsigned int>(150010, 999999));

}

void ai_predictor::feed_data(unsigned int data)
{
    INVALIDATE_BUS

/////////////// Uwzględnienie resolution ///////////////
    if (skipped_ticks_++ % resolution_ != 0)
    {
        return;
    }

    if (prev_ask_ == data)
    {
        skipped_ticks_--;
        return;
    }

    prev_ask_ = data;
////////////// uwzglednienie resolution -koniec ////////////

    ticks_collector_.feed_data(data);
    tracer_.put_value(data);

    int remain = ticks_collector_.get_data_remain();

    if (remain > 0)
    {
        hft_log(INFO) << "Predictor not ready, reading market mode. Remain ["
                      << remain << "]";
    }
}

void ai_predictor::load_input_bus(void)
{
    if (! is_data_ready())
    {
        hft_log(ERROR) << "Unable to load input bus: insufficient data "
                          "in market collector.";

        throw ai_predictor_exception("Insufficient data in market collector");
    }

    //
    // index2quantity(i) = i² + 175i + 100,      i = 0, ..., 19
    //    index2quantity(0)  = 100,
    //    index2quantity(19) = 3919
    //
    // index2offset(j) = 11j - 11,               j = 1, ..., 11
    //    index2offset(1)  = 0,
    //    index2offset(11) = 110.
    //

    for (int i = 0; i < 20; i++)
    {
        dichotomizer_ -> net -> set_pin(12*i, tracer_.get_trace());

        for (int j = 1; j < 12; j++)
        {
            dichotomizer_ -> net -> set_pin(12*i+j, ticks_collector_.get_btfa(index2quantity(i), index2offset(j)));
        }
    }
}

bool ai_predictor::is_data_ready(void)
{
    return ticks_collector_.is_valid();
}

std::string ai_predictor::export_hftr_line(int settelment)
{
    if (! is_data_ready())
    {
        hft_log(ERROR) << "Unable to export HFTR: insufficient data "
                          "in market collector.";

        throw ai_predictor_exception("Insufficient data in market collector");
    }

    std::ostringstream oss;

    oss << "OUTPUT=" << settelment;

    //
    // index2quantity(i) = i² + 175i + 100,      i = 0, ..., 19
    //    index2quantity(0)  = 100,
    //    index2quantity(19) = 3919
    //
    // index2offset(j) = 11j - 11,               j = 1, ..., 11
    //    index2offset(1)  = 0,
    //    index2offset(11) = 110.
    //

    for (int i = 0; i < 20; i++)
    {
        oss << '\t' << (12*i) << '=' << tracer_.get_trace();

        for (int j = 1; j < 12; j++)
        {
            oss << '\t' << (12*i+j) << '=' << ticks_collector_.get_btfa(index2quantity(i), index2offset(j));
        }
    }

    return oss.str();
}

double ai_predictor::import_hftr_line(const std::string &hftr)
{
    double output = -255.0;
    std::string key, value;
    boost::tokenizer<boost::char_separator<char> > tokens(hftr, sep1);

    INVALIDATE_BUS

    dichotomizer_ -> net -> reset_input_bus();

    for (auto token = tokens.begin(); token != tokens.end(); token++)
    {
        boost::tokenizer<boost::char_separator<char> > keyval(*token, sep2);

        auto it = keyval.begin();

        if (it == keyval.end())
        {
            throw ai_predictor_exception("Invalid HFTR: Required key on input data");
        }

        key = (*it);
        it++;

        if (it == keyval.end())
        {
            throw ai_predictor_exception("Invalid HFTR: Required value on input data");
        }

        value = (*it);

        if (key == "OUTPUT")
        {
            output = boost::lexical_cast<double>(value);
        }
        else
        {
            #ifdef HFT_TRACE
            hft_log(TRACE) << "Setting pin #" << key << " with " << value;
            #endif

            dichotomizer_ -> net -> set_pin(boost::lexical_cast<int>(key), boost::lexical_cast<double>(value));
        }
    }

    //
    // Check if we have output.
    //

    if (output == 255.0)
    {
        throw ai_predictor_exception("Invalid HFTR: Missing settelment");
    }

    //
    // Make input bus to be valid.
    //

    valid_bus_ = true;

    return output;
}

decision_type ai_predictor::proceed_advice(void)
{
    last_decision_ = NO_DECISION;

    VALIDATE_BUS

    dichotomizer_ -> net -> fire();

    double network_result = dichotomizer_ -> net -> get_output(0);

    last_decision_ = dichotomizer_ -> trigger(network_result);

    switch (last_decision_)
    {
        case NO_DECISION:
            #ifdef HFT_TRACE
            hft_log(TRACE) << "Decision dropped by dichotomizer";
            #endif
            break;
        case DECISION_LONG:
            for (auto &x : l_discriminator_)
            {
                x.net -> copy_input_bus(*(dichotomizer_ -> net));
                x.net -> fire();
                network_result = x.net -> get_output(0);
                last_decision_ = x.trigger(network_result);
 
                if (last_decision_ == DECISION_LONG)
                {
                    continue;
                }
                else if (last_decision_ == NO_DECISION)
                {
                    #ifdef HFT_TRACE
                    hft_log(TRACE) << "Decision dropped by L-discriminator #"
                                   << x.cid;
                    #endif

                    break;
                }
                else
                {
                    std::string err_msg = std::string("Illegal decision made by L-discriminator #")
                                          + boost::lexical_cast<std::string>(x.cid);

                    throw ai_predictor_exception(err_msg);
                }
            }
            break;
        case DECISION_SHORT:
            for (auto &x : s_discriminator_)
            {
                x.net -> copy_input_bus(*(dichotomizer_ -> net));
                x.net -> fire();
                network_result = x.net -> get_output(0);
                last_decision_ = x.trigger(network_result);

                if (last_decision_ == DECISION_SHORT)
                {
                    continue;
                }
                else if (last_decision_ == NO_DECISION)
                {
                    #ifdef HFT_TRACE
                    hft_log(TRACE) << "Decision dropped by S-discriminator #"
                                   << x.cid;
                    #endif

                    break;
                }
                else
                {
                    std::string err_msg = std::string("Illegal decision made by S-discriminator #")
                                          + boost::lexical_cast<std::string>(x.cid);

                    throw ai_predictor_exception(err_msg);
                }
            }
            break;
        default:
            throw ai_predictor_exception("Illegal decision made by dichotomizer");
    }

    return last_decision_;
}

double ai_predictor::feedback(double settelment)
{
    double ret = -2.0;

    if (last_decision_ == NO_DECISION)
    {
        hft_log(WARNING) << "Illegal attempt to feedback network after NO_DECISION response.";

        return ret;
    }

    if ((last_decision_ == DECISION_LONG && l_discriminator_.empty()) ||
            (last_decision_ == DECISION_SHORT && s_discriminator_.empty()))
    {
        ret = dichotomizer_ -> net -> get_output(0);

        dichotomizer_ -> net -> feedback(settelment);
    }
    else if (last_decision_ == DECISION_LONG)
    {
        ret = l_discriminator_.back().net -> get_output(0);

        l_discriminator_.back().net -> feedback(settelment /*< 0.0 ? 0.0 : 1.0*/);
    }
    else if (last_decision_ == DECISION_SHORT)
    {
        ret = s_discriminator_.back().net -> get_output(0);

        s_discriminator_.back().net -> feedback(settelment /*< 0.0 ? 1.0 : 0.0 */);
    }
    else
    {
        throw ai_predictor_exception("ai_predictor feedback panic - illegal decision decision");
    }

    return ret;
}

void ai_predictor::set_learn_coefficient(double lc)
{
    if (l_discriminator_.empty())
    {
        #ifdef HFT_TRACE
        hft_log(TRACE) << "Dichotomizer setup with learn coefficient ["
                        << lc << "].";
        #endif

        dichotomizer_ -> net -> set_learn_coefficient(lc);
    }
    else
    {
        #ifdef HFT_TRACE
        hft_log(TRACE) << "L-Discriminator #" << l_discriminator_.back().cid
                       << " setup with learn coefficient ["
                       << lc << "].\n";
        #endif

        l_discriminator_.back().net -> set_learn_coefficient(lc);
    }

    if (s_discriminator_.empty() && ! l_discriminator_.empty())
    {
        #ifdef HFT_TRACE
        hft_log(TRACE) << "Dichotomizer setup with learn coefficient ["
                        << lc << "].";
        #endif

        dichotomizer_ -> net -> set_learn_coefficient(lc);
    }
    else if (! s_discriminator_.empty())
    {
        #ifdef HFT_TRACE
        hft_log(TRACE) << "S-Discriminator #" << s_discriminator_.back().cid
                       << " setup with learn coefficient ["
                       << lc << "].\n";
        #endif

        s_discriminator_.back().net -> set_learn_coefficient(lc);
    }
}

void ai_predictor::factory_networks(const std::string &directory)
{
    std::string file_name = directory + std::string("/index.json");

    std::ifstream is(file_name, std::ifstream::binary);
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

            std::string msg = std::string("Error while reading file: ") + file_name;

            throw ai_predictor_exception(msg.c_str());
        }

        is.close();
    }
    else
    {
        std::string msg = std::string("Unable to open file for read: ") + file_name;

        throw ai_predictor_exception(msg.c_str());
    }

    cJSON *json = cJSON_Parse(&buffer.front());

    if (json == NULL)
    {
        std::string msg = std::string("Invalid json file: ") + file_name;

        throw ai_predictor_exception(msg.c_str());
    }

    //
    // Traversing over json data.
    // Example json structure:
    //

    /*

    {
        "dichotomizer": {
            "network data": "dychotomizer.json",
            "trigger": "(0.1, 0.2) => LONG; (-0.1, -0.2) => SHORT",
            "architecture": [10,5]
        },
        "discriminators": [
            {
                "type": "LONG",
                "cid": 1,
                "network data": "ld1.json",
                "threshold": 0.1,
                "architecture": [10,5]
            },

            {
                "type": "SHORT",
                "cid": 1,
                "network data": "sd1.json",
                "threshold": 0.1,
                "architecture": [5,4]
            }
        ]
    }

    */

    try
    {
        //
        // Obtain informations for dichotomizer.
        //

        cJSON *dichotomizer_json = cJSON_GetObjectItem(json, "dichotomizer");

        if (dichotomizer_json == NULL)
        {
            throw ai_predictor_exception("Undefined dichotomizer");
        }

        std::string network_data_file_name;
        std::string rule;
        neural_network::layers_architecture arch;

        cJSON *aux = cJSON_GetObjectItem(dichotomizer_json, "network data");

        if (aux == NULL)
        {
            throw ai_predictor_exception("Missing „network data” attribute dichotomizer definition");
        }

        network_data_file_name = directory + std::string("/") + std::string(aux -> valuestring);

        aux = cJSON_GetObjectItem(dichotomizer_json, "rule");

        if (aux == NULL)
        {
            throw ai_predictor_exception("Missing „rule” attribute dichotomizer definition");
        }

        rule = aux -> valuestring;

        aux = cJSON_GetObjectItem(dichotomizer_json, "architecture");

        arch.push_back(20);

        if (aux == NULL)
        {
            hft_log(WARNING) << "Hidden layer architecture unspecified for dichotomizer.";
        }
        else
        {
            unsigned int n;

            for (int i = 0; i < cJSON_GetArraySize(aux); i++)
            {
                cJSON *subitem = cJSON_GetArrayItem(aux, i);

                n = subitem -> valueint;

                arch.push_back(n);
            }
        }

        arch.push_back(1);

        dichotomizer_.reset(new ai_object(0, rule, network_data_file_name,
                                              arch, neuron::activate_function_class::SIGMOID_BIPOLAR));

        //
        // Proceed factory for discriminators.
        //

        l_discriminator_.clear();
        s_discriminator_.clear();

        cJSON *discriminators_json = cJSON_GetObjectItem(json, "discriminators");

        if (discriminators_json == NULL || cJSON_GetArraySize(discriminators_json) == 0)
        {
            hft_log(WARNING) << "No discriminators in definition";
        }
        else
        {
            std::string type = "";
            unsigned int cid;

            for (int j = 0; j < cJSON_GetArraySize(discriminators_json); j++)
            {
                network_data_file_name = directory;
                rule = "";
                arch.clear();

                cJSON *discriminator = cJSON_GetArrayItem(discriminators_json, j);

                aux = cJSON_GetObjectItem(discriminator, "type");

                if (aux == NULL)
                {
                    throw ai_predictor_exception("Missing „type” attribute in discriminator definition");
                }

                type = aux -> valuestring;

                if (type != "SHORT" && type != "LONG")
                {
                    throw ai_predictor_exception("Invalid value of „type” attribute in discriminator definition");
                }

                aux = cJSON_GetObjectItem(discriminator, "cid");

                if (aux == NULL)
                {
                    throw ai_predictor_exception("Missing „cid” attribute in discriminator definition");
                }

                cid = aux -> valueint;

                if (cid == 0)
                {
                    throw ai_predictor_exception("CID=0 is disallowed for discriminator");
                }

                aux = cJSON_GetObjectItem(discriminator, "network data");

                if (aux == NULL)
                {
                    throw ai_predictor_exception("Missing „network data” attribute discriminator definition");
                }

                network_data_file_name += std::string("/") + std::string(aux -> valuestring);

                aux = cJSON_GetObjectItem(discriminator, "rule");

                if (aux == NULL)
                {
                    throw ai_predictor_exception("Missing „rule” attribute discriminator definition");
                }

                rule = aux -> valuestring;

                aux = cJSON_GetObjectItem(discriminator, "architecture");

                arch.push_back(20);

                if (aux == NULL)
                {
                    hft_log(WARNING) << "Hidden layer architecture unspecified for discriminator.";
                }
                else
                {
                    unsigned int n;

                    for (int i = 0; i < cJSON_GetArraySize(aux); i++)
                    {
                        cJSON *subitem = cJSON_GetArrayItem(aux, i);

                        n = subitem -> valueint;

                        arch.push_back(n);
                    }
                }

                arch.push_back(1);

                //
                // Create object.
                //

                if (type == "LONG")
                {
                    l_discriminator_.push_back(ai_object(cid, rule, network_data_file_name, arch,
                                                             neuron::activate_function_class::SIGMOID_BIPOLAR));
                }
                else
                {
                    s_discriminator_.push_back(ai_object(cid, rule, network_data_file_name, arch,
                                                             neuron::activate_function_class::SIGMOID_BIPOLAR));
                }

            } // for ...

            l_discriminator_.sort();
            s_discriminator_.sort();

            //
            // CIDs verification.
            //

            cid = 0;

            for (auto &item : l_discriminator_)
            {
                if (item.cid != ++cid)
                {
                    std::string msg = std::string("Missing or duplicate discriminator in cascade L. Expected CID [")
                                    + boost::lexical_cast<std::string>(cid) + std::string("], Got [")
                                    + boost::lexical_cast<std::string>(item.cid) + std::string("]");

                    throw ai_predictor_exception(msg);
                }
            }

            cid = 0;

            for (auto &item : s_discriminator_)
            {
                if (item.cid != ++cid)
                {
                    std::string msg = std::string("Missing or duplicate discriminator in cascade S. Expected CID [")
                                    + boost::lexical_cast<std::string>(cid) + std::string("], Got [")
                                    + boost::lexical_cast<std::string>(item.cid) + std::string("]");

                    throw ai_predictor_exception(msg);
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        cJSON_Delete(json);

        throw ai_predictor_exception(e.what());
    }

    hft_log(INFO) << "Cascade initialization completed.";

    cJSON_Delete(json);
}

ai_predictor::ai_object::ai_object(int c, std::string &rule, const std::string &neuron_data_file_name,
                                       const neural_network::layers_architecture &arch,
                                           neuron::activate_function_class afc)
    : cid(c), trigger(rule)
{
    hft_log(INFO) << "Creating ai_object CID: [" << c << "], decision rules: ["
                  << rule << "] file name: [" << neuron_data_file_name
                  << "].";

    net.reset(new neural_network(12*20, arch));

    net -> load_neurons(neuron_data_file_name);

    for (int i = 0; i < arch.size(); i++)
    {
        net -> set_activate_function(i, afc);
    }
}
