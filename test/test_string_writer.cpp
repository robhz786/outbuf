//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/core/lightweight_test.hpp>
#include <boost/outbuf/string.hpp>
#include "test_utils.hpp"

template <bool NoExcept, typename CharT>
void test_successfull_append()
{
    auto tiny_str = test_utils::make_tiny_string<CharT>();
    auto double_str = test_utils::make_double_string<CharT>();
    auto expected_content = tiny_str + double_str;

    std::basic_string<CharT> str;
    boost::outbuf::basic_string_appender<NoExcept, CharT> ob(str);
    puts(ob, tiny_str.c_str(), tiny_str.size());
    puts(ob, double_str.c_str(), double_str.size());
    ob.finish();
    BOOST_TEST(str == expected_content);
}

template <bool NoExcept, typename CharT>
void test_successfull_make()
{
    auto tiny_str = test_utils::make_tiny_string<CharT>();
    auto double_str = test_utils::make_double_string<CharT>();
    auto expected_content = tiny_str + double_str;

    boost::outbuf::basic_string_maker<NoExcept, CharT> ob;
    puts(ob, tiny_str.c_str(), tiny_str.size());
    puts(ob, double_str.c_str(), double_str.size());
    BOOST_TEST(ob.finish() == expected_content);
}

template <bool NoExcept, typename CharT>
void test_corrupted_pos_too_small_on_recycle()
{
    boost::outbuf::basic_string_maker<NoExcept, CharT> ob;
    const auto bufsize = ob.size();
    auto double_str = test_utils::make_string<CharT>(bufsize * 2);
    auto half_str = test_utils::make_string<CharT>(bufsize / 2);
    puts(ob, double_str.c_str(), double_str.size());
    puts(ob, half_str.c_str(), half_str.size());
    test_utils::force_set_pos(ob, ob.end() - bufsize - 1);
    ob.recycle();
    BOOST_TEST(ob.finish() == double_str);
}

template <bool NoExcept, typename CharT>
void test_corrupted_pos_too_big_on_recycle()
{
    boost::outbuf::basic_string_maker<NoExcept, CharT> ob;
    auto str = test_utils::make_string<CharT>(ob.size());
    std::char_traits<CharT>::copy(ob.pos(), str.data(), str.size());
    test_utils::force_set_pos(ob, ob.end() + 1);
    ob.recycle();
    BOOST_TEST(ob.finish() == str);
}

template <bool NoExcept, typename CharT>
void test_corrupted_pos_too_small_on_finish()
{
    boost::outbuf::basic_string_maker<NoExcept, CharT> ob;
    const auto bufsize = ob.size();
    auto double_str = test_utils::make_double_string<CharT>();
    auto half_str = test_utils::make_half_string<CharT>();
    puts(ob, double_str.c_str(), double_str.size());
    puts(ob, half_str.c_str(), half_str.size());
    test_utils::force_set_pos(ob, ob.end() - bufsize - 1);
    BOOST_TEST(ob.finish() == double_str);
}

template <bool NoExcept, typename CharT>
void test_corrupted_pos_too_big_on_finish()
{
    boost::outbuf::basic_string_maker<NoExcept, CharT> ob;
    auto str = test_utils::make_string<CharT>(ob.size());
    std::char_traits<CharT>::copy(ob.pos(), str.data(), str.size());
    test_utils::force_set_pos(ob, ob.end() + 1);
    BOOST_TEST(ob.finish() == str);
}

