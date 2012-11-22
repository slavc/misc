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

bool match_object(const std::string &, std::string::size_type &);
bool match_space(const std::string &, std::string::size_type &);
bool match_char(const std::string &, std::string::size_type &, char ch);
bool match_brace_open(const std::string &, std::string::size_type &);
bool match_brace_close(const std::string &, std::string::size_type &);
bool match_bracket_open(const std::string &, std::string::size_type &);
bool match_bracket_close(const std::string &, std::string::size_type &);
bool match_comma(const std::string &, std::string::size_type &);
bool match_colon(const std::string &, std::string::size_type &);
bool match_dquote(const std::string &, std::string::size_type &);
bool match_pair(const std::string &, std::string::size_type &);
bool match_string(const std::string &, std::string::size_type &);
bool match_value(const std::string &, std::string::size_type &);
bool match_array(const std::string &, std::string::size_type &);
bool match_number(const std::string &, std::string::size_type &);
bool match_word(const std::string &, std::string::size_type &, const std::string &);
bool match_true(const std::string &, std::string::size_type &);
bool match_false(const std::string &, std::string::size_type &);
bool match_null(const std::string &, std::string::size_type &);

bool match_object(const std::string &s, std::string::size_type &pos) {
    logger logger { __func__ };

    if (pos >= s.size())
        return false;
    match_space(s, pos);

    if (!match_brace_open(s, pos))
        return false;

    do {
        match_pair(s, pos);
    } while (match_comma(s, pos));

    return match_brace_close(s, pos);
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

bool match_pair(const std::string &s, std::string::size_type &pos) {
    logger logger { __func__ };

    if (pos >= s.size())
        return false;
    match_space(s, pos);

    if (!match_string(s, pos))
        return false;
    if (!match_colon(s, pos))
        return false;
    if (!match_value(s, pos))
        return false;

    return true;
}


bool match_string(const std::string &s, std::string::size_type &pos) {
    logger logger { __func__ };

    if (pos >= s.size())
        return false;
    match_space(s, pos);

    if (!match_dquote(s, pos))
        return false;
    char prev { 'X' };
    while ((s[pos] != '"' && prev != '\\') && pos < s.size()) {
        prev = s[pos];
        ++pos;
    }
    return match_dquote(s, pos);
}

bool match_value(const std::string &s, std::string::size_type &pos) {
    logger logger { __func__ };

    if (pos >= s.size())
        return false;
    match_space(s, pos);

    bool (*funcs[])(const std::string &, std::string::size_type &) = {
        match_object,
        match_array,
        match_string,
        match_number,
        match_true,
        match_false,
        match_null,
    };

    for (auto f : funcs)
        if (f(s, pos))
            return true;
    return false;
}

bool match_array(const std::string &s, std::string::size_type &pos) {
    logger logger { __func__ };

    if (pos >= s.size())
        return false;
    match_space(s, pos);

    if (!match_bracket_open(s, pos))
        return false;
    do {
        match_value(s, pos);
    } while (match_comma(s, pos));
    return match_bracket_close(s, pos);
}

bool match_number(const std::string &s, std::string::size_type &pos) {
    logger logger { __func__ };

    if (pos >= s.size())
        return false;
    match_space(s, pos);

    std::string::size_type pos_copy(pos);
    if (s[pos] == '-')
        ++pos;
    if (!isdigit(s[pos]) || s[pos] == '0') {
        pos = pos_copy;
        return false;
    } else {
        ++pos;
    }

    while (isdigit(s[pos]) && pos < s.size())
        ++pos;
    return true;
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

bool match_true(const std::string &s, std::string::size_type &pos) {
    return match_word(s, pos, "true");
}

bool match_false(const std::string &s, std::string::size_type &pos) {
    return match_word(s, pos, "false");
}

bool match_null(const std::string &s, std::string::size_type &pos) {
    return match_word(s, pos, "null");
}

int main(int argc, char **argv) {
    for (--argc, ++argv; argc > 0; --argc, ++argv)  {
        std::string s(*argv);
        std::string::size_type pos { 0 };
        json::value val;
        bool result { match_value(s, pos) };
        std::cout << '"' << s << "\": " << std::boolalpha << result << " pos=" << pos << std::endl;
    }
    return 0;
}
