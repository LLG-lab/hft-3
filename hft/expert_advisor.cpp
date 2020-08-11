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

#include <vector>
#include <fstream>

#include <expert_advisor.hpp>
#include <overnight_swaps.hpp>
#include <boost/lexical_cast.hpp>

#define hft_log(__X__) \
    CLOG(__X__, "expert_advisor")

expert_advisor::expert_advisor(const std::string &instrument)
    : instrument_(instrument),
      dpips_limit_loss_(0),
      dpips_limit_profit_(0),
      mutable_networks_(false),
      vote_ratio_(0.0),
      decision_compensate_inverter_enabled_(false),
      ai_decision_(NO_DECISION),
      trade_on_positive_swaps_only_(false)
{
    el::Loggers::getLogger("expert_advisor", true);

    factory_advisor();
}

expert_advisor::~expert_advisor(void)
{
    hft_log(WARNING) << "Destroying expert_advisor";
}

bool expert_advisor::has_sufficient_data(void) const
{
    return is_collector_ready();
}

void expert_advisor::poke_tick(unsigned int tick)
{
    feed_data(tick);
}

void expert_advisor::notify_start_position(position_control::position_type pt, int open_price)
{
    position_controller_ -> setup(pt, open_price);
}

void expert_advisor::notify_close_position(int close_price)
{
    if (decision_compensate_inverter_.use_count() > 0)
    {
        int yield_pips;
        position_control::setup_info si = position_controller_ -> get_setup_info();

        switch (si.position)
        {
            case position_control::position_type::LONG:
                yield_pips = (close_price - si.price) / 10;
                break;
            case position_control::position_type::SHORT:
                yield_pips = (si.price - close_price) / 10;
                break;
            default:
                throw exception("Attempt to notify close not opened position");
        }

        decision_compensate_inverter_ -> insert_pips_yield(yield_pips);
    }

    position_controller_ -> reset();
}

bool expert_advisor::should_close_position(int current_price)
{
    if (trade_on_positive_swaps_only_)
    {
        position_control::setup_info si = position_controller_ -> get_setup_info();

        switch (si.position)
        {
            case position_control::position_type::LONG:
                if (! hft::is_positive_swap_long(instrument_))
                {
                    return true;
                }

                break;
            case position_control::position_type::SHORT:
                if (! hft::is_positive_swap_short(instrument_))
                {
                    return true;
                }

                break;
            default:
                throw exception("Not opened position");
        }
    }

    return (position_controller_ -> get_position_control_status(current_price) == position_control::control::CLOSE);
}

void expert_advisor::enable_cache(void)
{
    std::string cache_path;

    //
    // Enable cache for collector.
    //

    cache_path = std::string("/var/lib/hft/cache/collectors/") + instrument_;
    basic_artifical_inteligence::enable_market_collector_cache(cache_path);

    //
    // Enable cache for HCI, if HCI enabled.
    //

    if (decision_compensate_inverter_.use_count() > 0)
    {
        cache_path = std::string("/var/lib/hft/cache/HCIs/") + instrument_;
        decision_compensate_inverter_ -> enable_cache(cache_path);
        decision_compensate_inverter_enabled_ = true;
    }
}

decision_type expert_advisor::provide_advice(void)
{
    if (! has_sufficient_data())
    {
        return NO_DECISION;
    }
    else if (! is_valid_bus())
    {
        const unsigned int networks_number = get_neural_networks_number();

        if (mutable_networks_)
        {
            //
            // Check if networks has changed.
            //

            for (unsigned int nid = 0; nid < networks_number; nid++)
            {
                if (net_info_.has_changed(nid))
                {
                    hft_log(INFO) << "Neural network file ["
                                  << net_info_.get_name(nid)
                                  << "] has changed, reloading.";

                    load_network(nid, net_info_.get_name(nid));

                    net_info_.notify_change(nid);
                }
            }
        }

        //
        // Recalculate preliminary decision
        // based on Artifical Inteligence.
        //

        fireup_networks();

        unsigned int voting[2] = {0, 0};

        for (unsigned int nid = 0; nid < networks_number; nid++)
        {
            hft_log(INFO) << "Network #" << nid << " respond ["
                          << get_network_output(nid) << "]";

            switch ((*decision_)(get_network_output(nid)))
            {
                case DECISION_SHORT:
                    voting[0]++;
                    break;
                case DECISION_LONG:
                    voting[1]++;
                    break;
            }
        }

        hft_log(INFO) << "Voting result shorts [" << voting[0] << "/" << networks_number << "], "
                      << "longs [" << voting[1] << "/" << networks_number << "].";

        if (static_cast<double>(voting[0]) / static_cast<double>(networks_number) >= vote_ratio_)
        {
            ai_decision_ = DECISION_SHORT;
        }
        else if (static_cast<double>(voting[1]) / static_cast<double>(networks_number) >= vote_ratio_)
        {
            ai_decision_ = DECISION_LONG;
        }
        else
        {
            ai_decision_ = NO_DECISION;
        }
    }

    decision_type final_decision = ai_decision_;

    //
    // If HCI is enabled, take it into
    // account to get final decision.
    //

    if (decision_compensate_inverter_.use_count() > 0 && decision_compensate_inverter_enabled_)
    {
        final_decision = decision_compensate_inverter_ -> decision(final_decision);
    }

    //
    // If user requested to play only on position
    // direction having positive overnight swaps,
    // we have to lookup overnight swap table
    // and if final_decision indicates direction
    // having negative swap, change decision to
    // NO_DECISION.
    //

    if (trade_on_positive_swaps_only_)
    {
        if ( (final_decision == DECISION_LONG && !hft::is_positive_swap_long(instrument_)) ||
             (final_decision == DECISION_SHORT && !hft::is_positive_swap_short(instrument_)))
        {
            final_decision = NO_DECISION;
        }
    }

    //
    // Display final decision to the log.
    //

    switch (final_decision)
    {
        case DECISION_SHORT:
            hft_log(INFO) << "Decision is [DECISION_SHORT].";
            break;
        case DECISION_LONG:
            hft_log(INFO) << "Decision is [DECISION_LONG].";
            break;
        default:
            hft_log(INFO) << "Decision is [NO_DECISION].";
    }

    return final_decision;
}

