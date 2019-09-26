//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/core/lightweight_test.hpp>
#include <boost/outbuf.hpp>
#include "test_utils.hpp"

template <typename CharT>
void test_discarded_outbuf()
{
    boost::discarded_outbuf<CharT> dob;
    BOOST_TEST(!dob.good());

    auto tiny_str = test_utils::make_tiny_string<CharT>();
    auto double_str = test_utils::make_double_string<CharT>();

    write(dob, tiny_str.data(), tiny_str.size());
    write(dob, double_str.data(), double_str.size());

    dob.recycle();
    BOOST_TEST_GE(dob.size(), boost::min_size_after_recycle<CharT>());
    BOOST_TEST(dob.pos() == boost::outbuf_garbage_buf<CharT>());
    BOOST_TEST(dob.end() == boost::outbuf_garbage_buf_end<CharT>());
}

int main()
{
    test_discarded_outbuf<char>();
    test_discarded_outbuf<char16_t>();

    return boost::report_errors();
}
