#ifndef JSON_HPP
#define JSON_HPP

#include <iostream>
#include <iomanip>
#include <string>
#include <initializer_list>
#include <map>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <utility>

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

    class value {
        public:
            typedef std::vector<value> array_container;
            typedef std::vector<std::pair<std::string, value>> object_container;

            value() :
                m_type(NIL)
            {
                std::clog << "value()" << std::endl;
            }

            value(int i) :
                m_type(INTEGER),
                m_u(i)
            {
                std::clog << "value(int i)" << std::endl;
            }

            value(double d) :
                m_type(DOUBLE),
                m_u(d)
            {
                std::clog << "value(double d)" << std::endl;
            }

            value(const char *s) :
                m_type(STRING),
                m_u(new std::string(s))
            {
                std::clog << "value(const char *s)" << std::endl;
            }

            value(const std::string &s) :
                m_type(STRING),
                m_u(new std::string(s))
            {
                std::clog << "value(const std::string &s)" << std::endl;
            }

            value(bool b) :
                m_type(BOOLEAN),
                m_u(b)
            {
                std::clog << "value(bool b)" << std::endl;
            }

            value(const value &rhs) {
                std::clog << "value(const value &rhs)" << std::endl;
                *this = rhs;
            }

            value &operator=(const value &rhs) {
                std::clog << "value &operator=(const value &rhs)" << std::endl;
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

            virtual ~value() {
                std::clog << "virtual ~value()" << std::endl;
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

            std::string str() const {
                std::ostringstream os;
                switch (m_type) {
                case NIL:
                    os << "null";
                    break;
                case INTEGER:
                    os << m_u.i;
                    break;
                case DOUBLE:
                    os << m_u.d;
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

            value &push_back(const value &val) {
                if (m_type != ARRAY)
                    throw illegal_op();
                m_u.ptr.a->push_back(val);
                return *this;
            }

            class iterator {
                public:
                    typedef std::vector<value>::size_type size_type;

                    std::string key() {
                        if (m_val.m_type == OBJECT)
                            return (*m_val.m_u.ptr.o)[m_idx].first;
                        else
                            throw illegal_op();
                    }

                    size_type idx() {
                        return m_idx;
                    }

                    iterator &operator++() {
                        ++m_idx;
                        return *this;
                    }

                    iterator operator++(int) {
                        iterator prev { *this };
                        ++m_idx;
                        return prev;
                    }

                    value &operator*() {
                        if (m_val.m_type == ARRAY)
                            return (*m_val.m_u.ptr.a)[m_idx];
                        else if (m_val.m_type == OBJECT)
                            return (*m_val.m_u.ptr.o)[m_idx].second;
                        else
                            throw illegal_op { };
                    }

                    value *operator->() {
                        return &operator*();
                    }

                    bool operator==(const iterator &rhs) {
                        return &m_val == &rhs.m_val && m_idx == rhs.m_idx;
                    }

                    bool operator!=(const iterator &rhs) {
                        return !operator==(rhs);
                    }

                private:
                    iterator(size_type idx, value &val) :
                        m_idx(idx),
                        m_val(val)
                    {
                    }

                    size_type m_idx;
                    value &m_val;

                friend class value;
            };

            iterator begin() {
                if (m_type == ARRAY || m_type == OBJECT)
                    return { 0, *this };
                else
                    throw illegal_op();
            }

            iterator end() {
                if (m_type == ARRAY)
                    return { m_u.ptr.a->size(), *this };
                else if (m_type == OBJECT)
                    return { m_u.ptr.o->size(), *this };
                else
                    throw illegal_op();
            }

            value &operator[](const value &val) {
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

        protected:
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
            array() {
                this->m_type = ARRAY;
                this->m_u.ptr.a = new array_container();
            }

            array(std::initializer_list<value> l) {
                this->m_type = ARRAY;
                this->m_u.ptr.a = new array_container();
                for (auto i = l.begin(); i != l.end(); ++i)
                    this->m_u.ptr.a->push_back(*i);
            }
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
};

#endif

