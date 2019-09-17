//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#define _CRT_SECURE_NO_WARNINGS

#include <boost/core/lightweight_test.hpp>
#include <boost/outbuf/cfile.hpp>
#include <ctime>
#include <cstdlib>
#include "test_utils.hpp"


template <typename CharT>
void test_narrow_successfull_writing()
{
    auto tiny_str = test_utils::make_tiny_string<CharT>();
    auto double_str = test_utils::make_double_string<CharT>();

    std::FILE* file = std::tmpfile();
    boost::narrow_cfile_writer<CharT> writer(file);
    auto expected_content = tiny_str + double_str;

    write(writer, tiny_str.data(), tiny_str.size());
    write(writer, double_str.data(), double_str.size());
    auto status = writer.finish();
    std::fflush(file);
    std::rewind(file);
    auto obtained_content = test_utils::read_file<CharT>(file);
    std::fclose(file);

    BOOST_TEST(status.success);
    BOOST_TEST_EQ(status.count, obtained_content.size());
    BOOST_TEST(obtained_content == expected_content);
}

void test_wide_successfull_writing()
{
    auto tiny_str = test_utils::make_tiny_string<wchar_t>();
    auto double_str = test_utils::make_double_string<wchar_t>();

    std::FILE* file = std::tmpfile();
    boost::wide_cfile_writer writer(file);
    auto expected_content = tiny_str + double_str;

    write(writer, tiny_str.data(), tiny_str.size());
    write(writer, double_str.data(), double_str.size());
    auto status = writer.finish();
    std::fflush(file);
    std::rewind(file);
    auto obtained_content = test_utils::read_wfile(file);
    std::fclose(file);

    BOOST_TEST(status.success);
    BOOST_TEST_EQ(status.count, obtained_content.size());
    BOOST_TEST(obtained_content == expected_content);
}

template <typename CharT>
void test_narrow_failing_to_recycle()
{
    auto half_str = test_utils::make_half_string<CharT>();
    auto double_str = test_utils::make_double_string<CharT>();
    auto expected_content = half_str;

    auto path = test_utils::unique_tmp_file_name();
    std::FILE* file = std::fopen(path.c_str(), "w");
    boost::narrow_cfile_writer<CharT> writer(file);

    write(writer, half_str.data(), half_str.size());
    writer.recycle(); // first recycle shall work
    test_utils::turn_into_bad(writer);
    write(writer, double_str.data(), double_str.size());

    auto status = writer.finish();
    std::fclose(file);
    auto obtained_content = test_utils::read_file<CharT>(path.c_str());
    std::remove(path.c_str());

    BOOST_TEST(! status.success);
    BOOST_TEST_EQ(status.count, obtained_content.size());
    BOOST_TEST(obtained_content == expected_content);
}

void test_wide_failing_to_recycle()
{
    auto half_str = test_utils::make_half_string<wchar_t>();
    auto double_str = test_utils::make_double_string<wchar_t>();
    auto expected_content = test_utils::make_half_string<wchar_t>();

    auto path = test_utils::unique_tmp_file_name();
    std::FILE* file = std::fopen(path.c_str(), "w");
    boost::wide_cfile_writer writer(file);

    write(writer, half_str.data(), half_str.size());
    writer.recycle();
    test_utils::turn_into_bad(writer);
    write(writer, double_str.data(), double_str.size());

    auto status = writer.finish();
    std::fclose(file);
    auto obtained_content = test_utils::read_wfile(path.c_str());
    std::remove(path.c_str());

    BOOST_TEST(! status.success);
    BOOST_TEST_EQ(status.count, obtained_content.size());
    BOOST_TEST(obtained_content == expected_content);
}


template <typename CharT>
void test_narrow_failing_to_finish()
{
    auto double_str = test_utils::make_double_string<CharT>();
    auto half_str = test_utils::make_half_string<CharT>();
    auto expected_content = double_str;

    auto path = test_utils::unique_tmp_file_name();
    std::FILE* file = std::fopen(path.c_str(), "w");
    boost::narrow_cfile_writer<CharT> writer(file);

    write(writer, double_str.data(), double_str.size());
    writer.recycle();
    write(writer, half_str.data(), half_str.size());
    test_utils::turn_into_bad(writer);

    auto status = writer.finish();
    std::fclose(file);
    auto obtained_content = test_utils::read_file<CharT>(path.c_str());
    std::remove(path.c_str());

    BOOST_TEST(! status.success);
    BOOST_TEST_EQ(status.count, obtained_content.size());
    BOOST_TEST(obtained_content == expected_content);
}

void test_wide_failing_to_finish()
{
    auto double_str = test_utils::make_double_string<wchar_t>();
    auto half_str = test_utils::make_half_string<wchar_t>();
    auto expected_content = test_utils::make_double_string<char>();

    auto path = test_utils::unique_tmp_file_name();
    std::FILE* file = std::fopen(path.c_str(), "w");
    boost::wide_cfile_writer writer(file);

    write(writer, double_str.data(), double_str.size());
    writer.recycle();
    write(writer, half_str.data(), half_str.size());
    test_utils::turn_into_bad(writer);

    auto status = writer.finish();
    std::fclose(file);
    auto obtained_content = test_utils::read_file<char>(path.c_str());
    std::remove(path.c_str());

    BOOST_TEST(! status.success);
    BOOST_TEST_EQ(status.count, obtained_content.size());
    BOOST_TEST(obtained_content == expected_content);
}

int main()
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    test_narrow_successfull_writing<char>();
    test_narrow_successfull_writing<char16_t>();
    test_narrow_successfull_writing<char32_t>();
    test_narrow_successfull_writing<wchar_t>();

    test_narrow_failing_to_recycle<char>();
    test_narrow_failing_to_recycle<char16_t>();
    test_narrow_failing_to_recycle<char32_t>();
    test_narrow_failing_to_recycle<wchar_t>();

    test_narrow_failing_to_finish<char>();
    test_narrow_failing_to_finish<char16_t>();
    test_narrow_failing_to_finish<char32_t>();
    test_narrow_failing_to_finish<wchar_t>();

    test_wide_successfull_writing();
    test_wide_failing_to_recycle();
    test_wide_failing_to_finish();

    return boost::report_errors();
}
