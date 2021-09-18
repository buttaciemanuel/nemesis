#ifndef __CORE_H__
#define __CORE_H__

#include <complex>
#include <cstdlib>
#include <cstring>
#include <numeric>
#include <string>
#include <sstream>
#include <stack>
#include <vector>
#include <tuple>

#define __DEVELOPMENT__ 1

struct __char { std::int32_t codepoint; };

struct __stack_entry {
    const char* file;
    const char* function;
    unsigned int line;
    unsigned int column;
    constexpr __stack_entry(const char* file, const char* function, unsigned int line, unsigned int column) : file(file), function(function), line(line), column(column) {}
};

struct __stack_activation_record {
    __stack_activation_record(const char* file, const char* function, unsigned int line, unsigned int column);
    ~__stack_activation_record();
    void location(int line, int column);
};

class __chars_iterator;

class __chars;

template<typename T> class __slice;

constexpr __char __char_decode(const char* units);
constexpr std::size_t __char_encode(__char value, char* units);
template<typename... Args> void __impl_format(std::ostringstream& os, const char *fmt);
template<typename Arg, typename... Args> void __impl_format(std::ostringstream& os, const char *fmt, Arg arg, Args ...args);
template<typename Arg> void __impl_format(std::ostringstream& os, const char *fmt, Arg arg);
template<typename... Args> std::string __format(std::string format, Args ...args);
template<typename T> constexpr std::size_t __sizeof() { return sizeof(T); }
template<typename T> __slice<T> __allocate(std::size_t n);
template<typename T> void __deallocate(__slice<T> slice);
template<typename T> inline void __free(T* memory);
void __println(std::string s);
void __crash(std::string message, const char* file = nullptr, int line = 0, int column = 0);
void __assert(bool condition, std::string message, const char* file = nullptr, int line = 0, int column = 0);
void __exit(std::int32_t code);
void __stacktrace();

class __chars_iterator {
public:
    constexpr __chars_iterator(const char* data) : ptr_(data) {}
    constexpr __chars_iterator& operator++()
    {
        if ((*ptr_ & 0xf8) == 0xf0) {
            ptr_ += 4;
        }
        else if ((*ptr_ & 0xf0) == 0xe0) {
            ptr_ += 3;
        }
        else if ((*ptr_ & 0xe0) == 0xc0) {
            ptr_ += 2;
        }
        else if ((*ptr_ & 0x80) == 0x0) {
            ++ptr_;
        }

        return *this;
    }
    constexpr __chars_iterator operator++(int)
    {
        auto tmp = *this;
        operator++();
        return tmp;
    }
    constexpr __char operator*() const { return __char_decode(ptr_); }
    constexpr bool operator==(__chars_iterator other) const { return ptr_ == other.ptr_; }
    constexpr bool operator!=(__chars_iterator other) const { return ptr_ != other.ptr_; }
private:
    const char* ptr_;
};

class __chars {
public:
    constexpr __chars() : data_(nullptr), size_(0) {}
    constexpr __chars(const char* data) : data_(data), size_(strlen(data)) {}
    constexpr __chars(const char* data, std::size_t size) : data_(data), size_(size) {}
    constexpr std::size_t size() const { return size_; }
    constexpr const char* data() const { return data_; }
    constexpr std::size_t length() const
    {
        std::size_t count = 0;
        for (auto i = begin(); i != end(); ++i, ++count);
        return count; 
    }
    constexpr void set(const char* data)
    {
        data_ = data;
        size_ = strlen(data);
    }
    constexpr __chars_iterator begin() const { return __chars_iterator(data_); }
    constexpr __chars_iterator end() const { return __chars_iterator(data_ + size_); }
private:
    static constexpr std::size_t strlen(const char* s)
    {
        const char* ptr = s;
        for (; *ptr; ++ptr);
        return ptr - s;
    }

    const char* data_;
    std::size_t size_;
};

template<typename Stream> Stream& operator<<(Stream& os, __chars chars);

