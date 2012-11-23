#include <iostream>
#include <iomanip>
#include <string>
#include <cctype>

#include "json.hpp"

class logger {
    public:
        logger(const std::string &func_name) :
            m_func_name(func_name)
        {
            int i { s_level++ };
            while (i-- > 0)
                std::clog << s_indent;
            std::clog << m_func_name << " {" << std::endl;
        }

        ~logger() {
            int i { --s_level };
            while (i-- > 0)
                std::clog << s_indent;
            std::clog << "}" << std::endl;
        }

        template<typename ... Types> void print(Types ... args) {
            int i { s_level };
            while (i-- > 0)
                std::clog << s_indent;
            vprint(args...);
        }
    private:
        static int s_level;
        static const char *const s_indent;
        const std::string m_func_name;

        template<typename ValT, typename ... Types> void vprint(const ValT &val, Types ... args) {
            std::clog << val;
        }
        void vprint() {
            std::clog << std::endl;
        }
};
int logger::s_level { 0 };
const char *const logger::s_indent { "    " };

struct match_result {
    bool successful;
    json::value value;
};

match_result  match_object(const std::string &, std::string::size_type &);
bool          match_space(const std::string &, std::string::size_type &);
bool          match_char(const std::string &, std::string::size_type &, char ch);
bool          match_brace_open(const std::string &, std::string::size_type &);
bool          match_brace_close(const std::string &, std::string::size_type &);
bool          match_bracket_open(const std::string &, std::string::size_type &);
bool          match_bracket_close(const std::string &, std::string::size_type &);
bool          match_comma(const std::string &, std::string::size_type &);
bool          match_colon(const std::string &, std::string::size_type &);
bool          match_dquote(const std::string &, std::string::size_type &);
match_result  match_string(const std::string &, std::string::size_type &);
match_result  match_value(const std::string &, std::string::size_type &);
match_result  match_array(const std::string &, std::string::size_type &);
match_result  match_number(const std::string &, std::string::size_type &);
bool          match_word(const std::string &, std::string::size_type &, const std::string &);
match_result  match_true(const std::string &, std::string::size_type &);
match_result  match_false(const std::string &, std::string::size_type &);
match_result  match_null(const std::string &, std::string::size_type &);

match_result match_object(const std::string &s, std::string::size_type &pos) {
    logger logger { __func__ };

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

bool match_space(const std::string &s, std::string::size_type &pos) {
    logger logger { __func__ };

    if (pos >= s.size())
        return false;

    while (isspace(s[pos]))
        ++pos;

    return true;
}

bool match_char(const std::string &s, std::string::size_type &pos, char ch) {
    logger logger { __func__ };

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

bool match_brace_open(const std::string &s, std::string::size_type &pos) {
    return match_char(s, pos, '{');
}

bool match_brace_close(const std::string &s, std::string::size_type &pos) {
    return match_char(s, pos, '}');
}

bool match_bracket_open(const std::string &s, std::string::size_type &pos) {
    return match_char(s, pos, '[');
}

bool match_bracket_close(const std::string &s, std::string::size_type &pos) {
    return match_char(s, pos, ']');
}

bool match_comma(const std::string &s, std::string::size_type &pos) {
    return match_char(s, pos, ',');
}

bool match_colon(const std::string &s, std::string::size_type &pos) {
    return match_char(s, pos, ':');
}

bool match_dquote(const std::string &s, std::string::size_type &pos) {
    return match_char(s, pos, '"');
}

match_result match_string(const std::string &s, std::string::size_type &pos) {
    logger logger { __func__ };

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

match_result match_value(const std::string &s, std::string::size_type &pos) {
    logger logger { __func__ };

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

match_result match_array(const std::string &s, std::string::size_type &pos) {
    logger logger { __func__ };

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

match_result match_number(const std::string &s, std::string::size_type &pos) {
    logger logger { __func__ };

    if (pos >= s.size())
        return { false };
    match_space(s, pos);

    std::string::size_type pos_copy(pos);

    int sign = 1;
    int number = 0;

    if (s[pos] == '-') {
        sign = -1;
        ++pos;
    }
    if (!isdigit(s[pos]) || s[pos] == '0') {
        pos = pos_copy;
        return { false };
    } else {
        number *= 10;
        number += (s[pos] - '0');
        ++pos;
    }

    while (isdigit(s[pos]) && pos < s.size()) {
        number *= 10;
        number += (s[pos] - '0');
        ++pos;
    }
    return { true, json::value(sign * number) };
}

bool match_word(const std::string &s, std::string::size_type &pos, const std::string &word) {
    logger logger { __func__ };

    if (pos >= s.size())
        return false;
    match_space(s, pos);
    
    if (s.substr(pos, word.size()) == word) {
        pos += word.size();
        return true;
    } else
        return false;
}

match_result match_true(const std::string &s, std::string::size_type &pos) {
    return { match_word(s, pos, "true"), true };
}

match_result match_false(const std::string &s, std::string::size_type &pos) {
    return { match_word(s, pos, "false"), false };
}

match_result match_null(const std::string &s, std::string::size_type &pos) {
    return { match_word(s, pos, "null") };
}

int main(int argc, char **argv) {
    for (--argc, ++argv; argc > 0; --argc, ++argv)  {
        std::string s(*argv);
        std::string::size_type pos { 0 };
        json::value val;
        match_result result(match_value(s, pos));
        std::cout << " input string = ``" << s << "''" << std::endl;
        std::cout << " parse successful? " << std::boolalpha << result.successful << std::endl;
        std::cout << " json value = " << result.value.str() << std::endl;
    }
    return 0;
}
