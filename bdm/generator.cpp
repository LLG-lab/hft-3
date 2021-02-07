/**********************************************************************\
**                                                                    **
**                 -=≡≣ BitMex Data Manager  ≣≡=-                    **
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

#include <generator.hpp>
#include <file_operations.hpp>
#include <downloader.hpp>
#include <bdmdb.hpp>
#include <cJSON/cJSON.h>

#include <newt.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <random>
#include <fstream>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>

namespace bdm {
namespace generator {

#define BITMEX_CHUNK_SIZE 750

#define LOG \
    std::cout << "\e[96;1mBDM-GENERATOR>\033[0m "

#define LOGERROR \
    std::cerr << "\033[;31mBDM-GENERATOR>\033[0m "

class auto_suspend_resume
{
public:
    auto_suspend_resume(bool guimode)
        : guimode_(guimode) { if (guimode_) newtSuspend(); }
    ~auto_suspend_resume(void) { if (guimode_) newtResume(); }
private:
    const bool guimode_;
};

struct generated_network_info
{
    int start_timestamp;
    int end_timestamp;
} gni;

class file_auto_cleaner
{
public:

    ~file_auto_cleaner(void)
    {
        using namespace boost::filesystem;
        path p;

        for (auto &f : file_names_)
        {
            try
            {
                p = f;
                if (exists(p) && is_regular_file(p))
                {
                    LOG << "file auto cleaner: Removing intermediate temporay file: "
                        << f << "\n";

                    remove(p);
                }
                else
                {
                    LOG << "file auto cleaner: Skipping file: " << f
                        << ". File does not exist or it is not a regular file.\n";
                }
            }
            catch (...)
            {
                LOGERROR << "Error while removing file: " << f << "\n";
            }
        }
    }

    std::string register_file(const std::string &filename)
    {
        file_names_.push_back(filename);

        return filename;
    }

private:

    std::list<std::string> file_names_;
};

double course_convert(double XBTUSD)
{
    int x = (int) (XBTUSD * 10);
    return ((double)(x) / 100000);
}

void start_program(const std::string &prog_name,
                      const std::list<std::string> &params = std::list<std::string>())
{
    using namespace boost::process;

    child c(exe=prog_name, args=params, boost::process::std_out > stdout,
                boost::process::std_err > stderr);

    c.wait();
    int rc = c.exit_code();

    if (rc != 0)
    {
        std::string err_msg = std::string("Command [") + prog_name
                              + std::string("] exited with exit code ")
                              + std::to_string(rc);

        LOGERROR << err_msg << "\n";

        throw std::runtime_error(err_msg);
    }
}

std::string start_program_ret_output(const std::string &prog_name,
                      const std::list<std::string> &params = std::list<std::string>())
{
    using namespace boost::process;

    ipstream pipe_stream;

    std::string line = "", ret = "";

    child c(exe=prog_name, args=params, boost::process::std_out > pipe_stream,
                boost::process::std_err > stderr);

    while (c.running() && std::getline(pipe_stream, line))
    {
        ret += (line + std::string("\n"));
    }

    c.wait();
    int rc = c.exit_code();

    if (rc != 0)
    {
        std::string err_msg = std::string("Command [") + prog_name
                              + std::string("] exited with exit code ")
                              + std::to_string(rc);

        LOGERROR << err_msg << "\n";

        throw std::runtime_error(err_msg);
    }

    return ret;
}

std::string tmp_file(const std::string &extension)
{
    static const std::string chars = "QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm1234567890";

    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, chars.length()-1);

    std::string ret = "/tmp/";

    for (int i = 0; i < 10; i++)
    {
        ret.push_back(chars[dist(rng)]);
    }

    return ret + extension;
}

/*
 Adres przykładowy (czas na końcu wyrażony jest w UTC!):

 https://testnet.bitmex.com/api/v1/trade/bucketed?symbol=XBTUSD&count=7&binSize=1m&partial=false&reverse=true&endTime=2019-11-11+19%3A29

Przykładowy JSON na wyjściu:
[
{ "timestamp":"2019-11-11T19:38:00.000Z",
  "symbol":"XBTUSD",
  "open":8756,
  "high":8774.5,
  "low":8756,
  "close":8757,
  "trades":82,
  "volume":18743,
  "vwap":8765.0101,
  "lastSize":671,
  "turnover":213841119,
  "homeNotional":2.13841119,
  "foreignNotional":18743
},

*/

