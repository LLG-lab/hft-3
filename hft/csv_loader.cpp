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
 
#include <csv_loader.hpp>
#include <regex.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>

#include <sstream>

typedef boost::tokenizer<boost::char_separator<char> >  tokenizer;

static const boost::regex header_regex1("^\\s*Local\\stime\\s*,Ask\\s*,Bid\\s*,AskVolume\\s*,BidVolume\\s*$");
static const boost::regex header_regex2("^\\s*Gmt\\stime\\s*,Ask\\s*,Bid\\s*,AskVolume\\s*,BidVolume\\s*$"); //Gmt time,Ask,Bid,AskVolume,BidVolume

//
// Old dukascopy datetime format, example: 01.05.2017 23:00:00.095
// New dukascopy datetime format, example: 01.12.2017 00:00:00.192 GMT+0100
//

static const boost::regex datetime_regex("^\\d{2}\\.\\d{2}\\.\\d{4} \\d{2}:\\d{2}:\\d{2}\\.\\d{3}(\\sGMT\\+\\d{4})?$");

csv_loader::csv_loader(const std::string &file_name)
{
    load(file_name);
}

csv_loader::~csv_loader(void)
{
    if (infile_.is_open())
    {
        infile_.close();
    }
}

void csv_loader::load(std::string file_name)
{
    if (infile_.is_open())
    {
        infile_.close();
    }

    infile_.open(file_name.c_str(), std::ifstream::in);

    if (!infile_.is_open())
    {
        std::ostringstream error_msg;

        error_msg << "Unable to open file: „" << file_name << "”";

        throw csv_exception(error_msg.str());
    }
}

bool csv_loader::get_record(csv_loader::csv_record &out_rec)
{
    std::string line;
    boost::match_results<std::string::const_iterator> results;

    do
    {
        if (! std::getline(infile_, line))
        {
            return false;
        }
    } while (boost::regex_match(line, results, header_regex1) || boost::regex_match(line, results, header_regex2));

    boost::char_separator<char> sep(",\r", "", boost::drop_empty_tokens);
    tokenizer tokens(line, sep);

    std::vector<std::string> columns;

    for (auto tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter)
    {
        columns.push_back(*tok_iter);
    }

    if (columns.size() != CSV_TOTAL_COLUMNS)
    {
        std::ostringstream error_msg;

        error_msg << "Column number missmatch. Detected "
                  << columns.size() << " columns, should be "
                  << CSV_TOTAL_COLUMNS << " in line „"
                  << line << "”.";

        throw csv_exception(error_msg.str());
    }

    out_rec.ask = boost::lexical_cast<double>(columns[CSV_ASK]);
    out_rec.bid = boost::lexical_cast<double>(columns[CSV_BID]);
    out_rec.ask_volume = boost::lexical_cast<double>(columns[CSV_ASK_VOLUME]);
    out_rec.bid_volume = boost::lexical_cast<double>(columns[CSV_BID_VOLUME]);

    if (! boost::regex_match(columns[CSV_DATE], results, datetime_regex))
    {
        std::ostringstream error_msg;

        error_msg << "Date and time in first colum mismatch with pattern "
                  << "in line „" << line << "”: \"" << columns[CSV_DATE]
                  << "\".";

        throw csv_exception(error_msg.str());
    }

    boost::char_separator<char> datetime_sep(" .:", "", boost::drop_empty_tokens);
    tokenizer datetime_tokens(columns[CSV_DATE], datetime_sep);
    std::vector<std::string> datetime_items;

    for (auto tok_iter = datetime_tokens.begin(); tok_iter != datetime_tokens.end(); ++tok_iter)
    {
        datetime_items.push_back(*tok_iter);
    }

    if (datetime_items.size() != CSV_DATETIME_TOTAL_ITEMS &&
            datetime_items.size() != CSV_DATETIME_TOTAL_ITEMS+1)
    {
        std::ostringstream error_msg;

        error_msg << "Datetime items missmatch. Detected "
                  << datetime_items.size() << ", should be "
                  << CSV_DATETIME_TOTAL_ITEMS  << " or "
                  << CSV_DATETIME_TOTAL_ITEMS + 1 << " in line „"
                  << line << "”.";

        throw csv_exception(error_msg.str());
    }

    int day    = boost::lexical_cast<int>(datetime_items[CSV_DATETIME_DAY]);
    int month  = boost::lexical_cast<int>(datetime_items[CSV_DATETIME_MONTH]);
    int year   = boost::lexical_cast<int>(datetime_items[CSV_DATETIME_YEAR]);
    int hour   = boost::lexical_cast<int>(datetime_items[CSV_DATETIME_HOUR]);
    int minute = boost::lexical_cast<int>(datetime_items[CSV_DATETIME_MINUTE]);
    int second = boost::lexical_cast<int>(datetime_items[CSV_DATETIME_SECOND]);
    int millis = boost::lexical_cast<int>(datetime_items[CSV_DATETIME_MILLIS]);

//    time_t rawtime;
//    struct tm timeinfo;

//    time(&rawtime);
//    timeinfo = *localtime(&rawtime);

//    timeinfo.tm_year = year - 1900;
//    timeinfo.tm_mon  = csv_loader::validate_range("month", month - 1, 0, 11);  //months since January - [0,11]
//    timeinfo.tm_mday = csv_loader::validate_range("day", day, 1, 31);          //day of the month - [1,31] 
//    timeinfo.tm_hour = csv_loader::validate_range("hour", hour, 0, 23);        //hours since midnight - [0,23]
//    timeinfo.tm_min  = csv_loader::validate_range("minute", minute, 0, 59);    //minutes after the hour - [0,59]
//    timeinfo.tm_sec  = csv_loader::validate_range("second", second, 0, 59);

//    int tstamp = mktime(&timeinfo);

//    out_rec.milli_timestamp = tstamp;
//    out_rec.milli_timestamp *= 1000;
//    out_rec.milli_timestamp += millis;

    csv_loader::validate_range("year", year, 2000, 2100); // At least try to eliminate „exotic” years.
    csv_loader::validate_range("month", month, 1, 12);    // months since January - [1,12]
    csv_loader::validate_range("day", day, 1, 31);        // day of the month - [1,31] 
    csv_loader::validate_range("hour", hour, 0, 23);      // hours since midnight - [0,23]
    csv_loader::validate_range("minute", minute, 0, 59);  // minutes after the hour - [0,59]
    csv_loader::validate_range("second", second, 0, 59);  // seconds after the minute - [0,59]

    std::ostringstream req_time;

    req_time << year << '-';

    if (month < 10)
    {
        req_time << '0';
    }

    req_time << month << '-';

    if (day < 10)
    {
        req_time << '0';
    }

    req_time << day << ' ';

    if (hour < 10)
    {
        req_time << '0';
    }

    req_time << hour << ':';

    if (minute < 10)
    {
        req_time << '0';
    }

    req_time << minute << ':';

    if (second < 10)
    {
        req_time << '0';
    }

    req_time << second << ".000";

    out_rec.request_time = req_time.str();

    return true;
}

long csv_loader::get_record_position(void)
{
    return infile_.tellg();
}

void csv_loader::set_record_position(long position)
{
    infile_.clear();
    infile_.seekg(position);
}

int csv_loader::validate_range(const char *topic, int value, int min, int max)
{
    if (value >= min && value <= max)
    {
        return value;
    }

    std::ostringstream error_msg;

    error_msg << "Value " << value << " of „"
              << topic << "” out of range: ["
              << min << ", " << max << "]";

    throw csv_exception(error_msg.str());
}

