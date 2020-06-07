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

#ifndef __MARKET_DATA_COLLECTOR_HPP__
#define __MARKET_DATA_COLLECTOR_HPP__

#include <vector>
#include <binomial_approximation.hpp>

class market_data_collector 
{
public:

    DEFINE_CUSTOM_EXCEPTION_CLASS(collector_error, std::runtime_error)

    market_data_collector(size_t size);
    market_data_collector(void) = delete;
    ~market_data_collector(void);

    void feed_data(unsigned int data);
    bool is_valid(void) const;
    int get_data_remain(void) const;

    void enable_collector_cache(const std::string &cache_path);

    void set_btf_approximator_from_file(const std::string &file_name);
    void set_btf_approximator_from_buffer(const std::string &buffer);

    double get_btfa(unsigned int sample_size, unsigned int offset = 0) const;

private:

    void load_cache(void);
    void save_cache(void);

    binomial_approximation approximator_;

    std::vector<unsigned int> container_;
    const size_t size_;

    std::string cache_path_;
};

#endif /* __MARKET_DATA_COLLECTOR_HPP__ */
