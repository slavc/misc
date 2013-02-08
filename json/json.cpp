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

#include <string>
#include <sstream>

#include "json.hpp"

bool json::value::use_exceptions = true;

namespace json {
    value::value() :
        m_type(NIL)
    {
    }

    value::value(int i) :
        m_type(INTEGER),
        m_u(i)
    {
    }

    value::value(double d) :
        m_type(DOUBLE),
        m_u(d)
    {
    }

    value::value(const char *s) :
        m_type(STRING),
        m_u(new std::string(s))
    {
    }

    value::value(const std::string &s) :
        m_type(STRING),
        m_u(new std::string(s))
    {
    }

    value::value(bool b) :
        m_type(BOOLEAN),
        m_u(b)
    {
    }

    value::value(const value &rhs) {
        *this = rhs;
    }

    value &value::operator=(const value &rhs) {
        if (&rhs == this)
            return *this;

        switch (m_type) {
        case STRING:
            delete m_u.ptr.s;
            break;
        case ARRAY:
            delete m_u.ptr.a;
            break;
        case OBJECT:
            delete m_u.ptr.o;
            break;
        case NIL:
        case DOUBLE:
        case INTEGER:
        case BOOLEAN:
        default:
            break;
        }

        m_type = rhs.m_type;
        switch (m_type) {
        case ARRAY:
            m_u.ptr.a = new array_container(*rhs.m_u.ptr.a);
            break;
        case OBJECT:
            m_u.ptr.o = new object_container(*rhs.m_u.ptr.o);
            break;
        case STRING:
            m_u.ptr.s = new std::string(*rhs.m_u.ptr.s);
            break;
        case NIL:
        case INTEGER:
        case DOUBLE:
        case BOOLEAN:
            m_u = rhs.m_u;
            break;
        }

        return *this;
    }

    value::value(value &&v) noexcept {
        if (&v == this)
            return;

        m_type = v.m_type;
        m_u = v.m_u;

        v.m_type = NIL;
        v.m_u.ptr.s = nullptr;
    }

    value &value::operator=(value &&v) noexcept {
        if (&v == this)
            return *this;

        m_type = v.m_type;
        m_u = v.m_u;

        v.m_type = NIL;
        v.m_u.ptr.p = nullptr;

        return *this;
    }

    value::~value() {
        switch (m_type) {
        case ARRAY:
            delete m_u.ptr.a;
            break;
        case OBJECT:
            delete m_u.ptr.o;
            break;
        case STRING:
            delete m_u.ptr.s;
            break;
        default:
            break;
        }
    }

    std::string value::str() const {
        std::ostringstream os;
        switch (m_type) {
        case NIL:
            os << "null";
            break;
        case INTEGER:
            os << m_u.i;
            break;
        case DOUBLE:
            os << std::showpoint << m_u.d;
            break;
        case BOOLEAN:
            os << std::boolalpha << m_u.b;
            break;
        case STRING:
            os << '"' << *m_u.ptr.s << '"';
            break;
        case ARRAY:
            os << '[';
            for (auto i = m_u.ptr.a->begin(); i != m_u.ptr.a->end(); ++i) {
                if (i != m_u.ptr.a->begin())
                    os << ',';
                os << i->str();
            }
            os << ']';
            break;
        case OBJECT:
            os << '{';
            for (auto i = m_u.ptr.o->begin(); i != m_u.ptr.o->end(); ++i) {
                if (i != m_u.ptr.o->begin())
                    os << ',';
                os << '"' << i->first << '"' << ':' << i->second.str();
            }
            os << '}';
            break;
        default:
            throw invalid_object();
        }

        return os.str();
    }

    value &value::push_back(const value &val) {
        if (m_type != ARRAY)
            throw illegal_op();
        m_u.ptr.a->push_back(val);
        return *this;
    }

    value &value::operator[](const std::string &key) {
        static value nil;
        if (m_type == OBJECT)
            return (*m_u.ptr.o)[key];
        else if (use_exceptions)
            throw illegal_op();
        else {
            nil = value();
            return nil;
        }
    }

    const value &value::operator[](const std::string &key) const {
        static const value nil;
        if (m_type == OBJECT)
            return (*m_u.ptr.o)[key];
        else if (use_exceptions)
            throw illegal_op();
        else
            return nil;
    }

    value &value::operator[](size_t idx) {
        static value nil;
        if (m_type == ARRAY) {
            if (idx < 0 || idx >= m_u.ptr.a->size())
                throw out_of_bounds();
            return (*m_u.ptr.a)[idx];
        } else if (use_exceptions) {
            throw illegal_op();
        } else {
            nil = value();
            return nil;
        }
    }

    const value &value::operator[](size_t idx) const {
        static const value nil;
        if (m_type == ARRAY) {
            if (idx < 0 || idx >= m_u.ptr.a->size())
                throw out_of_bounds();
            return (*m_u.ptr.a)[idx];
        } else if (use_exceptions) {
            throw illegal_op();
        } else {
            return nil;
        }
    }

    bool value::operator==(const std::string &rhs) const {
        return m_type == STRING && *m_u.ptr.s == rhs;
    }

    value &value::erase(const std::string &key) {
        if (m_type != OBJECT)
            throw illegal_op();

        m_u.ptr.o->erase(key);

        return *this;
    }

    array::array() {
        this->m_type = ARRAY;
        this->m_u.ptr.a = new array_container();
    }

    array::array(std::initializer_list<value> l) {
        this->m_type = ARRAY;
        this->m_u.ptr.a = new array_container();
        for (auto i = l.begin(); i != l.end(); ++i)
            this->m_u.ptr.a->push_back(*i);
    }
}
