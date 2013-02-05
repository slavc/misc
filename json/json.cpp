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

#include <iostream>
#include <string>

#include "json.hpp"

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
        v.m_u.ptr.s = nullptr;

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
        case NIL:
        case INTEGER:
        case DOUBLE:
        case BOOLEAN:
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

    value::iterator value::begin() {
        if (m_type == ARRAY || m_type == OBJECT)
            return { 0, *this };
        else
            throw illegal_op();
    }

    value::iterator value::end() {
        if (m_type == ARRAY)
            return { m_u.ptr.a->size(), *this };
        else if (m_type == OBJECT)
            return { m_u.ptr.o->size(), *this };
        else
            throw illegal_op();
    }

    value &value::operator[](const value &key) {
        return get(key);
    }

    const value &value::operator[](const value &key) const {
        return const_cast<value *>(this)->get(key);
    }

    bool value::operator==(const std::string &rhs) const {
        return m_type == STRING && *m_u.ptr.s == rhs;
    }

    value &value::erase(const std::string &key) {
        if (m_type != OBJECT)
            throw illegal_op();

        m_u.ptr.o->erase(
            std::remove_if(
                m_u.ptr.o->begin(),
                m_u.ptr.o->end(),
                [&key] (std::pair<std::string, value> &p) {
                    return p.first == key;
                }
            )
        );

        return *this;
    }

    value &value::get(const value &val) {
        if (val.m_type == STRING && m_type == OBJECT) {
            const std::string key(*val.m_u.ptr.s);
            if (m_type != OBJECT)
                throw illegal_op();
            for (auto i = begin(); i != end(); ++i)
                if (i.key() == key)
                    return *i;
            this->m_u.ptr.o->push_back(std::make_pair(key, value { }));
            return this->m_u.ptr.o->back().second;
        } else if (val.m_type == INTEGER && m_type == ARRAY) {
            size_t idx(val.m_u.i);
            if (m_type != ARRAY)
                throw illegal_op();
            if (idx < 0 || idx >= m_u.ptr.a->size())
                throw out_of_bounds { };
            return (*m_u.ptr.a)[idx];
        } else
            throw illegal_op { };
    }

    std::string value::iterator::key() {
        if (m_val.m_type == OBJECT)
            return (*m_val.m_u.ptr.o)[m_idx].first;
        else
            throw illegal_op();
    }

    value::iterator::size_type value::iterator::idx() {
        return m_idx;
    }

    value::iterator &value::iterator::operator++() {
        ++m_idx;
        return *this;
    }

    value::iterator value::iterator::operator++(int) {
        iterator prev { *this };
        ++m_idx;
        return prev;
    }

    value &value::iterator::operator*() {
        if (m_val.m_type == ARRAY)
            return (*m_val.m_u.ptr.a)[m_idx];
        else if (m_val.m_type == OBJECT)
            return (*m_val.m_u.ptr.o)[m_idx].second;
        else
            throw illegal_op { };
    }

    value *value::iterator::operator->() {
        return &operator*();
    }

    bool value::iterator::operator==(const iterator &rhs) {
        return &m_val == &rhs.m_val && m_idx == rhs.m_idx;
    }

    bool value::iterator::operator!=(const iterator &rhs) {
        return !operator==(rhs);
    }

    value::iterator::iterator(value::iterator::size_type idx, value &val) :
        m_idx(idx),
        m_val(val)
    {
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
