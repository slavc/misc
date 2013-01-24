/*
 * Copyright (c) 2012 Sviatoslav Chagaev <sviatoslav.chagaev@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <string>
#include <cctype>
#include <utility>

#include "json.hpp"

struct match_result {
    bool successful;
    json::value value;
    match_result(match_result &&) = default;
    match_result &operator=(match_result &&) = default;
};

static match_result  match_object(const std::string &, std::string::size_type &);
static bool          match_space(const std::string &, std::string::size_type &);
static bool          match_char(const std::string &, std::string::size_type &, char ch);
static bool          match_brace_open(const std::string &, std::string::size_type &);
static bool          match_brace_close(const std::string &, std::string::size_type &);
static bool          match_bracket_open(const std::string &, std::string::size_type &);
static bool          match_bracket_close(const std::string &, std::string::size_type &);
static bool          match_comma(const std::string &, std::string::size_type &);
static bool          match_colon(const std::string &, std::string::size_type &);
static bool          match_dquote(const std::string &, std::string::size_type &);
static match_result  match_string(const std::string &, std::string::size_type &);
static match_result  match_value(const std::string &, std::string::size_type &);
static match_result  match_array(const std::string &, std::string::size_type &);
static match_result  match_number(const std::string &s, std::string::size_type &pos);
static bool          match_sign(const std::string &s, std::string::size_type &pos, int &sign);
static bool          match_int(const std::string &s, std::string::size_type &pos, int &i, bool must_start_nonzero = false);
static bool          match_point(const std::string &s, std::string::size_type &pos);
static bool          match_exp(const std::string &s, std::string::size_type &pos, int &i);
static bool          match_word(const std::string &, std::string::size_type &, const std::string &);
static match_result  match_true(const std::string &, std::string::size_type &);
static match_result  match_false(const std::string &, std::string::size_type &);
static match_result  match_null(const std::string &, std::string::size_type &);

json::value json::parse(const std::string &s) {
    std::string::size_type pos(0);
    match_result result(match_value(s, pos));

    if (result.successful)
        return result.value;
    else
        throw json::parse_error();
}

static match_result match_object(const std::string &s, std::string::size_type &pos) {
    if (pos >= s.size())
        return { false };
    match_space(s, pos);

    if (!match_brace_open(s, pos))
        return { false };

    json::object object;
    do {
        match_space(s, pos);

        match_result str_result(match_string(s, pos));
        if (!str_result.successful)
            return { false };

        match_space(s, pos);
        if (!match_colon(s, pos))
            return { false };

        match_space(s, pos);
        match_result val_result(match_value(s, pos));
        if (!val_result.successful)
            return { false };

        object[str_result.value] = val_result.value;
    } while (match_comma(s, pos));

    return { match_brace_close(s, pos), object };
}

static bool match_space(const std::string &s, std::string::size_type &pos) {
    if (pos >= s.size())
        return false;

    while (isspace(s[pos]))
        ++pos;

    return true;
}

static bool match_char(const std::string &s, std::string::size_type &pos, char ch) {
    if (pos >= s.size())
        return false;
    match_space(s, pos);

    if (s[pos] == ch) {
        ++pos;
        return true;
    } else {
        return false;
    }
}

static bool match_brace_open(const std::string &s, std::string::size_type &pos) {
    return match_char(s, pos, '{');
}

static bool match_brace_close(const std::string &s, std::string::size_type &pos) {
    return match_char(s, pos, '}');
}

static bool match_bracket_open(const std::string &s, std::string::size_type &pos) {
    return match_char(s, pos, '[');
}

static bool match_bracket_close(const std::string &s, std::string::size_type &pos) {
    return match_char(s, pos, ']');
}

static bool match_comma(const std::string &s, std::string::size_type &pos) {
    return match_char(s, pos, ',');
}

static bool match_colon(const std::string &s, std::string::size_type &pos) {
    return match_char(s, pos, ':');
}

static bool match_dquote(const std::string &s, std::string::size_type &pos) {
    return match_char(s, pos, '"');
}

static match_result match_string(const std::string &s, std::string::size_type &pos) {
    if (pos >= s.size())
        return { false };
    match_space(s, pos);

    if (!match_dquote(s, pos))
        return { false };
    std::ostringstream ostr;
    for (/* empty */; s[pos] != '"' && pos < s.size(); ++pos) {
        if (s[pos] == '\\' ) {
            if (++pos >= s.size())
                return { false };
            switch (s[pos]) {
            case 'r':
                ostr << '\r';
                break;
            case 'n':
                ostr << '\n';
                break;
            case 't':
                ostr << '\t';
                break;
            case 'f':
                ostr << '\f';
                break;
            case 'b':
                // FIXME implement backspace
                break;
            default:
                ostr << s[pos];
                break;
            }
        } else
            ostr << s[pos];
    }
    return { match_dquote(s, pos), { ostr.str() } };
}

