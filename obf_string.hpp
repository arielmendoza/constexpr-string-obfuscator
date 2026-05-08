#pragma once

#include <array>
#include <cstdint>
#include <cstddef>
#include <string>
#include <type_traits>

#ifndef OBF_BUILD_KEY
#define OBF_BUILD_KEY 0xA5B4I18V9D24134Gfcd
#endif

namespace obf {

    // ------------------------------------------------------------
    // Rotate left 64-bit
    // ------------------------------------------------------------

    constexpr std::uint64_t rotl64(std::uint64_t x, unsigned r) {
        return static_cast<std::uint64_t>(
            (x << r) | (x >> (64u - r))
            );
    }

    // ------------------------------------------------------------
    // constexpr hash without large multiplications
    //
    // This avoids MSVC Code Analysis warnings such as:
    // "Arithmetic overflow: '*' operation causes overflow at compile time."
    // ------------------------------------------------------------

    constexpr std::uint64_t hash_constexpr(
        const char* str,
        std::uint64_t hash = 0xCBF29CE484222325ull
    ) {
        return *str
            ? hash_constexpr(
                str + 1,
                rotl64(
                    hash ^ static_cast<std::uint8_t>(*str),
                    5u
                ) ^ 0x9E3779B97F4A7C15ull
            )
            : hash;
    }

    // ------------------------------------------------------------
    // Lightweight mixer without multiplication
    //
    // Not cryptographic.
    // Used only to generate byte masks for string obfuscation.
    // ------------------------------------------------------------

    constexpr std::uint64_t mix64(std::uint64_t x) {
        x ^= x >> 33u;
        x = rotl64(x, 17u) ^ 0xA0761D6478BD642Full;
        x ^= x >> 29u;
        x = rotl64(x, 31u) ^ 0xE7037ED1A0B428DBull;
        x ^= x >> 32u;

        return x;
    }

    constexpr std::uint8_t key_byte(std::uint64_t seed, std::size_t index) {
        const std::uint64_t idx = static_cast<std::uint64_t>(index);

        const std::uint64_t mixed = mix64(
            seed ^
            rotl64(
                idx ^ 0xD6E8FEB86659FD93ull,
                static_cast<unsigned>((idx & 31u) + 1u)
            )
        );

        return static_cast<std::uint8_t>(
            (mixed >> static_cast<unsigned>((idx & 7u) * 8u)) & 0xFFu
            );
    }

    // ------------------------------------------------------------
    // Best-effort memory wipe
    // ------------------------------------------------------------

    inline void secure_zero(void* ptr, std::size_t size) {
        volatile std::uint8_t* p = static_cast<volatile std::uint8_t*>(ptr);

        while (size--) {
            *p++ = 0;
        }
    }

    // ------------------------------------------------------------
    // Transform one character byte-by-byte
    //
    // Works with:
    //   char
    //   wchar_t
    //   char16_t
    //   char32_t
    // ------------------------------------------------------------

    template <typename CharT>
    constexpr CharT transform_char(CharT ch, std::uint64_t seed, std::size_t char_index) {
        static_assert(
            std::is_integral_v<CharT>,
            "CharT must be an integral character type"
            );

        using UnsignedCharT = std::make_unsigned_t<CharT>;

        UnsignedCharT value = static_cast<UnsignedCharT>(ch);

        for (std::size_t b = 0; b < sizeof(CharT); ++b) {
            const UnsignedCharT key = static_cast<UnsignedCharT>(
                key_byte(seed, char_index * sizeof(CharT) + b)
                );

            const UnsignedCharT mask = static_cast<UnsignedCharT>(
                key << static_cast<unsigned>(b * 8u)
                );

            value = static_cast<UnsignedCharT>(value ^ mask);
        }

        return static_cast<CharT>(value);
    }

    // ------------------------------------------------------------
    // Runtime decrypted string
    //
    // This object contains the plaintext temporarily.
    // It wipes its internal buffer in the destructor.
    // ------------------------------------------------------------

    template <typename CharT, std::size_t N>
    class runtime_string {
    public:
        using char_type = CharT;
        using string_type = std::basic_string<CharT>;

