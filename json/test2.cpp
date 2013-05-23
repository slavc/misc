#include <iostream>
#include <string>
#include <algorithm>

#include "json.hpp"

using namespace std;

enum return_codes {
    RC_SUCCESS,
    RC_FAILURE,
    RC_UNKNOWN_REQUEST_TYPE,
    RC_INVALID_SESSION_ID,
    RC_UNAUTHORIZED,
};

class Authenticator {
    private:
        map<string, string> accounts;

    public:
        Authenticator() {
            accounts["Jeff"] = "hunter2";
            accounts["Jack"] = "theripper";
        }

        bool auth(const string &login, const string &passwd) {
            const auto i = accounts.find(login);
            if (i != accounts.end() && i->second == passwd)
                return true;
            else
                return false;
        }
};

class Authorizer {
    private:
        map<string, vector<string>> authz; /* authorizations: who is authorized to do what actions */

    public:
        Authorizer() {
            authz["Jeff"] = { "get_ps", "get_time" };
            authz["Jack"] = { "get_ps", "get_time", "get_vmstat" };
        }

        bool is_authorized(const string &login, const string &action) {
            const auto i = authz.find(login);
            if (i == authz.end())
                return false;
            const auto &v = i->second;
            return find(v.begin(), v.end(), action) != v.end();
        }
};

class Session {
    public:
        Session(const string &login) :
            id(n++),
            login(login)
        {
        }

        int id;
        string login;

        static int n;
};
int Session::n = 0;

static Authenticator authenticator;
static Authorizer authorizer;
static vector<Session> sessions;

static json::value proc_msg(json::value& msg);
static json::value proc_login_msg(json::value &msg);
static json::value proc_sess_msg(json::value& msg);
static json::value unauthorized();
static json::value unknown_request();
static json::value invalid_session_id();
static json::value failure();
static json::value get_ps();
static json::value get_time();
static json::value get_vmstat();

static json::value proc_msg(json::value& msg) {
    if (msg.has("sess_id"))
        msg["data"] = proc_sess_msg(msg);
    else if (msg["type"] == "login")
        msg["data"] = proc_login_msg(msg);
    else
        msg["data"] = unknown_request();

    return msg;
}

static json::value proc_login_msg(json::value &msg) {
    json::value &data = msg["data"];

    if (authenticator.auth(data["login"].sval(), data["passwd"].sval())) {
        Session sess(data["login"].sval());
        sessions.push_back(sess);
        return json::object {
            "rc",      RC_SUCCESS,
            "sess_id", sess.id,
        };
    } else
        return failure();
}

static json::value proc_sess_msg(json::value& msg) {
    const auto sessi = find_if(
        begin(sessions),
        end(sessions),
        [&msg](Session &s) {
            return msg["sess_id"] == s.id;
        }
    );

    if (sessi == sessions.end())
        return invalid_session_id();
    if (!authorizer.is_authorized(sessi->login, msg["type"].sval()))
        return unauthorized();
    if (msg["type"] == "get_ps")
        return get_ps();
    else if (msg["type"] == "get_time")
        return get_time();
    else if (msg["type"] == "get_vmstat")
        return get_vmstat();
    else
        return unknown_request();
}

static json::value get_ps() {
    return json::object {
        "get_ps", true,
    };
}

static json::value get_time() {
    return json::object {
        "get_time", true,
    };
}

static json::value get_vmstat() {
    return json::object {
        "get_vmstat", true,
    };
}

static json::value unauthorized() {
    return json::object {
        "rc", RC_UNAUTHORIZED,
    };
}

static json::value unknown_request() {
    return json::object {
        "rc", RC_UNKNOWN_REQUEST_TYPE,
    };
}

static json::value invalid_session_id() {
    return json::object {
        "rc", RC_INVALID_SESSION_ID,
    };
}

static json::value failure() {
    return json::object {
        "rc", RC_FAILURE,
    };
}

int main(int argc, char **argv) {
    string line;

    while (getline(cin, line)) {
        try {
            json::value msg(json::parse(line));
            cout << proc_msg(msg).str() << endl;
        } catch (const json::exception &e) {
            cerr << "error while trying to process request: " << line << ": " << e.what() << endl;
        }
    }

    return 0;
}
