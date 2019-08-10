#ifndef BOOST_OUTBUF_HPP
#define BOOST_OUTBUF_HPP

//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <cstring>
#include <cwchar>

#if defined(__cpp_exceptions)
#include <exception>
#endif

#include <boost/assert.hpp>

namespace boost {
namespace outbuf {

static constexpr std::ptrdiff_t min_recycled_outbuf_size = 64;

namespace detail {

template <std::size_t CharSize>
struct underlying_outbuf_char_type_impl;

template <> struct underlying_outbuf_char_type_impl<1>{using type = std::uint8_t;};
template <> struct underlying_outbuf_char_type_impl<2>{using type = char16_t;};
template <> struct underlying_outbuf_char_type_impl<4>{using type = char32_t;};

} // namespace detail

template <typename CharT>
using underlying_outbuf_char_type
= typename detail::underlying_outbuf_char_type_impl<sizeof(CharT)>::type;

template <typename CharT>
class underlying_outbuf
{
public:

    static_assert( std::is_same<CharT, underlying_outbuf_char_type<CharT>>::value
                 , "Forbidden character type in underlying_outbuf" );

    using char_type = CharT;

    underlying_outbuf(const underlying_outbuf&) = delete;
    underlying_outbuf(underlying_outbuf&&) = delete;
    underlying_outbuf& operator=(const underlying_outbuf&) = delete;
    underlying_outbuf& operator=(underlying_outbuf&&) = delete;

    virtual ~underlying_outbuf() = default;

    CharT* pos() const noexcept
    {
        return _pos;
    }
    CharT* end() const noexcept
    {
        return _end;
    }
    std::size_t size() const noexcept
    {
        BOOST_ASSERT(_pos <= _end);
        return _end - _pos;
    }

    bool good() const noexcept
    {
        return _good;
    }
    void advance_to(CharT* p)
    {
        BOOST_ASSERT(_pos <= p);
        BOOST_ASSERT(p <= _end);
        _pos = p;
    }
    void advance(std::size_t n)
    {
        BOOST_ASSERT(pos() + n <= end());
        _pos += n;
    }
    void advance() noexcept
    {
        BOOST_ASSERT(pos() < end());
        ++_pos;
    }
    void ensure(std::size_t s)
    {
        BOOST_ASSERT(s <= boost::outbuf::min_recycled_outbuf_size);
        if (pos() + s > end())
        {
            recycle();
        }
        BOOST_ASSERT(pos() + s <= end());
    }

    virtual void recycle() = 0;

protected:

    underlying_outbuf(CharT* pos_, CharT* end_) noexcept
        : _pos(pos_), _end(end_)
    { }

    underlying_outbuf(CharT* pos_, std::size_t s) noexcept
        : _pos(pos_), _end(pos_ + s)
    { }

    void set_pos(CharT* p) noexcept
    { _pos = p; };
    void set_end(CharT* e) noexcept
    { _end = e; };
    void set_good(bool g) noexcept
    { _good = g; };

private:

    CharT* _pos;
    CharT* _end;
    bool _good = true;
};

template <typename CharT, bool NoExcept>
class basic_outbuf;

template <typename CharT>
class basic_outbuf<CharT, false>
    : public boost::outbuf::underlying_outbuf
        < boost::outbuf::underlying_outbuf_char_type<CharT> >
{
    using _underlying_char_t = boost::outbuf::underlying_outbuf_char_type<CharT>;
    using _underlying_impl = boost::outbuf::underlying_outbuf<_underlying_char_t>;

public:

    using char_type = CharT;

    basic_outbuf(const basic_outbuf&) = delete;
    basic_outbuf(basic_outbuf&&) = delete;
    basic_outbuf& operator=(const basic_outbuf&) = delete;
    basic_outbuf& operator=(basic_outbuf&&) = delete;

    virtual ~basic_outbuf() = default;

    CharT* pos() const noexcept
    {
        return reinterpret_cast<CharT*>(_underlying_impl::pos());
    }
    CharT* end() const noexcept
    {
        return reinterpret_cast<CharT*>(_underlying_impl::end());
    }
    void advance_to(CharT* p)
    {
        _underlying_impl::advance_to(reinterpret_cast<_underlying_char_t*>(p));
    }

protected:

    basic_outbuf(CharT* pos_, CharT* end_) noexcept
        : _underlying_impl( reinterpret_cast<_underlying_char_t*>(pos_)
                          , reinterpret_cast<_underlying_char_t*>(end_) )
    { }

    basic_outbuf(CharT* pos_, std::size_t s) noexcept
        : _underlying_impl(reinterpret_cast<_underlying_char_t*>(pos_), s)
    { }

    void set_pos(CharT* p) noexcept
    {
        _underlying_impl::set_pos(reinterpret_cast<_underlying_char_t*>(p));
    }
    void set_end(CharT* e) noexcept
    {
        _underlying_impl::set_end(reinterpret_cast<_underlying_char_t*>(e));
    }

};

template <typename CharT>
class basic_outbuf<CharT, true>: public basic_outbuf<CharT, false>
{
public:

