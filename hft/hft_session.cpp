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

#include <boost/algorithm/string.hpp>

#include <hft_session.hpp>

#define hft_log(__X__) \
    CLOG(__X__, "session")

//
// On production, should be undefined.
//

#undef HFT_DEBUG

hft_session::hft_session(boost::asio::io_service &io_service,
                             const boost::program_options::variables_map &config)
    : session_transport(io_service, config),
      config_(config)
{
    el::Loggers::getLogger("session", true);
}

hft_session::~hft_session(void)
{
    #ifdef HFT_DEBUG
    hft_log(INFO) << "Destructor got called";
    #endif
}

void hft_session::tick_notify(const hft::tick_message &msg,
                                  std::ostringstream &response)
{
    std::string instrument_format2 = boost::erase_all_copy(msg.instrument, "/");

    #ifdef HFT_DEBUG
    hft_log(DEBUG) << "Received tick notify :  Instrument ["
                   << msg.instrument << "], ask [" << msg.ask
                   << "], bankroll [" << msg.bankroll
                   << "], position status [" << msg.position_status
                   << "].";
    #endif

    //
    // Find appropriate instrument handler, then dispatch notify to it.
    //

    auto it = instrument_handlers_.find(instrument_format2);

    if (it == instrument_handlers_.end())
    {
        response << "ERROR;Unsubscribed handler for instrument "
                 << msg.instrument;

        return;
    }

    it -> second -> on_tick(msg, response);
}

void hft_session::historical_tick_notify(const hft::historical_tick_message &msg,
                                             std::ostringstream &response)
{
    std::string instrument_format2 = boost::erase_all_copy(msg.instrument, "/");

    #ifdef HFT_DEBUG
    hft_log(DEBUG) << "Received historical tick notify :  Instrument ["
                   << msg.instrument << "], ask [" << msg.ask
                   << "].";
    #endif

    //
    // Find appropriate instrument handler, then dispatch notify to it.
    //

    auto it = instrument_handlers_.find(instrument_format2);

    if (it == instrument_handlers_.end())
    {
        response << "ERROR;Unsubscribed handler for instrument "
                 << msg.instrument;

        return;
    }

    it -> second -> on_historical_tick(msg, response);

}

void hft_session::subscribe_notify(const hft::subscribe_instrument_message &msg,
                                       std::ostringstream &response)
{
    std::string instrument_format2;

    for (auto &instrument : msg.instruments)
    {
        instrument_format2 = boost::erase_all_copy(instrument, "/");

        if (instrument_handlers_.find(instrument_format2) == instrument_handlers_.end())
        {
            hft_log(INFO) << "Creating handler for instrument ["
                          << instrument_format2 << "]";

            try
            {
                std::shared_ptr<instrument_handler> handler;
                handler.reset(new instrument_handler(config_, instrument_format2));
                instrument_handlers_.insert(std::pair<std::string, std::shared_ptr<instrument_handler> >(instrument_format2, handler));
            }
            catch (std::exception &e)
            {
                hft_log(ERROR) << "Failed to create handler for instrument ["
                               << instrument_format2 << "] : " << e.what();
            }
        }
        else
        {
            hft_log(WARNING) << "Instrument handler [" << instrument_format2
                             << "] already subscribed, ignoring";
        }
    }

    response << "OK";
}

void hft_session::hci_setup_notify(const hft::hci_setup_message &msg,
                                       std::ostringstream &response)
{
    std::string instrument_format2 = boost::erase_all_copy(msg.instrument, "/");

    #ifdef HFT_DEBUG
    hft_log(DEBUG) << "Received HCI setup notify :  Instrument ["
                   << msg.instrument << "], option ["
                   << (msg.enable ? "hci_on" : "hci_off")
                   << "].";
    #endif

    //
    // Find appropriate instrument handler, then dispatch notify to it.
    //

    auto it = instrument_handlers_.find(instrument_format2);

    if (it == instrument_handlers_.end())
    {
        response << "ERROR;Unsubscribed handler for instrument "
                 << msg.instrument;

        return;
    }

    it -> second -> on_hci_setup(msg, response);
}
