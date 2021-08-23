#ifndef SAFE_HPP
#define SAFE_HPP

#include <cfenv>
#include <cstddef>
#include <cstdint>
#include <complex>
#include <ostream>
#include <limits>

#include <iostream>

namespace utils {
    struct precisions {
        static constexpr size_t max(size_t a, size_t b) { return a > b ? a : b; }
        static constexpr size_t bits8 = 0;
        static constexpr size_t bits16 = 1;
        static constexpr size_t bits32 = 2;
        static constexpr size_t bits64 = 3;
        static constexpr size_t bits128 = 4;
        static constexpr size_t bits256 = 5;
        static constexpr size_t bitsword = sizeof(size_t) == 8 ? bits64 : bits32;
        static constexpr size_t nprecisions = 6;
    };
    
    class safe_unsigned_int {
    public:
        static uint64_t min(size_t precision)
        {
            switch (precision) {
                case precisions::bits8:
                    return std::numeric_limits<uint8_t>::min();
                case precisions::bits16:
                    return std::numeric_limits<uint16_t>::min();
                case precisions::bits32:
                    return std::numeric_limits<uint32_t>::min();
                case precisions::bits64:
                    return std::numeric_limits<uint64_t>::min();
                case precisions::bits128:
                    return std::numeric_limits<uint64_t>::min();
                default:
                    break;
            }

            throw std::invalid_argument("utils::safe_unsigned_int::min(): invalid precision " + std::to_string(precision));
        }
        
        static uint64_t max(size_t precision)
        {
            switch (precision) {
                case precisions::bits8:
                    return std::numeric_limits<uint8_t>::max();
                case precisions::bits16:
                    return std::numeric_limits<uint16_t>::max();
                case precisions::bits32:
                    return std::numeric_limits<uint32_t>::max();
                case precisions::bits64:
                    return std::numeric_limits<uint64_t>::max();
                case precisions::bits128:
                    return std::numeric_limits<uint64_t>::max();
                default:
                    break;
            }

            throw std::invalid_argument("utils::safe_unsigned_int::max(): invalid precision " + std::to_string(precision));
        }

        explicit safe_unsigned_int(size_t precision) : precision_(precision), overflow_(0), value_(0) {}
        safe_unsigned_int() : safe_unsigned_int(precisions::bits32) {}
        
        size_t precision() const { return precision_; }
        void precision(size_t p) 
        { 
            precision_ = p;
            if (value_ > max(precision_)) overflow_ = 1;
        }
        
        void value(uint64_t val) 
        { 
            if (val > max(precision_)) overflow_ = 1;
            value_ = val; 
        }
        uint64_t value() const { return value_; }
        
        void size(size_t s) { precision(static_cast<size_t>(log2(s)) - 3); }
        size_t size() const { return 1 << (precision_ + 3); }
        
        bool overflow() const { return overflow_; }
        void overflow(bool set) { overflow_ = set; }

        safe_unsigned_int operator+() const { return *this; }

        safe_unsigned_int operator-() const
        {
            safe_unsigned_int result(*this);
            result.overflow(true);
            result.value(-result.value());
            return result;
        }

        safe_unsigned_int operator+(safe_unsigned_int b) const
        {
            safe_unsigned_int result(precisions::max(precision(), b.precision()));

            if (safe_unsigned_int::max(result.precision()) - value() < b.value()) {
                result.overflow(true);
            }
            
            result.value(value() + b.value());

            return result;
        }

        safe_unsigned_int operator-(safe_unsigned_int b) const
        {
            safe_unsigned_int result(precisions::max(precision(), b.precision()));

            if (value() < b.value()) {
                result.overflow(true);
            }
            
            result.value(value() - b.value());

            return result;
        }

        safe_unsigned_int operator*(safe_unsigned_int b) const
        {
            safe_unsigned_int result(precisions::max(precision(), b.precision()));

            if (value() > safe_unsigned_int::max(result.precision()) / b.value()) {
                result.overflow(true);
            }
            
            result.value(value() * b.value());

            return result;
        }

        safe_unsigned_int operator/(safe_unsigned_int b) const
        {
            safe_unsigned_int result(precisions::max(precision(), b.precision()));

            if (b.value() == 0) {
                result.overflow(true);
            }
            else {
                result.value(value() / b.value());
            }

            return result;
        }

        safe_unsigned_int operator%(safe_unsigned_int b) const
        {
            safe_unsigned_int result(precisions::max(precision(), b.precision()));

            if (b.value() == 0) {
                result.overflow(true);
            }
            else {
                result.value(value() % b.value());
            }
            
            return result;
        }