    virtual void recycle() noexcept = 0;

protected:

    using basic_outbuf<CharT, false>::basic_outbuf;
};

// global functions

namespace  detail{

template<typename CharT, bool NoExcept>
void puts_continuation
    ( boost::outbuf::basic_outbuf<CharT, NoExcept>& ob
    , const CharT* str
    , std::size_t len )
{
    auto space = ob.size();
    BOOST_ASSERT(space < len);
    std::memcpy(ob.pos(), str, space * sizeof(CharT));
    str += space;
    len -= space;
    ob.advance_to(ob.end());
    while (ob.good())
    {
        ob.recycle();
        space = ob.size();
        if (len <= space)
        {
            std::memcpy(ob.pos(), str, len * sizeof(CharT));
            ob.advance(len);
            break;
        }
        std::memcpy(ob.pos(), str, space * sizeof(CharT));
        len -= space;
        str += space;
        ob.advance_to(ob.end());
    }
}

} // namespace detail

template <typename CharT, bool NoExcept>
void puts( boost::outbuf::basic_outbuf<CharT, NoExcept>& ob
         , const CharT* str
         , std::size_t len )
{
    auto p = ob.pos();
    if (p + len <= ob.end()) // the common case
    {
        std::memcpy(p, str, len * sizeof(CharT));
        ob.advance(len);
    }
    else
    {
        boost::outbuf::detail::puts_continuation(ob, str, len);
    }
}

template <typename CharT, bool NoExcept>
void puts( boost::outbuf::basic_outbuf<CharT, NoExcept>& ob
         , const CharT* str
         , const CharT* str_end )
{
    BOOST_ASSERT(str_end >= str);
    boost::outbuf::puts(ob, str, str_end - str);
}

inline void puts( boost::outbuf::basic_outbuf<char, false>& ob, const char* str )
{
    boost::outbuf::puts(ob, str, std::strlen(str));
}

inline void puts( boost::outbuf::basic_outbuf<wchar_t, false>& ob, const wchar_t* str )
{
    boost::outbuf::puts(ob, str, std::wcslen(str));
}

template <typename CharT, bool NoExcept>
void putc( boost::outbuf::basic_outbuf<CharT, NoExcept>& ob, CharT c ) noexcept(NoExcept)
{
    auto p = ob.pos();
    if (p != ob.end())
    {
        *p = c;
        ob.advance_to(p+1);
    }
    else
    {
        ob.recycle();
        *ob.pos() = c;
        ob.advance();
    }
}

// type aliases

#if defined(__cpp_char8_t)
using u8outbuf  = basic_outbuf<char8_t, false>;
using u8outbuf_noexcept  = basic_outbuf<char8_t, true>;
#endif

using outbuf    = basic_outbuf<char, false>;
using u26outbuf = basic_outbuf<char16_t, false>;
using u32outbuf = basic_outbuf<char32_t, false>;
using woutbuf   = basic_outbuf<wchar_t, false>;

using outbuf_noexcept    = basic_outbuf<char, true>;
using u26outbuf_noexcept = basic_outbuf<char16_t, true>;
using u32outbuf_noexcept = basic_outbuf<char32_t, true>;
using woutbuf_noexcept   = basic_outbuf<wchar_t, true>;

namespace detail {

inline char32_t* _outbuf_garbage_buf()
{
    static char32_t arr[boost::outbuf::min_recycled_outbuf_size];
    return arr;
}

} // namespace detail

template <typename CharT>
inline CharT* outbuf_garbage_buf()
{
    return reinterpret_cast<CharT*>(boost::outbuf::detail::_outbuf_garbage_buf());
}

template <typename CharT>
inline CharT* outbuf_garbage_buf_end()
{
    return boost::outbuf::outbuf_garbage_buf<CharT>()
        + boost::outbuf::min_recycled_outbuf_size;
}

template <typename CharT>
class raw_string_writer final: public boost::outbuf::basic_outbuf<CharT, true>
{
public:

    raw_string_writer(CharT* dest, CharT* dest_end)
        : basic_outbuf<CharT, true>(dest, dest_end - 1)
    {
        BOOST_ASSERT(dest < dest_end);
    }

    raw_string_writer(CharT* dest, std::size_t len)
        : basic_outbuf<CharT, true>(dest, dest + len - 1)
    {
        BOOST_ASSERT(len != 0);
    }

    template <std::size_t N>
    raw_string_writer(CharT (&dest)[N])
        : basic_outbuf<CharT, true>(dest, dest + N - 1)
    {
    }

    void recycle() noexcept override
    {
        if (this->good())
        {
            _it = this->pos();
            this->set_good(false);
            this->set_end(outbuf_garbage_buf_end<CharT>());
        }
        this->set_pos(outbuf_garbage_buf<CharT>());
    }

