#ifndef SHA256_HPP
#define SHA256_HPP

#include <string>

namespace utils {
    class sha256 {
    public:
        sha256() : blocklen_(0), bitlen_(0), state_ { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 } {}

        sha256& update(const std::uint8_t* data, size_t length) {
            for (size_t i = 0 ; i < length ; ++i) {
                data_[blocklen_++] = data[i];
                if (blocklen_ == 64) {
                    transform();
                    bitlen_ += 512;
                    blocklen_ = 0;
                }
            }

            return *this;
        }

        std::string hexdigest() {
            std::uint8_t hash[32];
            std::stringstream s;

            pad();
            revert(hash);
            
	        s << std::setfill('0') << std::hex;
	        for (auto i = 0 ; i < 32 ; ++i) s << std::setw(2) << static_cast<unsigned int>(hash[i]);

	        return s.str();
        }
    private:
        std::uint8_t  data_[64];
        std::uint32_t blocklen_;
        std::uint64_t bitlen_;
        std::uint32_t state_[8];

        static constexpr std::uint32_t K[64] = {
            0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,
            0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
            0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,
            0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
            0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,
            0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
            0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,
            0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
            0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,
            0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
            0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,
            0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
            0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,
            0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
            0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,
            0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
        };

        static std::uint32_t rotr(std::uint32_t x, std::uint32_t n) { return (x >> n) | (x << (32 - n)); }

        static std::uint32_t choose(std::uint32_t e, std::uint32_t f, std::uint32_t g) { return (e & f) ^ (~e & g); }

        static std::uint32_t majority(std::uint32_t a, std::uint32_t b, std::uint32_t c) { return (a & (b | c)) | (b & c); }

        static std::uint32_t sig0(std::uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }

        static std::uint32_t sig1(std::uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

        void transform() {
            std::uint32_t maj, xor_a, ch, xor_e, sum, new_a, new_e, m[64];
            std::uint32_t state[8];

            for (auto i = 0, j = 0; i < 16; ++i, j += 4) m[i] = (data_[j] << 24) | (data_[j + 1] << 16) | (data_[j + 2] << 8) | (data_[j + 3]);

            for (auto k = 16 ; k < 64; ++k) m[k] = sig1(m[k - 2]) + m[k - 7] + sig0(m[k - 15]) + m[k - 16];

            for(auto i = 0 ; i < 8 ; ++i) state[i] = state_[i];

            for (auto i = 0; i < 64; ++i) {
                maj   = majority(state[0], state[1], state[2]);
                xor_a  = rotr(state[0], 2) ^ rotr(state[0], 13) ^ rotr(state[0], 22);

                ch = choose(state[4], state[5], state[6]);

                xor_e  = rotr(state[4], 6) ^ rotr(state[4], 11) ^ rotr(state[4], 25);

                sum  = m[i] + K[i] + state[7] + ch + xor_e;
                new_a = xor_a + maj + sum;
                new_e = state[3] + sum;

                state[7] = state[6];
                state[6] = state[5];
                state[5] = state[4];
                state[4] = new_e;
                state[3] = state[2];
                state[2] = state[1];
                state[1] = state[0];
                state[0] = new_a;
            }

            for (auto i = 0 ; i < 8 ; i++) state_[i] += state[i];
        }

        void pad() {
            unsigned i = blocklen_, end = blocklen_ < 56 ? 56 : 64;

            data_[i++] = 0x80;

            while (i < end) data_[i++] = 0x00;

            if (blocklen_ >= 56) {
                transform();
                std::memset(data_, 0, 56);
            }

            bitlen_ += blocklen_ * 8;
            data_[63] = bitlen_;
            data_[62] = bitlen_ >> 8;
            data_[61] = bitlen_ >> 16;
            data_[60] = bitlen_ >> 24;
            data_[59] = bitlen_ >> 32;
            data_[58] = bitlen_ >> 40;
            data_[57] = bitlen_ >> 48;
            data_[56] = bitlen_ >> 56;
            
            transform();
        }

        void revert(std::uint8_t * hash) {
            for (auto i = 0 ; i < 4 ; ++i) {
                for (auto j = 0 ; j < 8 ; ++j) {
                    hash[i + (j * 4)] = (state_[j] >> (24 - i * 8)) & 0x000000ff;
                }
            }
        }
    };
}

#endif