        safe_unsigned_int operator~() const
        {
            safe_unsigned_int result(*this);
            
            result.value(~result.value());
            
            return result;
        }

        safe_unsigned_int operator&(safe_unsigned_int b) const
        {
            safe_unsigned_int result(precisions::max(precision(), b.precision()));

            result.value(value() & b.value());

            return result;
        }

        safe_unsigned_int operator|(safe_unsigned_int b) const
        {
            safe_unsigned_int result(precisions::max(precision(), b.precision()));

            result.value(value() | b.value());

            return result;
        }

        safe_unsigned_int operator^(safe_unsigned_int b) const
        {
            safe_unsigned_int result(precisions::max(precision(), b.precision()));

            result.value(value() ^ b.value());

            return result;
        }

        safe_unsigned_int operator<<(safe_unsigned_int b) const
        {
            safe_unsigned_int result(*this);

            result.value(result.value() << b.value());

            return result;
        }

        safe_unsigned_int operator>>(safe_unsigned_int b) const
        {
            safe_unsigned_int result(*this);

            result.value(result.value() >> b.value());

            return result;
        }

        bool operator<(safe_unsigned_int b) const { return value_ < b.value_; }

        bool operator<=(safe_unsigned_int b) const { return value_ <= b.value_; }

        bool operator>(safe_unsigned_int b) const { return value_ > b.value_; }

        bool operator>=(safe_unsigned_int b) const { return value_ >= b.value_; }

        bool operator==(safe_unsigned_int b) const { return value_ == b.value_; }

        bool operator!=(safe_unsigned_int b) const { return value_ != b.value_; }
    private:
        unsigned precision_ : 3;
        unsigned overflow_ : 1;
        uint64_t value_;
    };

    class safe_signed_int {
    public:
        static int64_t min(size_t precision)
        {
            switch (precision) {
                case precisions::bits8:
                    return std::numeric_limits<int8_t>::min();
                case precisions::bits16:
                    return std::numeric_limits<int16_t>::min();
                case precisions::bits32:
                    return std::numeric_limits<int32_t>::min();
                case precisions::bits64:
                    return std::numeric_limits<int64_t>::min();
                case precisions::bits128:
                    return std::numeric_limits<int64_t>::min();
                default:
                    break;
            }

            throw std::invalid_argument("utils::safe_signed_int::min(): invalid precision " + std::to_string(precision));
        }
        
        static int64_t max(size_t precision)
        {
            switch (precision) {
                case precisions::bits8:
                    return std::numeric_limits<int8_t>::max();
                case precisions::bits16:
                    return std::numeric_limits<int16_t>::max();
                case precisions::bits32:
                    return std::numeric_limits<int32_t>::max();
                case precisions::bits64:
                    return std::numeric_limits<int64_t>::max();
                case precisions::bits128:
                    return std::numeric_limits<int64_t>::max();
                default:
                    break;
            }

            throw std::invalid_argument("utils::safe_signed_int::max(): invalid precision " + std::to_string(precision));
        }

        explicit safe_signed_int(size_t precision) : precision_(precision), overflow_(0), value_(0) {}
        safe_signed_int() : safe_signed_int(precisions::bits32) {}
        safe_signed_int(safe_unsigned_int u) : precision_(u.precision()), overflow_(static_cast<int64_t>(u.value()) < min(u.precision()) || static_cast<int64_t>(u.value()) > max(u.precision())), value_(u.value()) {}
        
        size_t precision() const { return precision_; }
        void precision(size_t p) 
        { 
            precision_ = p;
            if (value_ < min(precision_) || value_ > max(precision_)) overflow_ = 1;
        }
        
        void value(int64_t val) 
        {
            if (val < min(precision_) || val > max(precision_)) overflow_ = 1; 
            value_ = val; 
        }
        int64_t value() const { return value_; }
        
        size_t size() const { return 1 << (precision_ + 3); }
        void size(size_t s) { precision(static_cast<size_t>(log2(s)) - 3); }
        
        bool overflow() const { return overflow_; }
        void overflow(bool set) { overflow_ = set; }

        safe_signed_int operator+() const { return *this; }

        safe_signed_int operator-() const
        {
            safe_signed_int result(*this);

            if (result.value() == safe_signed_int::min(result.precision())) {
                result.overflow(true);
            }

            result.value(-result.value());

            return result;
        }

        safe_signed_int operator+(safe_signed_int b) const
        {
            safe_signed_int result(precisions::max(precision(), b.precision()));

            if ((b.value() > 0 && value() > safe_signed_int::max(result.precision()) - b.value()) ||
                (b.value() < 0 && value() < safe_signed_int::min(result.precision()) - b.value())) {
                result.overflow(true);
            }
            
            result.value(value() + b.value());

            return result;
        }

