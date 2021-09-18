#ifndef OBJECT_H
#define OBJECT_H
#include <map>

#include "VM.h"
#include "Value.h"

enum OperationCode;

enum ObjectType {
	OT_OBJECT, OT_TABLE, OT_ARRAY, OT_STRING, OT_FUNCTION, OT_IFUNCTION
};

class Object {
public:
	VM* vm;
	ObjectType type;
	struct {
		bool marked = false;
		Object* previous = nullptr;
		Object* previous_mark = nullptr;
	} gc_info;
	virtual void Operate(VM* vm, Operation operation);
	Object();
	virtual ~Object();
	virtual void MarkObjects(VM* vm);
	virtual int Size();
	virtual string ToString();
};


class ValueTableObject : public Object {
public:
	ValueTableObject* meta = nullptr;
	map<const char*, Value, cstrcmp> table;
	ValueTableObject();
	virtual void Operate(VM* vm, Operation operation);
	virtual void MarkObjects(VM* vm);
	virtual int Size();
	Value GetValue(const char* name);
	string ToString();
};

class ValueVectorObject : public Object {
public:
	vector<Value> vector;
	ValueVectorObject();
	virtual void Operate(VM* vm, Operation operation);
	virtual void MarkObjects(VM* vm);
	virtual int Size();
	string ToString();
};

class StringObject : public ValueVectorObject {
public:
	StringObject(const char* cstr);
	virtual void Operate(VM* vm, Operation operation);
	void Append(string string);
	string ToString();
};

class FunctionObject : public Object {
public:
	int begin;
	ValueTableObject* self = nullptr;
	vector<Value> closures;
	FunctionObject();
	void Call(ValueTableObject* self);
	virtual void Operate(VM* vm, Operation operation);
	virtual void MarkObjects(VM* vm);
	virtual int Size();
	string ToString();
};

class CFunctionObject : public Object {
public:
	CFunctionObject(int(&function)(VM* vm));
	virtual void Operate(VM* vm, Operation operation);
	virtual int Size();
	int(&function)(VM* vm);
};

class TabletypeObject : public Object {
public:
	int begin;
};

#endif