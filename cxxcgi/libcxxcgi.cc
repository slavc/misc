#include <string>
#include <map>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <stdexcept>

#include <sqlite3.h>

#define LIBCCCGI_CC
#include "cccgi.hh"

extern char **environ;

using namespace std;

namespace cccgi {

    string
    urlencode(const string &s) {
        string::size_type pos;
        ostringstream ss;

        ss << hex;
        for (pos = 0; pos < s.length(); ++pos) {
            if (isalpha(s[pos]) || isdigit(s[pos]))
                ss << s[pos];
            else
                ss << "%" << (unsigned int) s[pos];
        }

        return ss.str();
    }

    string
    urldecode(const string &s) {
        string::size_type pos;
        ostringstream ss;
        int i, c;

        for (pos = 0; pos < s.length(); ++pos) {
            if (s[pos] == '%' && isxdigit(s[pos+1]) && isxdigit(s[pos+2])) {
                for (i = c = 0; i < 2; ++i) {
                    c *= 16;
                    c += isdigit(s[++pos]) ? s[pos]-'0' : toupper(s[pos])-'A'+10;
                }
                ss << (char) c;
            } else {
                ss << s[pos];
            }
        }

        return ss.str();
    }

    string
    form_decode(const string &s) {
        string::size_type i, slen;
        char *buf, *p;
        int c;

        slen = s.length();

        if ((p = buf = new char[s.length()+1]) == NULL)
            return "";

        for (i = 0; i < slen; /* empty */) {
            if (s[i] == '+') {
                *p++ = ' ';
                ++i;
            } else if (s[i] == '%') {
                c = ' ';
                sscanf(s.c_str() + ++i, "%2x", &c);
                i += 2;
                *p++ = c;
            } else
                *p++ = s[i++];
        }
        *p = '\0';

        string retval(buf);

        delete[] buf;

        return retval;
    }

    string
    form_encode(const string &s) {
        string::size_type i, slen;
        string buf;
        char tmpbuf[3];

        for (i = 0, slen = s.length(); i < slen; ++i) {
            if (s[i] == ' ')
                buf += '+';
            else if (s[i] == '\t' || !isalnum(s[i])) {
                snprintf(tmpbuf, sizeof tmpbuf, "%X", (unsigned char) s[i]);
                buf += tmpbuf;
            } else
                buf += s[i];
        }

        return buf;
    }

    Get :: Get(): map<string, string>() {
        parseGet();
    }

    void
    Get :: parseGet(void) {
        int i;
        vector<string> pairs;
        vector<string>:: iterator it;
        string::size_type amp, pos;
        string getstr;

        for (i = 0; environ[i] != NULL; ++i) {
            if (!strncmp(environ[i], "QUERY_STRING", sizeof("QUERY_STRING") - 1))
                break;
        }
        if (environ[i] == NULL)
            return;

        getstr = environ[i];
        if ((pos = getstr.find('=')) == string::npos)
            return;
        getstr.erase(0, pos + 1);

        do {
            if ((amp = getstr.find('&')) == string::npos) {
                pairs.push_back(getstr);
            } else {
                pairs.push_back(getstr.substr(0, amp));
                getstr.erase(0, amp + 1);
            }
        } while (amp != string::npos);
        
        for (it = pairs.begin(); it != pairs.end(); ++it) {
            if ((pos = it->find('=')) == string::npos)
                continue;
            string var(it->substr(0, pos)), val(urldecode(it->substr(pos + 1)));
            (*this)[var] = val;
        }
    }

#   if 0
    Post :: Post(): map<string, ParseBuf>() {
        parsePost();
    }

