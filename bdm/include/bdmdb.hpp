/**********************************************************************\
**                                                                    **
**                 -=≡≣ BitMex Data Manager  ≣≡=-                    **
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

#ifndef __BDM_BDMDB_HPP__
#define __BDM_BDMDB_HPP__

#include <appconfig.hpp>
#include <vector>

namespace bdm {
namespace db {

typedef struct _history_candle_record
{
    _history_candle_record(int d, double b)
        : datetime(d), bid(b) {}

    int datetime;
    double bid;

} history_candle_record;

typedef std::vector<history_candle_record> history_candles_container;

typedef struct _network_record
{
    _network_record(int i, int st, int et, const std::string &net, const std::string &arch)
        : id(i), start_time(st), ent_time(et), network(net), architecture(arch) {}

    const int id;
    int start_time;
    int ent_time;
    std::string network;
    std::string architecture;

} network_record;

typedef std::vector<network_record> networks_container;

void initialize_database(const std::string &file_name);
void create_database(void);
void load_config(appconfig &cfg);
void save_config(const appconfig &cfg);
int get_last_history_record_time(mode_operation mode);
void get_last_history_records(mode_operation mode, int amount,
                                  history_candles_container &result);
void write_history_records(mode_operation mode, const history_candles_container &data);
void write_network(mode_operation mode, int start_time, int ent_time,
                       const std::string &network, const std::string architecture);
void get_networks(mode_operation mode, networks_container &result);
network_record get_last_network(mode_operation mode);
void delete_network(mode_operation mode, const network_record &network_data);

} /* namespace db */
} /* namespace bdm */

#endif /* __BDM_BDMDB_HPP__ */
