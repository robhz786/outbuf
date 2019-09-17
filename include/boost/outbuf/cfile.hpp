#ifndef BOOST_OUTBUF_FILE_HPP
#define BOOST_OUTBUF_FILE_HPP

//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <cstdio>
#include <boost/outbuf.hpp>

namespace boost {

template <typename CharT>
class narrow_cfile_writer final: public boost::basic_outbuf_noexcept<CharT>
{
public:

    explicit narrow_cfile_writer(std::FILE* dest_)
        : boost::basic_outbuf_noexcept<CharT>(_buf, _buf_size)
        , _dest(dest_)
    {
        BOOST_ASSERT(dest_ != nullptr);
    }
    
    narrow_cfile_writer() = delete;
    narrow_cfile_writer(const narrow_cfile_writer&) = delete;
    narrow_cfile_writer(narrow_cfile_writer&&) = delete;

    ~narrow_cfile_writer()
    {
    }

    void recycle() noexcept
    {
        auto p = this->pos();
        this->set_pos(_buf);
        if (this->good())
        {
            std::size_t count = p - _buf;
            auto count_inc = std::fwrite(_buf, sizeof(CharT), count, _dest);
            _count += count_inc;
            this->set_good(count == count_inc);
        }
    }

    struct result
    {
        std::size_t count;
        bool success;
    };

    result finish()
    {
        bool g = this->good();
        this->set_good(false);
        if (g)
        {
            std::size_t count = this->pos() - _buf;
            auto count_inc = std::fwrite(_buf, sizeof(CharT), count, _dest);
            _count += count_inc;
            g = (count == count_inc);
        }
        return {_count, g};
    }
    
private:

    std::FILE* _dest;
    std::size_t _count = 0;
    static constexpr std::size_t _buf_size
        = boost::min_size_after_recycle<CharT>();
    CharT _buf[_buf_size];
};

class wide_cfile_writer final: public boost::basic_outbuf_noexcept<wchar_t>
{
public:

    explicit wide_cfile_writer(std::FILE* dest_)
        : boost::basic_outbuf_noexcept<wchar_t>(_buf, _buf_size)
        , _dest(dest_)
    {
        BOOST_ASSERT(dest_ != nullptr);
    }

    wide_cfile_writer() = delete;
    wide_cfile_writer(const wide_cfile_writer&) = delete;
    wide_cfile_writer(wide_cfile_writer&&) = delete;

    ~wide_cfile_writer()
    {
    }

    void recycle() noexcept
    {
        auto p = this->pos();
        this->set_pos(_buf);
        if (this->good())
        {
            for (auto it = _buf; it != p; ++it, ++_count)
            {
                if(std::fputwc(*it, _dest) == WEOF)
                {
                    this->set_good(false);
                    break;
                }
            }
        }
    }

    struct result
    {
        std::size_t count;
        bool success;
    };

    result finish()
    {
        recycle();
        auto g = this->good();
        this->set_good(false);
        return {_count, g};    
    }
    
  private:

    std::FILE* _dest;
    std::size_t _count = 0;
    static constexpr std::size_t _buf_size
        = boost::min_size_after_recycle<wchar_t>();
    wchar_t _buf[_buf_size];  
};

} // namespace outbuf

#endif  // BOOST_OUTBUF_FILE_HPP

