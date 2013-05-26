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
        m_type(INTEGER)
    {
        m_u.i = i;
    }

    value::value(double d) :
        m_type(DOUBLE)
    {
        m_u.d = d;
    }

    value::value(const std::string &s) :
        m_type(STRING)
    {
        m_u.s = new std::string(s);
    }

    value::value(bool b) :
        m_type(BOOLEAN)
    {
        m_u.b = b;
    }

    value::value(const value &rhs) :
        m_type(NIL)
    {
        *this = rhs;
    }

    value &value::operator=(const value &rhs) {
        if (&rhs == this)
            return *this;

        if (m_type == rhs.m_type) {
            switch (m_type) {
            case STRING:
                *m_u.s = *rhs.m_u.s;
                break;
            case ARRAY:
                *m_u.a = *rhs.m_u.a;
                break;
            case OBJECT:
                *m_u.o = *rhs.m_u.o;
                break;
            case DOUBLE:
                m_u.d = rhs.m_u.d;
                break;
            case INTEGER:
                m_u.i = rhs.m_u.i;
                break;
            case BOOLEAN:
                m_u.b = rhs.m_u.b;
                break;
            default:
                break;
            }
        } else {
            switch (m_type) {
            case STRING:
                delete m_u.s;
                break;
            case ARRAY:
                delete m_u.a;
                break;
            case OBJECT:
                delete m_u.o;
                break;
            default:
                break;
            }

            switch (rhs.m_type) {
            case ARRAY:
                m_u.a = new array_container(*rhs.m_u.a);
                break;
            case OBJECT:
                m_u.o = new object_container(*rhs.m_u.o);
                break;
            case STRING:
                m_u.s = new std::string(*rhs.m_u.s);
                break;
            case INTEGER:
            case DOUBLE:
            case BOOLEAN:
                m_u = rhs.m_u;
            default:
                break;
            }
            m_type = rhs.m_type;
        }

        return *this;
    }

    value::value(value &&v) noexcept {
        if (&v == this)
            return;

        m_type = v.m_type;
        m_u = v.m_u;

        v.m_type = NIL;
    }

    value &value::operator=(value &&v) noexcept {
        if (&v == this)
            return *this;

        switch (m_type) {
        case STRING:
            delete m_u.s;
            break;
        case OBJECT:
            delete m_u.o;
            break;
        case ARRAY:
            delete m_u.a;
            break;
        default:
            break;
        }

        m_type = v.m_type;
        m_u = v.m_u;

        v.m_type = NIL;

        return *this;
    }


    value &value::operator=(double d) {
        switch (m_type) {
        case OBJECT:
            delete m_u.o;
            break;
        case ARRAY:
            delete m_u.a;
            break;
        case STRING:
            delete m_u.s;
            break;
        default:
            break;
        }
        m_type = DOUBLE;
        m_u.d = d;
        return *this;
    }

    value &value::operator=(int i) {
        switch (m_type) {
        case OBJECT:
            delete m_u.o;
            break;
        case ARRAY:
            delete m_u.a;
            break;
        case STRING:
            delete m_u.s;
            break;
        default:
            break;
        }
        m_type = INTEGER;
        m_u.i = i;
        return *this;
    }

    value &value::operator=(bool b) {
        switch (m_type) {
        case OBJECT:
            delete m_u.o;
            break;
        case ARRAY:
            delete m_u.a;
            break;
        case STRING:
            delete m_u.s;
            break;
        default:
            break;
        }
        m_type = BOOLEAN;
        m_u.b = b;
        return *this;
    }

    value &value::operator=(const std::string &s) {
        switch (m_type) {
        case ARRAY:
            delete m_u.a;
            break;
        case OBJECT:
            delete m_u.o;
            break;
        default:
            m_u.s = new std::string;
        case STRING:
            break;
        }
        m_type = STRING;
        *m_u.s = s;
        return *this;
    }

    value &value::operator=(std::string&& s) {
        switch (m_type) {
        case ARRAY:
            delete m_u.a;
            break;
        case OBJECT:
            delete m_u.o;
            break;
        default:
            m_u.s = new std::string;
        case STRING:
            break;
        }
        m_type = STRING;
        *m_u.s = s;
        return *this;
    }

    const std::string &value::sval() const {
        static std::string empty;
        if (m_type == STRING)
            return *m_u.s;
        else if (use_exceptions)
            throw illegal_op();
        else
            return empty;
    }

    int value::ival() const {
        if (m_type == INTEGER)
            return m_u.i;
        else if (use_exceptions)
            throw illegal_op();
        else
            return 0;
    }

    double value::dval() const {
        if (m_type == DOUBLE)
            return m_u.d;
        else if (use_exceptions)
            throw illegal_op();
        else
            return 0.0;
    }

    bool value::bval() const {
        if (m_type == BOOLEAN)
            return m_u.b;
        else if (use_exceptions)
            throw illegal_op();
        else
            return false;
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
            os << '"' << *m_u.s << '"';
            break;
        case ARRAY:
            os << '[';
            for (auto i = m_u.a->begin(); i != m_u.a->end(); ++i) {
                if (i != m_u.a->begin())
                    os << ',';
                os << i->str();
            }
            os << ']';
            break;
        case OBJECT:
            os << '{';
            for (auto i = m_u.o->begin(); i != m_u.o->end(); ++i) {
                if (i != m_u.o->begin())
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

    void value::push_back(const value &val) {
        if (m_type == ARRAY)
            m_u.a->push_back(val);
        else if (use_exceptions)
            throw illegal_op();
    }

    void value::push_back(value &&v) {
        if (m_type == ARRAY)
            m_u.a->push_back(std::move(v));
        else if (use_exceptions)
            throw illegal_op();
    }

    value &value::operator[](const std::string &key) {
        static value nil;
        if (m_type == OBJECT)
            return (*m_u.o)[key];
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
            return (*m_u.o)[key];
        else if (use_exceptions)
            throw illegal_op();
        else
            return nil;
    }

    value &value::operator[](size_t idx) {
        static value nil;
        if (m_type == ARRAY) {
            if (idx < 0 || idx >= m_u.a->size())
                throw out_of_bounds();
            return (*m_u.a)[idx];
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
            if (idx < 0 || idx >= m_u.a->size())
                throw out_of_bounds();
            return (*m_u.a)[idx];
        } else if (use_exceptions) {
            throw illegal_op();
        } else {
            return nil;
        }
    }


    value &value::erase(const std::string &key) {
        if (m_type == OBJECT)
            m_u.o->erase(key);
        else if (use_exceptions)
            throw illegal_op();
        return *this;
    }

    array::array() {
        this->m_type = ARRAY;
        this->m_u.a = new array_container();
    }

    array::array(std::initializer_list<value> l) {
        this->m_type = ARRAY;
        this->m_u.a = new array_container();
        for (auto i = l.begin(); i != l.end(); ++i)
            this->m_u.a->push_back(*i);
    }

    bool value::has(const std::string& key) const {
        if (m_type != OBJECT)
            throw illegal_op();
        if (m_u.o->find(key) != m_u.o->end())
            return true;
        else
            return false;
    }

    bool value::equals(const value& rhs) const {
        if (m_type != rhs.m_type)
            return false;
        switch (m_type) {
        case NIL:
            return true;
        case INTEGER:
            return m_u.i == rhs.m_u.i;
        case DOUBLE:
            return m_u.d == rhs.m_u.d;
        case BOOLEAN:
            return m_u.b == rhs.m_u.b;
        case STRING:
            return *m_u.s == *rhs.m_u.s;
        case ARRAY:
            return *m_u.a == *rhs.m_u.a;
        case OBJECT:
            return *m_u.o == *rhs.m_u.o;
        default:
            throw invalid_object();
        }
    }
}


