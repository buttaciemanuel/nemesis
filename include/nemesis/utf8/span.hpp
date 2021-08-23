/**
 * @file span.hpp
 * @author Emanuel Buttaci
 * This file contains functions and classes to manipulated UTF-8 encoded data
 * 
 */
#ifndef SPAN_HPP
#define SPAN_HPP

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace nemesis {
    /**
     * Byte type used in nemesis compiler
     */
    using byte = uint8_t;
    /**
     * Unicode value type used in nemesis compiler
     */
    using codepoint = uint32_t;
}

namespace nemesis {
    namespace utf8 {
        /**
         * Encodes the unicode code point as UTF-8 code units
         * 
         * @note UTF-8 encoding uses from 1 to 4 bytes, so units must be large enough
         * @param value Unicode code point (for example U+1927 (🤧) is encoded in bytes 0xf0 0x9f 0xa4 0xa7)
         * @param units UTF-8 code units
         * @return Number of code units used for the encoding
         */
        int encode(codepoint value, byte *units);

        /**
         * Decodes these code units into a unicode code point
         * 
         * @param units UTF-8 Code units
         * @return Decoded code point 
         */
        codepoint decode(const byte *units);

        /**
         * Computes number of columns occupied by a 
         * character displayed on the screen
         * 
         * @param val Unicode code point
         * @return Width as number of columns
         */
        int width(codepoint val);
        
        /**
         * Holds a reference to UTF-8 encoded string. It has a
         * double behaviour as it can be just an OPAQUE REFERENCE
         * to the buffer OR it can be a UNIQUE OWNER of the buffer.
         * @see Internally it is a sequence of raw bytes 
         * which are UTF-8 code units
         * @note A string span does not own memory by default,
         * so any dynamically allocated memory must be freed in
         * other ways. To make a span owner of its buffer it
         * must be constructed properly through its builder class
         */
        class span {
        public:
            class builder;
            class iterator;
            /**
             * Construct by default an empty span 
             */
            span() = default;

            /**
             * Construct a new string span object
             * 
             * @tparam CCharType char or unsigned char type
             * @param data Encoded raw byte units
             * @param size Number of total bytes to consider
             * @param owner Span will own its buffer
             */
            template<typename CCharType>
            span(const CCharType *data, int size, bool owner = false);

            /**
             * Construct a new string span object 
             * @note The constructor will look for '\0' terminator to compute size
             * 
             * @tparam CCharType char or unsigned char type
             * @param data Encoded raw byte units
             * @param owner Span will own its buffer
             */
            template<typename CCharType>
            explicit span(const CCharType *data, bool owner = false);

            /**
             * Construct a new span from iterators range
             * 
             * @param begin Iterator pointing to beginning of span
             * @param end Interator pointing to one character past the end of the span
             * @param owner Span will own its buffer
             */
            span(iterator begin, iterator end, bool owner = false);

            /**
             * Copy a span, it is a owner
             * then reallocation is made for the copy
             * @param other Copy of the original span
             */
            span(const span& other);

            /**
             * Move a span, transferring ownership to
             * the other span eventually
             * @param other New span (eventually owner)
             */
            span(span&& other);

            /**
             * Copy assign operator
             * @param other Copy of span
             */
            span& operator=(const span& other);

            /**
             * Move assign operator
             * @param other Move copy of span
             */
            span& operator=(span&& other);

            /**
             * Destructor
             * @note Calls span::clear()
             */
            ~span();

            /**
             * Clear the buffer
             * @note If it is a owner, then is deallocates the byte buffer
             * @note This is called by default in the constructor
             */
            void clear();
            
            /**
             * @return Number of bytes
             */
            int size() const;
            
            /**
             * @return Pointer to raw data
             */
            byte *data() const;

            /**
             * @return Pointer to raw data as C string
             */
            char *cdata() const;

            /**
             * @return String version
             */
            std::string string() const;

            /**
             * Constructs a subspan of characters from [start, end)
             * @note Time complexity is O(n) because of iteration
             * @note Subspan references to the same buffer is this one is not an owner
             * otherwise a new buffer is allocated for the subspan
             * @param start Index of first unicode character
             * @param end Index of last (excluded) character
             * 
             * @return Supspan
             */
            span subspan(int start, int end) const;