    struct result
    {
        CharT* ptr;
        bool truncated;
    };

    result finish()
    {
        if (this->good())
        {
            auto p = this->pos();
            *p = CharT();
            this->set_pos(p + (p != this->end()));
            return { p, false };
        }
        *_it = CharT();
        return { _it, true };
    }

private:

    CharT* _it;
};

namespace detail {

template <typename T>
class string_writer_mixin;

template <template <typename, bool> class Tmp, typename StringT>
class string_writer_mixin<Tmp<StringT, false>>
{
    using T = Tmp<StringT, false>;
    using char_type = typename StringT::value_type;

public:

    string_writer_mixin() = default;

    void do_recycle() noexcept
    {
        auto * p = static_cast<T*>(this)->pos();
        static_cast<T*>(this)->set_pos(_buf);

        if (p >= buf_begin() && static_cast<T*>(this)->good())
        {
            if (p > buf_end())
            {
                p = buf_end();
            }
            static_cast<T*>(this)->_str.append(buf_begin(), p);
        }
    }
    void do_finish()
    {
        if (static_cast<T*>(this)->good())
        {
            auto p = static_cast<T*>(this)->pos();
            static_cast<T*>(this)->set_good(false);
            static_cast<T*>(this)->_str.append(buf_begin(), p);
        }
    }

    char_type* buf_begin()
    {
        return _buf;
    }
    char_type* buf_end()
    {
        return _buf + boost::outbuf::min_recycled_outbuf_size;
    }

private:

    char_type _buf[boost::outbuf::min_recycled_outbuf_size];
};

#if defined(__cpp_exceptions)

template <template <typename, bool> class Tmp, typename StringT>
class string_writer_mixin<Tmp<StringT, true>>
{
    using T = Tmp<StringT, true>;
    using char_type = typename StringT::value_type;

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
                static_cast<T*>(this)->_str.append(buf_begin(), p);
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
        if (static_cast<T*>(this)->good())
        {
            static_cast<T*>(this)->set_good(false);
            auto p = static_cast<T*>(this)->pos();
            if (p >= buf_begin())
            {
                if (p > buf_end())
                {
                    p = buf_end();
                }
                static_cast<T*>(this)->_str.append(buf_begin(), p);
            }
        }
    }

    char_type* buf_begin()
    {
        return _buf;
    }
    char_type* buf_end()
    {
        return _buf + boost::outbuf::min_recycled_outbuf_size;
    }

    std::exception_ptr _eptr = nullptr;
    char_type _buf[boost::outbuf::min_recycled_outbuf_size];
};

#else // defined(__cpp_exceptions)

template <template <typename, bool> class Tmp, typename StringT>
class string_writer_mixin<Tmp<StringT, true>>
    : public string_writer_mixin<Tmp<StringT, false>>
{
};

#endif // defined(__cpp_exceptions)

} // namespace detail

template <typename StringT, bool NoExcept>
class string_appender
    : public boost::outbuf::basic_outbuf<typename StringT::value_type, NoExcept>
    , private boost::outbuf::detail::string_writer_mixin
        < string_appender<StringT, NoExcept> >
{
    static_assert(std::is_class<StringT>::value, "StringT must be a class type");

    template <typename> friend class detail::string_writer_mixin;
    using char_type = typename StringT::value_type;

public:

    string_appender(StringT& str_)
        : basic_outbuf<char_type, NoExcept>(this->buf_begin(), this->buf_end())
        , _str(str_)
    {
    }
    string_appender() = delete;
    string_appender(const string_appender&) = delete;
    string_appender(string_appender&&) = delete;
    ~string_appender() = default;

    void recycle() noexcept(NoExcept) override
    {
        this->do_recycle();
    }

    void finish()
    {
        this->do_finish();
    }

private:

    StringT& _str;
};

template <typename StringT, bool NoExcept>
class string_maker
    : public boost::outbuf::basic_outbuf<typename StringT::value_type, NoExcept>
    , private boost::outbuf::detail::string_writer_mixin
        < string_maker<StringT, NoExcept> >
{
    static_assert( std::is_class<StringT>::value
                 , "StringT must be a class type" );

    template <typename> friend class detail::string_writer_mixin;
    using char_type = typename StringT::value_type;

public:

    string_maker()
        : basic_outbuf<char_type, NoExcept>(this->buf_begin(), this->buf_end())
    {
    }

    string_maker(const string_maker&) = delete;
    string_maker(string_maker&&) = delete;
    ~string_maker() = default;

    void recycle() noexcept(NoExcept) override
    {
        this->do_recycle();
    }

    StringT finish()
    {
        this->do_finish();
        return std::move(_str);
    }

private:

    StringT _str;
};

} // namespace outbuf
} // namespace boost

#endif  // BOOST_OUTBUF_HPP