void expert_advisor::factory_advisor(void)
{
    //
    // Do basic setup.
    //

    set_opt(AI_OPTION_NEVER_HFTR_EXPORT, true);
    set_opt(AI_OPTION_HFT_MULTICORE, false);
    set_opt(AI_OPTION_VERBOSE, false);

    //
    // Load manifest file.
    //

    std::string path = std::string("/etc/hft/") + instrument_;
    std::string file_name = path + std::string("/manifest.json");
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

            throw exception(msg.c_str());
        }

        is.close();
    }
    else
    {
        std::string msg = std::string("Unable to open file for read: ") + file_name;

        throw exception(msg.c_str());
    }

    cJSON *json = cJSON_Parse(&buffer.front());

    if (json == NULL)
    {
        std::string msg = std::string("Invalid json file: ") + file_name;

        throw exception(msg.c_str());
    }

    //
    // EAV1: Have to parse json of following structure:
    //

/*
  {
      "eav" : 1,
      "rule" : "<0,1> = LONG",
      "architecture" : "[3 5 6]",
      "networks" : [ "net1.json",
                     "net2.json",
                     "net3.json"
                   ],
      "mutable_networks" : true,
      "collector_size" : 220,
      "vote_ratio" : 0.7,
      "dpips_limit_profit" : 500,
      "dpips_limit_loss" : 100,
      "binomial_approximation" : "approx.json",
      "granularity" : 15,
      "position_control_policy" : {
          (attributes specified for particular PCP)
      },
      "custom_handler_options" : {
          "invert_engine_decision" : true
      },
      "hysteresis_compensate_inverter" : {
           "capacity" : 7,
           "s_threshold" : 0.45,
           "i_threshold" : -0.45
      }
  }
*/

    try
    {
        cJSON *eav_json = cJSON_GetObjectItem(json, "eav");

        if (eav_json == NULL)
        {
            std::string err_msg = std::string("Undefined „eav” attribute in")
                                + file_name;

            throw exception(err_msg);
        }

        eav_ = eav_json -> valueint;

        if (eav_ != 1)
        {
            hft_log(ERROR) << "EAV# [" << eav_ << "] is not supported";

            std::string err_msg = "Unsupported EAV#";

            throw exception(err_msg);
        }

        //
        // „dpips_limit_profit” attribute.
        //

        cJSON *dpips_limit_json = cJSON_GetObjectItem(json, "dpips_limit_profit");

        if (dpips_limit_json == NULL)
        {
            std::string err_msg = std::string("Undefined „dpips_limit_profit” attribute in ")
                                  + file_name;

            throw exception(err_msg);
        }

        if (dpips_limit_json -> valueint < 0)
        {
            std::string err_msg = std::string("Illegal value of „dpips_limit_profit” : ")
                                  + boost::lexical_cast<std::string>(dpips_limit_json -> valueint);

            throw exception(err_msg);
        }

        dpips_limit_profit_ = dpips_limit_json -> valueint;

        //
        // „dpips_limit_loss” attribute.
        //

        dpips_limit_json = cJSON_GetObjectItem(json, "dpips_limit_loss");

        if (dpips_limit_json == NULL)
        {
            std::string err_msg = std::string("Undefined „dpips_limit_loss” attribute in ")
                                  + file_name;

            throw exception(err_msg);
        }

        if (dpips_limit_json -> valueint < 0)
        {
            std::string err_msg = std::string("Illegal value of „dpips_limit_loss” : ")
                                  + boost::lexical_cast<std::string>(dpips_limit_json -> valueint);

            throw exception(err_msg);
        }

        dpips_limit_loss_ = dpips_limit_json -> valueint;

        //
        // „granularity” attribute.
        //

        cJSON *granularity_json = cJSON_GetObjectItem(json, "granularity");

        if (granularity_json == NULL)
        {
            std::string err_msg = std::string("Undefined „granularity” attribute in ")
                                  + file_name;

            throw exception(err_msg);
        }

        if (granularity_json -> valueint < 0)
        {
            std::string err_msg = std::string("Illegal value of „granularity” : ")
                                  + boost::lexical_cast<std::string>(granularity_json -> valueint);

            throw exception(err_msg);
        }

        set_granularity(granularity_json -> valueint);

        //
        // „collector_size” attribute.
        //

        cJSON *collector_size_json = cJSON_GetObjectItem(json, "collector_size");

        if (collector_size_json == nullptr || collector_size_json -> type != cJSON_Number)
        {
            hft_log(WARNING) << "Incorrect type or undefined ‘collector_size’ attribute, using default [220].";

            create_collector(220);
        }
        else
        {
            create_collector(collector_size_json -> valueint);
        }

        //
        // „binomial_approximation” attribute.
        //

        cJSON *binomial_approximation_json = cJSON_GetObjectItem(json, "binomial_approximation");

        if (binomial_approximation_json == NULL)
        {
            std::string err_msg = std::string("Undefined „binomial_approximation” attribute in ")
                                  + file_name;

            throw exception(err_msg);
        }

        initialize_approximator_from_file(path + std::string("/") + std::string(binomial_approximation_json -> valuestring));

        //
        // „rule” attribute.
        //

        cJSON *rule_json = cJSON_GetObjectItem(json, "rule");

        if (rule_json == NULL)
        {
            std::string err_msg = std::string("Undefined „rule” attribute in ")
                                  + file_name;

            throw exception(err_msg);
        }

        decision_.reset(new decision_trigger(std::string(rule_json -> valuestring)));

        //
        // „architecture” attribute.
        //

        cJSON *architecture_json = cJSON_GetObjectItem(json, "architecture");

        if (architecture_json == NULL)
        {
            std::string err_msg = std::string("Undefined „architecture” attribute in ")
                                  + file_name;

            throw exception(err_msg);
        }

        neural_network::layers_architecture arch;
        arch = neural_network::parse_layers_architecture_from_string(std::string(architecture_json -> valuestring));

        //
        // „vote_ratio” attribute.
        //

        cJSON *vote_ratio_json = cJSON_GetObjectItem(json, "vote_ratio");

        if (vote_ratio_json == NULL)
        {
            std::string err_msg = std::string("Undefined „vote_ratio” attribute in ")
                                  + file_name;

            throw exception(err_msg);
        }

        vote_ratio_ = vote_ratio_json -> valuedouble;

        if (vote_ratio_ < 0.5 || vote_ratio_ > 1.0)
        {
            std::string err_msg = std::string("Illegal value of „vote_ratio” : ")
                                  + boost::lexical_cast<std::string>(vote_ratio_);

            throw exception(err_msg);
        }

        //
        // „networks” attribute.
        //

        cJSON *networks_json = cJSON_GetObjectItem(json, "networks");

        if (networks_json == NULL)
        {
            std::string err_msg = std::string("Undefined „networks” attribute in ")
                                  + file_name;

            throw exception(err_msg);
        }

        if (cJSON_GetArraySize(networks_json) == 0)
        {
            std::string err_msg = std::string("Empty set of „networks” in ")
                                  + file_name;

            throw exception(err_msg);
        }

        std::string network_file_name;
        unsigned int nid;

        for (int i = 0; i < cJSON_GetArraySize(networks_json); i++)
        {
            cJSON *subitem = cJSON_GetArrayItem(networks_json, i);

            network_file_name = path + std::string("/") + std::string(subitem -> valuestring);

            nid = add_neural_network(arch);
            load_network(nid, network_file_name);
            net_info_.register_file(nid, network_file_name);
        }

        //
        // „mutable_networks” attribute - flag indicates that
        // network files may change in mean while.
        //

        cJSON *mutable_networks_json = cJSON_GetObjectItem(json, "mutable_networks");

        if (mutable_networks_json == NULL)
        {
            hft_log(INFO) << "Attribute [mutable_networks] undefined in "
                          << file_name << ", assuming default value [false]";
        }
        else if (mutable_networks_json -> type == cJSON_False)
        {
            mutable_networks_ = false;
        }
        else if (mutable_networks_json -> type == cJSON_True)
        {
            mutable_networks_ = true;
        }
        else if (mutable_networks_json -> type == cJSON_Number)
        {
            mutable_networks_ = (mutable_networks_json -> valueint == 0) ? false : true;
        }
        else
        {
            std::string err_msg = std::string("Illegal type of „mutable_networks” attribute")
                                  + file_name;

            throw exception(err_msg);
        }

        //
        // Position controller informations.
        //

        cJSON *pcp_json = cJSON_GetObjectItem(json, "position_control_policy");

        if (pcp_json == NULL)
        {
            std::string err_msg = std::string("Undefined „position_control_policy” attribute in ")
                                  + file_name;

            throw exception(err_msg);
        }

        position_controller_.reset(new position_control_manager(dpips_limit_loss_, dpips_limit_profit_));
        position_controller_ -> configure_from_json(pcp_json);

        //
        // Custom handler options (not required).
        //

        /*      "custom_handler_options" : {
          "invert_engine_decision" : true
          },
        */

        cJSON *cho_json = cJSON_GetObjectItem(json, "custom_handler_options");

        if (cho_json != nullptr)
        {
            cJSON *invert_engine_decision_json = cJSON_GetObjectItem(cho_json, "invert_engine_decision");

            if (invert_engine_decision_json != nullptr)
            {
                if (invert_engine_decision_json -> type == cJSON_False)
                {
                    custom_handler_options_.invert_engine_decision = false;
                }
                else if (invert_engine_decision_json -> type == cJSON_True)
                {
                    custom_handler_options_.invert_engine_decision = true;
                }
                else if (invert_engine_decision_json -> type == cJSON_Number)
                {
                    custom_handler_options_.invert_engine_decision = (invert_engine_decision_json -> valueint == 0) ? false : true;
                }
                else
                {
                    std::string err_msg = std::string("Illegal type of „invert_engine_decision” attribute")
                                  + file_name;

                    throw exception(err_msg);
                }

                hft_log(INFO) << "Attribute [custom_handler_options.invert_engine_decision] setup for value ["
                              << (custom_handler_options_.invert_engine_decision ? "true" : "false") << "]";

            }
        }

        //
        // HCI informations (optional).
        //

        cJSON *hci_json = cJSON_GetObjectItem(json, "hysteresis_compensate_inverter");

        if (hci_json == nullptr)
        {
            hft_log(INFO) << "HCI feature disabled for instrument ["
                          << instrument_ << "].";
        }
        else
        {
            cJSON *hci_capacity_json = cJSON_GetObjectItem(hci_json, "capacity");

            if (hci_capacity_json == nullptr)
            {
                std::string err_msg = std::string("Unspecified „capacity” attribute within „hysteresis_compensate_inverter” in file ")
                                      + file_name;

                throw exception(err_msg);
            }

            cJSON *hci_s_threshold_json = cJSON_GetObjectItem(hci_json, "s_threshold");

            if (hci_s_threshold_json == nullptr)
            {
                std::string err_msg = std::string("Unspecified „s_threshold” attribute within „hysteresis_compensate_inverter” in file ")
                                      + file_name;

                throw exception(err_msg);
            }

            cJSON *hci_i_threshold_json = cJSON_GetObjectItem(hci_json, "i_threshold");

            if (hci_i_threshold_json == nullptr)
            {
                std::string err_msg = std::string("Unspecified „i_threshold” attribute within „hysteresis_compensate_inverter” in file ")
                                      + file_name;

                throw exception(err_msg);
            }

            int capacity = hci_capacity_json -> valueint;
            double st = hci_s_threshold_json -> valuedouble;
            double it = hci_i_threshold_json -> valuedouble;

            if (it < st)
            {
                std::string err_msg = std::string("Value of attribute „i_threshold” cannot be greater than value of attribute „s_threshold” in file ")
                                      + file_name;

                throw exception(err_msg);
            }

            if (capacity <= 0 || capacity > 100)
            {
                std::string err_msg = std::string("Illegal value of attribute „capacity”. Correct range is (1 … 100) in file ")
                                      + file_name;

                throw exception(err_msg);
            }

            decision_compensate_inverter_.reset(new hci(capacity, st, it));

            hft_log(INFO) << "Created HCI with parameters: capacity ["
                          << capacity << "], straight threshold ["
                          << st << "], invert threshold [" << it << "].";
        }
    }
    catch (const std::exception &e)
    {
        cJSON_Delete(json);

        throw exception(e.what());
    }

    cJSON_Delete(json);
    set_learn_coefficient(0.0);

    hft_log(INFO) << "Expert Advisor [" << path
                  << "] initialization completed.";
}