    Post :: parsePost() {
        const char *penv;
        long i, content_length;
        ssize_t nbytes;
        char *buf;

        if ((penv = getenv("CONTENT_LENGTH")) == NULL)
            return;
        content_length = atol(penv);
        if (content_length < 1)
            return;

        buf = new char[content_length];
        if (read(STDIN_FILENO, buf, content_length) != content_length) {
            delete[] buf;
            return;
        }

        if ((penv = getenv("CONTENT_TYPE")) == NULL)
            return;

        if (strstr(penv, "application/x-www-form-urlencoded")) {
            const char *name_start, *name_end, *val_start, *val_end;
            char *name, *val;
            size_t name_len, val_len, decoded_val_len;

            name_start = buf;
            while (*name_start != '\0') {
                name_end = strchr(name_start, '=');
                if (name_end == NULL)
                    break;
                --name_end;
                if (name_start > name_end)
                    break;

                val_start = name_end + 2;
                val_end = strchr(val_start, '&');
                if (val_end == NULL)
                    val_end = val_start + strlen(val_start) - 1;
                else
                    --val_end;
                if (val_start > val_end)
                    break;

                name_len = name_end - name_start;
                val_len = val_end - val_start;

                name = new char[name_len + 1];
                val = new char[val_len + 1];

                memcpy(name, name_start, name_len);
                memcpy(val, val_start, val_len);

                name[name_len] = '\0';
                val[val_len] = '\0';

                string decoded_val(form_decode(val));

                this->insert(make_pair(string(name), PostBuf(NULL, strdup(decoded_val.c_str()), decoded_val.length())));

                delete[] name;
                delete[] val;

                name_start = val_end + 1;
            }
        } else if (strstr(penv, "application/form-data")) {
#           warning Parsing of application/form-data is not implemented.
            clog << "Parsing of application/form-data is not implemented" << endl;
        }

        delete[] buf;
    }
#endif

    Get GET;

    /*** DBConnectionSqlite ***/

    DBConnectionSqlite :: DBConnectionSqlite():
        db(NULL)
    {
    }

    DBConnectionSqlite :: DBConnectionSqlite(const string &connect_string):
        connect_string(connect_string), db(NULL)
    {
        connect(connect_string);
    }

    DBConnectionSqlite :: ~DBConnectionSqlite() {
        if (db)
            sqlite3_close(db);
    }

    bool
    DBConnectionSqlite :: connect(const string &connect_string) {
        if (sqlite3_open(connect_string.c_str(), &db) == -1)
            return false;
    }

    bool
    DBConnectionSqlite :: isConnected(void) {
        return db != NULL;
    }

    /*** DBStreamSqlite ***/

    DBStreamSqlite :: DBStreamSqlite(DBConnectionSqlite &dbconn):
        DBStream(dbconn), dbconn(dbconn), stmt(NULL), status(0), cur_col(0), ncols(0), cur_var(0), nvars(0)
    {
        clog << typeid(*this).name() << "::" << __func__ << "1()" << endl;

        if (!dbconn.isConnected())
            throw runtime_error("invalid argument: dbconn is not connected");
    }

    DBStreamSqlite :: DBStreamSqlite(DBConnectionSqlite &dbconn, const string &s):
        DBStream(dbconn, s), dbconn(dbconn), stmt(NULL), status(0), cur_col(0), ncols(0), cur_var(0), nvars(0)
    {
        clog << typeid(*this).name() << "::" << __func__ << "2()" << endl;

        if (!dbconn.isConnected())
            throw runtime_error("invalid argument: dbconn is not connected");
        
        this->exec(s);
    }

    DBStreamSqlite :: ~DBStreamSqlite() {
        clog << typeid(*this).name() << "::" << __func__ << "()" << endl;

        if (stmt)
            sqlite3_finalize(stmt);
    }

    DBStreamSqlite &
    DBStreamSqlite :: exec(const string &s) {
        clog << typeid(*this).name() << "::" << __func__ << "()" << endl;

        this->assignSql(s);

        return *this;
    }

    bool
    DBStreamSqlite :: hasRows(void) {
        clog << typeid(*this).name() << "::" << __func__ << "()" << endl;

        return status == SQLITE_ROW;
    }

    DBStreamSqlite &
    DBStreamSqlite :: operator >> (long &l) {
        clog << typeid(*this).name() << "::" << __func__ << "()" << endl;

        if (status != SQLITE_ROW) {
            throw runtime_error("there are no rows to fetch");
        }

        l = sqlite3_column_int(stmt, cur_col++);

        if (cur_col >= ncols)
            this->nextRow();

        return *this;
    }

    DBStreamSqlite &
    DBStreamSqlite :: operator >> (string &s) {
        clog << typeid(*this).name() << "::" << __func__ << "()" << endl;

        if (status != SQLITE_ROW) {
            throw runtime_error("there are no rows to fetch");
        }

        const char *p = (const char *) sqlite3_column_text(stmt, cur_col++);

        s = p == NULL ? "" : p;

        if (cur_col >= ncols)
            this->nextRow();

        return *this;
    }

    DBStreamSqlite &
    DBStreamSqlite :: operator << (long l) {
        clog << typeid(*this).name() << "::" << __func__ << "()" << endl;

        ostringstream ss;

        if (nvars == 0)
            throw runtime_error("there are no variables to be subsituted");

        ss << l;

        var_substs[cur_var++].val = ss.str();

        if (cur_var >= nvars) {
            this->substAllVars();
            this->execBakedSql();
            sql = raw_sql;
            cur_var = 0;
        }

        return *this;
    }

