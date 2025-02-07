#pragma once

#include "Utility.hpp"

namespace hsd
{
    template <typename CharT, usize N>
    struct basic_string_literal
    {
        CharT data[N]{};
        using char_type = CharT;

        consteval basic_string_literal() = default;

        consteval basic_string_literal(const CharT (&str)[N])
        {
            copy_n(str, N, data);
        }

        template <usize N2>
        consteval auto operator+(
            const basic_string_literal<CharT, N2>& rhs) const
        {
            basic_string_literal<CharT, N + N2> _str;
            copy_n(data, N, _str.data);
            copy_n(rhs.data, N2, (_str.data + N - 1));
            return _str;
        }

        template <usize N2>
        consteval auto operator+(const CharT (&rhs)[N2]) const
        {
            basic_string_literal<CharT, N + N2> _str;
            copy_n(data, N, _str.data);
            copy_n(rhs, N2, (_str.data + N - 1));
            return _str;
        }

        consteval basic_string_literal(const CharT* str, usize len)
        {
            copy_n(str, len, data);
        }

        consteval usize size() const
        {
            return N;
        }
    };

    template <typename CharT, usize N1, usize N2>
    static consteval auto operator+(const CharT (&lhs)[N1], 
        const basic_string_literal<CharT, N2>& rhs)
    {
        basic_string_literal<CharT, N1 + N2> _str;
        copy_n(lhs, N1, _str.data);
        copy_n(rhs.data, N2, (_str.data + N1 - 1));
        return _str;
    }

    template <usize N>
    using string_literal = basic_string_literal<char, N>;
    template <usize N>
    using wstring_literal = basic_string_literal<wchar, N>;
    template <usize N>
    using u8string_literal = basic_string_literal<char8, N>;
    template <usize N>
    using u16string_literal = basic_string_literal<char16, N>;
    template <usize N>
    using u32string_literal = basic_string_literal<char32, N>;
} // namespace hsd
