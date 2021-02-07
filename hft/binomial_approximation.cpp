/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System  ≣≡=-              **
**                              b                                     **
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

#include <iostream>
#include <fstream>

#include <binomial_approximation.hpp>
#include <boost/lexical_cast.hpp>
#include <cJSON/cJSON.h>


double binomial_approximation::get_draw(int oscillation) const
{
    if (oscillation == 0)
    {
        return get_amount() / 2.0;
    }
    else if (oscillation < 0)
    {
        return get_amount() - get_draw((-1) * oscillation);
    }

    if (distribution_.empty())
    {
        throw exception("No distribution loaded");
    }

    auto it = distribution_.find(oscillation);

    if (it == distribution_.end())
    {
        std::map<unsigned int, double>::const_reverse_iterator jt = distribution_.rbegin();
        unsigned int k = oscillation - (jt -> first);

        return (jt -> second + k);
    }
    else
    {
        return it -> second;
    }
}

void binomial_approximation::load_data_from_file(const std::string &approximation_file_name)
{
    std::string line, json_str;
    std::ifstream json_file(approximation_file_name.c_str());
    const std::string eol = "\n";

    if (! json_file.is_open())
    {
        throw exception(std::string("Unable to open file for read : ") + approximation_file_name);
    }

    while (std::getline(json_file, line))
    {
        json_str += (line + eol);
    }

    load_data_from_string(json_str);
}

void binomial_approximation::load_data_from_string(const std::string &data)
{
    cJSON *json = cJSON_Parse(data.c_str());

    if (json == NULL)
    {
        std::string err_msg("JSON parse error ");

        const char *json_emsg = cJSON_GetErrorPtr();

        if (json_emsg != NULL)
        {
            err_msg += std::string(json_emsg);
        }

        cJSON_Delete(json);

        throw exception(err_msg);
    }

    cJSON *n = cJSON_GetObjectItem(json, "n");

    if (n == NULL)
    {
        throw exception("Missing „n” attribute in json data");
    }

    amount_ = n -> valuedouble;

    cJSON *distribution = cJSON_GetObjectItem(json, "distribution");

    if (distribution == NULL)
    {
        throw exception("Missing „distribution” attribute in json data");
    }

    cJSON *element = NULL;
    int size = cJSON_GetArraySize(distribution);
    unsigned int index = 0;

    for (int i = 0; i < size; i++)
    {
        element = cJSON_GetArrayItem(distribution, i);
       // std::cout << element -> string << " " << element -> valuedouble << "\n";

        index = boost::lexical_cast<unsigned int>(element -> string);

        distribution_[index] = element -> valuedouble;
    }

    cJSON_Delete(json);

    //
    // FIXME: Add consistency checks of produced distribution approx.
    //
}