template<typename T>
class __slice_iterator {
public:
    constexpr __slice_iterator(T* data) : ptr_(data) {}
    constexpr __slice_iterator& operator++()
    {
        ++ptr_;
        return *this;
    }
    constexpr __slice_iterator operator++(int)
    {
        auto old = *this;
        operator++();
        return old;
    }
    constexpr T& operator*() const { return *ptr_; }
private:
    T* ptr_;
};

template<typename T>
class __slice {
public:
    constexpr __slice(T* data, std::size_t size) : data_(data), size_(size) {}
    constexpr __slice(std::initializer_list<T> init, std::size_t size) : data_(const_cast<T*>(init.begin())), size_(size) {}
    constexpr std::size_t size() const { return size_; }
    constexpr T* data() const { return data_; }
    constexpr T& operator[](std::size_t index) const 
    {
#if __DEVELOPMENT__
        if (index >= size_) __crash(__format("slice index out of bounds, ? when size is ?", index, size_));
#endif 
        return data_[index]; 
    }
    constexpr __slice<T> slice(std::size_t begin, std::size_t end) const
    {
#if __DEVELOPMENT__
        if (begin >= size_) __crash(__format("slice start index out of bounds, ? when size is ?", begin, size_));
        if (end > size_) __crash(__format("slice end index out of bounds, ? when size is ?", end, size_));
#endif
        return __slice<T>(data_ + begin, end - begin); 
    }
    constexpr __slice_iterator<T> begin() const { return __slice_iterator<T>(data_); }
    constexpr __slice_iterator<T> end() const { return __slice_iterator<T>(data_ + size_); }
private:
    T* data_;
    std::size_t size_;
};

template<typename T>
class __rational {
    static_assert(std::is_integral<T>::value && std::is_signed<T>::value, "__rational must have signed integral type");
public:
    constexpr __rational() : numerator_(0), denominator_(1) {}
    constexpr __rational(T numerator, T denominator = 1) { set(numerator, denominator); }
    template<typename FloatType>
    constexpr __rational(FloatType real) { approximate(real, numerator_, denominator_); }
    constexpr T numerator() const { return numerator_; }
    constexpr T denominator() const { return denominator_; }
    constexpr void set(T numerator, T denominator)
    {
        T divisor = gcd(numerator, denominator);
        
        if (divisor != 0) {
            numerator_ = numerator / divisor;
            denominator_ = denominator / divisor;
        }
#if __DEVELOPMENT__
        if (denominator == 0) __crash("denominator of rational number cannot be zero, damn!");
#endif
    }
    template<typename FloatType>
    constexpr FloatType real() const
    {
        static_assert(std::is_floating_point<FloatType>::value, "__rational conversion to real requires floating point type");
        return static_cast<FloatType>(numerator_) / denominator_;
    }
    constexpr operator float() const { return real<float>(); }
    constexpr operator double() const { return real<double>(); }
    constexpr operator long double() const { return real<long double>(); }
    constexpr __rational operator+() const { return *this; }
    constexpr __rational operator-() const { return __rational(-numerator_, denominator_); }
    constexpr __rational operator+(__rational b) const
    {
        T common = lcm(numerator_, denominator_);
        return __rational(numerator_ * common / denominator_ + b.numerator_ * common / b.denominator_, common);
    }
    constexpr __rational operator-(__rational b) const
    {
        T common = lcm(numerator_, denominator_);
        return __rational (numerator_ * common / denominator_ - b.numerator_ * common / b.denominator_, common);
    }
    constexpr __rational operator*(__rational b) const { return __rational(numerator_ * b.numerator_, denominator_ * b.denominator_); }
    constexpr __rational operator/(__rational b) const { return __rational(numerator_ * b.denominator(), denominator_ * b.numerator()); }
    bool operator<(__rational b) const { return numerator_ * b.denominator_ < b.numerator_ * denominator_; }
    bool operator<=(__rational b) const { return numerator_ * b.denominator_ <= b.numerator_ * denominator_; }
    bool operator>(__rational b) const { return numerator_ * b.denominator_ > b.numerator_ * denominator_; }
    bool operator>=(__rational b) const { return numerator_ * b.denominator_ >= b.numerator_ * denominator_; }
    bool operator==(__rational b) const { return numerator_ * b.denominator_ == b.numerator_ * denominator_; }
    bool operator!=(__rational b) const { return numerator_ * b.denominator_ != b.numerator_ * denominator_; }
private:
    static constexpr T gcd(T x, T y) { return y == 0 ? x : gcd(y, x % y); }
    static constexpr T lcm(T x, T y) { return x * y / gcd(x, y); }
    template<typename FloatType>
    static constexpr void approximate(FloatType real, T& numerator, T& denominator, T max_denominator = 1000)
    {
        static_assert(std::is_floating_point<FloatType>::value, "__rational approximation from real requires floating point type");

        T a, h[3] = { 0, 1, 0 }, k[3] = { 1, 0, 0 };
        T x, d, n = 1;
        int i = 0, neg = 0;
    
        if (max_denominator <= 1) { 
            denominator = 1; 
            numerator = static_cast<T>(real); 
            return; 
        }
    
        if (real < 0) { 
            neg = 1; 
            real = -real; 
        }
    
        for (; real != std::floor(real); n <<= 1, real *= 2);

        d = real;
    
        for (i = 0; i < 64; i++) {
            a = n ? d / n : 0;
            if (i && !a) break;
    
            x = d; d = n; n = x % n;
    
            x = a;
            if (k[1] * a + k[0] >= max_denominator) {
                x = (max_denominator - k[0]) / k[1];
                if (x * 2 >= a || k[1] >= max_denominator)
                    i = 65;
                else
                    break;
            }
    
            h[2] = x * h[1] + h[0]; h[0] = h[1]; h[1] = h[2];
            k[2] = x * k[1] + k[0]; k[0] = k[1]; k[1] = k[2];
        }

        denominator = k[1];
        numerator = neg ? -h[1] : h[1];
    }