        safe_signed_int operator-(safe_signed_int b) const
        {
            safe_signed_int result(precisions::max(precision(), b.precision()));

            if ((b.value() > 0 && value() < safe_signed_int::min(result.precision()) + b.value()) ||
                (b.value() < 0 && value() > safe_signed_int::max(result.precision()) + b.value())) {
                result.overflow(true);
            }
            
            result.value(value() - b.value());

            return result;
        }

        safe_signed_int operator*(safe_signed_int b) const
        {
            safe_signed_int result(precisions::max(precision(), b.precision()));
            int64_t min = safe_signed_int::min(result.precision()), max = safe_signed_int::max(result.precision());

            if (value() > 0) {
                if (b.value() > 0) {
                    if (value() > max / b.value()) {
                        result.overflow(true);
                    }
                }
                else if (b.value() < min / value()) {
                    result.overflow(true);
                }
            }
            else if (b.value() > 0) {
                if (value() < min / b.value()) {
                    result.overflow(true);
                }
            }
            else if (value() != 0 && b.value() < max / value()) {
                result.overflow(true);
            }
            
            result.value(value() * b.value());

            return result;
        }

        safe_signed_int operator/(safe_signed_int b) const
        {
            safe_signed_int result(precisions::max(precision(), b.precision()));

            if (b.value() == 0 || (value() == safe_signed_int::min(result.precision()) && b.value() == -1)) {
                result.overflow(true);
            }
            else {
                result.value(value() / b.value());
            }

            return result;
        }

        safe_signed_int operator%(safe_signed_int b) const
        {
            safe_signed_int result(precisions::max(precision(), b.precision()));

            if (b.value() == 0 || (value() == safe_signed_int::min(result.precision()) && b.value() == -1)) {
                result.overflow(true);
            }
            else {
                result.value(value() % b.value());
            }
            return result;
        }

        safe_signed_int operator~() const
        {
            safe_signed_int result(*this);

            result.value(~result.value());

            return result;
        }

        safe_signed_int operator&(safe_signed_int b) const
        {
            safe_signed_int result(precisions::max(precision(), b.precision()));

            result.value(value() & b.value());

            return result;
        }

        safe_signed_int operator|(safe_signed_int b) const
        {
            safe_signed_int result(precisions::max(precision(), b.precision()));

            result.value(value() | b.value());

            return result;
        }

        safe_signed_int operator^(safe_signed_int b) const
        {
            safe_signed_int result(precisions::max(precision(), b.precision()));

            result.value(value() ^ b.value());

            return result;
        }

        safe_signed_int operator<<(safe_signed_int b) const 
        {
            safe_signed_int result(*this);

            if (b.value() < 0) result.overflow(true);

            result.value(result.value() << b.value());

            return result;
        }

        safe_signed_int operator>>(safe_signed_int b) const
        {
            safe_signed_int result(*this);

            if (b.value() < 0) result.overflow(true);

            result.value(result.value() >> b.value());

            return result;
        }

        bool operator<(safe_signed_int b) const { return value_ < b.value_; }

        bool operator<=(safe_signed_int b) const { return value_ <= b.value_; }

        bool operator>(safe_signed_int b) const { return value_ > b.value_; }

        bool operator>=(safe_signed_int b) const { return value_ >= b.value_; }

        bool operator==(safe_signed_int b) const { return value_ == b.value_; }

        bool operator!=(safe_signed_int b) const { return value_ != b.value_; }
    private:
        unsigned precision_ : 3;
        unsigned overflow_ : 1;
        int64_t value_;
    };

    using qfloat = long double;

    class safe_float {
    public:
        static qfloat min(size_t precision)
        {
            switch (precision) {
                case precisions::bits32:
                    return std::numeric_limits<float>::min();
                case precisions::bits64:
                    return std::numeric_limits<double>::min();
                case precisions::bits128:
                    return std::numeric_limits<qfloat>::min();
                default:
                    break;
            }

            throw std::invalid_argument("utils::safe_float::min(): invalid precision " + std::to_string(precision));
        }
        
        static qfloat max(size_t precision)
        {
            switch (precision) {
                case precisions::bits32:
                    return std::numeric_limits<float>::max();
                case precisions::bits64:
                    return std::numeric_limits<double>::max();
                case precisions::bits128:
                    return std::numeric_limits<qfloat>::max();
                default:
                    break;
            }

            throw std::invalid_argument("utils::safe_float::max(): invalid precision " + std::to_string(precision));
        }

