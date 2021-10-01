/* Rapidjson helper
*/
#ifndef _RJH__
#define _RJH__

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include <string>

class RJH
{
private:
	static rapidjson::Document::AllocatorType* allocator;

public:

	// Create rapidjson document
	static rapidjson::Document createDocument();
	
	// Create rapidjson object
	static rapidjson::Value createObject();

	// Create rapidjson array
	static rapidjson::Value createArray();

	// Add 'cop' object to the 'owner' object with 'name'
	static void addMember(rapidjson::Value& owner, rapidjson::Value& cop, rapidjson::Value::StringRefType name, rapidjson::Document::AllocatorType* all = nullptr);

	// Add 'float' property for the rapidjson object with 'name'
	static void addFloatProp(rapidjson::Value& obj, float val, rapidjson::Value::StringRefType name, rapidjson::Document::AllocatorType* all = nullptr);

	// Add 'int' property for the rapidjson object with 'name'
	static void addIntProp(rapidjson::Value& obj, int val, rapidjson::Value::StringRefType name, rapidjson::Document::AllocatorType* all = nullptr);

	// Add 'uint' property for the rapidjson object with 'name'
	static void addUintProp(rapidjson::Value& obj, unsigned int val, rapidjson::Value::StringRefType name, rapidjson::Document::AllocatorType* all = nullptr);

	// Add 'string' property for the rapidjson object with 'name'
	static void addStrProp(rapidjson::Value& obj, rapidjson::Value::StringRefType val, rapidjson::Value::StringRefType name, rapidjson::Document::AllocatorType* all = nullptr);

	// Add 'bool' property for the rapidjson object with 'name'
	static void addBoolProp(rapidjson::Value& obj, bool val, rapidjson::Value::StringRefType name, rapidjson::Document::AllocatorType* all = nullptr);

	// Add 'array' element
	static void addArrayEl(rapidjson::Value& arr, rapidjson::Value& el, rapidjson::Document::AllocatorType* all = nullptr);
	static void addStrArrayEl(rapidjson::Value& arr, const std::string& val, rapidjson::Document::AllocatorType* all = nullptr);
	static void addFloatArrayEl(rapidjson::Value& arr, float val, rapidjson::Document::AllocatorType* all = nullptr);

	// Save document to the file
	static void saveDocument(rapidjson::Document& d, const std::string& file_name, bool is_pretty = false);

	//======

	// Get array element
	static rapidjson::Value::MemberIterator getArray(rapidjson::Value& obj_from, rapidjson::Value::StringRefType name);
	static bool getArray(rapidjson::Value::ValueIterator& obj_from, rapidjson::Value::StringRefType name, rapidjson::Value::MemberIterator& it);

	// Get object element
	static rapidjson::Value::MemberIterator getObject(rapidjson::Value& obj_from, rapidjson::Value::StringRefType name);
	static rapidjson::Value::MemberIterator getObject(rapidjson::Value::ValueIterator& obj_from, rapidjson::Value::StringRefType name);

	// Get bool element
	static bool getBool(rapidjson::Value& obj_from, rapidjson::Value::StringRefType name, bool def_val = false);
	static bool getBool(rapidjson::Value::MemberIterator& obj_from, rapidjson::Value::StringRefType name, bool def_val = false);
	static bool getBool(rapidjson::Value::ValueIterator& obj_from, rapidjson::Value::StringRefType name, bool def_val = false);

	// Get float element
	static float getFloat(rapidjson::Value& obj_from, rapidjson::Value::StringRefType name, float def_val = 0.f);
	static float getFloat(rapidjson::Value::MemberIterator& obj_from, rapidjson::Value::StringRefType name, float def_val = 0.f);
	static float getFloat(rapidjson::Value::ValueIterator& obj_from, rapidjson::Value::StringRefType name, float def_val = 0.f);

	// Get int element
	static int getInt(rapidjson::Value& obj_from, rapidjson::Value::StringRefType name, int def_val = 0);
	static int getInt(rapidjson::Value::MemberIterator& obj_from, rapidjson::Value::StringRefType name, int def_val = 0);
	static int getInt(rapidjson::Value::ValueIterator& obj_from, rapidjson::Value::StringRefType name, int def_val = 0);

	// Get uint element
	static unsigned int getUint(rapidjson::Value& obj_from, rapidjson::Value::StringRefType name, unsigned int def_val = 0);
	static unsigned int getUint(rapidjson::Value::MemberIterator& obj_from, rapidjson::Value::StringRefType name, unsigned int def_val = 0);
	static unsigned int getUint(rapidjson::Value::ValueIterator& obj_from, rapidjson::Value::StringRefType name, unsigned int def_val = 0);

	// Get String element
	static std::string getString(rapidjson::Value& obj_from, rapidjson::Value::StringRefType name, const std::string& def_val);

	// Get first value pointer from array
	static rapidjson::Value::ValueIterator begin(rapidjson::Value::MemberIterator& arr);
	static rapidjson::Value::ValueIterator begin(rapidjson::Value& arr);

	// Get last value pointer from array
	static rapidjson::Value::ValueIterator end(rapidjson::Value::MemberIterator& arr);
	static rapidjson::Value::ValueIterator end(rapidjson::Value& arr);

	// Get array value by index
	static rapidjson::Value& elem(rapidjson::Value::MemberIterator& arr, int i);

	// Check if property exists in the object
	static bool isExists(rapidjson::Value::ValueIterator& obj_from, rapidjson::Value::StringRefType name);
	static bool isExists(rapidjson::Value& obj_from, rapidjson::Value::StringRefType name);
};

#endif