            /**
             * Constructs a subspan of characters from [start, end) with iterators
             * @note Time complexity is O(1) because iterators already contain byte indices
             * @note Subspan references to the same buffer is this one is not an owner
             * otherwise a new buffer is allocated for the subspan
             * @param start Iterator to first unicode character
             * @param end Iterator to last (excluded) character
             * 
             * @return Supspan
             */
            span subspan(iterator start, iterator end) const;
            
            /**
             * Returns the unicode value of encoded character
             * @note Time complexity is O(n) because of iteration
             * @param index Index of character
             * @return Code point of encoded UTF-8 character 
             */
            codepoint at(int index) const;

            /**
             * @see span#at
             */
            codepoint operator[](int index) const;

            /**
             * @brief Searches first occurrence of character
             * @param character Character to search
             * @return Iterator to position or end()
             */
            iterator find(codepoint character) const;

            /**
             * Computes the number of unicode characters inside the string
             * @note Time complexity is O(n) because of iteration
             * @return Number of unicode characters.
             */
            int length() const;

            /**
             * Computes total width occupied by characters as column spaces 
             * @note Time complexity is O(n) because of linear traversal
             * @return Total number of column spaces
             */
            int width() const;

            /**
             * Tells whether the span owns its buffer
             * @return true if the span has control over its buffer existence
             * @return false otherwise
             */
            bool owns() const;
            
            /**
             * Constructs an iterator pointing at the beginning
             * 
             * @return iterator 
             */
            iterator begin() const;

            /**
             * Constructs an iterator pointing at past the end
             * 
             * @return iterator 
             */
            iterator end() const;

            /**
             * Compare lexicographically two string spans
             * 
             * @param other Other span to be compared
             * @return 0 If equals
             * @return 1 If this span is greater than the other
             * @return -1 Otherwise
             */
            int compare(const span& other) const;

            /**
             * Compares two string spans for equality
             * 
             * @param other Other span to be compared
             * @return true If compare(other) == 0
             * @return false Otherwise
             * @see span#compare
             */
            bool operator==(const span& other) const;

            /**
             * This builder constructs a owning span which has control over the
             * underlying buffer life
             * 
             */
            class builder {
                friend class span;
            public:
                /**
                 * Constructs a builder
                 */
                builder();
                /**
                 * Add a unicode character at the tail of the buffer
                 * @param character Unicode character (code point)
                 * @note the character is then encoded in UTF-8
                 * @return reference to the builder itslef
                 */
                builder& add(codepoint character);
                /**
                 * Add all the bytes in the buffer
                 * @tparam CCharType Character type
                 * @param data Bytes
                 * @param n Number of bytes
                 * @return reference to the builder itslef
                 */
                template<typename CCharType>
                builder& concat(CCharType *data, size_t n);
                /**
                 * Add all the bytes in the buffer
                 * @tparam CCharType Character type
                 * @param data Bytes
                 * @return reference to the builder itslef
                 */
                template<typename CCharType>
                builder& concat(CCharType *data);
                /**
                 * Builds a owner span over the buffer
                 * @return Owner span
                 */
                span build() const;
            private:
                /**
                 * Underlying UTF-8 byte buffer
                 */
                std::vector<byte> bytes_;
            };

            /**
             * This iterator walk through the string and returns its unicode values.
             */
            class iterator {
                friend class span;
            public:
                iterator() = default;
                /**
                 * Construct a new iterator object, called by span
                 * 
                 * @param ptr Pointer to raw code units;
                 */
                iterator(byte *ptr);
                /**
                 * Increments iterator position
                 * 
                 * @return Non incremented iterator
                 */
                iterator operator++(int);

                /**
                 * Increments iterator position
                 * 
                 * @return Reference to this iterator
                 */
                iterator& operator++();

                /**
                 * Increments iterator position of counter
                 * 
                 * @note count MUST be POSITIVE
                 * @param count Number of character position to advance
                 * @return Reference to this iterator
                 */
                iterator operator+(int count) const;

                /**
                 * Returns unicode value of current UTF-8 encoded character
                 * @return codepoint 
                 */
                codepoint operator*() const;

