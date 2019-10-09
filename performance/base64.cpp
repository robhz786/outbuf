//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/outbuf.hpp>
#include "char_array_streambuf.hpp"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <cstring>

inline char base64_encode(std::uint8_t hextet)
{
    std::uint8_t ch =
        hextet < 26 ?  static_cast<std::uint8_t>('A') + hextet :
        hextet < 52 ?  static_cast<std::uint8_t>('a') + hextet - 26 :
        hextet < 62 ?  static_cast<std::uint8_t>('0') + hextet - 52 :
        hextet == 62 ? static_cast<std::uint8_t>('+') :
      /*hextet == 63*/ static_cast<std::uint8_t>('/') ;

    return ch;
}

void base64_encode_3bytes(char* dest, const std::uint8_t* data)
{
    dest[0] = base64_encode(data[0] >> 2);
    dest[1] = base64_encode(((data[0] & 0x03) << 4) | ((data[1] & 0xF0) >> 4));
    dest[2] = base64_encode(((data[1] & 0x0F) << 2) | ((data[2] & 0xC0) >> 6));
    dest[3] = base64_encode(data[2] & 0x3F);
}

void base64_encode_2_last_bytes(char* dest, const std::uint8_t* data)
{
    dest[0] = base64_encode(data[0] >> 2);
    dest[1] = base64_encode(((data[0] & 0x03) << 4) | ((data[1] & 0xF0) >> 4));
    dest[2] = base64_encode((data[1] & 0x0F) << 2);
    dest[3] = '=';
}

void base64_encode_last_byte(char* dest, const std::uint8_t data)
{
    dest[0] = base64_encode(data >> 2);
    dest[1] = base64_encode((data & 0x03) << 4);
    dest[2] = '=';
    dest[3] = '=';
}

struct base64_result
{
    char* dest_it;
    const std::uint8_t* data_it;
};

base64_result to_base64( char* dest
                       , char* dest_end
                       , const std::uint8_t* data
                       , const std::uint8_t* data_end )
{
    std::size_t data_len = data_end - data;
    std::size_t dest_len = dest_end - dest;

    while(data_len >= 3)
    {
        if (dest_len < 4)
        {
            return {dest, data};
        }
        base64_encode_3bytes(dest, data);
        dest += 4;
        data += 3;
        dest_len -= 4;
        data_len -= 3;
    }
    if (data_len != 0)
    {
        if (dest_len < 4)
        {
            return {dest, data};
        }
        if (data_len == 2)
        {
            base64_encode_2_last_bytes(dest, data);
            data += 2;
        }
        else
        {
            base64_encode_last_byte(dest, data[0]);
            ++ data;
        }
        dest += 4;
    }
    return {dest, data};
}

void to_base64( boost::outbuf& dest
              , const std::uint8_t* data
              , const std::uint8_t* data_end )
{
    while(dest.good())
    {
        auto res = to_base64(dest.pos(), dest.end(), data, data_end);
        dest.advance_to(res.dest_it);
        data = res.data_it;
        if (data == data_end)
        {
            break;
        }
        dest.recycle();
    }
}

template <std::size_t BufSize = 80>
void to_base64( std::streambuf& dest
              , const std::uint8_t* data
              , const std::uint8_t* data_end )
{
    char buf[BufSize];
    char* const buf_end = buf + BufSize;
    do
    {
        auto res = to_base64(buf, buf_end, data, data_end);
        dest.sputn(buf, (res.dest_it - buf));
        data = res.data_it;
    }
    while(data != data_end);
}


int main()
{
    constexpr std::size_t loop_size = 20000;
    constexpr std::size_t data_size = 100000;
    std::uint8_t data[data_size];
    std::uint8_t* const data_end = data + data_size;
    for(std::size_t i=0; i < data_size; ++i)
    {
        data[i] = static_cast<std::uint8_t>(i);
    }

    constexpr std::size_t dest_size = (4 * (data_size + 3)) / 3;
    char dest[dest_size];
    char* const dest_end = dest + dest_size;

    {
        auto t1 = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < loop_size; ++i)
        {
            to_base64(dest, dest_end, data, data_end);
        }
        auto t2 = std::chrono::steady_clock::now();
        std::chrono::steady_clock::duration  dt = (t2 - t1);
        std::cout << dt.count() << " , " << std::flush;
    }

    {
        auto t1 = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < loop_size; ++i)
        {
            boost::cstr_writer writer(dest);
            to_base64(writer, data, data_end);
            writer.finish();
        }
        auto t2 = std::chrono::steady_clock::now();
        std::chrono::steady_clock::duration  dt = (t2 - t1);
        std::cout << dt.count() << " , " << std::flush;
    }

    {
        auto t1 = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < loop_size; ++i)
        {
            char_array_streambuf writer(dest, dest_size);
            to_base64(writer, data, data_end);
        }
        auto t2 = std::chrono::steady_clock::now();
        std::chrono::steady_clock::duration  dt = (t2 - t1);
        std::cout << dt.count() << " , " << std::flush; // '\n';
    }
}