        static qfloat inf(size_t precision)
        {
            switch (precision) {
                case precisions::bits32:
                    return std::numeric_limits<float>::infinity();
                case precisions::bits64:
                    return std::numeric_limits<double>::infinity();
                case precisions::bits128:
                    return std::numeric_limits<qfloat>::infinity();
                default:
                    break;
            }

            throw std::invalid_argument("utils::safe_float::inf(): invalid precision " + std::to_string(precision));
        }

        static qfloat nan(size_t precision)
        {
            switch (precision) {
                case precisions::bits32:
                    return std::numeric_limits<float>::quiet_NaN();
                case precisions::bits64:
                    return std::numeric_limits<double>::quiet_NaN();
                case precisions::bits128:
                    return std::numeric_limits<qfloat>::quiet_NaN();
                default:
                    break;
            }

            throw std::invalid_argument("utils::safe_float::nan(): invalid precision " + std::to_string(precision));
        }

        explicit safe_float(size_t precision) : precision_(precision), overflow_(0), underflow_(0), invalid_(0), zerodiv_(0), inexact_(0), value_(0) {}
        safe_float() : safe_float(precisions::bits32) {}
        safe_float(safe_unsigned_int u) : precision_(precisions::max(precisions::bits32, u.precision())), overflow_(0), underflow_(0), invalid_(0), zerodiv_(0), inexact_(0), value_(u.value()) {}
        safe_float(safe_signed_int s) : precision_(precisions::max(precisions::bits32, s.precision())), overflow_(0), underflow_(0), invalid_(0), zerodiv_(0), inexact_(0), value_(s.value()) {}
        
        size_t precision() const { return precision_; }
        void precision(size_t p) 
        { 
            precision_ = p;
        }
        
        void value(qfloat val) { value_ = val;  }
        qfloat value() const { return value_; }
        
        size_t size() const { return 1 << (precision_ + 3); }
        void size(size_t s) { precision(static_cast<size_t>(log2(s)) - 3); }
        
        bool overflow() const { return overflow_; }
        void overflow(bool set) { overflow_ = set; }

        bool undeflow() const { return underflow_; }
        void undeflow(bool set) { underflow_ = set; }

        bool invalid() const { return invalid_; }
        void invalid(bool set) { invalid_ = set; }

        bool zerodiv() const { return zerodiv_; }
        void zerodiv(bool set) { zerodiv_ = set; }

        bool inexact() const { return inexact_; }
        void inexact(bool set) { inexact_ = set; }

        class guard {
        public:
            guard(safe_float& f) : value_(f) { std::feclearexcept(FE_ALL_EXCEPT); }
            ~guard() {
                if (fetestexcept(FE_OVERFLOW)) value_.overflow(true);
                if (fetestexcept(FE_UNDERFLOW)) value_.undeflow(true);
                if (fetestexcept(FE_DIVBYZERO)) value_.zerodiv(true);
                if (fetestexcept(FE_INVALID)) value_.invalid(true);
                if (fetestexcept(FE_INEXACT)) value_.inexact(true);
                std::feclearexcept(FE_ALL_EXCEPT);
            }  
        private:
            safe_float& value_;
        };

        safe_float operator+() const { return *this; }

        safe_float operator-() const 
        { 
            safe_float result(*this);
            safe_float::guard guard(result);
            result.value(-result.value());
            return result; 
        }

        safe_float operator+(safe_float b) const
        {
            safe_float result(precisions::max(precision(), b.precision()));
            safe_float::guard guard(result);

            switch (result.precision()) {
                case precisions::bits32:
                    result.value(static_cast<float>(value()) + static_cast<float>(b.value()));    
                case precisions::bits64:
                    result.value(static_cast<double>(value()) + static_cast<double>(b.value()));  
                case precisions::bits128:
                    result.value(static_cast<qfloat>(value()) + static_cast<qfloat>(b.value()));  
                default:
                    break;
            }

            return result;
        }

        safe_float operator-(safe_float b) const
        {
            safe_float result(precisions::max(precision(), b.precision()));
            safe_float::guard guard(result);

            switch (result.precision()) {
                case precisions::bits32:
                    result.value(static_cast<float>(value()) - static_cast<float>(b.value()));    
                case precisions::bits64:
                    result.value(static_cast<double>(value()) - static_cast<double>(b.value()));  
                case precisions::bits128:
                    result.value(static_cast<qfloat>(value()) - static_cast<qfloat>(b.value()));  
                default:
                    break;
            }

            return result;
        }

