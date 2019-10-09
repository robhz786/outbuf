//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/outbuf.hpp>
#include <boost/outbuf/streambuf.hpp>
#include <boost/outbuf/string.hpp>
#include "char_array_streambuf.hpp"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

struct element_123
{
    std::string field_a;
    std::string field_b;
    std::string field_c;
};

struct element_abc
{
    std::string field_1;
    std::string field_2;
    std::vector<element_123> field_3;
};


void to_json(boost::outbuf& dest, const element_abc& data)
{
    write(dest, "{\n  \"field_1\" : \"");
    write(dest, data.field_1.data(), data.field_1.size());
    write(dest, "\",\n  \"field_2\" : \"");
    write(dest, data.field_2.data(), data.field_2.size());    
    write(dest, "\",\n  \"field_3\" : [\n");
    for (auto it = data.field_3.begin(); it != data.field_3.end(); ++it) {
        const auto& elm = *it;
        write(dest, "    {\n      \"field_a\" : \"");
        write(dest, elm.field_a.data(), elm.field_a.size());
        write(dest, "\",\n      \"field_b\" : \"");
        write(dest, elm.field_b.data(), elm.field_b.size());
        write(dest, "\",\n      \"field_c\" : \"");
        write(dest, elm.field_c.data(), elm.field_c.size());
        if (it + 1 == data.field_3.end()) {
            write(dest, "\"\n    }\n");
        } else {
            write(dest, "\"\n    },\n");
        }        
    }
    write(dest, "  ]\n}\n");
}

inline void write(std::streambuf& dest, const char* str)
{
    dest.sputn(str, strlen(str));
}

inline void write(std::streambuf& dest, const char* str, std::size_t len)
{
    dest.sputn(str, len);
}

void to_json(std::streambuf& dest, const element_abc& data)
{
    write(dest, "{\n  \"field_1\" : \"");
    write(dest, data.field_1.data(), data.field_1.size());
    write(dest, "\",\n  \"field_2\" : \"");
    write(dest, data.field_2.data(), data.field_2.size());    
    write(dest, "\",\n  \"field_3\" : [\n");
    for (auto it = data.field_3.begin(); it != data.field_3.end(); ++it) {
        const auto& elm = *it;
        write(dest, "    {\n      \"field_a\" : \"");
        write(dest, elm.field_a.data(), elm.field_a.size());
        write(dest, "\",\n      \"field_b\" : \"");
        write(dest, elm.field_b.data(), elm.field_b.size());
        write(dest, "\",\n      \"field_c\" : \"");
        write(dest, elm.field_c.data(), elm.field_c.size());
        if (it + 1 == data.field_3.end()) {
            write(dest, "\"\n    }\n");
        } else {
            write(dest, "\"\n    },\n");
        }        
    }
    write(dest, "  ]\n}\n");

}


element_abc create_sample_data()
{
    element_abc data;
    data.field_1 = "blah blah blah";
    data.field_2 = "bleh bleh bleh";
    for (int i = 0; i < 5; ++i)
    {
        char buff[40];
        element_123 e;
        sprintf(buff, "qwert %d", i);
        e.field_a = buff;
        sprintf(buff, "asdfg %d", i);
        e.field_b = buff;
        sprintf(buff, "zxcvb %d", i);
        e.field_c = buff;

        data.field_3.push_back(e);
    }
    
    return data;
}


int main()
{
    auto data = create_sample_data();
    constexpr std::size_t buff_size = 1000;
    char buff[buff_size];
    char* const buff_end = buff + buff_size;

    constexpr long long loop_size = 20000000;

#if 1

    {        
        auto t1 = std::chrono::steady_clock::now();
        for (long long i = 0; i < loop_size; ++i)
        {
            boost::cstr_writer writer(buff);
            to_json(writer, data);
            writer.finish();
        }
        auto t2 = std::chrono::steady_clock::now();

        std::chrono::steady_clock::duration  dt = (t2 - t1);
        std::cout << dt.count() << " , " << std::flush;
    }

#else
    
    {
        auto t1 = std::chrono::steady_clock::now();
        for (long long i = 0; i < loop_size; ++i)
        {
            char_array_streambuf writer(buff, buff_size);
            to_json(writer, data);
        }
        auto t2 = std::chrono::steady_clock::now();

        std::chrono::steady_clock::duration  dt = (t2 - t1);
        std::cout << dt.count() << " , " << std::flush;
    }

#endif
    
    return 0;
}


