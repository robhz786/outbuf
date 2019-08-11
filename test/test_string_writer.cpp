//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/core/lightweight_test.hpp>
#include <boost/outbuf/string.hpp>

int main()
{

    {
        std::string str;
        boost::outbuf::string_appender<true> ob(str);
        puts(ob, "Hello");
        puts(ob, " World");
        ob.finish();
        BOOST_TEST_EQ(str, "Hello World");
    }

    {
        boost::outbuf::string_maker<true> ob;
        puts(ob, "Hello");
        puts(ob, " World");
        BOOST_TEST_EQ(ob.finish(), "Hello World");
    }

    {
        std::string str;
        boost::outbuf::string_appender<false> ob(str);
        puts(ob, "Hello");
        puts(ob, " World");
        ob.finish();
        BOOST_TEST_EQ(str, "Hello World");
    }

    {
        boost::outbuf::string_maker<false> ob;
        puts(ob, "Hello");
        puts(ob, " World");
        BOOST_TEST_EQ(ob.finish(), "Hello World");
    }

    return boost::report_errors();
}
