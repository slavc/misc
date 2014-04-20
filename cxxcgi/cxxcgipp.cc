/*
    cccgipp

    Preprocessor for cccgi.

    Converts an HTML (XML, ...) file with embedded C++ into
    a normal C++ source file.

    Example of an input file:

<:
    #include <iostream>
    #include <string>
    #include <vector>

    using namespace std;

    int
    main(int argc, char **argv) {
        cout << "Content-Type: text/html" << endl << endl;
:>
        <html>
            <head>
                <title><= argv[0] =></title>
            </head>
            <body>
                <div>
                    <:
                        int i;
                        for (i = 0; i < 10; ++i) {
                    :>
                            <p>Paragraph <=i=></p>
                    <:
                        }
                    :>
                </div>
            </body>
        </html>

<:
        return 0;
    }
:>


    Everything between <: :> gets copied into the output file verbatim.
    Everything outside, is copied like
        cout << "<html><blabla ..."
    Everything in <= => is copied verbatim like
        cout << somevar + some_other_var

*/

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#include <unistd.h>
#include <err.h>

using namespace std;

class TextBuf {
    public:
        string buf;
        enum e_types {
            HTML,
            CC,
            CC_VALUE
        } type;

        TextBuf(): type(HTML) { }
        TextBuf(e_types type, const string &buf): type(type), buf(buf) { }

        operator string () {
            return buf;
        }
};
ostream &operator << (ostream &os, vector<TextBuf> &v) {
    vector<TextBuf>::iterator it;
    const char *type;

    for (it = v.begin(); it != v.end(); ++it) {
        if (it != v.begin())
            os << "--------------------------------------------------------------------------------" << endl;
        switch (it->type) {
        case TextBuf::CC:
            type = "CC";
            break;
        case TextBuf::CC_VALUE:
            type = "CC_VALUE";
            break;
        default:
        case TextBuf::HTML:
            type = "HTML";
            break;
        }

        os << type << ": \"" << it->buf << "\"" << endl;
    }

    return os;
}

class Strpos {
    public:
        string::size_type pos;
        string str;

        Strpos(string::size_type pos, const string &str): pos(pos), str(str) {
        }

        bool operator < (const Strpos &sp) const {
            return this->pos < sp.pos;
        }
        
        operator string () {
            return str;
        }

        bool operator == (const string &s) {
            return str == s;
        }
};
ostream &operator << (ostream &os, vector<Strpos> &v) {
    vector<Strpos>::iterator it;

    for (it = v.begin(); it != v.end(); ++it) {
        if (it != v.begin())
            os << ", ";
        os << it->str << " " << it->pos;
    }

    return os;
}

void usage(void);
bool cccgipp(const string &infilename, const string &outfilename);

int
main(int argc, char **argv) {
    const char *optstring = "i:o:";
    int ch;
    string infilename, outfilename;

    while ((ch = getopt(argc, argv, optstring)) != -1) {
        switch (ch) {
        case 'i':
            infilename = optarg;
            break;
        case 'o':
            outfilename = optarg;
            break;
        default:
            usage();
            break;
        }
    }

    if (infilename.empty())
        usage();
    if (outfilename.empty()) {
        string::size_type pos;

        outfilename = infilename;

        if ((pos = outfilename.rfind(".")) != string::npos) {
            outfilename.replace(pos, outfilename.length() - pos, ".cc");
        } else {
            outfilename.append(".cc");
        }
    }

    if (cccgipp(infilename, outfilename))
        return 0;
    else
        return 1;
}

void
usage(void) {
    cout << "usage: cccgipp -i <input_filename> [-o <output_filename>]" << endl;
}

