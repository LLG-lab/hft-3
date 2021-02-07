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

#include <cmath>
#include <fstream>
#include <sstream>

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#include <easylogging++.h>

#include <mdc.hpp>

#define hft_log(__X__) \
    CLOG(__X__, "market_data_collector")

market_data_collector::market_data_collector(size_t size)
    : size_(size)
{
    el::Loggers::getLogger("market_data_collector", true);

    if (size_ == 0)
    {
        throw collector_error("Zero-lenght market collector");
    }
    else if (size_ == 1)
    {
        throw collector_error("Insufficient market data collector size : 1");
    }

    container_.reserve(size);
}

market_data_collector::~market_data_collector(void)
{
    if (cache_path_.length())
    {
        save_cache();
    }
}

void market_data_collector::feed_data(unsigned int data)
{
    if (! is_valid())
    {
        container_.push_back(data);
    }
    else
    {
        //
        // Shift items.
        //

        for (size_t i = 1; i < size_; i++)
        {
            container_[i-1] = container_[i];
        }

        //
        // Replace last element.
        //

        container_[size_-1] = data;
    }
}

bool market_data_collector::is_valid(void) const
{
    return (size_ == container_.size());
}

int market_data_collector::get_data_remain(void) const
{
    return size_ - container_.size();
}

void market_data_collector::enable_collector_cache(const std::string &cache_path)
{
    if (cache_path.length())
    {
        cache_path_ = cache_path;
        load_cache();
    }
}

void market_data_collector::set_btf_approximator_from_file(const std::string &file_name)
{
    hft_log(INFO) << "BTF Approximator set to [" << file_name << "].";

    approximator_.load_data_from_file(file_name);
}

void market_data_collector::set_btf_approximator_from_buffer(const std::string &buffer)
{
    hft_log(INFO) << "BTF Approximator configured directly from JSON string.";

    approximator_.load_data_from_string(buffer);
}

double market_data_collector::get_btfa(unsigned int sample_size, unsigned int offset) const
{
    unsigned int n = 0;
    long double sum = 0.0;
    int diff = 0;

    for (unsigned int i = offset; i < container_.size() - 1; i++)
    {
        if (n == sample_size)
        {
            //
            // Calculate statistic value and return.
            //

            long double total = (approximator_.get_amount()) * sample_size;

            long double S_numerator = (2.0 * sum) - (total);
            long double S_denominator = 2.0 * sqrt(total * 0.25);

            return (S_numerator / S_denominator);
        }
        else
        {
            diff = int(container_[i]) - int(container_[i+1]);

            sum += (approximator_.get_draw(diff));
            n++;
        }
    }

    hft_log(ERROR) << "Insufficient data in collector. Requested sample size ["
                   << sample_size << "] with offset [" << offset << "] "
                   << "Achieved sample: [" << n << "]";

    throw collector_error("Insufficient data in collector");
}

void market_data_collector::load_cache(void)
{
    std::fstream cache_file;
    cache_file.open(cache_path_, std::fstream::in);

    if (cache_file.fail())
    {
        hft_log(ERROR) << "Failed to open file [" << cache_path_ << "].";

        return;
    }

    std::string line;
    std::getline(cache_file, line);

    if (cache_file.fail())
    {
        hft_log(ERROR) << "Fail to read data from file [" << cache_path_ << "].";

        return;
    }

    boost::char_separator<char> sep(",");
    boost::tokenizer<boost::char_separator<char> > tokens(line, sep);

    container_.clear();

    for (auto &v : tokens)
    {
        container_.push_back(boost::lexical_cast<unsigned int>(v));

        if (is_valid())
        {
            break;
        }
    }

    hft_log(INFO) << "Loaded [" << container_.size() << "] items from ["
                  << cache_path_ << "].";

    if (is_valid())
    {
        hft_log(INFO) << "Data is sufficient after cache load";
    }
    else
    {
        hft_log(INFO) << "After cache load still requires ["
                      << get_data_remain() << "] items.";
    }
}

void market_data_collector::save_cache(void)
{
    hft_log(INFO) << "Saving market collector to file ["
                  << cache_path_ << "].";

    std::fstream cache_file;
    cache_file.open(cache_path_, std::fstream::out);

    if (cache_file.fail())
    {
        hft_log(ERROR) << "Fail to create file [" << cache_path_ << "].";

        return;
    }

    std::ostringstream line;

    for (auto &x : container_)
    {
        line << x << ",";
    }

    cache_file << line.str() << "\n";
}
