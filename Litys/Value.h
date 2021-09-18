#ifndef VALUE_H
#define VALUE_H

#include <string>

class Object;

enum ValueType {
	V_NIL,
	V_INTEGER,
	V_NUMBER,
	V_CSTRING,
	V_OBJECT,
	V_BOOL,
	V_DOUBLE16,
};

struct Value {
public:
	ValueType type;
	union {
		struct {
			short a, b;
		} double16;
		int integer;
		double number;
		const char* c_str;
		bool boolean;
		Object* object;
	} as;
	Value();
	Value(int integer);
	Value(bool boolean);
	Value(double number);
	Value(const char* cstr);
	Value(short a, short b);
	Value(Object* object);
	std::string ToString();
};

#endif