        runtime_string(const std::array<CharT, N>& encrypted, std::uint64_t seed) {
            for (std::size_t i = 0; i < N; ++i) {
                buffer_[i] = transform_char(encrypted[i], seed, i);
            }
        }

        runtime_string(const runtime_string&) = delete;
        runtime_string& operator=(const runtime_string&) = delete;

        runtime_string(runtime_string&& other) noexcept {
            for (std::size_t i = 0; i < N; ++i) {
                buffer_[i] = other.buffer_[i];
            }

            secure_zero(other.buffer_.data(), N * sizeof(CharT));
        }

        runtime_string& operator=(runtime_string&& other) noexcept {
            if (this != &other) {
                secure_zero(buffer_.data(), N * sizeof(CharT));

                for (std::size_t i = 0; i < N; ++i) {
                    buffer_[i] = other.buffer_[i];
                }

                secure_zero(other.buffer_.data(), N * sizeof(CharT));
            }

            return *this;
        }

        ~runtime_string() {
            secure_zero(buffer_.data(), N * sizeof(CharT));
        }

        const CharT* c_str() const {
            return buffer_.data();
        }

        const CharT* data() const {
            return buffer_.data();
        }

        constexpr std::size_t size() const {
            return N > 0 ? N - 1 : 0;
        }

        constexpr bool empty() const {
            return size() == 0;
        }

        string_type str() const {
            return string_type(buffer_.data(), size());
        }

        operator string_type() const {
            return str();
        }

    private:
        std::array<CharT, N> buffer_{};
    };

    // ------------------------------------------------------------
    // Compile-time encrypted string
    // ------------------------------------------------------------

    template <typename CharT, std::size_t N>
    class encrypted_string {
    public:
        using char_type = CharT;

        constexpr encrypted_string(const CharT(&plain)[N], std::uint64_t seed)
            : encrypted_{},
            seed_(seed) {
            for (std::size_t i = 0; i < N; ++i) {
                encrypted_[i] = transform_char(plain[i], seed_, i);
            }
        }

        runtime_string<CharT, N> decrypt() const {
            return runtime_string<CharT, N>(encrypted_, seed_);
        }

    private:
        std::array<CharT, N> encrypted_;
        std::uint64_t seed_;
    };

} // namespace obf

// ------------------------------------------------------------
// Seed generation
//
// Notes for MSVC:
//
// - Do not use __LINE__ here in Debug /ZI.
//   MSVC may transform it into __LINE__Var and break constexpr.
//
// - This version avoids large constexpr multiplications to prevent
//   Code Analysis arithmetic overflow warnings.
//
// - __COUNTER__ gives a different seed per OBF(...) usage.
// ------------------------------------------------------------

#define OBF_MAKE_SEED()                                                     \
    (                                                                       \
        static_cast<std::uint64_t>(OBF_BUILD_KEY) ^                         \
        obf::hash_constexpr(__FILE__) ^                                     \
        obf::hash_constexpr(__DATE__) ^                                     \
        obf::hash_constexpr(__TIME__) ^                                     \
        obf::rotl64(                                                        \
            static_cast<std::uint64_t>(__COUNTER__) ^                       \
            0x85EBCA77C2B2AE63ull,                                         \
            13u                                                             \
        )                                                                   \
    )

// ------------------------------------------------------------
// Main macro
//
// Usage:
//   std::string s = OBF("hello");
//   std::wstring ws = OBF(L"hello");
//   auto tmp = OBF(L"hello");
//   SomeWindowsApiW(OBF(L"hello").c_str());
// ------------------------------------------------------------

#define OBF(str)                                                            \
    ([] {                                                                   \
        using obf_char_t = std::remove_cv_t<                                \
            std::remove_reference_t<decltype((str)[0])>                     \
        >;                                                                  \
                                                                            \
        constexpr std::uint64_t obf_seed = OBF_MAKE_SEED();                 \
                                                                            \
        constexpr obf::encrypted_string<                                    \
            obf_char_t,                                                     \
            sizeof(str) / sizeof((str)[0])                                  \
        > obf_encrypted((str), obf_seed);                                   \
                                                                            \
        return obf_encrypted.decrypt();                                     \
    }())