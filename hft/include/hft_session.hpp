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

#ifndef __HFT_SESSION_HPP__
#define __HFT_SESSION_HPP__

#include <instrument_handler.hpp>
#include <session_transport.hpp>

#include <memory>

class hft_session : public session_transport
{
public:

    hft_session(boost::asio::io_service &io_service,
                    const boost::program_options::variables_map &config);

    ~hft_session(void);

private:

    typedef std::map<std::string, std::shared_ptr<instrument_handler> > instrument_handler_container;

    void tick_notify(const hft::tick_message &msg,
                         std::ostringstream &response);

    void historical_tick_notify(const hft::historical_tick_message &msg,
                                    std::ostringstream &response);

    void subscribe_notify(const hft::subscribe_instrument_message &msg,
                              std::ostringstream &response);

    void hci_setup_notify(const hft::hci_setup_message &msg,
                              std::ostringstream &response);

    const boost::program_options::variables_map &config_;
    instrument_handler_container instrument_handlers_;
};

#endif /* __HFT_SESSION_HPP__ */
