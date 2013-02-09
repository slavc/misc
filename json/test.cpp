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

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include <sys/time.h>

#include "json.hpp"

static const size_t usec_in_sec = 1000000;

static void test() {
    json::object obj {
        "key1", 123,
        "key2", 42.42,
        "key3", true,
        "key4", false,
        "key5", json::array {
            123,
            444,
            555,
        },
    };

    std::string s(obj.str());

    json::value obj2(json::parse(s));

    std::cout << obj.str() << std::endl;
    std::cout << obj2.str() << std::endl;
}

int main(int argc, char **argv) {
    if (argc > 1) {
        for (--argc, ++argv; argc > 0; --argc, ++argv) {
            std::ifstream in(*argv);

            if (in) {
                std::ostringstream ostr;
                std::string line;

                while (getline(in, line))
                    ostr << line << std::endl;
                in.close();

                std::string buf(ostr.str());
                ostr.str("");

                /*
                struct timeval tv1, tv2;
                gettimeofday(&tv1, NULL);
                */
                json::value val(json::parse(buf));
                //gettimeofday(&tv2, NULL);

                /*
                unsigned long long usec1 = tv1.tv_sec * usec_in_sec + tv1.tv_usec;
                unsigned long long usec2 = tv2.tv_sec * usec_in_sec + tv2.tv_usec;
                unsigned long long dusec = usec2 - usec1;

                unsigned int sec = dusec / usec_in_sec;
                unsigned int usec = dusec % usec_in_sec;
                */

                //std::cout << *argv << ": " << sec << "s " << usec << "us" << std::endl;

                //std::cout << val.str() << std::endl;
            }
        }
    } else {
        test();
    }

    return 0;
}
