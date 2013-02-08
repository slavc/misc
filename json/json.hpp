/*
 * Copyright (c) 2012, 2013 Sviatoslav Chagaev <sviatoslav.chagaev@gmail.com>
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

#include <string>
#include <vector>
#include <unordered_map>
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

            /* Types */

            typedef std::vector<value>::size_type size_type;

            /* Structors and copy-control */

            value();
            value(int);
            value(bool);
            value(double);
            value(const char *);
            value(const std::string &);
            value(const value &);
            value &operator=(const value &);
            value(value &&) noexcept;
            value &operator=(value &&) noexcept;
            ~value();

            value &operator=(double d) {
                switch (m_type) {
                case OBJECT:
                    delete m_u.ptr.o;
                    break;
                case ARRAY:
                    delete m_u.ptr.a;
                    break;
                case STRING:
                    delete m_u.ptr.s;
                    break;
                default:
                    break;
                }
                m_type = DOUBLE;
                m_u.d = d;
                return *this;
            }

            value &operator=(int i) {
                switch (m_type) {
                case OBJECT:
                    delete m_u.ptr.o;
                    break;
                case ARRAY:
                    delete m_u.ptr.a;
                    break;
                case STRING:
                    delete m_u.ptr.s;
                    break;
                default:
                    break;
                }
                m_type = INTEGER;
                m_u.i = i;
                return *this;
            }

            value &operator=(bool b) {
                switch (m_type) {
                case OBJECT:
                    delete m_u.ptr.o;
                    break;
                case ARRAY:
                    delete m_u.ptr.a;
                    break;
                case STRING:
                    delete m_u.ptr.s;
                    break;
                default:
                    break;
                }
                m_type = BOOLEAN;
                m_u.b = b;
                return *this;
            }

            value &operator=(const std::string &s) {
                switch (m_type) {
                case OBJECT:
                    delete m_u.ptr.o;
                    break;
                case ARRAY:
                    delete m_u.ptr.a;
                    break;
                case STRING:
                    break;
                default:
                    m_u.ptr.s = new std::string;
                    break;
                }
                m_type = STRING;
                *m_u.ptr.s = s;
                return *this;
            }

            std::string sval() const {
                if (m_type == STRING)
                    return *m_u.ptr.s;
                else if (use_exceptions)
                    throw illegal_op();
                else
                    return "";
            }

            /* Methods */

            std::string  str        ()                       const;
            value&       push_back  (const value &);
            value&       push_back  (value &&v) {
                if (m_type == ARRAY)
                    m_u.ptr.a->push_back(std::move(v));
                else if (use_exceptions)
                    throw illegal_op();
                return *this;
            }
            value&       operator[] (const std::string &);
            const value& operator[] (const std::string &)    const;
            value&       operator[] (size_t);
            const value& operator[] (size_t)                 const;
            bool         operator== (const std::string &rhs) const;
            value&       erase      (const std::string &key);

            /* Static members */

            static bool use_exceptions;

        protected:

            /* Types */

            typedef std::vector<value>           array_container;
            typedef std::unordered_map<std::string, value> object_container;

            /* Members */

            enum types m_type;

            union val {
                union ptr {
                    ptr()                    : p(nullptr) { }
                    ptr(std::string *s)      : p(s)       { }
                    ptr(array_container *a)  : p(a)       { }
                    ptr(object_container *o) : p(o)       { }

                    void              *p;
                    std::string       *s;
                    array_container   *a;
                    object_container  *o;
                } ptr;

                val()                    : ptr()  { }
                val(int i)               : i(i)   { }
                val(double d)            : d(d)   { }
                val(bool b)              : b(b)   { }
                val(std::string *s)      : ptr(s) { }
                val(array_container *a)  : ptr(a) { }
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
                this->m_u.ptr.o->emplace(key, val);
                vctor(args...);
            }

            void vctor() {
            }
    };

    value parse(const std::string &s);
};


#endif

