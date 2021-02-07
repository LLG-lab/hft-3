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

#include <hft_utils.hpp>

#include <cstdio>
#include <ctime>
#include <stdexcept>

#include <boost/lexical_cast.hpp>

namespace hft {

static struct instrument_info_type
{
    const char *identifiers[4];
    char dpips_transform;
    const char *description;

} instrument_info[] =
{
    // Chosen currency pairs.
    { { "AUDUSD",          "AUD/USD",          nullptr }, '5', "Australian Dollar vs US Dollar" },
    { { "EURUSD",          "EUR/USD",          nullptr }, '5', "Euro vs US Dollar" },
    { { "GBPUSD",          "GBP/USD",          nullptr }, '5', "Pound Sterling vs US Dollar" },
    { { "NZDUSD",          "NZD/USD",          nullptr }, '5', "New Zealand Dollar vs US Dollar" },
    { { "USDCAD",          "USD/CAD",          nullptr }, '5', "US Dollar vs Canadian Dollar" },
    { { "USDCHF",          "USD/CHF",          nullptr }, '5', "US Dollar vs Swiss Franc" },
    { { "USDJPY",          "USD/JPY",          nullptr }, '3', "US Dollar vs Japanese Yen" },
    { { "USDCNH",          "USD/CNH",          nullptr }, '5', "US Dollar vs Offshore Chinese Renminbi" },
    { { "USDCZK",          "USD/CZK",          nullptr }, '4', "US Dollar vs Czech Koruna" },
    { { "USDDKK",          "USD/DKK",          nullptr }, '5', "US Dollar vs Danish Krone" },
    { { "USDHKD",          "USD/HKD",          nullptr }, '5', "US Dollar vs Hong Kong Dollar" },
    { { "USDHUF",          "USD/HUF",          nullptr }, '3', "US Dollar vs Hungarian Forint" },
    { { "USDILS",          "USD/ILS",          nullptr }, '5', "US Dollar vs Israeli Shekel" },
    { { "USDMXN",          "USD/MXN",          nullptr }, '5', "US Dollar vs Mexican Peso" },
    { { "USDNOK",          "USD/NOK",          nullptr }, '5', "US Dollar vs Norwegian Krone" },
    { { "USDPLN",          "USD/PLN",          nullptr }, '5', "US Dollar vs Polish Zloty" },
    { { "USDRON",          "USD/RON",          nullptr }, '4', "US Dollar vs Romanian Leu" },
    { { "USDRUB",          "USD/RUB",          nullptr }, '5', "US Dollar vs Russian Ruble" },
    { { "USDSEK",          "USD/SEK",          nullptr }, '5', "US Dollar vs Swedish Krona" },
    { { "USDSGD",          "USD/SGD",          nullptr }, '5', "US Dollar vs Singapore Dollar" },
    { { "USDTHB",          "USD/THB",          nullptr }, '4', "US Dollar vs Thai Baht" },
    { { "USDTRY",          "USD/TRY",          nullptr }, '5', "US Dollar vs Turkish Lira" },
    { { "USDZAR",          "USD/ZAR",          nullptr }, '5', "US Dollar vs South African Rand" },
    // Metals.
    { { "XAGUSD",          "XAG/USD",          nullptr }, '3', "Spot silver" },
    { { "XAUUSD",          "XAU/USD",          nullptr }, '3', "Spot gold" },
    // Agricultural commodities.
    { { "COCOA.CMDUSD",    "COCOA.CMD/USD",    nullptr }, '3', "NY Cocoa" },
    { { "COFFEE.CMDUSX",   "COFFEE.CMD/USX",   nullptr }, '3', "Coffee Arabica" },
    { { "COTTON.CMDUSX",   "COTTON.CMD/USX",   nullptr }, '3', "Cotton" },
    { { "OJUICE.CMDUSX",   "OJUICE.CMD/USX",   nullptr }, '3', "Orange Juice" },
    { { "SOYBEAN.CMDUSX",  "SOYBEAN.CMD/USX",  nullptr }, '3', "Soybean" },
    { { "SUGAR.CMDUSD",    "SUGAR.CMD/USD",    nullptr }, '3', "London Sugar No.5" },
    // Energy & Metals commodities.
    { { "DIESEL.CMDUSD",   "DIESEL.CMD/USD",   nullptr }, '3', "Gas oil" },
    { { "BRENT.CMDUSD",    "BRENT.CMD/USD",    nullptr }, '3', "US Brent Crude Oil" },
    { { "LIGHT.CMDUSD",    "LIGHT.CMD/USD",    nullptr }, '3', "US Light Crude Oil" },
    { { "GAS.CMDUSD",      "GAS.CMD/USD",      nullptr }, '4', "Natural Gas" },
    { { "COPPER.CMDUSD",   "COPPER.CMD/USD",   nullptr }, '4', "High Grade Copper" },
    // Indices (CFD) America.
    { { "DOLLAR.IDXUSD",   "DOLLAR.IDX/USD",   nullptr }, '3', "US Dollar Index" },
    { { "USA30.IDXUSD",    "USA30.IDX/USD",    nullptr }, '3', "USA 30 Index" },
    { { "USATECH.IDXUSD",  "USATECH.IDX/USD",  nullptr }, '3', "USA 100 Technical Index" },
    { { "USA500.IDXUSD",   "USA500.IDX/USD",   nullptr }, '3', "USA 500 Index" },
    { { "USSC2000.IDXUSD", "USSC2000.IDX/USD", nullptr }, '3', "US Small Cap 2000" },
    // Indices (CFD) Asia / Pacific.
    { { "CHI.IDXUSD",      "CHI.IDX/USD",      nullptr }, '3', "China A50 Index" },
    { { "HKG.IDXHKD",      "HKG.IDX/HKD",      nullptr }, '3', "Hong Kong 40 Index" },
    { { "JPN.IDXJPY",      "JPN.IDX/JPY",      nullptr }, '3', "Japan 225" },
    { { "AUS.IDXAUD",      "AUS.IDX/AUD",      nullptr }, '3', "Australia 200 Index" },
    { { "IND.IDXUSD",      "IND.IDX/USD",      nullptr }, '3', "India 50 Index" },
    { { "SGD.IDXSGD",      "SGD.IDX/SGD",      nullptr }, '3', "Singapore Blue Chip Cash Index" },
    // Indices (CFD) Europe.
    { { "FRA.IDXEUR",      "FRA.IDX/EUR",      nullptr }, '3', "France 40 Index" },
    { { "DEU.IDXEUR",      "DEU.IDX/EUR",      nullptr }, '3', "Germany 30 Index" },
    { { "EUS.IDXEUR",      "EUS.IDX/EUR",      nullptr }, '3', "Europe 50 Index" },
    { { "GBR.IDXGBP",      "GBR.IDX/GBP",      nullptr }, '3', "UK 100 Index" },
    { { "ESP.IDXEUR",      "ESP.IDX/EUR",      nullptr }, '3', "Spain 35 Index" },
    { { "CHE.IDXCHF",      "CHE.IDX/CHF",      nullptr }, '3', "Switzerland 20 Index" },
    { { "NLD.IDXEUR",      "NLD.IDX/EUR",      nullptr }, '3', "Netherlands 25 Index" },
    { { "PLN.IDXPLN",      "PLN.IDX/PLN",      nullptr }, '3', "Poland 20 Index" },
    // Crypto (CFD).
    { { "BTCUSD",          "BTC/USD",          nullptr }, '1', "Bitcoin vs US Dollar" },
    { { "ETHUSD",          "ETH/USD",          nullptr }, '1', "Ether vs US Dollar" },
    { { "LTCUSD",          "LTC/USD",          nullptr }, '1', "Litecoin vs US Dollar" }
};

#define IISIZE (sizeof(instrument_info) / sizeof(instrument_info_type))

instrument_type instrument2type(const std::string &instrstr)
{
    for (unsigned int i = 0; i < IISIZE; i++)
    {
        for (unsigned int j = 0; j < 4; j++)
        {
            if (instrument_info[i].identifiers[j] == nullptr)
            {
                break;
            }
            else if (instrstr == instrument_info[i].identifiers[j])
            {
                return i;
            }
        }
    }

    return UNRECOGNIZED_INSTRUMENT;
}

const char *get_instrument_description(instrument_type t)
{
    return ((t < IISIZE) ? instrument_info[t].description : nullptr);
}

static char instrument_type2transform_exp(instrument_type t)
{
    return ((t < IISIZE) ? instrument_info[t].dpips_transform : 0);
}

unsigned int floating2dpips(const std::string &data, instrument_type t)
{
    double numeric = boost::lexical_cast<double>(data);
    char buffer[15];
    char fmt[] = { '%', '0', '.', instrument_type2transform_exp(t), 'f', 0};
    int i = 0, j = 0;

    int status = snprintf(buffer, 15, fmt, numeric);

    if (status <= 0 || status >= 15)
    {
        std::string msg = std::string("Floating point operation error: ")
                          + data;

        throw std::runtime_error(msg);
    }

    //
    // Traverse through all string including null character.
    //

    while (i <= status)
    {
        if (buffer[i] != '.')
        {
            buffer[j++] = buffer[i];
        }

        ++i;
    }

    return boost::lexical_cast<unsigned int>(buffer);
}

std::string timestamp_to_datetime_str(long timestamp)
{
    const time_t rawtime = (const time_t) timestamp;

    struct tm *dt;
    char timestr[40];
    char buffer [40];

    dt = localtime(&rawtime);

    strftime(timestr, sizeof(timestr), "%c", dt);
    sprintf(buffer,"%s", timestr);

    return std::string(buffer);
}


} /* namespace hft */