bool
cccgipp(const string &infilename, const string &outfilename) {
    ifstream in(infilename.c_str());
    ofstream out(outfilename.c_str());
    string buf, line;

    if (!in) {
        cerr << "failed to open input file " << infilename << endl;
        return false;
    }
    if (!out) {
        cerr << "failed to open output file " << outfilename << endl;
        return false;
    }

    /* Load the input file */

    while (getline(in, line))
        buf.append(line).append(1, '\n');
    in.close();

    /* Find all delimiters which separate C++ code from HTML */

    vector<Strpos> mark_posns;
    string::size_type pos;
    const char *marks[] = { "<:", ":>", "<=", "=>", NULL }, **p;

    for (p = marks; *p; ++p) {
        for (pos = 0; (pos = buf.find(*p, pos)) != string::npos; pos += 2) {
            mark_posns.push_back(Strpos(pos, *p));
        }
    }
    sort(mark_posns.begin(), mark_posns.end()); // by position

    clog << mark_posns << endl;

    /* Fix up the vector
     *  - <= and => are legal C++ and might appear inside <: :>, erase them if they do
     *  - delete those :> which appear inside C++ code inside quotes
     */

    vector<Strpos>::iterator b, e, it;

    for (b = mark_posns.begin(); b != mark_posns.end(); /* empty */) {
        if (*b == "<:") {
            if ((e = find(b + 1, mark_posns.end(), ":>")) == mark_posns.end()) {
                cerr << "delimiter mismatch, missing :>" << endl;
                return false;
            }

            if (b + 1 < e)
                b = mark_posns.erase(b + 1, e) + 1;
            else
                b = e + 1;
        } else {
            b += 2;
        }
    }

    for (b = mark_posns.begin(); b != mark_posns.end(); /* empty */) {
        if (*b == "<:") {
            e = find(b + 1, mark_posns.end(), ":>");

            if (e == mark_posns.end())
                break;
        } else {
            ++b;
            continue;
        }

        bool in_quotes = false;
        for (pos = b->pos + 2; pos < e->pos && pos < buf.length(); ++pos) {
            if (buf[pos] == '"' && (pos == 0 || buf[pos-1] != '\\'))
                in_quotes = !in_quotes;
        }

        if (in_quotes)
            b = mark_posns.erase(e);
        else
            b = e + 1;
    }

    if (mark_posns.size() % 2 != 0) {
        cerr << "delimiter mismatch" << endl;
        return false;
    }

    /* Delete the delimiters from the text, since they will get in the way */

    for (b = mark_posns.begin(); b != mark_posns.end(); ++b) {
        buf.erase(b->pos, 2);
        for (it = b + 1; it != mark_posns.end(); ++it)
            it->pos -= 2;
    }

    /* Add the non-existent in reality HTML delimiters, to ease further processing */

    for (b = mark_posns.begin(); b != mark_posns.end(); /* empty */) {
        clog << mark_posns << endl;

        if (b == mark_posns.begin()) {
            if (b->pos > 0) {
                b = mark_posns.insert(b, Strpos(b->pos, "H>"));
                b = mark_posns.insert(b, Strpos(0, "<H"));
                b += 2;
            }
        } else {
            if (b->pos - (b - 1)->pos > 0) {
                b = mark_posns.insert(b, Strpos(b->pos, "H>"));
                b = mark_posns.insert(b, Strpos((b - 1)->pos, "<H"));
                b += 2;
            }

            if (b == (mark_posns.end() - 2) && ((b + 1)->pos < buf.length() - 1)) {
                mark_posns.insert(mark_posns.end(), Strpos((b + 1)->pos, "<H"));
                mark_posns.insert(mark_posns.end(), Strpos(buf.length(), "H>"));
                break;
            }
        }
        b += 2;
    }
    clog << mark_posns << endl;

    /* Split into separate buffers, assigning a type (HTML, C++, C++ value embedded in HTML) */

    vector<TextBuf> bufs;

    for (b = mark_posns.begin(); b != mark_posns.end(); b = e + 1) {
        e = b + 1;

        string str(buf.substr(b->pos, e->pos - b->pos));

        if (*b == "<:") {
            bufs.push_back(TextBuf(TextBuf::CC, str));
        } else if (*b == "<=") {
            bufs.push_back(TextBuf(TextBuf::CC_VALUE, str));
        } else if (*b == "<H" && !str.empty() && str.find_first_not_of(" \t\n") != string::npos) {
            /* leave only one space at the end of the string */
            for (pos = str.length(); pos > 0 && isspace(str[pos-1]); --pos)
                /* empty */;
            if (str[pos] == '\n')
                ++pos;
            str.erase(pos, str.length() - pos);
            clog << "erasing <H from pos " << pos << " (tot len = " << str.length() << ")" << endl;
            bufs.push_back(TextBuf(TextBuf::HTML, str));
        }
    }

    clog << bufs;

    clog << buf;

    /* Write the resulting C++ source code file */

    bool in_cout = false;

    for (vector<TextBuf>::iterator tb = bufs.begin(); tb != bufs.end(); ++tb) {
        string b(tb->buf);

        switch (tb->type) {
        case TextBuf::CC:
            if (in_cout) {
                out << ";" << endl;
                in_cout = false;
            }
            out << b;
            break;
        case TextBuf::HTML:
            for (pos = 0; (pos = b.find('"', pos)) != string::npos; pos += 2)
                b.replace(pos, 1, "\\\"");
            b = '"' + b + '"';
            for (pos = 0; (pos = b.find('\n', pos)) != string::npos; pos += 2)
                b.replace(pos, 1, "\\n");
            /* fall through */
        case TextBuf::CC_VALUE:
            if (!in_cout) {
                out << "cout ";
                in_cout = true;
            }
            out << "<< " << b;
            break;
        }
    }
    if (in_cout)
        out << ";";
    out << endl;

    out.close();

    return true;
}