    T numerator_, denominator_; 
};

struct __none {};

template<typename T> __slice<T> __allocate(std::size_t n) { return __slice<T>(new T[n], n); }

template<typename T> void __deallocate(__slice<T> slice) { delete[] slice.data(); }

template<typename T> inline void __free(T* memory) { delete[] memory; }

template<typename T> constexpr std::size_t __get_size(__slice<T> slice) { return slice.size(); }

template<std::size_t N, typename T> constexpr T& __array_at(T* array, std::size_t index) 
{
#if __DEVELOPMENT__
    if (index >= N) __crash(__format("array index out of bounds, ? when size is ?", index, N));
#endif 
    return array[index]; 
}

template<std::size_t N, typename T> constexpr __slice<T> __get_slice(T* array, std::size_t begin, std::size_t end)
{
#if __DEVELOPMENT__
    if (begin >= N) __crash(__format("array start index out of bounds, ? when size is ?", begin, N));
    if (end > N) __crash(__format("array end index out of bounds, ? when size is ?", end, N));
#endif
    return __slice<T>(array + begin, end - begin); 
}

template<std::size_t N, typename T> bool __array_equals(const T (&x)[N], const T (&y)[N])
{
    std::size_t i = 0;
    for (; i < N && x[i] == y[i]; ++i);
    return i == N;
}

constexpr std::size_t __get_size(__chars arg) { return arg.size(); }

constexpr std::size_t __get_length(__chars arg) { return arg.length(); }

std::size_t __get_size(std::string arg);

std::size_t __get_length(std::string arg);

template<typename T> constexpr T __get_numerator(__rational<T> arg) { return arg.numerator(); }

template<typename T> constexpr T __get_denominator(__rational<T> arg) { return arg.denominator(); }

template<typename T> constexpr T __get_real(std::complex<T> arg) { return arg.real(); }

template<typename T> constexpr T __get_imaginary(std::complex<T> arg) { return arg.imag(); }

