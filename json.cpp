#include "json.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
using namespace std;

#define __DEBUG

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

class Jbase;
struct json_object* dfsToJObj(const Jbase* obj);
Jbase* dfsFromJObj(struct json_object* jsonObj);

class Jbase
{
public:
    virtual Jtype getType() const = 0;
    virtual ~Jbase() {}
    string toJsonString() const
    {
        struct json_object* jsonObj = dfsToJObj(this);
        string res = json_object_to_json_string(jsonObj);
        json_object_put(jsonObj);
        return res;
    }
    static Jbase* fromJsonString(const string _jsonStr)
    {
        struct json_object* jsonObj = json_tokener_parse(_jsonStr.c_str());
        if (is_error(jsonObj))
        {
            #ifdef __DEBUG
                printf("error parsing json: %s\n", json_tokener_errors[-(unsigned long)jsonObj]);
            #endif
            return NULL;
        }
        Jbase* res = dfsFromJObj(jsonObj);
        json_object_put(jsonObj);
        return res;
    }
};

template <typename Type>
class J : public Jbase
{
private:
    Type _val;
public:
    J() : _val(Type()) {}
    J(const Type _v) : _val(_v) {}
    Type get() const { return _val; }
    virtual Jtype getType() const;
};
template<> Jtype J<int>::getType() const { return JSON_INT; }
template<> Jtype J<double>::getType() const { return JSON_DOUBLE; }
template<> Jtype J<string>::getType() const { return JSON_STRING; }
template<> Jtype J<bool>::getType() const { return JSON_BOOLEAN; }

template<>
class J<array> : public Jbase
{
private:
    vector<Jbase*> _jarr;
public:
    virtual Jtype getType() const { return JSON_ARRAY; }
    int length() const { return _jarr.size(); }
    void add(Jbase* _val) { _jarr.push_back(_val); }
    void put(const int _idx, Jbase* _val)
    {
        if (_jarr.size() > _idx)
        {
            if (_jarr[_idx] != NULL)
                delete _jarr[_idx];
            _jarr[_idx] = _val;
        }
        else
        {
            while (_jarr.size() < _idx) _jarr.push_back(NULL);
            _jarr.push_back(_val);
        }
    }
    Jbase* get(const int _idx)
    {
        if (_idx >= _jarr.size()) return NULL;
        return _jarr[_idx];
    }
    const Jbase* get(const int _idx) const
    {
        if (_idx >= _jarr.size()) return NULL;
        return _jarr[_idx];
    }
    //vector<Jbase*>& getArray() { return _jarr; }
    virtual string toJsonString() const
    {

    }
    virtual ~J<array>()
    {
        for (int i = 0; i < _jarr.size(); ++i)
            if (_jarr[i] != NULL) delete _jarr[i];
    }
};

template<>
class J<object> : public Jbase
{
private:
    map<string, Jbase*> _jmap;
    set<string> _jkeys;
    void _clear()
    {
        _jkeys.clear();
        for (map<string, Jbase*>::iterator iter = _jmap.begin(); iter != _jmap.end(); ++iter)
        {
            if (iter->second)
                delete iter->second;
        }
        _jmap.clear();
    }
public:
    virtual Jtype getType() const { return JSON_OBJECT; }
    void add(const string _key, Jbase* _val)
    {
        del(_key);
        _jkeys.insert(_key);
        _jmap[_key] = _val;
    }
    void del(const string _key)
    {
        set<string>::iterator iter = _jkeys.find(_key);
        if (iter != _jkeys.end())
        {
            _jkeys.erase(iter);
            _jmap.erase(_jmap.find(_key));
        }
    }
    Jbase* get(const string _key) { return _jmap[_key]; }
    const Jbase* get(const string _key) const
    {
        map<string, Jbase*>::const_iterator iter = _jmap.find(_key);
        if (iter != _jmap.end()) return iter->second;
        return NULL;
    }
    //map<string, Jbase*>& getMap() { return _jmap; }
    const set<string>& getKeyArray() const { return _jkeys; }

    virtual ~J<object>()
    {
        _clear();
    }
};

Jbase* dfsFromJObj(struct json_object* jsonObj)
{
    if (jsonObj == NULL) return NULL;
    enum json_type type = json_object_get_type(jsonObj);
    switch(type)
    {
    case json_type_boolean:
        return new J<bool>(json_object_get_boolean(jsonObj));
    case json_type_double:
        return new J<double>(json_object_get_double(jsonObj));
    case json_type_int:
        return new J<int>(json_object_get_int(jsonObj));
    case json_type_string:
        return new J<string>(json_object_get_string(jsonObj));
    default: break;
    }
    Jbase* res;
    if (type == json_type_object)
    {
        res = new J<object>;
        json_object_object_foreach(jsonObj, key, val)
        {
            ((J<object>*)res)->add(key, dfsFromJObj(val));
        }
    }
    else //json_type_array
    {
        res = new J<array>;
        int length = json_object_array_length(jsonObj);
        for (int i = 0; i < length; ++i)
        {
            ((J<array>*)res)->add(dfsFromJObj(json_object_array_get_idx(jsonObj, i)));
        }
    }
    return res;
}

struct json_object* dfsToJObj(const Jbase* obj)
{
    if (obj == NULL) return NULL;
    switch(obj->getType())
    {
    case JSON_INT:
        return json_object_new_int(((const J<int>*)obj)->get());
    case JSON_DOUBLE:
        return json_object_new_double(((const J<double>*)obj)->get());
    case JSON_STRING:
        return json_object_new_string(((const J<string>*)obj)->get().c_str());
    case JSON_BOOLEAN:
        return json_object_new_boolean(((const J<bool>*)obj)->get());
    default:
        break;
    }
    struct json_object* res;
    if (obj->getType() == JSON_OBJECT)
    {
        res = json_object_new_object();
        const J<object>* jobj = (const J<object>*)obj;
        const set<string>& keys = jobj->getKeyArray();
        for (set<string>::const_iterator iter = keys.begin(); iter != keys.end(); ++iter)
        {
            json_object_object_add(res, iter->c_str(), dfsToJObj(jobj->get(*iter)));
        }
    }
    else
    {
        res = json_object_new_array();
        const J<array>* jobj = (const J<array>*)obj;
        for (int i = 0; i < jobj->length(); ++i)
        {
            json_object_array_add(res, dfsToJObj(jobj->get(i)));
        }
    }
    return res;
}

string readFile(string filename)
{
    ifstream fin(filename.c_str());
    string line, filedata;
    while (getline(fin, line)) filedata += line;
    return filedata;
}

int main()
{
    string jsonstr = readFile("/home/flyear/桌面/1.json");
    cout << jsonstr << endl;
    /*
    J<object> *obj = new J<object>;
    obj->add("a", new J<int>(1));
    obj->add("b", new J<double>(1.2));
    obj->add("c", new J<string>("hello world"));
    obj->add("d", new J<bool>(true));
    obj->add("e", NULL);
    J<object>* temp1 = new J<object>;
    temp1->add("1.a", new J<string>("hello world"));
    J<array>* temp2 = new J<array>;
    temp2->add(new J<int>(1));
    temp2->put(3, new J<bool>(true));
    temp1->add("1.b", temp2);
    J<object>* temp3 = new J<object>;
    temp3->add("1.b.obj", new J<int>(1));
    temp2->add(temp3);
    obj->add("f", temp1);
    Jbase* obj2 = Jbase::fromJsonString(obj->toJsonString());
    delete obj;
    printf("%s\n", obj2->toJsonString().c_str());*/
    return 0;
}