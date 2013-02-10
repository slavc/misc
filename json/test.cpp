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

#include "json.hpp"

static void test();

int main(int argc, char **argv) {
    if (argc > 1) {
        for (--argc, ++argv; argc > 0; --argc, ++argv) {
            std::ifstream in(*argv);

            if (!in) {
                std::cerr << *argv << ": failed to open" << std::endl;
                continue;
            }

            std::string line, buf;
            while (std::getline(in, line))
                buf.append(line);
            in.close();

            json::value val(json::parse(buf));
        }
    } else {
        test();
    }

    return 0;
}

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
            json::object {
                "aaa", true,
                "bbb", false,
                "ccc", 42,
                "ddd", 1.0/9.0,
                "eee", json::value(),
                "fff", json::array {
                    "some text",
                    42,
                    3.0/9.0,
                    true,
                    false,
                    json::value(),
                },
            },
        },
    };

    std::string s(obj.str());

    json::value obj2(json::parse(s));

    std::cout << obj.str() << std::endl;
    std::cout << obj2.str() << std::endl;
}

