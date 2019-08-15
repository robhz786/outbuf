#ifndef BOOST_OUTBUF_TEST_TESTUTILS_HPP
#define BOOST_OUTBUF_TEST_TESTUTILS_HPP

//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <string>
#include <stdlib.h>
#include <algorithm>
#if defined(_WIN32)
#include <windows.h>
#endif
#include <boost/outbuf.hpp>

namespace test_utils {

std::string unique_tmp_file_name()
{

#if defined(_WIN32)

    char dirname[MAX_PATH];
    auto dirlen = GetTempPathA(MAX_PATH, name);
    char fullname[MAX_PATH];
    sprintf(fullname, "%s\\test_boost_outbuf_%x.txt", dirname, std::rand());
   return fullname;

#else // defined(_WIN32)

   char fullname[200];
   sprintf(fullname, "/tmp/test_boost_outbuf_%x.txt", std::rand());
   return fullname;

#endif  // defined(_WIN32)
}



std::wstring read_wfile(std::FILE* file)
{
    std::wstring result;
    wint_t ch = fgetwc(file);
    while(ch != WEOF)
    {
        result += static_cast<wchar_t>(ch);
        ch = fgetwc(file);
    };

    return result;
}

template <typename CharT>
std::basic_string<CharT> read_file(std::FILE* file)
{
    constexpr std::size_t buff_size = 500;
    CharT buff[buff_size];
    std::basic_string<CharT> result;
    std::size_t read_size = 0;
    do
    {
        read_size = std::fread(buff, sizeof(buff[0]), buff_size, file);
        result.append(buff, read_size);
    }
    while(read_size == buff_size);

    return result;
}

template <typename CharT>
std::basic_string<CharT> read_file(const char* filename)
{
    std::basic_string<CharT> result;

    std::FILE* file = nullptr;

    file = filename == nullptr ? nullptr : std::fopen(filename, "r");
    if(file != nullptr)
    {
        result = read_file<CharT>(file);
    }
    if (file != nullptr)
    {
        fclose(file);
    }

    return result;
}

template <typename CharT>
struct char_generator
{
    CharT operator()()
    {
        ch = ch == 0x79 ? 0x20 : (ch + 1);
        return ch;
    }
    CharT ch = 0x20;
};

template <typename CharT>
std::basic_string<CharT> make_string(std::size_t size)
{
    std::basic_string<CharT> str(size, CharT('x'));
    char_generator<CharT> gen;
    std::generate(str.begin(), str.end(), gen);
    return str;
}

template <typename CharT>
inline std::basic_string<CharT> make_half_string()
{
    constexpr auto bufsize = boost::outbuf::min_size_after_recycle<CharT>();
    return make_string<CharT>(bufsize / 2);
}

template <typename CharT>
inline std::basic_string<CharT> make_double_string()
{
    constexpr auto bufsize = boost::outbuf::min_size_after_recycle<CharT>();
    return make_string<CharT>(2 * bufsize);
}

template <typename CharT>
std::basic_string<CharT> make_tiny_string()
{
    return make_string<CharT>(5);
}

template <typename CharT>
inline void turn_into_bad(boost::outbuf::underlying_outbuf<CharT>& ob)
{
    boost::outbuf::detail::outbuf_test_tool::turn_into_bad(ob);
}

} // namespace test_utils

#endif // BOOST_OUTBUF_TEST_TESTUTILS_HPP

