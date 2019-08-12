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
class underlying_outbuf
{
public:

    static_assert( std::is_same<CharT, underlying_outbuf_char_type<CharT>>::value
                 , "Forbidden character type in underlying_outbuf" );

    using char_type = CharT;
    static constexpr std::ptrdiff_t min_size_after_recycle = 64;

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
        BOOST_ASSERT(s <= min_size_after_recycle);
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

template <bool NoExcept, typename CharT>
class basic_outbuf;

template <typename CharT>
class basic_outbuf<false, CharT>
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

} // namespace detail

template <bool NoExcept, typename CharT>
void puts( boost::outbuf::basic_outbuf<NoExcept, CharT>& ob
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
void puts( boost::outbuf::basic_outbuf<NoExcept, CharT>& ob
         , const CharT* str
         , const CharT* str_end )
{
    BOOST_ASSERT(str_end >= str);
    boost::outbuf::puts(ob, str, str_end - str);
}

inline void puts( boost::outbuf::basic_outbuf<true, char>& ob
                , const char* str )
{
    boost::outbuf::puts<true, char>(ob, str, std::strlen(str));
}

inline void puts( boost::outbuf::basic_outbuf<false, char>& ob
                , const char* str )
{
    boost::outbuf::puts<false, char>(ob, str, std::strlen(str));
}

inline void puts( boost::outbuf::basic_outbuf<true, wchar_t>& ob
                , const wchar_t* str )
{
    boost::outbuf::puts<true, wchar_t>(ob, str, std::wcslen(str));
}

inline void puts( boost::outbuf::basic_outbuf<false, wchar_t>& ob
                , const wchar_t* str )
{
    boost::outbuf::puts<false, wchar_t>(ob, str, std::wcslen(str));
}

template <bool NoExcept, typename CharT>
void putc( boost::outbuf::basic_outbuf<NoExcept, CharT>& ob, CharT c )
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
};


inline char32_t* _outbuf_garbage_buf()
{
    constexpr std::size_t s1
        = (basic_outbuf<true, char>::min_size_after_recycle + 1) / 4;
    constexpr std::size_t s2
        = (basic_outbuf<true, char16_t>::min_size_after_recycle + 1) / 2;
    constexpr std::size_t s4
        = basic_outbuf<true, char32_t>::min_size_after_recycle;
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
        + boost::outbuf::basic_outbuf<false, CharT>::min_size_after_recycle;
}

template <typename CharT>
class raw_string_writer final: public boost::outbuf::basic_outbuf<true, CharT>
{
public:

    raw_string_writer(CharT* dest, CharT* dest_end)
        : basic_outbuf<true, CharT>(dest, dest_end - 1)
    {
        BOOST_ASSERT(dest < dest_end);
    }

    raw_string_writer(CharT* dest, std::size_t len)
        : basic_outbuf<true, CharT>(dest, dest + len - 1)
    {
        BOOST_ASSERT(len != 0);
    }

    template <std::size_t N>
    raw_string_writer(CharT (&dest)[N])
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

} // namespace outbuf
} // namespace boost

#endif  // BOOST_OUTBUF_HPP

