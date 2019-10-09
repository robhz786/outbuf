#ifndef CHAR_ARRAY_STREAMBUF_HPP
#define CHAR_ARRAY_STREAMBUF_HPP

// Copied and Pasted the code from
// https://artofcode.wordpress.com/2010/12/12/deriving-from-stdstreambuf/
// by krzysztoftomaszewski

#include <streambuf>

class char_array_streambuf : public std::streambuf {
public:
    char_array_streambuf(const char *data, unsigned int len);
 
private:
    int_type underflow();
    int_type uflow();
    int_type pbackfail(int_type ch);
    std::streamsize showmanyc();
 
    const char * const begin_;
    const char * const end_;
    const char * current_;
};
 
inline char_array_streambuf::char_array_streambuf(const char *data, unsigned int len)
    : begin_(data), end_(data + len), current_(data) { }
 
inline char_array_streambuf::int_type char_array_streambuf::underflow() {
    if (current_ == end_) {
        return traits_type::eof();
    }
    return traits_type::to_int_type(*current_);
}
 
inline char_array_streambuf::int_type char_array_streambuf::uflow() {
    if (current_ == end_) {
        return traits_type::eof();
    }
    return traits_type::to_int_type(*current_++);
}
 
inline char_array_streambuf::int_type char_array_streambuf::pbackfail(int_type ch) {
    if (current_ == begin_ || (ch != traits_type::eof() && ch != current_[-1])) {
        return traits_type::eof();
    }
    return traits_type::to_int_type(*--current_);
}
 
inline std::streamsize char_array_streambuf::showmanyc() {
    return end_ - current_;
}


#endif