        safe_float operator*(safe_float b) const
        {
            safe_float result(precisions::max(precision(), b.precision()));
            safe_float::guard guard(result);

            switch (result.precision()) {
                case precisions::bits32:
                    result.value(static_cast<float>(value()) * static_cast<float>(b.value()));    
                case precisions::bits64:
                    result.value(static_cast<double>(value()) * static_cast<double>(b.value()));  
                case precisions::bits128:
                    result.value(static_cast<qfloat>(value()) * static_cast<qfloat>(b.value()));  
                default:
                    break;
            }

            return result;
        }

        safe_float operator/(safe_float b) const
        {
            safe_float result(precisions::max(precision(), b.precision()));
            safe_float::guard guard(result);

            switch (result.precision()) {
                case precisions::bits32:
                    result.value(static_cast<float>(value()) / static_cast<float>(b.value()));    
                case precisions::bits64:
                    result.value(static_cast<double>(value()) / static_cast<double>(b.value()));  
                case precisions::bits128:
                    result.value(static_cast<qfloat>(value()) / static_cast<qfloat>(b.value()));  
                default:
                    break;
            }

            return result;
        }

        safe_float& operator+=(safe_float b)
        {
            safe_float::guard guard(*this);

            *this = *this + b;

            return *this;
        }

        safe_float& operator-=(safe_float b)
        {
            safe_float::guard guard(*this);

            *this = *this - b;

            return *this;
        }

        bool operator<(safe_float b) const { return value_ < b.value_; }

        bool operator<=(safe_float b) const { return value_ <= b.value_; }

        bool operator>(safe_float b) const { return value_ > b.value_; }

        bool operator>=(safe_float b) const { return value_ >= b.value_; }

        bool operator==(safe_float b) const { return value_ == b.value_; }

        bool operator!=(safe_float b) const { return value_ != b.value_; }
    private:
        unsigned precision_ : 3;
        unsigned overflow_ : 1;
        unsigned underflow_ : 1;
        unsigned invalid_ : 1;
        unsigned zerodiv_ : 1;
        unsigned inexact_ : 1;
        qfloat value_;
    };

    class safe_complex {
    public:
        explicit safe_complex(size_t precision) : value_(safe_float(precision - 1), safe_float(precision - 1)) {}
        safe_complex() : safe_complex(precisions::bits64) {}
        safe_complex(safe_unsigned_int u) : value_(safe_float(u), safe_float(u.precision())) {}
        safe_complex(safe_signed_int s) : value_(safe_float(s), safe_float(s.precision())) {}
        safe_complex(safe_float f) : value_(f, safe_float(f.precision())) {}

        ~safe_complex() {}
        
        size_t precision() const { return 1 + value_.real().precision(); }
        void precision(size_t p) 
        {
            auto r = value_.real(), i = value_.imag();
            
            r.precision(p - 1);
            i.precision(p - 1);
            value_.real(r);
            value_.imag(i);
        }

        void value(std::complex<safe_float> val) { value_ = val; }
        std::complex<safe_float> value() const { return value_; }

        void real(safe_float val) 
        {
            auto r = val, i = value_.imag();
            
            r.precision(precisions::max(val.precision(), i.precision()));
            i.precision(r.precision());
            value_.real(r);
            value_.imag(i);
        }
        safe_float real() const { return value_.real(); }

        void imag(safe_float val)
        {
            auto r = value_.real(), i = val;
            
            r.precision(precisions::max(r.precision(), val.precision()));
            i.precision(r.precision());
            value_.real(r);
            value_.imag(i);
        }
        safe_float imag() const { return value_.imag(); }
        
        size_t size() const { return 1 << (value_.real().precision() + 4); }
        void size(size_t s)
        {
            auto r = value_.real(), i = value_.imag();
            
            r.size(s / 2);
            i.size(s / 2);
            value_.real(r);
            value_.imag(i);
        }
        
        bool overflow() const { return value_.real().overflow() || value_.imag().overflow(); }

        bool undeflow() const { return value_.real().undeflow() || value_.imag().undeflow(); }

        bool invalid() const { return value_.real().invalid() || value_.imag().invalid(); }

        bool zerodiv() const { return value_.real().zerodiv() || value_.imag().zerodiv(); }

        bool inexact() const { return value_.real().inexact() || value_.imag().inexact(); }

        utils::safe_complex operator+() { return *this; }

