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

#include <unistd.h>
#include <fstream>
#include <sstream>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <cJSON/cJSON.h>

#include <marketplace_gateway_process.hpp>

#include <easylogging++.h>

#define hft_log(__X__) \
    CLOG(__X__, "gateway")

namespace fs = boost::filesystem;

marketplace_gateway_process::marketplace_gateway_process(boost::asio::io_context &ioctx, boost::program_options::variables_map &config)
    : ioctx_(ioctx),
      config_(config)
{
    std::string log_file_name = marketplace_gateway_process::prepare_log_file();

    el::Loggers::getLogger("gateway", true);

    if (config_["marketplace.bridges_config"].as<std::string>().length() == 0)
    {
        throw exception("Undefined bridges configuration");
    }

    parse_proc_list_json(file_get_contents(config_["marketplace.bridges_config"].as<std::string>()));

    for (auto &proc_item : process_list_)
    {
        hft_log(INFO) << "Starting external gateway ["
                      << proc_item.label << "]";

        boost::filesystem::path path_find = proc_item.program;
        auto start_program = boost::process::search_path(path_find);

        if (start_program.empty())
        {
            std::string err_msg = std::string("Unable to find program to start [")
                                  + proc_item.program + std::string("]");

            throw exception(err_msg);        
        }
        else
        {
            hft_log(INFO) << "Found bridge application: [" << start_program << ']';
        }

        proc_item.child.reset(
            new boost::process::child(
                    boost::process::exe=start_program.c_str(),
                    boost::process::args=proc_item.argv,
                    (boost::process::std_out & boost::process::std_err) > log_file_name,
                    boost::process::std_in < (*proc_item.process_stdin),
                    boost::process::start_dir=proc_item.start_dir,
                    boost::process::on_exit=boost::bind(&marketplace_gateway_process::process_exit_notify, this, _1, _2),
                    ioctx_
            )
        );

        std::error_code ec;

        if (! proc_item.child -> running(ec))
        {
            hft_log(ERROR) << "Bridge process failed with error: ["
                           << ec << ']';

            throw exception("Failed to start marketplace bridge process");
        }
        else
        {
            hft_log(INFO) << "Bridge process successfully started, " << ec;
        }

        proc_item.process_stdin.reset(new boost::process::opstream());
        (*proc_item.process_stdin) << create_process_configuration() << std::endl;
        proc_item.process_stdin -> flush();
        proc_item.process_stdin -> pipe().close();
    }
}

marketplace_gateway_process::~marketplace_gateway_process(void)
{
    // FIXME: Spróbować najpierw zakończyć proces „po dobremu”
    //        a jesli sie nie zakończy w odpowiednim czasie,
    //        wtedy terminate(). W ten sposób damy szansę na
    //        odpalenie destruktorów w procesie gatewaya.

    for (auto &bpi : process_list_)
    {
        if (bpi.child.use_count() && bpi.child -> running())
        {
            hft_log(INFO) << "Destroying gateway process ["
                          << bpi.label << "]";

            bpi.child -> terminate();
            bpi.child -> wait();
        }
    }
}

std::string marketplace_gateway_process::create_process_configuration(void) const
{
    std::ostringstream oss;

    oss << "{\"connection_port\":"
        << config_["networking.listen_port"].as<short>()
        << "}";

    return oss.str();
}

std::string marketplace_gateway_process::prepare_log_file(void)
{
    std::string regular_log_file = "/var/log/hft/bridge.log";

    if (fs::exists(fs::path(regular_log_file)))
    {
        //
        // There is remainder log file from
        // previous session, going to rotate
        //

        fs::rename(fs::path(regular_log_file), fs::path(find_free_name(regular_log_file)));
    }

    return regular_log_file;
}

std::string marketplace_gateway_process::find_free_name(const std::string &file_name)
{
    int index = 0;
    std::string candidate;
    std::ostringstream oss;

    auto date = boost::posix_time::second_clock::local_time().date();

    oss << file_name << "-" << date.year()
                     << "-" << date.month()
                     << "-" << date.day();

    std::string backup_file = oss.str();

    while (true)
    {
        candidate = backup_file + std::string("_") + boost::lexical_cast<std::string>(index++);

        if (! fs::exists(fs::path(candidate)))
        {
            break;
        }
    }

    return candidate;
}

