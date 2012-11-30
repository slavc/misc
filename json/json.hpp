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

#ifndef JSON_HPP
#define JSON_HPP

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>

namespace json {
    enum types {
        NIL,
        INTEGER,
        DOUBLE,
        BOOLEAN,
        STRING,
        ARRAY,
        OBJECT
    };

    class exception : public std::exception {
    };

    class invalid_object : public exception {
    };

    class illegal_conv : public exception {
    };

    class illegal_op : public exception {
    };

    class out_of_bounds : public exception {
    };

    class parse_error : public exception {
    };

    class value {
        public:

            class iterator {
                public:
                    typedef std::vector<value>::size_type size_type;

                    std::string key();
                    size_type idx();
                    iterator &operator++();
                    iterator operator++(int);
                    value &operator*();
                    value *operator->();
                    bool operator==(const iterator &);
                    bool operator!=(const iterator &);


                private:
                    iterator(size_type, value &);

                    size_type m_idx;
                    value &m_val;

                friend class value;
            };

            value();
            value(int);
            value(bool);
            value(double);
            value(const char *);
            value(const std::string &);
            value(const value &);
            value &operator=(const value &);
            virtual ~value();

            std::string str() const;
            value &push_back(const value &);
            iterator begin();
            iterator end();
            value &operator[](const value &);

        protected:
            typedef std::vector<value> array_container;
            typedef std::vector<std::pair<std::string, value>> object_container;
            enum types m_type;
            union val {
                union ptr {
                    ptr() : s(nullptr) { }
                    ptr(std::string *s) : s(s) { }
                    ptr(array_container *a) : a(a) { }
                    ptr(object_container *o) : o(o) { }

                    std::string       *s;
                    array_container   *a; // array
                    object_container  *o; // object
                } ptr;

                val() : ptr() { }
                val(int i) : i(i) { }
                val(double d) : d(d) { }
                val(bool b) : b(b) { }
                val(std::string *s) : ptr(s) { }
                val(array_container *a) : ptr(a) { }
                val(object_container *o) : ptr(o) { }

                int    i;
                double d;
                bool   b;
            } m_u;
    };

    class array : public value {
        public:
            array();
            array(std::initializer_list<value>);
    };

    class object : public value {
        public:
            object() {
                this->m_type = OBJECT;
                this->m_u.ptr.o = new object_container();
            }

            template<typename ... Types> object(Types ... args) {
                this->m_type = OBJECT;
                this->m_u.ptr.o = new object_container();
                vctor(args...);
            }

        private:
            template<typename ... Types> void vctor(const std::string &key, const value &val, Types ... args) {
                auto i = this->m_u.ptr.o->begin();
                for (/* empty */; i != this->m_u.ptr.o->end(); ++i)
                    if (i->first == key)
                        break;

                if (i == this->m_u.ptr.o->end())
                    this->m_u.ptr.o->push_back(std::make_pair(key, val));
                else
                    *i = std::make_pair(key, val);

                vctor(args...);
            }

            void vctor() {
            }
    };

    value parse(const std::string &s);
};

#endif