        safe_complex operator-() 
        {
            std::feclearexcept(FE_ALL_EXCEPT);

            safe_complex result(*this);
            safe_float re = real(), im = imag();
            std::complex<qfloat> z { real().value(), imag().value() };

            z = -z;
            re.value(z.real());
            im.value(z.imag());
            
            if (fetestexcept(FE_OVERFLOW)) { re.overflow(true); im.overflow(true); }
            if (fetestexcept(FE_UNDERFLOW)) { re.undeflow(true); im.undeflow(true); }
            if (fetestexcept(FE_DIVBYZERO)) { re.zerodiv(true); im.zerodiv(true); }
            if (fetestexcept(FE_INVALID)) { re.invalid(true); im.invalid(true); }
            if (fetestexcept(FE_INEXACT)) { re.inexact(true); im.inexact(true); }

            result.value({ re, im });

            return result;
        }

        safe_complex operator+(safe_complex b) 
        {
            std::feclearexcept(FE_ALL_EXCEPT);

            safe_complex result(precisions::max(precision(), b.precision()));
            safe_float re = real(), im = imag();
            std::complex<qfloat> z1 { real().value(), imag().value() }, z2 { b.real().value(), b.imag().value() };

            z1 = z1 + z2;
            re.value(z1.real());
            im.value(z1.imag());
            
            if (fetestexcept(FE_OVERFLOW)) { re.overflow(true); im.overflow(true); }
            if (fetestexcept(FE_UNDERFLOW)) { re.undeflow(true); im.undeflow(true); }
            if (fetestexcept(FE_DIVBYZERO)) { re.zerodiv(true); im.zerodiv(true); }
            if (fetestexcept(FE_INVALID)) { re.invalid(true); im.invalid(true); }
            if (fetestexcept(FE_INEXACT)) { re.inexact(true); im.inexact(true); }

            result.value({ re, im });
            
            return result;
        }

        safe_complex operator-(safe_complex b) 
        {
            std::feclearexcept(FE_ALL_EXCEPT);

            safe_complex result(precisions::max(precision(), b.precision()));
            safe_float re = real(), im = imag();
            std::complex<qfloat> z1 { real().value(), imag().value() }, z2 { b.real().value(), b.imag().value() };

            z1 = z1 - z2;
            re.value(z1.real());
            im.value(z1.imag());
            
            if (fetestexcept(FE_OVERFLOW)) { re.overflow(true); im.overflow(true); }
            if (fetestexcept(FE_UNDERFLOW)) { re.undeflow(true); im.undeflow(true); }
            if (fetestexcept(FE_DIVBYZERO)) { re.zerodiv(true); im.zerodiv(true); }
            if (fetestexcept(FE_INVALID)) { re.invalid(true); im.invalid(true); }
            if (fetestexcept(FE_INEXACT)) { re.inexact(true); im.inexact(true); }

            result.value({ re, im });
            
            return result;
        }

        safe_complex operator*(safe_complex b) 
        {
            std::feclearexcept(FE_ALL_EXCEPT);

            safe_complex result(precisions::max(precision(), b.precision()));
            safe_float re = real(), im = imag();
            std::complex<qfloat> z1 { real().value(), imag().value() }, z2 { b.real().value(), b.imag().value() };

            z1 = z1 * z2;
            re.value(z1.real());
            im.value(z1.imag());
            
            if (fetestexcept(FE_OVERFLOW)) { re.overflow(true); im.overflow(true); }
            if (fetestexcept(FE_UNDERFLOW)) { re.undeflow(true); im.undeflow(true); }
            if (fetestexcept(FE_DIVBYZERO)) { re.zerodiv(true); im.zerodiv(true); }
            if (fetestexcept(FE_INVALID)) { re.invalid(true); im.invalid(true); }
            if (fetestexcept(FE_INEXACT)) { re.inexact(true); im.inexact(true); }

            result.value({ re, im });
            
            return result;
        }

        safe_complex operator/(safe_complex b) 
        {
            std::feclearexcept(FE_ALL_EXCEPT);

            safe_complex result(precisions::max(precision(), b.precision()));
            safe_float re = real(), im = imag();
            std::complex<qfloat> z1 { real().value(), imag().value() }, z2 { b.real().value(), b.imag().value() };

            z1 = z1 / z2;
            re.value(z1.real());
            im.value(z1.imag());

            if (fetestexcept(FE_OVERFLOW)) { re.overflow(true); im.overflow(true); }
            if (fetestexcept(FE_UNDERFLOW)) { re.undeflow(true); im.undeflow(true); }
            if (fetestexcept(FE_DIVBYZERO)) { re.zerodiv(true); im.zerodiv(true); }
            if (fetestexcept(FE_INVALID)) { re.invalid(true); im.invalid(true); }
            if (fetestexcept(FE_INEXACT)) { re.inexact(true); im.inexact(true); }

            result.value({ re, im });
            
            return result;
        }