std::string marketplace_gateway_process::file_get_contents(const std::string &filename)
{
    std::fstream in_stream;
    std::string line, ret;

    in_stream.open(filename, std::fstream::in);

    if (in_stream.fail())
    {
        std::string msg = std::string("Unable to open file: ")
                          + filename;

        throw std::runtime_error(msg);
    }

    while (! in_stream.eof())
    {
        std::getline(in_stream, line);
        ret += (line + std::string("\n"));
    }

    in_stream.close();

    return ret;
}

void marketplace_gateway_process::parse_proc_list_json(const std::string &data)
{
    bridge_process_info bpi;

    cJSON *json = cJSON_Parse(&data.front());

    if (json == nullptr)
    {
        throw exception("JSON data syntax error");
    }

    try
    {
        cJSON *bridges_json = cJSON_GetObjectItem(json, "bridges");

        if (bridges_json == nullptr)
        {
            throw exception("Undefined ‘bridges’ attribute");
        }
        else if (bridges_json -> type != cJSON_Array)
        {
            throw exception("Attribute ‘bridges’ must be array");
        }

        cJSON *bridge_proc_json = bridges_json -> child;

        while (bridge_proc_json)
        {
            cJSON *active_json = cJSON_GetObjectItem(bridge_proc_json, "active");

            if (active_json == nullptr)
            {
                throw exception("Undefined ‘active’ attribute");
            }
            else if (active_json -> type == cJSON_True || (active_json -> type == cJSON_Number && active_json -> valueint == 1))
            {
            }
            else if (active_json -> type == cJSON_False || (active_json -> type == cJSON_Number && active_json -> valueint == 0))
            {
                bridge_proc_json = bridge_proc_json -> next;
                continue;
            }
            else
            {
                throw exception("Attribute ‘active’ must be type boolean");
            }

            bpi.argv.clear();

            cJSON *label_json = cJSON_GetObjectItem(bridge_proc_json, "label");

            if (label_json == nullptr)
            {
                throw exception("Undefined ‘label’ attribute");
            }
            else if (label_json -> type != cJSON_String)
            {
                throw exception("Attribute ‘label’ must be type string");
            }

            bpi.label = label_json -> valuestring;

            cJSON *program_json = cJSON_GetObjectItem(bridge_proc_json, "program");

            if (program_json == nullptr)
            {
                throw exception("Undefined ‘program’ attribute");
            }
            else if (program_json -> type != cJSON_String)
            {
                throw exception("Attribute ‘program’ must be type string");
            }

            bpi.program = program_json -> valuestring;

            cJSON *start_dir_json = cJSON_GetObjectItem(bridge_proc_json, "start_dir");

            if (start_dir_json == nullptr)
            {
                throw exception("Undefined ‘start_dir’ attribute");
            }
            else if (start_dir_json -> type != cJSON_String)
            {
                throw exception("Attribute ‘start_dir’ must be type string");
            }

            bpi.start_dir = start_dir_json -> valuestring;

            cJSON *argv_json = cJSON_GetObjectItem(bridge_proc_json, "argv");

            if (argv_json == nullptr)
            {
                throw exception("Undefined ‘argv’ attribute");
            }
            else if (argv_json -> type != cJSON_Array)
            {
                throw exception("Attribute ‘argv’ must be array");
            }

            cJSON *arg_json = argv_json -> child;

            while (arg_json)
            {
                if (arg_json -> type != cJSON_String)
                {
                    throw exception("Item of ‘argv’ array must be type string");
                }

                bpi.argv.push_back(arg_json -> valuestring);

                arg_json = arg_json -> next;
            }

            process_list_.push_back(bpi);

            bridge_proc_json = bridge_proc_json -> next;
        }
    }
    catch (const exception &e)
    {
        cJSON_Delete(json);

        throw e;
    }

    cJSON_Delete(json);
}

void marketplace_gateway_process::process_exit_notify(int, const std::error_code &ec)
{
    for (auto &bpi : process_list_)
    {
        if (bpi.child.use_count() && ! bpi.child -> running())
        {
            hft_log(ERROR) << "Bridge process [" << bpi.label
                           << "] terminated unexpectedly, error: ["
                           << ec << ']';
        }
    }

    throw exception("Bridge process terminated unexpectedly");
}
