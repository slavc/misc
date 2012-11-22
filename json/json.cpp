#include <iostream>

#include "json.hpp"

void test_vctor() {
    json::object request {
        "request", "get_terminal_info",
        "seq",     1234,
        "data",    json::array { "ATM0001", "ATM0002", "ATM0003" }
    };

    json::object response {
        "response", "get_terminal_info",
        "seq",      1234,
        "data",     json::array {
            json::object {
                "name",      "ATM0001",
                "op_status", "INS",
                "devices",   json::array {
                    json::object {
                        "name",   "CASH_DISPENSER",
                        "status", "OKI",
                    },
                    json::object {
                        "name",   "CARD_READER",
                        "status", "OKI",
                    },
                },
            },
            json::object {
                "name",      "ATM0002",
                "op_status", "INS",
                "devices",   json::array {
                    json::object {
                        "name",   "CASH_DISPENSER",
                        "status", "OKI",
                    },
                    json::object {
                        "name",   "CARD_READER",
                        "status", "OKI",
                    },
                },
            },
            json::object {
                "name",      "ATM0003",
                "op_status", "INS",
                "devices",   json::array {
                    json::object {
                        "name",   "CASH_DISPENSER",
                        "status", "OKI",
                    },
                    json::object {
                        "name",   "CARD_READER",
                        "status", "OKI",
                    },
                },
            },
        },
    };

    std::cout << "--- " << __func__ << " ---" << std::endl;
    std::cout << request.str() << std::endl;
    std::cout << response.str() << std::endl;

    for (auto i = response.begin(); i != response.end(); ++i) {
        std::cout << i.key() << ':' << i->str() << std::endl;
    }
    /*
    for (auto i = response["data"]; i != response.end(); ++i) {
        std::cout << i.idx() << ":" << i.value() << std::endl;
    }
    */
}

void test_subscript() {
    json::object object;

    object["key1"] = json::value(true);

    std::cout << "--- " << __func__ << " ---" << std::endl;
    std::cout << object.str() << std::endl;
}

int main(int argc, char **argv) {
    test_vctor();
    test_subscript();

    return 0;
}