    DBStreamSqlite &
    DBStreamSqlite :: operator << (const string &s) {
        clog << typeid(*this).name() << "::" << __func__ << "()" << endl;

        if (nvars == 0)
            throw runtime_error("there are no variables to be subsituted");

        var_substs[cur_var++].val = s;

        if (cur_var >= nvars) {
            this->substAllVars();
            this->execBakedSql();
            sql = raw_sql;
            cur_var = 0;
        }

        return *this;
    }

    int
    DBStreamSqlite :: nextRow(void) {
        clog << typeid(*this).name() << "::" << __func__ << "()" << endl;

        if ((status = sqlite3_step(stmt)) == SQLITE_ROW) {
            cur_col = 0;
            if (!ncols)
                ncols = sqlite3_column_count(stmt);
        }

        return status;
    }

    vector<DBStreamSqlite::Varsubst>::size_type
    DBStreamSqlite :: findVars(const string &s) {
        clog << typeid(*this).name() << "::" << __func__ << "()" << endl;

        string::size_type pos, end_pos, len;

        var_substs.clear();

        for (pos = 0; (pos = s.find(':', pos)) != string::npos; /* empty */) {
            if (s[pos] == ':') {
                if (pos != 0 && s[pos-1] == '\\') {
                    ++pos;
                    continue;
                }
                for (end_pos = pos + 1; isalnum(s[end_pos]) || s[end_pos] == '_'; ++end_pos)
                    /* empty */;
                if ((len = end_pos - pos) > 1) {
                    var_substs.push_back(Varsubst(pos, len));
                }
                pos = end_pos;
            } else
                ++pos;
        }

        nvars = var_substs.size();

        return nvars;
    }

    void
    DBStreamSqlite :: substAllVars(void) {
        clog << typeid(*this).name() << "::" << __func__ << "()" << endl;

        size_t i;
        long shift;

        for (i = shift = 0; i < var_substs.size(); ++i) {
            Varsubst &vs = var_substs[i];

            sql.replace(vs.pos + shift, vs.len, vs.val);

            shift -= vs.len;
            shift += vs.val.length();
        }
    }

    void
    DBStreamSqlite :: assignSql(const string &s) {
        clog << typeid(*this).name() << "::" << __func__ << "()" << endl;

        ncols = 0;

        if (stmt) {
            sqlite3_finalize(stmt);
            stmt = NULL;
        }

        raw_sql = s;
        sql = s;

        this->findVars(raw_sql);

        if (nvars == 0)
            this->execBakedSql();
    }

    void
    DBStreamSqlite :: execBakedSql(void) {
        clog << typeid(*this).name() << "::" << __func__ << "()" << endl;

        if (stmt) {
            sqlite3_finalize(stmt);
            stmt = NULL;
        }

        if (sqlite3_prepare_v2(dbconn.db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
            ostringstream ss;

            ss << "failed to prepare SQL statement \"" << sql << "\": " << sqlite3_errmsg(dbconn.db);

            throw runtime_error(ss.str());
        }

        clog << "executing sql query: " << sql << endl;

        this->nextRow();

        if (status != SQLITE_ROW && status != SQLITE_DONE) {
            ostringstream ss;

            ss << "failed to execute SQL statement \"" << sql << "\": " << status << " - " << sqlite3_errmsg(dbconn.db);

            throw runtime_error(ss.str());
        }
    }
};

#ifdef TEST_LIBCCCGI
using namespace cccgi;

int
main(int argc, char **argv) {
    int i;

    DBConnectionSqlite dbconn("test.db");
    DBStreamSqlite dbs(dbconn);

    /*
    for (i = 1; i < argc; ++i) {
        cout << urldecode(urlencode(argv[i])) << endl;
    }

    for (Get::iterator it = GET.begin(); it != GET.end(); ++it) {
        cout << it->first << " = " << it->second << endl;
    }
    */

    for (i = 0; i < 3; ++i) {
        dbs("select id, category, description from categories order by appearance_sequence, id");
        cout << "categories: ";
        while (dbs) {
            long id;
            string cat, descr;

            dbs >> id >> cat >> descr;

            cout << id << " \"" << cat << "\" \"" << descr << "\"" << "; ";
        }
        cout << endl;
    }

    return 0;
}
#endif // TEST_LIBCCCGI
