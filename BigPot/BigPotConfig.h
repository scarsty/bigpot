#pragma once

//还是用json算了
//#define USINGJSON

#include "BigPotBase.h"

#ifdef USINGJSON
#include "json/json.h"
#else
#include "tinyxml2/tinyxml2.h"
#include <stdint.h>
using namespace tinyxml2;
#endif

#include "hash/sha3.h"

class BigPotConfig : public BigPotBase
{
private:
	string _content;
	string _filename;
	SHA3 _sha3;
#ifdef USINGJSON
	Json::Value _value, _record;
#else
	XMLDocument _doc;
	XMLElement *_root, *_record;

	XMLElement* getElement(XMLElement *parent, const char * name)
	{
		auto p = parent->FirstChildElement(name);
		if (p)
		{
			return p;
		}
		else
		{
			p = parent->InsertFirstChild(_doc.NewElement(name))->ToElement();
			p->SetText("");
			return p;
		}
	}

#endif


public:
	BigPotConfig();
	virtual ~BigPotConfig();

	void init();
	void write();
	//以下函数注意默认值
#ifdef USINGJSON
	int getInteger(const char * name, int def = 0)
	{
		return _value[name].isInt() ? _value[name].asInt() : def;
	}
	string getString(const char * name, string def = "")
	{
		return _value[name].isString() ? _value[name].asString() : def;
	}
	double getDouble(const char * name, double def = 0.0)
	{
		return _value[name].isDouble() ? _value[name].asDouble() : def;
	}
	bool getBool(bool &v, const char * name)
	{
		return _value[name].isBool() ? _value[name].asBool() : false;
	}
	void setInteger(int v, const char * name) { _value[name] = v; }
	void setString(const string v, const char * name) { _value[name] = v; }
	void setDouble(double v, const char * name) { _value[name] = v; }
	void setBool(bool v, const char * name) { _value[name] = v; }

	int getRecord(const char * name)
	{
		auto name_s = _sha3(name);
		if (!_record.isMember(name_s))
		{
			return 0;
		}
		return _record[name_s].isInt() ? _record[name_s].asInt() : 0;
	}
	void setRecord(int v, const char * name) { _record[_sha3(name)] = v; }
	void clearRecord() { _record.clear(); }
#else
	//xml只有字串，故首先完成字串功能
	string getString(const char * name, string def = "")
	{
		auto p = _root->FirstChildElement(name);
		if (p && p->FirstChild())
		{
			return p->GetText();
		}
		else
		{
			return def;
		}
	}

	int getInteger(const char * name, int def = 0)
	{
		return atoi(getString(name, formatString("%d", def)).c_str());
	}

	double getDouble(const char * name, double def = 0.0)
	{
		return atof(getString(name, formatString("%f", def)).c_str());
	}

	/*bool getBool(bool &v, const char * name)
	{
		return atoi(getString(name, "0").c_str()) != 0;
	}*/

	void setString(const string v, const char * name) 
	{
		getElement(_root, name)->SetText(v.c_str());
	}

	void setInteger(int v, const char * name) 
	{
		setString(formatString("%d", v), name);
	}

	void setDouble(double v, const char * name) 
	{
		setString(formatString("%f", v), name);
	}

	void setBool(bool v, const char * name)
	{
		setString(formatString("%d", v != 0), name);
	}
	//记录
	int getRecord(const char * name)
	{
		return atoi(getElement(_record, ("_" + _sha3(getFilenameWithoutPath(name))).c_str())->GetText());
	}

	void setRecord(int v, const char * name) 
	{
		getElement(_record, ("_" + _sha3(getFilenameWithoutPath(name))).c_str())
			->SetText(formatString("%d", v).c_str());
	}
	void clearRecord() 
	{
		if (_record)
		{
			_record->DeleteChildren();
		}
	}
	//string replace(string str, const char *string_to_replace, const char *new_string);
#endif
};

