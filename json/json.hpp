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
#include <map>
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
            value(value &&) noexcept;
            inline ~value() {
                switch (m_type) {
                case ARRAY:
                    delete m_u.ptr.a;
                    return;
                case OBJECT:
                    delete m_u.ptr.o;
                    return;
                case STRING:
                    delete m_u.ptr.s;
                    return;
                default:
                    return;
                }
            }

            value& operator=(const value&);
            value& operator=(value&&) noexcept;
            value& operator=(double d);
            value& operator=(int i);
            value& operator=(bool b);
            value& operator=(const std::string& s);
            value& operator=(std::string&& s);

            /* Methods */

            const std::string& sval       ()                    const;
            int                ival       ()                    const;
            double             dval       ()                    const;
            bool               bval       ()                    const;

            std::string        str        ()                    const;
            void               push_back  (const value&);
            void               push_back  (value&& v);
            value&             operator[] (const std::string&);
            const value&       operator[] (const std::string&)  const;
            value&             operator[] (size_t);
            const value&       operator[] (size_t)              const;
            bool               operator== (const std::string&)  const;
            value&             erase      (const std::string&);

            /* Static members */

            static bool use_exceptions;

        protected:

            /* Types */

            typedef std::vector<value>           array_container;
            typedef std::map<std::string, value> object_container;

            /* Members */

            enum types m_type;

            union val {
                union ptr {
                    ptr()                                 { }
                    ptr(std::string      *s) : p(s)       { }
                    ptr(array_container  *a) : p(a)       { }
                    ptr(object_container *o) : p(o)       { }

                    void              *p;
                    std::string       *s;
                    array_container   *a;
                    object_container  *o;
                } ptr;

                val()                             { }
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
            template<typename ... Types> void vctor(const std::string& key, const value& val, Types ... args) {
                (*this->m_u.ptr.o)[key] = val;
                vctor(args...);
            }

            void vctor() {
            }
    };

    value parse(const std::string &s);
};


#endif

