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

#include <memory>
#include <sstream>
#include <stdexcept>

#include <sqlite3pp/sqlite3pp.h>
#include <sqlite3pp/sqlite3ppext.h>

#include <bdmdb.hpp>

#define TEST

#ifdef TEST
#include <iostream>
#endif

namespace bdm {
namespace db {

static std::shared_ptr<sqlite3pp::database> database_ptr;

void initialize_database(const std::string &file_name)
{
    database_ptr = std::make_shared<sqlite3pp::database>(file_name.c_str());
}

void create_database(void)
{
    if (database_ptr.use_count() == 0)
    {
        throw std::runtime_error("create_database: Database uninitialized");
    }

    database_ptr -> execute(
        "CREATE TABLE tb_history_testnet ("
        "    datetime INTEGER PRIMARY KEY,"
        "    bid REAL"
        ");"
    );

    database_ptr -> execute(
        "CREATE TABLE tb_history_prod ("
        "    datetime INTEGER PRIMARY KEY,"
        "    bid REAL"
        ");"
    );

    database_ptr -> execute(
        "CREATE TABLE  tb_testnet_networks ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    start_time INTEGER,"
        "    ent_time INTEGER,"
        "    network TEXT,"
        "    architecture TEXT"
        ");"
    );

    database_ptr -> execute(
        "CREATE TABLE  tb_prod_networks ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    start_time INTEGER,"
        "    ent_time INTEGER,"
        "    network TEXT,"
        "    architecture TEXT"
        ");"
    );

    database_ptr -> execute(
        "CREATE TABLE tb_approximator ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    added TEXT,"
        "    deleted INTEGER,"
        "    content TEXT"
        ");"
    );

    database_ptr -> execute(
        "CREATE TABLE tb_configuration ("
        "    id INTEGER PRIMARY KEY,"
        "    trade_setup_target INTEGER,"
        "    market_collector_size INTEGER,"
        "    hftr_offset INTEGER,"
        "    hft_program_path TEXT,"
        "    records_per_hftr INTEGER,"
        "    architecture TEXT,"
        "    learn_coeff REAL,"
        "    train_goal REAL"
        ");"
    );

    //
    // Write dummy configuration.
    //

    sqlite3pp::command cmd(*database_ptr,
        "INSERT INTO tb_configuration ("
        "    id,"
        "    trade_setup_target,"
        "    market_collector_size,"
        "    hftr_offset,"
        "    hft_program_path,"
        "    records_per_hftr,"
        "    architecture,"
        "    learn_coeff,"
        "    train_goal"
        ") VALUES ("
        "    :id,"
        "    :trade_setup_target,"
        "    :market_collector_size,"
        "    :hftr_offset,"
        "    :hft_program_path,"
        "    :records_per_hftr,"
        "    :architecture,"
        "    :learn_coeff,"
        "    :train_goal"
        ");"
    );
    cmd.bind(":id", "1", sqlite3pp::nocopy);
    cmd.bind(":trade_setup_target", "10", sqlite3pp::nocopy);
    cmd.bind(":market_collector_size", "220", sqlite3pp::nocopy);
    cmd.bind(":hftr_offset", "100", sqlite3pp::nocopy);
    cmd.bind(":hft_program_path", "", sqlite3pp::nocopy);
    cmd.bind(":records_per_hftr", "0", sqlite3pp::nocopy);
    cmd.bind(":architecture", "", sqlite3pp::nocopy);
    cmd.bind(":learn_coeff", "0", sqlite3pp::nocopy);
    cmd.bind(":train_goal", "0", sqlite3pp::nocopy);
    cmd.execute();
}

void load_config(appconfig &cfg)
{
    sqlite3pp::query qry(*database_ptr,
        "SELECT"
        "    `trade_setup_target`,"
        "    `market_collector_size`,"
        "    `hftr_offset`,"
        "    `hft_program_path`,"
        "    `records_per_hftr`,"
        "    `architecture`,"
        "    `learn_coeff`,"
        "    `train_goal`"
        "FROM tb_configuration WHERE id=1;"
    );

    sqlite3pp::query::iterator row = qry.begin();

    if (row == qry.end())
    {
        throw std::runtime_error("load_config: No configuration in databse");
    }

    if (qry.column_count() != 8)
    {
        throw std::runtime_error("load_config: Bad config tuple");
    }

    cfg.trade_setup_target    = (*row).get<int>(0);
    cfg.market_collector_size = (*row).get<int>(1);
    cfg.hftr_offset           = (*row).get<int>(2);
    cfg.hft_program_path      = (*row).get<const char *>(3);
    cfg.records_per_hftr      = (*row).get<int>(4);
    cfg.network_arch          = (*row).get<const char *>(5);
    cfg.learn_coeeficient     = (*row).get<double>(6);
    cfg.train_goal            = (*row).get<double>(7);
}

void save_config(const appconfig &cfg)
{
    sqlite3pp::command cmd(*database_ptr,
        "UPDATE tb_configuration SET"
        "    `trade_setup_target`=:trade_setup_target,"
        "    `market_collector_size`=:market_collector_size,"
        "    `hftr_offset`=:hftr_offset,"
        "    `hft_program_path`=:hft_program_path,"
        "    `records_per_hftr`=:records_per_hftr,"
        "    `architecture`=:architecture,"
        "    `learn_coeff`=:learn_coeff,"
        "    `train_goal`=:train_goal"
        " WHERE id=1;"
    );

    std::string s1 = std::to_string(cfg.trade_setup_target);
    std::string s2 = std::to_string(cfg.market_collector_size);
    std::string s3 = std::to_string(cfg.hftr_offset);
    std::string s4 = std::to_string(cfg.records_per_hftr);
    std::string s5 = std::to_string(cfg.learn_coeeficient);
    std::string s6 = std::to_string(cfg.train_goal);

    cmd.bind(":trade_setup_target", s1.c_str(), sqlite3pp::nocopy);
    cmd.bind(":market_collector_size", s2.c_str(), sqlite3pp::nocopy);
    cmd.bind(":hftr_offset", s3.c_str(), sqlite3pp::nocopy);
    cmd.bind(":hft_program_path", cfg.hft_program_path.c_str(), sqlite3pp::nocopy);
    cmd.bind(":records_per_hftr", s4.c_str(), sqlite3pp::nocopy);
    cmd.bind(":architecture", cfg.network_arch.c_str(), sqlite3pp::nocopy);
    cmd.bind(":learn_coeff", s5.c_str(), sqlite3pp::nocopy);
    cmd.bind(":train_goal", s6.c_str(), sqlite3pp::nocopy);
    cmd.execute();
}

int get_last_history_record_time(mode_operation mode)
{
    std::string query;

    if (mode == PRODUCTION)
    {
        query = "SELECT MAX(datetime) FROM tb_history_prod;";
    }
    else if (mode == TESTNET)
    {
        query = "SELECT MAX(datetime) FROM tb_history_testnet;";
    }
    else
    {
        throw std::runtime_error("get_last_history_record_time: Illegal mode operation");
    }

    sqlite3pp::query qry(*database_ptr, query.c_str());

    sqlite3pp::query::iterator row = qry.begin();

    if (row == qry.end())
    {
        // Table is empty.
        return 0;
    }

    return (*row).get<int>(0);
}

void get_last_history_records(mode_operation mode, int amount,
                                  history_candles_container &result)
{
    // Pobieranie N ostatnich świec (w tym przypadku 5)
    // select * from (select * from tb_history_prod order by datetime desc limit 5) order by datetime asc;

    std::string query;
    std::ostringstream oss;

    if (mode == PRODUCTION)
    {
        oss << "SELECT `datetime`, `bid` FROM (SELECT `datetime`, `bid` FROM tb_history_prod ORDER BY `datetime` desc LIMIT "
            << amount << ") ORDER BY `datetime` asc;";
    }
    else if (mode == TESTNET)
    {
        oss << "SELECT `datetime`, `bid` FROM (SELECT `datetime`, `bid` FROM tb_history_testnet ORDER BY `datetime` desc LIMIT "
            << amount << ") ORDER BY `datetime` asc;";
    }
    else
    {
        throw std::runtime_error("get_last_history_records: Illegal mode operation");
    }

    query = oss.str();

    result.clear();
    result.reserve(amount);

    sqlite3pp::query qry(*database_ptr, query.c_str());

    sqlite3pp::query::iterator row = qry.begin();

    while (row != qry.end())
    {
        result.push_back(history_candle_record((*row).get<int>(0), (*row).get<double>(1)));
        ++row;
    }
}

void write_history_records(mode_operation mode, const history_candles_container &data)
{
    std::ostringstream oss;
    std::string table_name;

    if (mode == PRODUCTION)
    {
        table_name = "tb_history_prod";
    }
    else if (mode == TESTNET)
    {
        table_name = "tb_history_testnet";
    }
    else
    {
        throw std::runtime_error("write_history_records: Illegal mode operation");
    }

    oss << "BEGIN TRANSACTION;\n";

    for (auto &record : data)
    {
        oss << "INSERT INTO " << table_name << " (";
        oss << "  `datetime`,";
        oss << "  `bid`";
        oss << " ) VALUES ( ";
        oss << record.datetime << ", " << record.bid << ");\n";
    }

    oss << "COMMIT;";

    if (database_ptr -> execute(oss.str().c_str()) != 0)
    {
        std::string msg = std::string("Database returned error :") + std::string(database_ptr -> error_msg());

        database_ptr -> execute("ROLLBACK;");

        #ifdef TEST
        std::cerr << oss.str() << "\n";
        #endif

        throw std::runtime_error(msg);
    }
}

void write_network(mode_operation mode, int start_time, int ent_time,
                       const std::string &network, const std::string architecture)
{
    std::ostringstream oss;
    std::string table_name;

    if (mode == PRODUCTION)
    {
        table_name = "tb_prod_networks";
    }
    else if (mode == TESTNET)
    {
        table_name = "tb_testnet_networks";
    }
    else
    {
        throw std::runtime_error("write_network: Illegal mode operation");
    }

    oss << "INSERT INTO " << table_name << " (";
    oss << "    `start_time`,";
    oss << "    `ent_time`,";
    oss << "    `network`,";
    oss << "    `architecture`";
    oss << " ) VALUES ( ";
    oss << "    " << start_time << ",";
    oss << "    " << ent_time << ",";
    oss << "    :network,";
    oss << "    :architecture";
    oss << " );";

    sqlite3pp::command cmd(*database_ptr, oss.str().c_str());

    cmd.bind(":network", network.c_str(), sqlite3pp::nocopy);
    cmd.bind(":architecture", architecture.c_str(), sqlite3pp::nocopy);

    if (cmd.execute() != 0)
    {
        throw std::runtime_error(std::string("Database returned error :") + std::string(database_ptr -> error_msg()));
    }
}

void get_networks(mode_operation mode, networks_container &result)
{
    std::string query;

    if (mode == PRODUCTION)
    {
        query = "SELECT `id`, `start_time`, `ent_time`, `network`, `architecture` "
                "FROM tb_prod_networks ORDER BY `ent_time` DESC;";
    }
    else if (mode == TESTNET)
    {
        query = "SELECT `id`, `start_time`, `ent_time`, `network`, `architecture` "
                "FROM tb_testnet_networks ORDER BY `ent_time` DESC;";
    }
    else
    {
        throw std::runtime_error("get_networks: Illegal mode operation");
    }

    sqlite3pp::query qry(*database_ptr, query.c_str());

    result.clear();

    sqlite3pp::query::iterator row = qry.begin();

    while (row != qry.end())
    {
        result.push_back(network_record((*row).get<int>(0), (*row).get<int>(1), (*row).get<int>(2), (*row).get<const char *>(3), (*row).get<const char *>(4) ));
        ++row;
    }
}

network_record get_last_network(mode_operation mode)
{
    std::string query;

    if (mode == PRODUCTION)
    {
        query = "SELECT `id`, `start_time`, `ent_time`, `network`, `architecture` "
                "FROM tb_prod_networks ORDER BY `ent_time` DESC LIMIT 1;";
    }
    else if (mode == TESTNET)
    {
        query = "SELECT `id`, `start_time`, `ent_time`, `network`, `architecture` "
                "FROM tb_testnet_networks ORDER BY `ent_time` DESC LIMIT 1;";
    }
    else
    {
        throw std::runtime_error("get_last_network: Illegal mode operation");
    }

    sqlite3pp::query qry(*database_ptr, query.c_str());
    sqlite3pp::query::iterator row = qry.begin();

    return network_record((*row).get<int>(0), (*row).get<int>(1), (*row).get<int>(2), (*row).get<const char *>(3), (*row).get<const char *>(4) );
}

void delete_network(mode_operation mode, const network_record &network_data)
{
    std::ostringstream oss;

    if (mode == PRODUCTION)
    {
        oss << "DELETE FROM tb_prod_networks WHERE id=" << network_data.id << ";";
    }
    else if (mode == TESTNET)
    {
        oss << "DELETE FROM tb_testnet_networks WHERE id=" << network_data.id << ";";
    }
    else
    {
        throw std::runtime_error("delete_network: Illegal mode operation");
    }

    sqlite3pp::command cmd(*database_ptr, oss.str().c_str());

    if (cmd.execute() != 0)
    {
        throw std::runtime_error(std::string("Database returned error :") + std::string(database_ptr -> error_msg()));
    }
}

} /* namespace db */
} /* namespace bdm */
