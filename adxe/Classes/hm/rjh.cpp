#include "rjh.h"
#include <fstream>

rapidjson::Document::AllocatorType* RJH::allocator = nullptr;

rapidjson::Document RJH::createDocument()
{
	rapidjson::Document d;
	d.SetObject();
	allocator = &d.GetAllocator();
	return std::move(d);
}

rapidjson::Value RJH::createObject()
{
	rapidjson::Value v(rapidjson::kObjectType);
	return std::move(v);
}

rapidjson::Value RJH::createArray()
{
	rapidjson::Value v(rapidjson::kArrayType);
	return std::move(v);
}

void RJH::addMember(rapidjson::Value& owner, rapidjson::Value& cop, rapidjson::Value::StringRefType name, rapidjson::Document::AllocatorType* all)
{   
	if (!all)
		owner.AddMember(name, cop, *allocator);
	else
		owner.AddMember(name, cop, *all);
}

void RJH::addFloatProp(rapidjson::Value& obj, float val, rapidjson::Value::StringRefType name, rapidjson::Document::AllocatorType* all)
{
	rapidjson::Value v(rapidjson::kNumberType);
	v.SetFloat(val);
	if (!all)
		obj.AddMember(name, v, *allocator);
	else
		obj.AddMember(name, v, *all);
}

void RJH::addIntProp(rapidjson::Value& obj, int val, rapidjson::Value::StringRefType name, rapidjson::Document::AllocatorType* all)
{
	rapidjson::Value v(rapidjson::kNumberType);
	v.SetInt(val);
	if (!all)
		obj.AddMember(name, v, *allocator);
	else
		obj.AddMember(name, v, *all);
}

void RJH::addUintProp(rapidjson::Value& obj, unsigned int val, rapidjson::Value::StringRefType name, rapidjson::Document::AllocatorType* all)
{
	rapidjson::Value v(rapidjson::kNumberType);
	v.SetUint(val);
	if (!all)
		obj.AddMember(name, v, *allocator);
	else
		obj.AddMember(name, v, *all);
}

void RJH::addStrProp(rapidjson::Value& obj, rapidjson::Value::StringRefType val, rapidjson::Value::StringRefType name, rapidjson::Document::AllocatorType* all)
{
	rapidjson::Value v(rapidjson::kStringType);
	if (!all)
	{
		v.SetString(val, *allocator);
		obj.AddMember(name, v, *allocator);
	}
	else
	{
		v.SetString(val, *all);
		obj.AddMember(name, v, *all);
	}
}

void RJH::addBoolProp(rapidjson::Value& obj, bool val, rapidjson::Value::StringRefType name, rapidjson::Document::AllocatorType* all)
{
	rapidjson::Value v(rapidjson::kNumberType);
	v.SetBool(val);
	if (!all)
		obj.AddMember(name, v, *allocator);
	else
		obj.AddMember(name, v, *all);
}

void RJH::addArrayEl(rapidjson::Value& arr, rapidjson::Value& el, rapidjson::Document::AllocatorType* all)
{
	assert(arr.IsArray());
	if (!all)
		arr.GetArray().PushBack(el, *allocator);
	else
		arr.GetArray().PushBack(el, *all);
}

void RJH::addStrArrayEl(rapidjson::Value& arr, const std::string& val, rapidjson::Document::AllocatorType* all)
{
	assert(arr.IsArray());
	if (!all)
	{
		rapidjson::Value v;
		v.SetString(rapidjson::Value::StringRefType(val.c_str()), *allocator);
		arr.GetArray().PushBack(v, *allocator);
	}
	else
	{
		rapidjson::Value v;
		v.SetString(rapidjson::Value::StringRefType(val.c_str()), *all);
		arr.GetArray().PushBack(v, *all);
	}
}

void RJH::addFloatArrayEl(rapidjson::Value& arr, float val, rapidjson::Document::AllocatorType* all)
{
	assert(arr.IsArray());
	if (!all)
		arr.GetArray().PushBack(val, *allocator);
	else
		arr.GetArray().PushBack(val, *all);
}

void RJH::saveDocument(rapidjson::Document& d, const ::std::string& file_name, bool is_pretty)
{
	struct Stream {
		std::ofstream of;
		typedef char Ch;
		void Put(Ch ch) { of.put(ch); }
		void Flush() {}
		Stream() {}
		Stream(const std::string& p) : of(p) {}
	};

	Stream stream(file_name);

	if (is_pretty)
	{
		rapidjson::PrettyWriter<Stream> writer(stream);
		d.Accept(writer);
	}
	else
	{
		rapidjson::Writer<Stream> writer(stream);
		d.Accept(writer);
	}

	allocator = nullptr;
}

rapidjson::Value::MemberIterator RJH::getArray(rapidjson::Value& obj_from, rapidjson::Value::StringRefType name)
{
	auto val = obj_from.FindMember(name);
	if (val == obj_from.MemberEnd())
		assert(0);
	assert(val->value.IsArray());
	return val;
}

bool RJH::getArray(rapidjson::Value::ValueIterator& obj_from, rapidjson::Value::StringRefType name, rapidjson::Value::MemberIterator& it)
{
	auto val = obj_from->FindMember(name);
	if (val == obj_from->MemberEnd())
		return false;
	assert(val->value.IsArray());
	it = val;
	return true;
}

rapidjson::Value::MemberIterator RJH::getObject(rapidjson::Value& obj_from, rapidjson::Value::StringRefType name)
{
	auto val = obj_from.FindMember(name);
	if (val == obj_from.MemberEnd())
		assert(0);
	assert(val->value.IsObject());
	return val;
}

