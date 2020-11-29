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

#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>

#include <easylogging++.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <hci.hpp>
#include <text_file_reader.hpp>

#undef HCI_TEST

#ifdef HCI_TEST
#include <iostream>
#endif

#define hft_log(__X__) \
    CLOG(__X__, "HCI")

hci::hci(size_t capacity, double st, double it)
    : state_(state::STRAIGHT),
      straight_threshold_(st),
      invert_threshold_(it),
      index_(0),
      capacity_(capacity),
      buffer_(new int[capacity]),
      integral_buffer_(new int[capacity]),
      debug_(false)
{
    el::Loggers::getLogger("HCI", true);

    if (capacity == 0)
    {
        delete [] buffer_;
        delete [] integral_buffer_;

        throw exception("Illegal capacity value passed");
    }
}

hci::~hci(void)
{
    if (file_name_.length() > 0)
    {
        save_object_state();
    }

    delete [] buffer_;
    delete [] integral_buffer_;
}

void hci::insert_pips_yield(int pips_yield)
{
    if (state_ == state::INVERT)
    {
        //
        // If decision was made based on
        // inverted decision, we should
        // invert response to get the
        // original.
        //

        pips_yield *= (-1);
    }

    if (index_ < capacity_)
    {
        buffer_[index_++] = pips_yield;
        return;
    }

    for (size_t i = 1; i < capacity_; i++)
    {
        buffer_[i-1] = buffer_[i];
    }

    buffer_[capacity_-1] = pips_yield;

    // 1. Calculate gain and offset.

    double a, b;

    // Calculate Integral.
    int s = 0;
    for (int i = 0; i < capacity_; i++)
    {
        s += buffer_[i];
        integral_buffer_[i] = s;
    }

    #ifdef HCI_TEST
    // Display buffers.
    for (int x = 0; x < capacity_; x++)
    {
        std::cout << "x=" << x << ", y=" << buffer_[x] << ", Y=" << integral_buffer_[x] << "\n";
    }
    #endif

    double s1 = 0.0, s2 = 0.0, s3 = 0.0, s4 = 0.0;
    for (int x = 0; x < capacity_; x++)
    {
        s1 += x * integral_buffer_[x];
        s2 += x * x;
        s3 += x;
        s4 += integral_buffer_[x];
    }

    a = (s1 * capacity_ - s3 * s4) / (s2 * capacity_ - s3 * s3);
    b = (s4 - a * s3) / capacity_;

    if (debug_)
    {
        hft_log(DEBUG) << "(hci::insert_pips_yield) Got pips_yield ["
                       << pips_yield << "], Function: y(x) = "
                       << a << "*x + " << b << " HCI state ["
                       << get_state_str();
    }

    // 2. Update state if needed.

    if (state_ == state::STRAIGHT && a >= invert_threshold_)
    {
        state_ = state::INVERT;
    }
    else if (state_ == state::INVERT && a <= straight_threshold_)
    {
        state_ = state::STRAIGHT;
    }

    if (file_name_.length() > 0)
    {
        save_object_state();
    }
}

decision_type hci::decision(decision_type d) const
{
    switch (d)
    {
        case DECISION_SHORT:
            if (state_ == state::STRAIGHT)
            {
                return DECISION_SHORT;
            }
            else
            {
                return DECISION_LONG;
            }
        case DECISION_LONG:
            if (state_ == state::STRAIGHT)
            {
                return DECISION_LONG;
            }
            else
            {
                return DECISION_SHORT;
            }
    }

    return NO_DECISION;
}

void hci::enable_cache(const std::string &file_name)
{
    if (file_name.length() > 0)
    {
        file_name_ = file_name;
        load_object_state();
    }
}

std::string hci::get_state_str(void) const
{
    std::ostringstream data;

    switch (state_)
    {
        case state::STRAIGHT:
            data << "0:";
            break;
        case state::INVERT:
            data << "1:";
            break;
        default:
            throw exception("Illegal state");
    }

    for (size_t i = 0; i < std::min(index_, capacity_); i++)
    {
        if (i > 0)
        {
            data << ',';
        }

        data << buffer_[i];
    }

    return data.str();
}

void hci::load_object_state(void)
{
    std::string data;

    try
    {
        text_file_reader file(file_name_);
        file.read_line(data);
    }
    catch (const text_file_reader::text_file_reader_exception &e)
    {
        hft_log(ERROR) << "Failed to load data from file ["
                       << file_name_ << "] : " << e.what();

        return;
    }

    //
    // Setup default settings.
    //

    index_ = 0;
    state_ = state::STRAIGHT;

    boost::trim(data);

    if (data.length() == 0)
    {
        return;
    }

    //
    // Parsing stuff here.
    //

    std::vector<std::string> series1, series2;

    boost::split(series1, data, boost::is_any_of(":"));

    if (series1.size() != 2)
    {
        hft_log(ERROR) << "Invalid data ”"
                       << (std::string)(data.length() <= 100 ? data : (data.substr(0, 100) + std::string("(...)")))
                       << "” in file ["
                       << file_name_ << "].";

        return;
    }

    try
    {
        switch (boost::lexical_cast<int>(series1.at(0)))
        {
            case 0:
                state_ = state::STRAIGHT;

                hft_log(INFO) << "[" << file_name_
                              << "] Got state [STRAIGHT].";

                break;
            case 1:
                state_ = state::INVERT;

                hft_log(INFO) << "[" << file_name_
                              << "] Got state [INVERT].";

                break;
            default:
                hft_log(ERROR) << "Invalid state token in file ["
                               << file_name_ << "].";

                return;
        }
    }
    catch (const boost::bad_lexical_cast &e)
    {
        hft_log(ERROR) << "Invalid state token in file ["
                       << file_name_ << "] : " << e.what();

        return;
    }

    boost::split(series2, series1.at(1), boost::is_any_of(","));

    try
    {
        for (index_ = 0; index_ < std::min(series2.size(), capacity_); index_++)
        {
            buffer_[index_] = boost::lexical_cast<int>(series2.at(index_));

            hft_log(INFO) << " buffer[" << index_
                          << "] ← " << buffer_[index_];
        }
    }
    catch (const boost::bad_lexical_cast &e)
    {
        index_ = 0;
        state_ = state::STRAIGHT;

        hft_log(ERROR) << "Unconvertible to integer token ["
                       << series2.at(index_) << "] in file ["
                       << file_name_ << "] : " << e.what();

        return;
    }

    hft_log(INFO) << "File [" << file_name_ << "] loaded.";
}

void hci::save_object_state(void)
{
    if (index_ == 0)
    {
        return; // Nothing to save.
    }

    std::fstream save_file;
    save_file.open(file_name_, std::fstream::out);

    if (save_file.fail())
    {
        hft_log(ERROR) << "Unable to open file for writing: ["
                       << file_name_ << "].";

        return;
    }

    save_file << get_state_str() << std::endl;
}
