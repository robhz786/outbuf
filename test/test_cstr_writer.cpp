//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/core/lightweight_test.hpp>
#include <boost/outbuf.hpp>

int main()
{
    {
        const char s1a[] = "Hello";
        const char s1b[] = " World";
        const char s2[] = "Second string content";

        const char expected[]
            = "Hello World\0Second string content\0Third string cont";

        char buff[sizeof(expected)];

        boost::basic_cstr_writer<char> sw(buff);
        boost::write(sw, s1a);
        boost::write(sw, s1b);
        auto r1 = sw.finish();

        BOOST_TEST(! r1.truncated);
        BOOST_TEST_EQ(*r1.ptr, '\0');
        BOOST_TEST_EQ(r1.ptr, &buff[11]);
        BOOST_TEST_CSTR_EQ(buff, "Hello World");
        BOOST_TEST(! sw.good());

        boost::write(sw, s2);
        auto r2 = sw.finish();
        BOOST_TEST(! sw.good());
        BOOST_TEST(r2.truncated);
        BOOST_TEST_EQ(*r2.ptr, '\0');
        BOOST_TEST_EQ(r2.ptr, r1.ptr);
    }

    {
        char buff[8];
        boost::basic_cstr_writer<char> sw(buff);
        write(sw, "Hello");
        write(sw, " World");
        write(sw, "blah blah blah");
        auto r = sw.finish();

        BOOST_TEST(r.truncated);
        BOOST_TEST_EQ(*r.ptr, '\0');
        BOOST_TEST_EQ(r.ptr, &buff[7]);
        BOOST_TEST_CSTR_EQ(buff, "Hello W");
    }

    return boost::report_errors();
}