                /**
                 * Compares two iterator instances
                 * 
                 * @param other Other iterator to compare
                 * @return true If they point to the same character of the same span
                 * @return false Otherwise
                 */
                bool operator==(const iterator& other) const;

                /**
                 * Compares two iterator instances
                 * 
                 * @param other Other iterator to compare
                 * @return false If they point to the same character of the same span
                 * @return true Otherwise
                 */
                bool operator!=(const iterator& other) const;

                /**
                 * Compares two iterator instances
                 * 
                 * @param other Other iterator to compare
                 * @return false If i1 < i2
                 * @return true Otherwise
                 */
                bool operator<(const iterator& other) const;

                /**
                 * Compares two iterator instances
                 * 
                 * @param other Other iterator to compare
                 * @return false If i2 > i1
                 * @return true Otherwise
                 */
                bool operator>(const iterator& other) const;
        private:
                /**
                 * Pointer to current code unit
                 */
                byte *ptr_ = nullptr;
            };
        private:
            /**
             * Raw code units.
             */
            byte *units_ = nullptr;

            /**
             * Number of raw bytes.
             */
            int size_ = 0;

            /**
             * This flags is true if the
             * span owns the byte buffer
             * which means that it deallocates
             * it on destruction
             */
            bool owns_ = false;
        };

        template<typename CCharType>
        span::span(const CCharType *span, int size, bool owner) :
            size_{size},
            owns_{owner}
        {
            static_assert(std::is_same<CCharType, char>::value || 
                        std::is_same<CCharType, unsigned char>::value,
                        "span::span(): buffer type must be char or unsigned char");

            if (owner) {
                units_ = new byte[size];
                std::copy(reinterpret_cast<const byte*>(span), reinterpret_cast<const byte*>(span) + size, units_);
            }
            else {
                units_ = const_cast<byte*>(reinterpret_cast<const byte*>(span));
            }

        }

        template<typename CCharType>
        span::span(const CCharType *span, bool owner) :
            span::span(span, static_cast<int>(std::strlen(reinterpret_cast<const char*>(span))), owner)
        {}

        template<typename CCharType>
        span::builder& span::builder::concat(CCharType *data, size_t n)
        {
            for (size_t i = 0; i < n; ++i) bytes_.push_back(data[i]);

            return *this;
        }

        template<typename CCharType>
        span::builder& span::builder::concat(CCharType *data)
        {
            return concat(data, std::strlen(static_cast<const char*>(data)));   
        }
    }
}

/**
 * Prints a UTF-8 encoded string to an output stream
 * 
 * @tparam StreamType Type of output stream
 * @param os Reference to output stream
 * @param buf UTF-8 encoded span
 * @return Reference to output stream
 */
template <typename StreamType>
StreamType& operator<<(StreamType& os, const nemesis::utf8::span& buf) 
{
    for (nemesis::byte *ptr = buf.data(); ptr < buf.data() + buf.size(); ++ptr) {
        os << *ptr;
    }

    return os;
}

namespace std {
    /**
     * FNV-1 hash algorithms with parameters
     * depending on the size of the hash in bits
     */
    template<std::size_t bits>
    struct fnv1 {};

    template<>
    struct fnv1<32> {
        using type = std::uint32_t;
        static constexpr type prime = 16777619u;
        static constexpr type basis = 2166136261u;
    };

    template<>
    struct fnv1<64> {
        using type = std::uint64_t;
        static constexpr type prime = 1099511628211ul;
        static constexpr type basis = 14695981039346656037ul;
    };

    /**
     * Computes FNV-1 hash of bytes
     */
    template<> struct hash<nemesis::utf8::span> {
        /**
         * @param s span of bytes
         * @return Hash of bytes
         */
        std::size_t operator()(const nemesis::utf8::span& s) const noexcept
        {
            std::size_t hash = fnv1<sizeof(std::size_t) * 8>::basis;
            
            for (nemesis::byte *p = s.data(); p < s.data() + s.size(); ++p) {
                hash *= fnv1<sizeof(std::size_t) * 8>::prime;
                hash ^= static_cast<std::size_t>(*p);
            }
            
            return hash;
        }
    };
}

#endif // SPAN_HPP