        bool operator==(safe_complex b) const { return value_ == b.value_; }

        bool operator!=(safe_complex b) const { return value_ != b.value_; }
    private:
        std::complex<safe_float> value_;
    };

    class safe_rational {
        static int64_t gcd(int64_t x, int64_t y)
        {
            if (y == 0) return x;
            return gcd(y, x % y);
        }

        static int64_t lcm(int64_t x, int64_t y)
        {
            return x * y / gcd(x, y);
        }
    public:
        explicit safe_rational(size_t precision) : numerator_(precision - 1), denominator_(precision - 1) {}
        safe_rational(safe_signed_int numerator, safe_signed_int denominator)
        {
            value(numerator, denominator);
        }
        safe_rational() : safe_rational(precisions::bits64) 
        {
            numerator_.value(0);
            denominator_.value(1);
        }
        safe_rational(safe_unsigned_int u) : numerator_(u), denominator_(u.precision()) 
        {
            denominator_.value(1);
        }
        safe_rational(safe_signed_int s) : numerator_(s), denominator_(s.precision()) 
        {
            denominator_.value(1);
        }
        safe_rational(safe_float f) : safe_rational(f.precision() + 1)
        {
            int64_t n, d;
            approximate(f.value(), n, d);
            numerator_.value(n);
            denominator_.value(d);
        }

        safe_signed_int numerator() const { return numerator_; }
        safe_signed_int denominator() const { return denominator_; }
        
        safe_float real() const
        {
            safe_float re(numerator_.precision());
            re.value(static_cast<qfloat>(numerator_.value()) / denominator_.value());
            return re;
        }

        void value(safe_signed_int numerator, safe_signed_int denominator)
        {
            precision(1 + precisions::max(numerator.precision(), denominator.precision()));
            int64_t divisor = gcd(numerator.value(), denominator.value());
            
            if (divisor != 0) {
                numerator_.value(numerator.value() / divisor);
                denominator_.value(denominator.value() / divisor);
            }

            if (denominator.value() == 0) numerator_.overflow(true);
        }

        size_t precision() const { return 1 + numerator_.precision(); }
        void precision(size_t p) 
        {
            numerator_.precision(p - 1);
            denominator_.precision(p - 1);
        }

        size_t size() const { return 1 << (numerator_.precision() + 4); }
        void size(size_t s) 
        { 
            numerator_.size(s / 2);
            denominator_.size(s / 2);
        }

        bool overflow() const { return numerator_.overflow() || denominator_.overflow(); }

        safe_rational operator+() const { return *this; }

        safe_rational operator-() const 
        { 
            safe_rational result(*this);
            result.numerator_ = -result.numerator_;
            return result; 
        }

        safe_rational operator+(safe_rational b) const
        {
            safe_rational result(precisions::max(precision(), b.precision()));
            safe_signed_int common(result.precision() - 1);
            common.value(lcm(numerator_.value(), denominator_.value()));

            if (common.value() != 0) {
                result.value(numerator_ * common / denominator_ + b.numerator_ * common / b.denominator_, common);
            }
            
            if (b.numerator().value() == 0) result.numerator_.overflow(true);

            return result;
        }

        safe_rational operator-(safe_rational b) const
        {
            safe_rational result(precisions::max(precision(), b.precision()));
            safe_signed_int common(result.precision() - 1);
            common.value(lcm(numerator_.value(), denominator_.value()));

            if (common.value() != 0) {
                result.value(numerator_ * common / denominator_ - b.numerator_ * common / b.denominator_, common);
            }
            
            if (b.numerator().value() == 0) result.numerator_.overflow(true);

            return result;
        }

        safe_rational operator*(safe_rational b) const
        {
            safe_rational result(precisions::max(precision(), b.precision()));

            result.value(numerator_ * b.numerator(), denominator_ * b.denominator());

            if (b.numerator().value() == 0) result.numerator_.overflow(true);

            return result;
        }

        safe_rational operator/(safe_rational b) const
        {
            safe_rational result(precisions::max(precision(), b.precision()));

            result.value(numerator_ * b.denominator(), denominator_ * b.numerator());

            if (b.numerator().value() == 0) result.numerator_.overflow(true);

            return result;
        }

        bool operator<(safe_rational b) const { return numerator_.value() * b.denominator_.value() < b.numerator_.value() * denominator_.value(); }

        bool operator<=(safe_rational b) const { return numerator_.value() * b.denominator_.value() <= b.numerator_.value() * denominator_.value(); }

