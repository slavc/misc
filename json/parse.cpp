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

#include <cctype>
#include <string>
#include <sstream>
#include <utility>
#include <algorithm>

#include "json.hpp"

#define SKIP_SPACE \
for (register size_t s_size(s.size()); pos < s_size && isspace(s[pos]); ++pos) \
    /* empty */; \
if (pos >= s.size()) \
    return false;

static bool match_value(const std::string &s, json::value &retval, std::string::size_type &pos);
static bool match_object(const std::string &s, json::value &retval, std::string::size_type &pos);
static bool match_string(const std::string &s, json::value &retval, std::string::size_type &pos);
static bool match_array(const std::string &s, json::value &retval, std::string::size_type &pos);
static bool match_number(const std::string &s, json::value &retval, std::string::size_type &pos);
static bool match_int(const std::string &s, std::string::size_type &pos, int &i, bool must_start_nonzero = true);
static bool match_point(const std::string &s, std::string::size_type &pos);
static bool match_exp(const std::string &s, std::string::size_type &pos, int &i);

json::value json::parse(const std::string &s) {
    std::string::size_type pos(0);
    json::value val;
    if (match_value(s, val, pos))
        return std::move(val);
    else if (json::value::use_exceptions)
        throw json::parse_error();
    else
        return val;
}

static bool match_value(const std::string &s, json::value &retval, std::string::size_type &pos) {
    SKIP_SPACE;

    static const struct {
        char from;
        char to;
        bool (*match_func)(const std::string &, json::value &, std::string::size_type &);
    } char2func[] = {
        { '"', '"', match_string },
        { '{', '{', match_object },
        { '[', '[', match_array  },
        { '-', '-', match_number },
        // FIXME naive, ASCII isn't the only charset
        { '0', '9', match_number },
    };
    for (auto &i : char2func) {
        if (s[pos] >= i.from && s[pos] <= i.to)
            return i.match_func(s, retval, pos);
    }

    static const struct {
        std::string term;
        json::value val;
    } terminals[] = {
        { "true",  json::value(true)  },
        { "false", json::value(false) },
        { "null",  json::value()      },
    };
    for (auto &i : terminals) {
        if (s.compare(pos, i.term.size(), i.term) == 0) {
            pos += i.term.size();
            retval = i.val;
            return true;
        }
    }

    return false;
}

static bool match_object(const std::string &s, json::value &retval, std::string::size_type &pos) {
    json::object obj;
    json::value  key;
    json::value  val;

    ++pos;
    for ( ;; ) {
        SKIP_SPACE;
        if (match_string(s, key, pos)) {
            SKIP_SPACE;
            if (s[pos] == ':')
                ++pos;
            else
                return false;
            SKIP_SPACE;
            if (!match_value(s, val, pos))
                return false;
            obj[key.sval()] = std::move(val);
            SKIP_SPACE;
            if (s[pos] == '}') {
                ++pos;
                break;
            } else if (s[pos] != ',') {
                return false;
            } else {
                ++pos;
            }
        } else if (s[pos] == '}') {
            ++pos;
        }
    }
    retval = std::move(obj);
    return true;
}

static const char *gen_escape_tab() {
    const size_t nelems((unsigned char) ~ 0);
    static char tab[nelems];

    for (size_t i = 0; i < nelems; ++i)
        tab[i] = (char) i;
    tab[(size_t) 'r'] = '\r';
    tab[(size_t) 'n'] = '\n';
    tab[(size_t) 't'] = '\t';
    tab[(size_t) 'f'] = '\f';

    return tab;
}

static bool match_string(const std::string &s, json::value &retval, std::string::size_type &pos) {
    static const char *escape_tab = gen_escape_tab();
    static bool        do_escape;
    static std::string str;

    str.reserve(32);

    for (++pos, do_escape = false; pos < s.size(); ++pos) {
        if (s[pos] == '\\') {
            do_escape = true;
        } else if (do_escape) {
            str.push_back(escape_tab[(size_t) s[pos]]);
            do_escape = false;
        } else if (s[pos] == '"') {
            break;
        } else {
            str.push_back(s[pos]);
        }
    }
    if (s[pos] != '"') {
        return false;
    } else {
        ++pos;
        retval = str;
        str.clear();
        return true;
    }
}

static bool match_array(const std::string &s, json::value &retval, std::string::size_type &pos) {
    json::array arr;
    json::value val;

    ++pos;
    for ( ;; ) {
        SKIP_SPACE;
        if (!match_value(s, val, pos))
            return false;
        arr.push_back(std::move(val));
        SKIP_SPACE
        if (s[pos] == ']') {
            ++pos;
            break;
        } else if (s[pos] != ',') {
            return false;
        } else {
            ++pos;
        }
    }
    retval = std::move(arr);
    return true;
}

static bool match_number(const std::string &s, json::value &retval, std::string::size_type &pos) {
    int sign;
    int intpart, fracpart, exp;
    bool point_matched, exp_matched;

    sign = 1;
    if (s[pos] == '-') {
        sign = -1;
        ++pos;
    } else if (s[pos] == '+') {
        sign = 1;
        ++pos;
    }

    if (!match_int(s, pos, intpart))
        return false;
    if ((point_matched = match_point(s, pos)))
        match_int(s, pos, fracpart, false);
    exp_matched = match_exp(s, pos, exp);

    if (point_matched || exp_matched) {
        double d;
        d = intpart * sign;
        double frac(fracpart);
        while (frac >= 1.0)
            frac /= 10.0;
        d += frac;
        if (exp_matched) {
            if (exp > 0)
                while (exp-- > 0)
                    d *= 10.0;
            else {
                exp = -exp;
                while (exp-- > 0)
                    d /= 10.0;
            }
        }
        retval = d;
        return true;
    } else {
        retval = intpart * sign;
        return true;
    }
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
    
    if (s[pos] == 'e' || s[pos] == 'E')
        ++pos;
    else
        return false;

    if (s[pos] == '-') {
        sign = -1;
        ++pos;
    } else if (s[pos] == '+') {
        sign = 1;
        ++pos;
    }

    if (!match_int(s, pos, val, true))
        return false;

    i = val * sign;

    return true;
}

