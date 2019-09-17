#ifndef BOOST_OUTBUF_STREAMBUF_HPP
#define BOOST_OUTBUF_STREAMBUF_HPP

//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <streambuf>
#if defined(__cpp_exceptions)
#include <exception>
#endif

#include <boost/outbuf.hpp>

namespace boost {

template <typename CharT, typename Traits = std::char_traits<CharT> >
class basic_streambuf_writer final: public boost::basic_outbuf<CharT>
{
public:

    explicit basic_streambuf_writer(std::basic_streambuf<CharT, Traits>& dest_)
        : boost::basic_outbuf<CharT>(_buf, _buf_size)
        , _dest(dest_)
    {
    }

    basic_streambuf_writer() = delete;
    basic_streambuf_writer(const basic_streambuf_writer&) = delete;
    basic_streambuf_writer(basic_streambuf_writer&&) = delete;

    ~basic_streambuf_writer()
    {
    }

    void recycle() override
    {
        std::streamsize count = this->pos() - _buf;
        this->set_pos(_buf);
        if (this->good())
        {
            auto count_inc = _dest.sputn(_buf, count);
            _count += count_inc;
            this->set_good(count_inc == count);
        }
    }

    struct result
    {
        std::streamsize count;
        bool success;
    };

    result finish()
    {
        std::streamsize count = this->pos() - _buf;
        auto g = this->good();
        this->set_pos(_buf);
        this->set_good(false);
        if (g)
        {
            auto count_inc = _dest.sputn(_buf, count);
            _count += count_inc;
            g = (count_inc == count);
        }
        return {_count, g};
    }

private:

    std::basic_streambuf<CharT, Traits>& _dest;
    std::streamsize _count = 0;
    static constexpr std::size_t _buf_size
        = boost::min_size_after_recycle<CharT>();
    CharT _buf[_buf_size];
};

// template <typename CharT, typename Traits>
// class basic_streambuf_writer_noexcept final
//     : public boost::basic_outbuf<true, CharT>
// {
// public:

//     explicit basic_streambuf_writer_noexcept(std::basic_streambuf<CharT, Traits>& dest_)
//         : boost::basic_outbuf<true, CharT>(_buf, _buf_size)
//         , _dest(dest_)
//     {
//     }

//     basic_streambuf_writer() = delete;
//     basic_streambuf_writer(const basic_streambuf_writer&) = delete;
//     basic_streambuf_writer(basic_streambuf_writer&&) = delete;

//     ~basic_streambuf_writer()
//     {
//     }

//     void recycle() noexcept
//     {
//         auto p = this->pos();
//         this->set_pos(_buf);
//         if (this->good() && p > _buf)
//         {
//             if (p > _buf + buff_size)
//             {
//                 p = _buf + buff_size;
//             }
//             try
//             {
//                 auto count_inc = _dest.sputn(_buf, count);
//                 _count += count_inc;
//                 this->set_good(count_written == count);
//             }
//             catch(...)
//             {
//                 _eptr = std::current_exception();
//                 this->set_good(false);
//             }
//         }
//     }

//     struct result
//     {
//         std::streamsize count;
//         bool success;
//     };

//     result finish()
//     {
//         std::streamsize count = this->pos() - _buf;
//         auto g = this->good();
//         this->set_pos(_buf);
//         this->set_good(false);
//         if (_eptr != nullptr)
//         {
//             std::rethrow_exception(_eptr);
//         }
//         if (g)
//         {
//             auto count_inc = _dest.sputn(_buf, count);
//             _count += count_inc;
//             g = (count_inc == count)
//         }
//         return {_count, g};
//     }

// private:

//     std::basic_streambuf<CharT, Traits>& _dest;
//     std::streamsize _count = 0;
//     static constexpr std::size_t _buf_size
//         = boost::underlying_outbuf<CharT>::min_size_after_recycle;
//     CharT _buf[_buf_size];
// };

using streambuf_writer
    = basic_streambuf_writer<char, std::char_traits<char> >;

using wstreambuf_writer
    = basic_streambuf_writer<wchar_t, std::char_traits<wchar_t> >;


} // namespace boost

#endif  // BOOST_OUTBUF_STREAMBUF_HPP