        bool operator>(safe_rational b) const { return numerator_.value() * b.denominator_.value() > b.numerator_.value() * denominator_.value(); }

        bool operator>=(safe_rational b) const { return numerator_.value() * b.denominator_.value() >= b.numerator_.value() * denominator_.value(); }

        bool operator==(safe_rational b) const { return numerator_.value() * b.denominator_.value() == b.numerator_.value() * denominator_.value(); }

        bool operator!=(safe_rational b) const { return numerator_.value() * b.denominator_.value() != b.numerator_.value() * denominator_.value(); }
    private:
        static void approximate(qfloat real, int64_t& numerator, int64_t& denominator, int64_t max_denominator = 1000)
        {
            int64_t a, h[3] = { 0, 1, 0 }, k[3] = { 1, 0, 0 };
            int64_t x, d, n = 1;
            int i, neg = 0;
        
            if (max_denominator <= 1) { 
                denominator = 1; 
                numerator = static_cast<int64_t>(real); 
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

        safe_signed_int numerator_, denominator_;
    };

    inline uint64_t intpow(uint64_t base, uint64_t exp)
    {
        if (exp == 0) {
            return 1;
        }
        else if (exp % 2 == 0) {
            uint64_t half = intpow(base, exp / 2);
            return half * half;
        }
        
        return base * intpow(base, exp - 1);
    }

    inline safe_rational pow(safe_signed_int a, safe_signed_int b)
    {
        safe_rational result(1 + precisions::max(a.precision(), b.precision()));
        safe_signed_int num(result.precision() - 1);
        safe_signed_int den(result.precision() - 1);

        if (b.value() < 0) {
            num.value(1);
            den.value(intpow(a.value(), -b.value()));
        }
        else {
            num.value(intpow(a.value(), b.value()));
            den.value(1);
        }

        result.value(num, den);

        return result;
    }

    inline safe_float powf(safe_float a, safe_float b)
    {
        std::feclearexcept(FE_ALL_EXCEPT);

        safe_float result(precisions::max(a.precision(), b.precision()));

        result.value(std::pow(a.value(), b.value()));

        if (fetestexcept(FE_OVERFLOW)) result.overflow(true);
        if (fetestexcept(FE_UNDERFLOW)) result.undeflow(true);
        if (fetestexcept(FE_DIVBYZERO)) result.zerodiv(true);
        if (fetestexcept(FE_INVALID)) result.invalid(true);
        if (fetestexcept(FE_INEXACT)) result.inexact(true);
        
        std::feclearexcept(FE_ALL_EXCEPT);

        return result;
    }

    inline safe_complex powc(safe_complex a, safe_complex b)
    {
        std::feclearexcept(FE_ALL_EXCEPT);

        safe_complex result(precisions::max(a.precision(), b.precision()));
        std::complex<qfloat> base(a.value().real().value(), a.value().imag().value()), exp(b.value().real().value(), b.value().imag().value()), power = std::pow(base, exp);
        safe_float re(result.precision() - 1), im(result.precision() - 1);

        re.value(power.real());
        im.value(power.imag());

        if (fetestexcept(FE_OVERFLOW)) { re.overflow(true); im.overflow(true); }
        if (fetestexcept(FE_UNDERFLOW)) { re.undeflow(true); im.undeflow(true); }
        if (fetestexcept(FE_DIVBYZERO)) { re.zerodiv(true); im.zerodiv(true); }
        if (fetestexcept(FE_INVALID)) { re.invalid(true); im.invalid(true); }
        if (fetestexcept(FE_INEXACT)) { re.inexact(true); im.inexact(true); }

        result.value({re, im});

        return result;
    }
}

inline std::ostream& operator<<(std::ostream& os, utils::safe_unsigned_int a)
{
    return os << a.value();
}

inline std::ostream& operator<<(std::ostream& os, utils::safe_signed_int a)
{
    return os << a.value();
}

inline std::ostream& operator<<(std::ostream& os, utils::safe_float a)
{
    return os << a.value();
}

inline std::ostream& operator<<(std::ostream& os, utils::safe_complex a)
{
    os << a.value().real().value();
    if (a.value().imag().value() < 0) os << a.value().imag().value() << 'i';
    else os << '+' << a.value().imag().value() << 'i';
    return os;
}

inline std::ostream& operator<<(std::ostream& os, utils::safe_rational a)
{
    if (a.numerator().value() * a.denominator().value() < 0) os << '-';
    os << std::abs(a.numerator().value()) << '/' << std::abs(a.denominator().value());
    return os;
}

#endif