template <bool NoExcept, typename CharT>
class string_maker_that_throws
    : public boost::outbuf::basic_outbuf<NoExcept, CharT>
    , private boost::outbuf::detail::string_writer_mixin
        < string_maker_that_throws<NoExcept, CharT>, NoExcept, CharT >
{
public:

    using string_type = std::basic_string<CharT>;

    string_maker_that_throws()
        : boost::outbuf::basic_outbuf<NoExcept, CharT>
            ( boost::outbuf::outbuf_garbage_buf<CharT>()
            , boost::outbuf::outbuf_garbage_buf_end<CharT>() )
    {
        this->set_pos(this->buf_begin());
        this->set_end(this->buf_end());
    }

    string_maker_that_throws(const string_maker_that_throws&) = delete;
    string_maker_that_throws(string_maker_that_throws&&) = delete;
    ~string_maker_that_throws() = default;

    void recycle() noexcept(NoExcept) override
    {
        this->do_recycle();
    }

    string_type finish()
    {
        this->do_finish();
        return std::move(_str);
    }

    void throw_on_next_append(bool throw_= true)
    {
        _throw = throw_;
    }

private:

    template <typename, bool, typename>
    friend class boost::outbuf::detail::string_writer_mixin;

    void _append(const CharT* begin, const CharT* end)
    {
        if (_throw)
        {
            throw std::bad_alloc();
        }
        _str.append(begin, end);
    }

    string_type _str;
    bool _throw = false;
};



template <typename CharT>
void test_recycle_catches_exception()
{
    string_maker_that_throws<true, CharT> ob;
    auto double_str = test_utils::make_double_string<CharT>();
    auto half_str = test_utils::make_half_string<CharT>();

    puts(ob, double_str.c_str(), double_str.size());
    ob.recycle();
    ob.throw_on_next_append();

    puts(ob, half_str.c_str(), half_str.size());
    ob.recycle();
    BOOST_TEST(!ob.good());

    puts(ob, half_str.c_str(), half_str.size());
    ob.recycle();
    BOOST_TEST(!ob.good());

    BOOST_TEST_THROWS((void)ob.finish(), std::bad_alloc);
    BOOST_TEST(!ob.good());

    BOOST_TEST_THROWS((void)ob.finish(), std::bad_alloc);
    BOOST_TEST(!ob.good());
}

template <typename CharT>
void test_recycle_that_throws()
{
    string_maker_that_throws<false, CharT> ob;
    auto double_str = test_utils::make_double_string<CharT>();
    auto half_str = test_utils::make_half_string<CharT>();
    auto expected_content = double_str;
    puts(ob, double_str.c_str(), double_str.size());
    ob.recycle();
    ob.throw_on_next_append();

    puts(ob, half_str.c_str(), half_str.size());
    BOOST_TEST_THROWS(ob.recycle(), std::bad_alloc);
    BOOST_TEST(!ob.good());

    ob.throw_on_next_append(false);

    puts(ob, half_str.c_str(), half_str.size());
    ob.recycle(); // must be no-op
    BOOST_TEST(!ob.good());

    BOOST_TEST(ob.finish() == expected_content);
    BOOST_TEST(!ob.good());
}


int main()
{
    test_successfull_append<true, char>();
    test_successfull_append<true, char16_t>();
    test_successfull_append<false, char>();
    test_successfull_append<false, char16_t>();

    test_successfull_make<true, char>();
    test_successfull_make<true, char16_t>();
    test_successfull_make<false, char>();
    test_successfull_make<false, char16_t>();

    test_corrupted_pos_too_small_on_recycle<true, char>();
    test_corrupted_pos_too_small_on_recycle<true, char16_t>();
    test_corrupted_pos_too_small_on_recycle<false, char>();
    test_corrupted_pos_too_small_on_recycle<false, char16_t>();

    test_corrupted_pos_too_big_on_recycle<true, char>();
    test_corrupted_pos_too_big_on_recycle<true, char16_t>();
    test_corrupted_pos_too_big_on_recycle<false, char>();
    test_corrupted_pos_too_big_on_recycle<false, char16_t>();

    test_corrupted_pos_too_small_on_finish<true, char>();
    test_corrupted_pos_too_small_on_finish<true, char16_t>();
    test_corrupted_pos_too_small_on_finish<false, char>();
    test_corrupted_pos_too_small_on_finish<false, char16_t>();

    test_corrupted_pos_too_big_on_finish<true, char>();
    test_corrupted_pos_too_big_on_finish<true, char16_t>();
    test_corrupted_pos_too_big_on_finish<false, char>();
    test_corrupted_pos_too_big_on_finish<false, char16_t>();

    test_recycle_catches_exception<char32_t>();
    test_recycle_catches_exception<wchar_t>();
    test_recycle_that_throws<char>();
    test_recycle_that_throws<char16_t>();

    return boost::report_errors();
}
