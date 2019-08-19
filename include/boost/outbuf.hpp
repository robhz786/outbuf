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

namespace detail {

class outbuf_test_tool;

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
constexpr std::size_t min_size_after_recycle()
{
    return 64;
}

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
        BOOST_ASSERT(s <= boost::outbuf::min_size_after_recycle<CharT>());
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
    friend class boost::outbuf::detail::outbuf_test_tool;
};

template <typename CharT>
using get_underlying_outbuf
    = boost::outbuf::underlying_outbuf
          < boost::outbuf::underlying_outbuf_char_type<CharT> >;

template <bool NoExcept, typename CharT>
class basic_outbuf;

template <typename CharT>
class basic_outbuf<false, CharT>
    : private boost::outbuf::get_underlying_outbuf<CharT>
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
    void advance(std::size_t n)
    {
        _underlying_impl::advance(n);
    }    
    _underlying_impl& as_underlying()
    {
        return *this;
    }

    using _underlying_impl::size;
    using _underlying_impl::good;
    using _underlying_impl::ensure;
    using _underlying_impl::recycle;
    
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

    using _underlying_impl::set_good;
};

template <typename CharT>
class basic_outbuf<true, CharT>: public basic_outbuf<false, CharT>
{
public:

    virtual void recycle() noexcept = 0;

protected:

    using basic_outbuf<false, CharT>::basic_outbuf;
};

// global functions

namespace  detail{

template<bool NoExcept, typename CharT>
void puts_continuation
    ( boost::outbuf::basic_outbuf<NoExcept, CharT>& ob
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


template <bool NoExcept, typename CharT>
void do_puts( boost::outbuf::basic_outbuf<NoExcept, CharT>& ob
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
        boost::outbuf::detail::puts_continuation<NoExcept, CharT>(ob, str, len);
    }
}

template <bool NoExcept, typename CharT>
void do_putc( boost::outbuf::basic_outbuf<NoExcept, CharT>& ob, CharT c )
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


} // namespace detail

template <typename CharT>
inline void puts( boost::outbuf::basic_outbuf<true, CharT>& ob
                , const CharT* str
                , std::size_t len )
{
    boost::outbuf::detail::do_puts<true, CharT>(ob, str, len);
}

template <typename CharT>
inline void puts( boost::outbuf::basic_outbuf<false, CharT>& ob
                , const CharT* str
                , std::size_t len )
{
    boost::outbuf::detail::do_puts<false, CharT>(ob, str, len);
}

template <typename CharT>
inline void puts( boost::outbuf::basic_outbuf<true, CharT>& ob
                , const CharT* str
                , const CharT* str_end )
{
    BOOST_ASSERT(str_end >= str);
    boost::outbuf::detail::do_puts<true, CharT>(ob, str, str_end - str);
}

template <typename CharT>
inline void puts( boost::outbuf::basic_outbuf<false, CharT>& ob
                , const CharT* str
                , const CharT* str_end )
{
    BOOST_ASSERT(str_end >= str);
    boost::outbuf::detail::do_puts<false, CharT>(ob, str, str_end - str);
}

inline void puts( boost::outbuf::basic_outbuf<true, char>& ob
                , const char* str )
{
    boost::outbuf::detail::do_puts<true, char>(ob, str, std::strlen(str));
}

inline void puts( boost::outbuf::basic_outbuf<false, char>& ob
                , const char* str )
{
    boost::outbuf::detail::do_puts<false, char>(ob, str, std::strlen(str));
}

inline void puts( boost::outbuf::basic_outbuf<true, wchar_t>& ob
                , const wchar_t* str )
{
    boost::outbuf::detail::do_puts<true, wchar_t>(ob, str, std::wcslen(str));
}

inline void puts( boost::outbuf::basic_outbuf<false, wchar_t>& ob
                , const wchar_t* str )
{
    boost::outbuf::detail::do_puts<false, wchar_t>(ob, str, std::wcslen(str));
}

template <typename CharT>
inline void putc( boost::outbuf::basic_outbuf<true, CharT>& ob, CharT c )
{
    boost::outbuf::detail::do_puts<true, CharT>(ob, c);
}

template <typename CharT>
inline void putc( boost::outbuf::basic_outbuf<false, CharT>& ob, CharT c )
{
    boost::outbuf::detail::do_puts<false, CharT>(ob, c);
}

// type aliases

#if defined(__cpp_char8_t)
using u8outbuf  = basic_outbuf<false, char8_t>;
using u8outbuf_noexcept  = basic_outbuf<true, char8_t>;
#endif

using outbuf    = basic_outbuf<false, char>;
using u26outbuf = basic_outbuf<false, char16_t>;
using u32outbuf = basic_outbuf<false, char32_t>;
using woutbuf   = basic_outbuf<false, wchar_t>;

using outbuf_noexcept    = basic_outbuf<true, char>;
using u26outbuf_noexcept = basic_outbuf<true, char16_t>;
using u32outbuf_noexcept = basic_outbuf<true, char32_t>;
using woutbuf_noexcept   = basic_outbuf<true, wchar_t>;

namespace detail {

class outbuf_test_tool
{
public:
    template<typename CharT>
    static void turn_into_bad(underlying_outbuf<CharT>& ob)
    {
        ob.set_good(false);
    }
    template<typename CharT>
    static void force_set_pos(underlying_outbuf<CharT>& ob, CharT* pos)
    {
        ob.set_pos(pos);
    }
    
};


inline char32_t* _outbuf_garbage_buf()
{
    constexpr std::size_t s1
        = (boost::outbuf::min_size_after_recycle<char>() + 1) / 4;
    constexpr std::size_t s2
        = (boost::outbuf::min_size_after_recycle<char16_t>() + 1) / 2;
    constexpr std::size_t s4
        = boost::outbuf::min_size_after_recycle<char32_t>();
    constexpr std::size_t max_s1_s2 = s1 > s2 ? s1 : s2;
    constexpr std::size_t max_s1_s2_s4 = max_s1_s2 > s4 ? max_s1_s2 : s4;

    static char32_t arr[max_s1_s2_s4];
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
        + boost::outbuf::min_size_after_recycle<CharT>();
}

template <typename CharT>
class basic_cstr_writer final: public boost::outbuf::basic_outbuf<true, CharT>
{
public:

    basic_cstr_writer(CharT* dest, CharT* dest_end)
        : basic_outbuf<true, CharT>(dest, dest_end - 1)
    {
        BOOST_ASSERT(dest < dest_end);
    }

    basic_cstr_writer(CharT* dest, std::size_t len)
        : basic_outbuf<true, CharT>(dest, dest + len - 1)
    {
        BOOST_ASSERT(len != 0);
    }

    template <std::size_t N>
    basic_cstr_writer(CharT (&dest)[N])
        : basic_outbuf<true, CharT>(dest, dest + N - 1)
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

#if defined(__cpp_char8_t)
using u8cstr_writer = basic_cstr_writer<char8_t>;
#endif
using cstr_writer = basic_cstr_writer<char>;
using u16cstr_writer = basic_cstr_writer<char16_t>;
using u32cstr_writer = basic_cstr_writer<char32_t>;
using wcstr_writer = basic_cstr_writer<wchar_t>;


} // namespace outbuf
} // namespace boost

#endif  // BOOST_OUTBUF_HPP