rapidjson::Value::MemberIterator RJH::getObject(rapidjson::Value::ValueIterator& obj_from, rapidjson::Value::StringRefType name)
{
	auto v = obj_from->FindMember(name);
	assert(v->value.IsObject());
	return v;
}

bool RJH::getBool(rapidjson::Value& obj_from, rapidjson::Value::StringRefType name, bool def_val)
{
	auto val = obj_from.FindMember(name);
	if (val == obj_from.MemberEnd())
		return def_val;
	assert(val->value.IsBool());
	return val->value.GetBool();
}

bool RJH::getBool(rapidjson::Value::MemberIterator& obj_from, rapidjson::Value::StringRefType name, bool def_val)
{
	auto val = obj_from->value.FindMember(name);
	if (val == obj_from->value.MemberEnd())
		return def_val;
	assert(val->value.IsBool());
	return val->value.GetBool();
}

bool RJH::getBool(rapidjson::Value::ValueIterator& obj_from, rapidjson::Value::StringRefType name, bool def_val)
{
	auto val = obj_from->FindMember(name);
	if (val == obj_from->MemberEnd())
		return def_val;
	assert(val->value.IsBool());
	return val->value.GetBool();
}

float RJH::getFloat(rapidjson::Value& obj_from, rapidjson::Value::StringRefType name, float def_val)
{
	auto val = obj_from.FindMember(name);
	if (val == obj_from.MemberEnd())
		return def_val;
	assert(val->value.IsFloat());
	return val->value.GetFloat();
}

float RJH::getFloat(rapidjson::Value::MemberIterator& obj_from, rapidjson::Value::StringRefType name, float def_val)
{
	auto val = obj_from->value.FindMember(name);
	if (val == obj_from->value.MemberEnd())
		return def_val;
	assert(val->value.IsFloat());
	return val->value.GetFloat();
}

float RJH::getFloat(rapidjson::Value::ValueIterator& obj_from, rapidjson::Value::StringRefType name, float def_val)
{
	auto val = obj_from->FindMember(name);
	if (val == obj_from->MemberEnd())
		return def_val;
	assert(val->value.IsFloat());
	return val->value.GetFloat();
}

int RJH::getInt(rapidjson::Value& obj_from, rapidjson::Value::StringRefType name, int def_val)
{
	auto val = obj_from.FindMember(name);
	if (val == obj_from.MemberEnd())
		return def_val;
	assert(val->value.IsInt());
	return val->value.GetInt();
}

int RJH::getInt(rapidjson::Value::MemberIterator& obj_from, rapidjson::Value::StringRefType name, int def_val)
{
	auto val = obj_from->value.FindMember(name);
	if (val == obj_from->value.MemberEnd())
		return def_val;
	assert(val->value.IsInt());
	return val->value.GetInt();
}

int RJH::getInt(rapidjson::Value::ValueIterator& obj_from, rapidjson::Value::StringRefType name, int def_val)
{
	auto val = obj_from->FindMember(name);
	if (val == obj_from->MemberEnd())
		return def_val;
	if (val->value.IsInt())
	    return val->value.GetInt();
	assert(val->value.IsUint());
	return val->value.GetUint();
}

unsigned int RJH::getUint(rapidjson::Value& obj_from, rapidjson::Value::StringRefType name, unsigned int def_val)
{
	auto val = obj_from.FindMember(name);
	if (val == obj_from.MemberEnd())
		return def_val;
	assert(val->value.IsUint());
	return val->value.GetUint();
}

unsigned int RJH::getUint(rapidjson::Value::MemberIterator& obj_from, rapidjson::Value::StringRefType name, unsigned int def_val)
{
	auto val = obj_from->value.FindMember(name);
	if (val == obj_from->value.MemberEnd())
		return def_val;
	assert(val->value.IsUint());
	return val->value.GetUint();
}

unsigned int RJH::getUint(rapidjson::Value::ValueIterator& obj_from, rapidjson::Value::StringRefType name, unsigned int def_val)
{
	auto val = obj_from->FindMember(name);
	if (val == obj_from->MemberEnd())
		return def_val;
	assert(val->value.IsUint());
	return val->value.GetUint();
}

std::string RJH::getString(rapidjson::Value& obj_from, rapidjson::Value::StringRefType name, const std::string& def_val)
{
	auto val = obj_from.FindMember(name);
	if (val == obj_from.MemberEnd())
		return def_val;
	assert(val->value.IsString());
	return val->value.GetString();
}

rapidjson::Value::ValueIterator RJH::begin(rapidjson::Value::MemberIterator& arr)
{
	assert(arr->value.IsArray());
	return arr->value.Begin();
}

rapidjson::Value::ValueIterator RJH::begin(rapidjson::Value& arr)
{
	assert(arr.IsArray());
	return arr.Begin();
}

rapidjson::Value::ValueIterator RJH::end(rapidjson::Value::MemberIterator& arr)
{
	assert(arr->value.IsArray());
	return arr->value.End();
}

rapidjson::Value::ValueIterator RJH::end(rapidjson::Value& arr)
{
	assert(arr.IsArray());
	return arr.End();
}

rapidjson::Value& RJH::elem(rapidjson::Value::MemberIterator& arr, int i)
{
	assert(arr->value.IsArray());
	return arr->value[i];
}

bool RJH::isExists(rapidjson::Value::ValueIterator& obj_from, rapidjson::Value::StringRefType name)
{
	return obj_from->FindMember(name) != obj_from->MemberEnd();
}

bool RJH::isExists(rapidjson::Value& obj_from, rapidjson::Value::StringRefType name)
{
	return obj_from.FindMember(name) != obj_from.MemberEnd();
}