static match_result match_value(const std::string &s, std::string::size_type &pos) {
    if (pos >= s.size())
        return { false };
    match_space(s, pos);

    match_result (*funcs[])(const std::string &, std::string::size_type &) = {
        match_object,
        match_array,
        match_string,
        match_number,
        match_true,
        match_false,
        match_null,
    };

    for (auto f : funcs) {
        match_result result(f(s, pos));
        if (result.successful)
            return { true, result.value };
    }
    return { false };
}

static match_result match_array(const std::string &s, std::string::size_type &pos) {
    if (pos >= s.size())
        return { false };
    match_space(s, pos);

    if (!match_bracket_open(s, pos))
        return { false };
    json::array array;
    do {
        match_result result(match_value(s, pos));
        if (result.successful)
            array.push_back(result.value);
    } while (match_comma(s, pos));
    return { match_bracket_close(s, pos), array };
}

static match_result match_number(const std::string &s, std::string::size_type &pos) {
    int sign;
    int intpart, fracpart, exp;
    bool point_match, exp_match;

    sign = 1;

    match_sign(s, pos, sign);
    if (!match_int(s, pos, intpart))
        return { false };
    if ((point_match = match_point(s, pos)))
        match_int(s, pos, fracpart, false);
    exp_match = match_exp(s, pos, exp);

    if (point_match || exp_match) {
        double d;
        d = intpart * sign;
        double frac(fracpart);
        while (frac >= 1.0)
            frac /= 10.0;
        d += frac;
        if (exp_match) {
            if (exp > 0)
                while (exp-- > 0)
                    d *= 10.0;
            else {
                exp = -exp;
                while (exp-- > 0)
                    d /= 10.0;
            }
        }
        return { true, d };
    } else
        return { true, intpart * sign };
}

static bool match_sign(const std::string &s, std::string::size_type &pos, int &sign) {
    if (s[pos] == '+') {
        sign = 1;
        ++pos;
        return true;
    } else if (s[pos] == '-') {
        sign = -1;
        ++pos;
        return true;
    } else
        return false;
}

static bool match_int(const std::string &s, std::string::size_type &pos, int &i, bool must_start_nonzero) {
    std::string::size_type start_pos(pos);
    
    int val = 0;
    while (isdigit(s[pos])) {
        val *= 10;
        // FIXME naive, ASCII isn't the only charset
        val += s[pos] - '0';
        ++pos;
    }

    if (must_start_nonzero && pos - start_pos > 1 && s[start_pos] == '0') {
        // leading zeros are not allowed
        pos = start_pos;
        return false;
    }

    if (pos - start_pos > 1 && s[start_pos] == '0')
        return false;

    if (pos - start_pos > 0) {
        i = val;
        return true;
    } else
        return false;
}

static bool match_point(const std::string &s, std::string::size_type &pos) {
    if (s[pos] == '.') {
        ++pos;
        return true;
    } else
        return false;
}

static bool match_exp(const std::string &s, std::string::size_type &pos, int &i) {
    int sign = 1;
    int val;

    if (!match_char(s, pos, 'e') && !match_char(s, pos, 'E'))
        return false;
    match_sign(s, pos, sign);
    if (!match_int(s, pos, val, true))
        return false;

    i = val * sign;

    return true;
}

static bool match_word(const std::string &s, std::string::size_type &pos, const std::string &word) {
    if (pos >= s.size())
        return false;
    match_space(s, pos);
    
    if (s.substr(pos, word.size()) == word) {
        pos += word.size();
        return true;
    } else
        return false;
}

static match_result match_true(const std::string &s, std::string::size_type &pos) {
    return { match_word(s, pos, "true"), true };
}

static match_result match_false(const std::string &s, std::string::size_type &pos) {
    return { match_word(s, pos, "false"), false };
}

static match_result match_null(const std::string &s, std::string::size_type &pos) {
    return { match_word(s, pos, "null") };
}