constexpr std::size_t __char_encode(__char value, char* units)
{
    std::size_t size = 0;

    if (value.codepoint <= 0x7f) {
        units[0] = value.codepoint & 0x7f;
        units[1] = 0;
        size = 1;
    }
    else if (value.codepoint <= 0x7ff) {
        units[0] = 0xc0 | (0x1f & (value.codepoint >> 6));
        units[1] = 0x80 | (0x3f & value.codepoint);
        units[2] = 0;
        size = 2;
    }
    else if (value.codepoint <= 0xffff) {
        units[0] = 0xe0 | (0xf & (value.codepoint >> 12));
        units[1] = 0x80 | (0x3f & (value.codepoint >> 6));
        units[2] = 0x80 | (0x3f & value.codepoint);
        units[3] = 0;
        size = 3;
    }
    else if (value.codepoint <= 0x10ffff) {
        units[0] = 0xf0 | (0x7 & (value.codepoint >> 18));
        units[1] = 0x80 | (0x3f & (value.codepoint >> 12));
        units[2] = 0x80 | (0x3f & (value.codepoint >> 6));
        units[3] = 0x80 | (0x3f & value.codepoint);
        units[4] = 0;
        size = 4;
    }
    
    return size;
}

constexpr __char __char_decode(const char* units)
{
    std::int32_t value = 0x0;
    
    if ((units[0] & 0xf8) == 0xf0) {
        value = units[0] & 0x7;
        value <<= 6;
        value |= units[1] & 0x3f;
        value <<= 6;
        value |= units[2] & 0x3f;
        value <<= 6;
        value |= units[3] & 0x3f;
    }
    else if ((units[0] & 0xf0) == 0xe0) {
        value = units[0] & 0xf;
        value <<= 6;
        value |= units[1] & 0x3f;
        value <<= 6;
        value |= units[2] & 0x3f;
    }
    else if ((units[0] & 0xe0) == 0xc0) {
        value = units[0] & 0x1f;
        value <<= 6;
        value |= units[1] & 0x3f;
    }
    else if ((units[0] & 0x80) == 0x0) {
        value = units[0] & 0x7f;
    }

    return __char{value};
}

template<typename Stream> Stream& operator<<(Stream& os, __char character) 
{
    char encoded[5] = {0};
    __char_encode(character, encoded);
    for (const char* p = encoded; *p; ++p) os << *p;
    return os;
}

template<typename Stream> Stream& operator<<(Stream& os, __chars chars) 
{
    for (const char* ptr = chars.data(); ptr < chars.data() + chars.size(); ++ptr) os << *ptr;
    return os;
}

template<typename Stream, typename T> Stream& operator<<(Stream& os, __rational<T> rational) 
{
    if (rational.numerator() * rational.denominator() < 0) os << '-';
    os << std::abs(rational.numerator());
    if (rational.denominator() != 1 && rational.denominator() != -1) os << '/' << std::abs(rational.denominator());
    return os;
}

template<typename Stream, typename T> Stream& operator<<(Stream& os, std::complex<T> complex) 
{
    os << complex.real();
    if (complex.imag() < 0) os << complex.imag();
    else os << '+' << complex.imag();
    os << 'i';
    return os;
}

template<typename... Args> void __impl_format(std::ostringstream& os, const char *fmt)
{
    for (const char* ptr = fmt; *ptr; ++ptr) os << *ptr;
}

template<typename Arg, typename... Args> void __impl_format(std::ostringstream& os, const char *fmt, Arg arg, Args ...args)
{
    while (*fmt != '\0' && *fmt != '?') os << *(fmt++);
    if (*(fmt++) == '?') {
        os << arg;
        __impl_format(os, fmt, args...);
    }
}

template<typename Arg> void __impl_format(std::ostringstream& os, const char *fmt, Arg arg)
{
    while (*fmt != '\0' && *fmt != '?') os << *(fmt++);
    if (*(fmt++) == '?') {
        os << arg;
        __impl_format(os, fmt);
    }
}

template<typename... Args> std::string __format(std::string format, Args ...args)
{
    std::ostringstream output;
    __impl_format(output, format.data(), args...);
    return output.str();
}

#endif