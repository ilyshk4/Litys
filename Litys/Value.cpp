#include "Value.h"
#include "Object.h"

Value::Value() { type = V_NIL; }
Value::Value(int integer) { as.integer = integer; type = V_INTEGER; }
Value::Value(bool boolean) { as.boolean = boolean; type = V_BOOL; }
Value::Value(double number) { as.number = number; type = V_NUMBER; }
Value::Value(const char* cstr) { as.c_str = cstr; type = V_CSTRING; }
Value::Value(short a, short b) { as.double16.a = a; as.double16.b = b; type = V_DOUBLE16; }
Value::Value(Object* object) { as.object = object; type = V_OBJECT; }

std::string Value::ToString()
{
	if (type == V_BOOL)
		return as.boolean ? "true" : "false";

	if (type == V_NIL)
		return "nil";

	if (type == V_INTEGER)
		return to_string(as.integer);

	if (type == V_NUMBER)
	{
		string str = to_string(as.number);
		str.erase(str.find_last_not_of('0') + 1, std::string::npos);
		str.erase(str.find_last_not_of('.') + 1, std::string::npos);
		return str;
	}

	if (type == V_CSTRING)
		return string(as.c_str);

	if (type == V_OBJECT)
		return as.object->ToString();

	if (type == V_DOUBLE16)
		return to_string(as.double16.a) + " " + to_string(as.double16.b);

	return string();
}
