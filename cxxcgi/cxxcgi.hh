#ifndef CCCGI_HH
#define CCCGI_HH

#include <string>
#include <map>
#include <vector>
#include <cstdlib>

#include <sqlite3.h>

namespace cccgi {
    std::string urlencode(const std::string &);
    std::string urldecode(const std::string &);

    class Get: public std::map<std::string, std::string> {
        public:
            Get();
        private:
            void parseGet(void);
    };

    class PostBuf {
        char *filename;
        char *buf;
        size_t buflen;

        PostBuf():
          filename(NULL), buf(NULL), buflen(0)
        { }
        PostBuf(char *filename, char *buf, size_t buflen):
          filename(filename), buf(buf), buflen(buflen)
        { }
        ~PostBuf() {
            free(buf);
        }
        operator char* () {
            return buf;
        }
    };
    class Post: public std::map<std::string, PostBuf> {
        public:
            Post();
        private:
            void parsePost(void);
    };

    /*
    class GetVars {
        
        public:

            Get();
            
            bool hasKey(const std::string &key);
            const std::string &operator [] (const std::string &key);

        private:

            void init(void);
    };

    class PostVars {
        
        public:

            PostVars();

            bool hasKey(const std::string &key);
            ??? blob? const std::string &operator [] (const std::string &key);

        private:
            
            void init(void);
    };

    class CookieVars {
        
        public:
            
            CookieVars();

            bool hasKey(const std::string &key);
            const Cookie &operator [] (const std::string &key);

        private:

            void init(void);
    };
    */

#ifndef LIBCCCGI_CC
    extern Get GET;
#endif

    class DBConnection {

        public:

            DBConnection() { }
            DBConnection(const std::string &connect_string) { }
            ~DBConnection() { }

            virtual bool connect(const std::string &connect_string) = 0;
            virtual bool isConnected(void) = 0;

            // copying is forbidden (these are only declared but not defined)
            DBConnection(const DBConnection &dbc);
            DBConnection &operator = (const DBConnection &dbc);
    };

    class DBStream {

        public:

            // constructors throw exception if an unconnected DBConnection is passed
            DBStream(DBConnection &dbconn) { }
            DBStream(DBConnection &dbconn, const std::string &sql) { }
            ~DBStream() { }

            virtual DBStream &exec(const std::string &sql) = 0;
            virtual bool hasRows(void) = 0;

            virtual DBStream &operator << (const long &l) = 0;
            virtual DBStream &operator << (const std::string &s) = 0;
            
            virtual DBStream &operator >> (long &l) = 0;
            virtual DBStream &operator >> (std::string &s) = 0;

            // copying is forbidden (these are only declared but not defined)
            DBStream(const DBStream &dbc);
            DBStream &operator = (const DBStream &dbc);
    };

    class DBStreamSqlite;

    class DBConnectionSqlite: public DBConnection {

        public:

            DBConnectionSqlite();
            DBConnectionSqlite(const std::string &connect_string);
            ~DBConnectionSqlite();

            bool connect(const std::string &connect_string);
            bool isConnected(void);

        private:

            std::string connect_string;
            sqlite3 *db;

        friend class DBStreamSqlite;
    };

    class DBStreamSqlite: public DBStream {

        public:

             DBStreamSqlite(DBConnectionSqlite &dbconn);
             DBStreamSqlite(DBConnectionSqlite &dbconn, const std::string &s);
            ~DBStreamSqlite();

            DBStreamSqlite &exec(const std::string &sql);
            bool            hasRows(void);

            DBStreamSqlite &operator >> (long &l);
            DBStreamSqlite &operator >> (std::string &s);

            DBStreamSqlite &operator << (long l);
            DBStreamSqlite &operator << (const std::string &s);

        private:

            class Varsubst {
                
                public:

                    std::string::size_type pos;
                    std::string::size_type len;
                    std::string val;

                    Varsubst():
                        pos(0), len(0)
                    {
                    }

                    Varsubst(std::string::size_type pos, std::string::size_type len):
                        pos(pos), len(len)
                    {
                    }
            };

            std::string raw_sql;
            std::string sql;
            DBConnectionSqlite &dbconn;
            sqlite3_stmt *stmt;
            int status;

            int cur_col;
            int ncols;

            int cur_var;
            int nvars;
            std::vector<Varsubst> var_substs;

            int nextRow(void);
            std::vector<Varsubst>::size_type findVars(const std::string &s);
            void substAllVars(void);
            void assignSql(const std::string &s);
            void execBakedSql(void);
    };
};

#endif

