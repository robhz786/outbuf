#ifndef BOOST_OUTBUF_STRING_HPP
#define BOOST_OUTBUF_STRING_HPP

//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <string>
#if defined(__cpp_exceptions)
#include <exception>
#endif

#include <boost/outbuf.hpp>

namespace boost {
namespace outbuf {
namespace detail {

template <typename T, bool NoExcept, typename CharT>
class string_writer_mixin;

template <typename T, typename CharT>
class string_writer_mixin<T, false, CharT>
{
public:

    string_writer_mixin() = default;

    void do_recycle()
    {
        auto * p = static_cast<T*>(this)->pos();
        static_cast<T*>(this)->set_pos(_buf);

        if (p >= buf_begin() && static_cast<T*>(this)->good())
        {
            if (p > buf_end())
            {
                p = buf_end();
            }
            static_cast<T*>(this)->set_good(false);
            static_cast<T*>(this)->_append(buf_begin(), p);
            static_cast<T*>(this)->set_good(true);            
        }
    }
    void do_finish()
    {
        auto * p = static_cast<T*>(this)->pos();
        if (p >= buf_begin() && static_cast<T*>(this)->good())
        {
            if (p > buf_end())
            {
                p = buf_end();
            }
            static_cast<T*>(this)->set_good(false);
            static_cast<T*>(this)->_append(buf_begin(), p);
        }
    }

    CharT* buf_begin()
    {
        return _buf;
    }
    CharT* buf_end()
    {
        return _buf + _buf_size;
    }

private:

    static constexpr std::size_t _buf_size
        = boost::outbuf::min_size_after_recycle<CharT>();
    CharT _buf[_buf_size];
};

#if defined(__cpp_exceptions)

template <typename T, typename CharT>
class string_writer_mixin<T, true, CharT>
{
public:

    string_writer_mixin() = default;

    void do_recycle() noexcept
    {
        auto * p = static_cast<T*>(this)->pos();
        static_cast<T*>(this)->set_pos(buf_begin());

        if (static_cast<T*>(this)->good() && buf_begin() <= p)
        {
            if (p > buf_end())
            {
                p = buf_end();
            }
            try
            {
                static_cast<T*>(this)->_append(buf_begin(), p);
            }
            catch(...)
            {
                _eptr = std::current_exception();
                static_cast<T*>(this)->set_good(false);
            }
        }
    }

    void do_finish()
    {
        if (_eptr != nullptr)
        {
            std::rethrow_exception(_eptr);
        }
        BOOST_ASSERT(static_cast<T*>(this)->good());
        auto * p = static_cast<T*>(this)->pos();
        if (buf_begin() <= p)
        {
            if (p > buf_end())
            {
                p = buf_end();
            }
            static_cast<T*>(this)->set_good(false);
            auto p = static_cast<T*>(this)->pos();
            if (p >= buf_begin())
            {
                if (p > buf_end())
                {
                    p = buf_end();
                }
                static_cast<T*>(this)->_append(buf_begin(), p);
            }
        }
    }

    CharT* buf_begin()
    {
        return _buf;
    }
    CharT* buf_end()
    {
        return _buf + _buf_size;
    }

private:

    std::exception_ptr _eptr = nullptr;
    static constexpr std::size_t _buf_size
        = boost::outbuf::min_size_after_recycle<CharT>();
    CharT _buf[_buf_size];
};

#else // defined(__cpp_exceptions)

template <typename T, typename CharT>
class string_writer_mixin<T, true, CharT>
    : public string_writer_mixin<T, false, CharT>
{
};

#endif // defined(__cpp_exceptions)

} // namespace detail

template < bool NoExcept
         , typename CharT
         , typename Traits = std::char_traits<CharT>
         , typename Allocator = std::allocator<CharT> >
class basic_string_appender
    : public boost::outbuf::basic_outbuf<NoExcept, CharT>
    , private boost::outbuf::detail::string_writer_mixin
        < basic_string_appender<NoExcept, CharT, Traits, Allocator>
        , NoExcept
        , CharT >
{
public:

    using string_type = std::basic_string<CharT, Traits>;

    basic_string_appender(string_type& str_)
        : basic_outbuf<NoExcept, CharT>
            ( boost::outbuf::outbuf_garbage_buf<CharT>()
            , boost::outbuf::outbuf_garbage_buf_end<CharT>() )
        , _str(str_)
    {
        this->set_pos(this->buf_begin());
        this->set_end(this->buf_end());
    }
    basic_string_appender() = delete;
    basic_string_appender(const basic_string_appender&) = delete;
    basic_string_appender(basic_string_appender&&) = delete;
    ~basic_string_appender() = default;

    void recycle() noexcept(NoExcept) override
    {
        this->do_recycle();
    }

    void finish()
    {
        this->do_finish();
    }

private:

    template <typename, bool, typename>
    friend class detail::string_writer_mixin;

    void _append(const CharT* begin, const CharT* end)
    {
        _str.append(begin, end);
    }
    
    string_type& _str;
};

template < bool NoExcept
         , typename CharT
         , typename Traits = std::char_traits<CharT>
         , typename Allocator = std::allocator<CharT> >
class basic_string_maker
    : public boost::outbuf::basic_outbuf<NoExcept, CharT>
    , private boost::outbuf::detail::string_writer_mixin
        < basic_string_maker<NoExcept, CharT, Traits, Allocator>
        , NoExcept
        , CharT >
{
public:

    using string_type = std::basic_string<CharT, Traits>;

    basic_string_maker()
        : basic_outbuf<NoExcept, CharT>
            ( boost::outbuf::outbuf_garbage_buf<CharT>()
            , boost::outbuf::outbuf_garbage_buf_end<CharT>() )
    {
        this->set_pos(this->buf_begin());
        this->set_end(this->buf_end());
    }

    basic_string_maker(const basic_string_maker&) = delete;
    basic_string_maker(basic_string_maker&&) = delete;
    ~basic_string_maker() = default;

    void recycle() noexcept(NoExcept) override
    {
        this->do_recycle();
    }

    string_type finish()
    {
        this->do_finish();
        return std::move(_str);
    }

private:

    template <typename, bool, typename>
    friend class detail::string_writer_mixin;

    void _append(const CharT* begin, const CharT* end)
    {
        _str.append(begin, end);
    }

    string_type _str;
};

template <bool NoExcept>
using string_appender = basic_string_appender<NoExcept, char>;
template <bool NoExcept>
using u16string_appender = basic_string_appender<NoExcept, char16_t>;
template <bool NoExcept>
using u32string_appender = basic_string_appender<NoExcept, char32_t>;
template <bool NoExcept>
using wstring_appender = basic_string_appender<NoExcept, wchar_t>;

#if defined(__cpp_char8_t)
template <bool NoExcept>
using u8string_appender = basic_string_appender<NoExcept, char8_t>;
#endif

template <bool NoExcept>
using string_maker = basic_string_maker<NoExcept, char>;
template <bool NoExcept>
using u16string_maker = basic_string_maker<NoExcept, char16_t>;
template <bool NoExcept>
using u32string_maker = basic_string_maker<NoExcept, char32_t>;
template <bool NoExcept>
using wstring_maker = basic_string_maker<NoExcept, wchar_t>;

#if defined(__cpp_char8_t)
template <bool NoExcept>
using u8string_maker = basic_string_maker<NoExcept, char8_t>;
#endif


} // namespace outbuf
} // namespace boost

#endif  // BOOST_OUTBUF_STRING_HPP