uint64_t universal_time_now_millis(void)
{
  using namespace std::chrono;

  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

static int universal_time_now(void)
{
  using namespace std::chrono;

  system_clock::time_point tp = system_clock::now();
  system_clock::duration dtn = tp.time_since_epoch();

  return duration_cast<seconds>(dtn).count();
}

static std::string get_url_time(int timestamp)
{
    using namespace boost::posix_time;
    using namespace boost::gregorian;

    std::ostringstream oss;

    boost::posix_time::ptime t(
         date(1970, Jan, 1),  // The epoch.
         boost::posix_time::seconds(timestamp) + // Number of seconds.
         boost::posix_time::microseconds(0)  // And the micros too.
    );

    int year   = t.date().year();
    int month  = t.date().month();
    int day    = t.date().day();
    int hour   = t.time_of_day().hours();
    int minute = t.time_of_day().minutes();

    oss << t.date().year() << '-';

    if (month < 10) oss << '0';
    oss << month << '-';
    if (day < 10) oss << '0';
    oss << day << '+';
    if (hour < 10) oss << '0';
    oss << hour << "%3A";
    if (minute < 10) oss << '0';
    oss << minute;

    return oss.str();
}

static std::string timestamp2str(int timestamp)
{
    using namespace boost::posix_time;
    using namespace boost::gregorian;

    boost::posix_time::ptime t(
         date(1970, Jan, 1),  // The epoch.
         boost::posix_time::seconds(timestamp) + // Number of seconds.
         boost::posix_time::microseconds(0)  // And the micros too.
    );

    return to_simple_string(t);
}

void retrieve_candles_from_bitmex(downloader &d, const std::string &uri,
                                      int amount, int end_time_stamp)
{
    if (amount > BITMEX_CHUNK_SIZE)
    {
        LOGERROR << "retrieve_candles_from_bitmex: Limit 750 exceeded\n";

        throw std::runtime_error("retrieve_candles_from_bitmex: Limit 750 exceeded");
    }

    //
    // Assembly URL.
    // Example one:
    // https://www.bitmex.com/api/v1/trade/bucketed?symbol=XBTUSD&count=7&binSize=1m&partial=false&reverse=true&endTime=2019-11-11+19%3A29
    //

    std::ostringstream url_stream;

    url_stream << uri << "?symbol=XBTUSD&count=" << amount
               << "&binSize=1m&partial=false&reverse=true&endTime="
               << get_url_time(end_time_stamp);

    d.download_data(url_stream.str(), "", {"Connection: Keep-Alive", "Keep-Alive: 90"});
}

int make_timestamp_from_timestr(const char *timestr)
{
    using namespace boost::posix_time;
    using namespace boost::gregorian;

    std::string t(timestr);

    for (int i = 0; i < t.length(); i++)
    {
        if ((t[i] < '0' || t[i] > '9') && t[i] != '.' && t[i] != '-' && t[i] != ':')
        {
            t[i] = ' ';
        }
    }

    ptime pt;

    try
    {
        pt = time_from_string(boost::trim_right_copy(t));
    }
    catch (const std::exception &e)
    {
        std::string msg = std::string("Error with parsing time: `")
                          + boost::trim_right_copy(t)
                          + std::string("'");

        LOGERROR << msg << "\n";

        throw std::runtime_error(msg);
    }

    ptime epoch(date(1970, Jan, 1));
    time_duration d = pt - epoch;

    return d.total_seconds();
}

static void json_to_candles_container(const std::string &data,
                                          db::history_candles_container &hcs)
{
    hcs.clear();

    cJSON *json = cJSON_Parse(data.c_str());

    if (json == NULL)
    {
        std::string msg = std::string("Invalid json data from BitMex remote server:\n") + data;

        LOGERROR << msg << "\n";

        throw std::runtime_error(msg.c_str());
    }

    if (json -> type != cJSON_Array)
    {
        std::string msg = std::string("Data from BitMex remote server expected to be array, shit has given:\n") + data;

        LOGERROR << msg << "\n";

        throw std::runtime_error(msg.c_str());
    }

    cJSON *candle = json -> child;
    cJSON *time_json, *bid_json;

    const char *timestamp;
    double bid;

    while (candle)
    {
        // Operacja na elemencie tablicy 'candle';
        time_json = cJSON_GetObjectItem(candle, "timestamp");

        if (time_json == nullptr)
        {
            LOGERROR << "No `timestamp' field in json data\n";

            throw std::runtime_error("No `timestamp' field in json data");
        }
        else if (time_json -> type != cJSON_String)
        {
            LOGERROR << "Filed `timestamp' is expected to be type string\n";

            throw std::runtime_error("Filed `timestamp' is expected to be type string");
        }

        timestamp = time_json -> valuestring;

        bid_json = cJSON_GetObjectItem(candle, "close");

        if (bid_json == nullptr)
        {
            LOGERROR << "No `close' field in json data\n";

            throw std::runtime_error("No `close' field in json data");
        }
        else if (bid_json -> type != cJSON_Number)
        {
            LOGERROR << "Filed `close' is expected to be type numeric\n";

            throw std::runtime_error("Filed `close' is expected to be type numeric");
        }

        bid = bid_json -> valuedouble;

        hcs.push_back(db::history_candle_record(make_timestamp_from_timestr(timestamp), bid));

        candle = candle -> next;
    }

    cJSON_Delete(json);
}

void proceed(mode_operation mode, const appconfig &config, bool guimode)
{
    auto_suspend_resume switch_mode(guimode);
    file_auto_cleaner file_cleaner;

    std::string bitmex_uri;

    LOG << "Got called for mode [";
    switch (mode)
    {
        case PRODUCTION:
            std::cout << "Production]\n";
            bitmex_uri = "https://www.bitmex.com/api/v1/trade/bucketed";
            break;
        case TESTNET:
            std::cout << "Testnet]\n";
            bitmex_uri = "https://testnet.bitmex.com/api/v1/trade/bucketed";
            break;
        default:
            throw std::runtime_error("generator: Invalid mode");
    }

    //
    // What time is it now (in UTC)?
    //

    const int now = (universal_time_now() / 60) * 60 - 60;

    //
    // At this point we should check how many historical
    // data have to be retrieved from BitMex remote server.
    //

    int t = db::get_last_history_record_time(mode);

    if (t == 0)
    {
        //
        // Database is empty. Have to download
        // full data for hftr generation.
        //

        gni.start_timestamp = now - 60 * config.records_per_hftr;
    }
    else
    {
        LOG << "Last record timestamp: [" << t << "], i.e. "
            << timestamp2str(t) << " UTC.\n";

        gni.start_timestamp = t;
    }

    LOG << "Now is [" << now << "], i.e. "
        << timestamp2str(now) << " UTC.\n";

    //
    // How many candles we have to retrieve?
    //

    const int total_num_candles = (now - gni.start_timestamp) / 60;

    int chunks = total_num_candles / BITMEX_CHUNK_SIZE;
    int remainder = total_num_candles % BITMEX_CHUNK_SIZE;

    downloader::init_globals();
    downloader bitmex_retriever;
    db::history_candles_container hcs;
    uint64_t start_operation, end_operation;
    int time_to_sleep;

    int end = gni.start_timestamp;
    while (true)
    {
        start_operation = universal_time_now_millis();

        if (chunks > 0)
        {
            end += 60 * BITMEX_CHUNK_SIZE;
            // Download.

            LOG << "Downloading candles " << BITMEX_CHUNK_SIZE << " of "
                << chunks*BITMEX_CHUNK_SIZE + remainder << " from "
                << bitmex_uri << "\n";

            retrieve_candles_from_bitmex(bitmex_retriever, bitmex_uri, BITMEX_CHUNK_SIZE, end);
            --chunks;
        }
        else if (remainder > 0)
        {
            end += 60 * remainder;
            // Download.

            LOG << "Downloading candles " << remainder << " of " << remainder
                << " from " << bitmex_uri << "\n";

            retrieve_candles_from_bitmex(bitmex_retriever, bitmex_uri, remainder, end);
            remainder = 0;
        }
        else
        {
            LOG << "Download complete.\n";

            break;
        }

        LOG << "Parsing json...\n";

        json_to_candles_container(bitmex_retriever.get_buffer(), hcs);

        LOG << "Writing to database " << hcs.size() << " rows...\n";

        db::write_history_records(mode, hcs);

        end_operation = universal_time_now_millis();
        time_to_sleep = 2000 - (int)(end_operation - start_operation);

        if (time_to_sleep > 0)
        {
            LOG << "Sleeping " << time_to_sleep << " milliseconds.\n";
            usleep(time_to_sleep * 1000);
        }
    }

    LOG << "Retrieving last " << config.records_per_hftr
        << " records from database...\n";

    db::get_last_history_records(mode, config.records_per_hftr, hcs);

    if (hcs.empty())
    {
        LOGERROR << "PANIC! No data received from db.\n";

        throw std::runtime_error("No data received from database!");
    }

    LOG << "Received " << hcs.size() << " records.\n";
    LOG << "Now verifying data consistency...\n";

    int prev = 0;
    for (auto &item : hcs)
    {
        if (prev != 0 && (item.datetime - prev) != 60)
        {
            LOGERROR << "Bad data: prev [" << prev << "], current ["
                     << item.datetime << "].\n";

            throw std::runtime_error("Invalid data in database!");
        }

        prev = item.datetime;
    }

    LOG << "Data correct.\n";

    gni.start_timestamp = hcs.begin() -> datetime;
    gni.end_timestamp   = hcs.rbegin() -> datetime;

    LOG << "Sample for neural network training starts at [" << timestamp2str(gni.start_timestamp)
        << " UTC] ends at [" << timestamp2str(gni.end_timestamp) << " UTC].\n";

    //
    // Generate CSV.
    //

    std::string csv_filename = file_cleaner.register_file(tmp_file(".csv"));
    std::ofstream csv_stream;
    csv_stream.open(csv_filename.c_str(), std::ofstream::out);

    if (csv_stream.fail())
    {
        std::string msg = std::string("Failed to create file: ")
                          + csv_filename;

        LOGERROR << msg << "\n";

        throw std::runtime_error(msg);
    }

    LOG << "Writing CSV data into temporary intermediate file: "
        << csv_filename << "\n";

    csv_stream << "Local time,Ask,Bid,AskVolume,BidVolume\r\n";

    double bid;

    for (auto &item : hcs)
    {
        bid = course_convert(item.bid);
        csv_stream << "01.01.2016 00:00:00.000 GMT+0200,"
                   << bid << ',' << bid << ",1,1\r\n";
    }

    csv_stream.close();

    //
    // Generate approximation file.
    //

    std::string approx_data = 
        "{\n"
        "\t\"n\" : 383950,\n"
        "\t\"distribution\" :\n"
        "\t{\n"
        "\t\t\"1\" : 191977.499924,\n"
        "\t\t\"2\" : 191983.499742,\n"
        "\t\t\"3\" : 191988.999708,\n"
        "\t\t\"4\" : 191993.999604\n"
        "\t}\n"
        "}\n";

    std::string json_filename = file_cleaner.register_file(tmp_file(".json"));
    std::ofstream json_stream;
    json_stream.open(json_filename.c_str(), std::ofstream::out);

    if (json_stream.fail())
    {
        std::string msg = std::string("Failed to create file: ")
                           + json_filename;

        LOGERROR << msg << "\n";

        throw std::runtime_error(msg);
    }

    json_stream << approx_data;
    json_stream.close();

    //
    // Generate hftr.
    //

    std::string hftr_file_name = file_cleaner.register_file(tmp_file(".hftr"));

    LOG << "Generating hftr " << hftr_file_name << "...\n";

    start_program(config.hft_program_path,
                    {
                        "hftr-generator",
                        "-i",
                        csv_filename,
                        "-o",
                        hftr_file_name,
                        "-a",
                        json_filename,
                        "-u",
                        std::to_string(config.trade_setup_target),
                        "-d",
                        std::to_string(config.trade_setup_target),
                        "-O",
                        std::to_string(config.hftr_offset),
                        "-C",
                        std::to_string(config.market_collector_size)
                    });

    //
    // Generate neural network.
    //

    std::string network_file_name = file_cleaner.register_file(tmp_file(".json"));
    std::string output;

    while (true)
    {
        //
        // Train network.
        //

        start_program(config.hft_program_path,
                        {
                            "ai-trainer",
                            "-f",
                            network_file_name,
                            "-t",
                            hftr_file_name,
                            "-a",
                            config.network_arch,
                            "-s",
                            std::to_string(config.learn_coeeficient) + std::string(":10"),
                            "-C",
                            std::to_string(config.market_collector_size)
                        });
        //
        // Check.
        //

        output = start_program_ret_output(config.hft_program_path,
                                            {
                                                "ai-trainer",
                                                "-f",
                                                network_file_name,
                                                "-t",
                                                hftr_file_name,
                                                "-a",
                                                config.network_arch,
                                                "-s",
                                                "0:1",
                                                "-C",
                                                std::to_string(config.market_collector_size),
                                                "-j",
                                                "1"
                                            });

        output = boost::trim_copy(output);

        std::size_t index = output.find("JSON\t");

        if (index == std::string::npos)
        {
            std::string msg = std::string("Failed to parse output of ")
                              + config.hft_program_path;

            LOGERROR << msg << "\n";
            LOGERROR << output << "\n";

            throw std::runtime_error(msg);
        }

        index += 5; // += strlen("JSON\t")
        output = output.substr(index, output.length() - index);

        cJSON *json = cJSON_Parse(output.c_str());

        if (json == NULL)
        {
            std::string msg = std::string("Invalid json data from HFT:\n") + output;

            LOGERROR << msg << "\n";

            throw std::runtime_error(msg.c_str());
        }

        cJSON *average_error_json = cJSON_GetObjectItem(json, "average_absolute_error");

        if (average_error_json == nullptr)
        {
            LOGERROR << "No `average_error_json' field in json data\n";

            throw std::runtime_error("No `average_error_json' field in json data");
        }
        else if (average_error_json -> type != cJSON_Number)
        {
            LOGERROR << "Filed `average_error_json' is expected to be type numeric\n";

            throw std::runtime_error("Filed `average_error_json' is expected to be type numeric");
        }

        double average_error = average_error_json -> valuedouble;

        cJSON_Delete(json);

        if (average_error < config.train_goal)
        {
            LOG << "Effective train error: [" << average_error << "] - sufficient.\n";

            break;
        }
        else
        {
            LOG << "Effective train error: [" << average_error << "], continue learning.\n";
        }
    }

    //
    // Loading network content.
    //

    LOG << "Loading generated file: " << network_file_name << "\n";

    std::string network_content = file_get_contents(network_file_name);

    LOG << "Writing network to database.\n";

    db::write_network(mode, gni.start_timestamp, gni.end_timestamp,
                          network_content, config.network_arch);

    LOG << "*** Complete.\n";
}

} /* namespace generator */
} /* namespace bdm */
