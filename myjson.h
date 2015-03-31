#ifndef MYJSON_H__
#define MYJSON_H__

#include "json.h"
#include <vector>
#include <set>
#include <map>
#include <string>
using std::vector;
using std::set;
using std::map;
using std::string;

enum Jtype
{
    JSON_OBJECT,
    JSON_INT,
    JSON_DOUBLE,
    JSON_ARRAY,
    JSON_STRING,
    JSON_BOOLEAN
};

struct array{};
struct object{};

class Jbase
{
public:
    virtual Jtype getType() const = 0;
    virtual ~Jbase();
    string toJsonString() const;
    static Jbase* fromJsonString(const string _jsonStr);
};

template <typename Type>
class J : public Jbase
{
private:
    Type _val;
public:
    J();
    J(const Type _v);
    Type get() const;
    virtual Jtype getType() const;
};

template<>
class J<array> : public Jbase
{
private:
    vector<Jbase*> _jarr;
public:
    virtual Jtype getType() const;
    int length() const;
    void add(Jbase* _val);
    void put(const int _idx, Jbase* _val);
    Jbase* get(const int _idx);
    const Jbase* get(const int _idx) const;
    virtual ~J<array>();
};

template<>
class J<object> : public Jbase
{
private:
    map<string, Jbase*> _jmap;
    set<string> _jkeys;
    void _clear();
public:
    virtual Jtype getType() const;
    void add(const string _key, Jbase* _val);
    void del(const string _key);
    Jbase* get(const string _key);
    const Jbase* get(const string _key) const;
    const set<string>& getKeyArray() const;
    virtual ~J<object>();
};

extern string readFile(string filename);

#endif