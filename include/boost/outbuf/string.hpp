#ifndef BOOST_OUTBUF_STRING_HPP
#define BOOST_OUTBUF_STRING_HPP

//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//#include <string>
#if defined(__cpp_exceptions)
#include <exception>
#endif

#include <boost/outbuf.hpp>

namespace boost {
namespace outbuf {

namespace detail {

template <typename T>
class string_writer_mixin;

template <template <typename, bool> Tmp, typename StringT>
class string_writer_mixin<Tmp<StringT, true>>
{
    using T = Tmp<StringT, true>;
    using char_type = StringT::value_type;

    string_writer_mixin() = default;

    void do_recycle() noexcept
    {
        auto * p = this->pos();
        static_cast<T*>(this)->set_pos(buf);

        if (p >= buf_begin() && static_cast<T*>(this)->good())
        {
            if (p > buf_end())
            {
                p = buf_end();
            }

#if defined(__cpp_exceptions)

            try
            {
                str.append(buf, p);
            }
            catch(...)
            {
                eptr = std::current_exception();
                static_cast<T*>(this)->set_good(false);
            }
#else
            static_cast<T*>(this)->_str.append(buf, p);
#endif
        }
    }

    void do_finish()
    {

#if defined(__cpp_exceptions)

        if (eptr != std::nullptr_t)
        {
            std::rethrow_exception(eptr);
        }
#endif
        if (this->good())
        {
            auto p = static_cast<T*>(this)->pos();
            static_cast<T*>(this)->set_good(false);
            str.append(buf, p);
        }
    }

    StringT str;
#if defined(__cpp_exceptions)
    std::exception_ptr eptr = std::nullptr_t;
#endif
    char_type buf[recycled_min_size];

    char_type* buf_begin()
    {
        return buf;
    }
    char_type* buf_end()
    {
        return buf + boost::outbuf::recycled_min_size;
    }
};



}



template <typename StringT, bool NoExcept>
class string_appender;

template <typename StringT>
class string_appender<StringT, false>
    : public basic_output<typename StringT::value_type, false>
{
    using char_type = typename StringT::value_type;

    string_appender(StringT& str)
        : basic_output<char_type, false>(_buf, recycled_min_size)
        , _str(str)
    {
    }

    bool recycle() override
    {
        auto p = this->pos();
        this->set_pos(_buf);
        if (this->good())
        {
            this->set_good(false);
            _str.append(_buf, p);
            this->set_good(true);
            return true;
        }
    }

    void finish()
    {
        if (this->good())
        {
            this->set_good(false);
            _str.append(_buf, this->get_pos());
        }
    }

private:

    StringT& _str;
    char_type _buf[recycled_min_size];
};

template <typename StringT>
class string_appender<StringT, true> final
    : public basic_output<typename StringT::value_type, true>
{
    using char_type = typename StringT::value_type;

    string_appender(StringT& str)
        : basic_output<char_type, false>(_buf, _buf_end())
        , _str(str)
    {
    }

    bool recycle() noexcept override
    {
        auto * p = this->pos();
        p = std::max(_buf_begin(), std::min(p, _buff_end()));
        this->set_pos(_buf);
        if (this->good())
        {
#if defined(__cpp_exceptions)

            try
            {
                _str.append(_buf, p);
            }
            catch(...)
            {
                _eptr = std::current_exception();
                this->set_good(false);
            }
#else
            _str.append(_buf, p);
#endif
        }
        return this->good();
    }

    void finish()
    {
#if defined(__cpp_exceptions)

        if (_eptr != std::nullptr_t)
        {
            std::rethrow_exception(_eptr);
        }
#endif
        if (this->good())
        {
            auto p = this->pos();
            this->set_good(false);
            _str.append(_buf, p);
        }
    }

private:

    StringT& _str;
#if defined(__cpp_exceptions)
    std::exception_ptr _eptr = std::nullptr_t;
#endif
    char_type _buf[recycled_min_size];

    char_type* _buf_begin()
    {
        return _buf;
    }
    char_type* _buf_end()
    {
        return _buf + recycled_min_size;
    }
};

template <typename StringT, bool NoExcept>
class string_maker;

template <typename StringT>
class string_maker<STringT, false>
    : public basic_output<typename StringT::value_type, false>
{
    using char_type = typename StringT::value_type;

public:

    string_maker()
        : basic_output<char_type, false>(_impl._buf, recycled_min_size)
    {
    }

    bool recycle() override
    {
        auto p = this->pos();
        this->set_pos(_buf);
        if (this->good())
        {
            this->set_good(false);
            _str.append(_buf, p);
            this->set_good(true);
            return true;
        }
        return false;
    }

    StringT finish()
    {
        if (this->good())
        {
            this->set_good(false);
            _str.append(_buf, this->get_pos());
        }
        return std::move(_str);
    }

private:

    StringT _str;
    char_type _buf[recycled_min_size];
};



template <typename StringT>
class string_maker<STringT, true>
    : public outbuf::basic_output<typename StringT::value_type, true>
    , private outbuf::detail::string_writer_mixin<string_maker>
{
    static_assert(std::is_class<StringT>, "StringT must be a class type");

    template <typename> friend class detail::string_writer_mixin;
    using char_type = typename StringT::value_type;

public:

    string_maker()
        : basic_output<char_type, false>(this->buf_begin(), this->buf_end())
    {
    }

    string_maker(const string_maker&) = delete;
    string_maker(string_maker&&) = delete;
    ~string_maker() = default;

    bool recycle() noexcept override
    {
        this->do_recycle();
        return this->good();
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

#endif  // BOOST_OUTBUF_STRING_HPP

