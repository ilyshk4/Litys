#include <sstream>
#include <string>
#include <iostream>

#include "Object.h"

Object::~Object() {}

void Object::MarkObjects(VM* vm) {}

int Object::Size()
{
	return sizeof(Object);
}

void Object::Operate(VM* vm, Operation operation)
{
	switch (operation.code)
	{
	case OP_EQUAL:
		vm->Push(Value(this == vm->Pop().as.object));
		break;
	case OP_NOT_EQUAL:
		vm->Push(Value(this != vm->Pop().as.object));
		break;
	default:
		break;
	}
}

Object::Object() : type(OT_OBJECT) {}

string Object::ToString()
{
	const void* address = static_cast<const void*>(this);
	std::stringstream ss;
	ss << address;
	std::string str = ss.str();

	return "<object at " + str + ">";
}

StringObject::StringObject(const char* cstr)
{
	Append(std::string(cstr));
	type = OT_STRING;
}

void StringObject::Operate(VM* vm, Operation operation)
{
	switch (operation.code)
	{
	case OP_ADD:
		Append(vm->Pop().ToString());
		vm->Push(Value(this));
		break;
	default:
		Object::Operate(vm, operation);
	}
}

void StringObject::Append(string string)
{
	for (auto j : string)
		vector.push_back((int)j);
}

ValueTableObject::ValueTableObject() { type = OT_TABLE; }

void ValueTableObject::Operate(VM* vm, Operation operation)
{
	if (operation.code == OP_ADD) {
		auto value = GetValue("__add");
		if (value.type == V_OBJECT && value.as.object->type == OT_FUNCTION) {
			FunctionObject* object = static_cast<FunctionObject*>(value.as.object);
			object->Call(this);
		}
	}
}

void ValueTableObject::MarkObjects(VM* vm)
{
	if (meta != nullptr)
		vm->MarkObject(meta);
	for (auto &i : table)
		vm->MarkValue(i.second);
}

int ValueTableObject::Size()
{
	return sizeof(ValueTableObject) + (sizeof(const char*) + sizeof(Value)) * table.size();
}

Value ValueTableObject::GetValue(const char* name)
{
	auto t = &table;
	while (t != nullptr) {
		auto r1 = t->find(name);
		if (r1 != t->end())
		{
			return (*r1).second;
		}
		else if (meta != nullptr) {
			t = &(meta->table);
		}
		else break;
	}
	return Value();
}

string ValueTableObject::ToString()
{
	auto value = GetValue("__to_string");
	if (value.type == V_OBJECT && value.as.object->type == OT_FUNCTION) {
		auto object = static_cast<FunctionObject*>(value.as.object);
		object->Call(this);
		return static_cast<StringObject*>(object->vm->Pop().as.object)->ToString();
	}
	else {
		string result = "{ ";
		int j = 0;
		for (auto i : table)
		{
			j++;
			string name = string(i.first);
			string value = i.second.ToString();
			result += "'" + name + "': " + value + (j != table.size() ? ", " : "");
		}
		result += " }";
		return result;
	}
}

ValueVectorObject::ValueVectorObject() { type = OT_ARRAY; }
void ValueVectorObject::Operate(VM* vm, Operation operation)
{
	switch (operation.code)
	{
	case OP_ADD:
		vector.push_back(vm->Pop());
		vm->Push(Value(this));
		break;
	default:
		Object::Operate(vm, operation);
		break;
	}
}

void ValueVectorObject::MarkObjects(VM* vm)
{
	for (auto i : vector)
		vm->MarkValue(i);
}

int ValueVectorObject::Size()
{
	return sizeof(ValueVectorObject) + sizeof(Value) * vector.size();
}

string ValueVectorObject::ToString()
{
	string result = "[";
	for (int i = 0; i < vector.size(); i++)
	{
		string object = vector[i].ToString();
		if (i != vector.size() - 1)
			object += ", ";
		for (auto j : object)
			result += j;
	}
	result += "]";
	return result;
}

string StringObject::ToString()
{
	string result;
	for (auto i : vector)
		result += (char)i.as.integer;
	return result;
}

FunctionObject::FunctionObject() { type = OT_FUNCTION; }

void FunctionObject::Operate(VM* vm, Operation operation)
{
	if (operation.code == OP_CALL)
	{
		vm->callee = this;
		vm->frame->return_address = vm->current;
		vm->current = begin;
	} else
		Object::Operate(vm, operation);
}

void FunctionObject::Call(ValueTableObject* self)
{
	this->self = self;
	vm->exit_on_return = true;
	Operate(vm, OP_CALL);
	vm->Run();
	vm->exit_on_return = false;
}

void FunctionObject::MarkObjects(VM* vm)
{
	for (auto i : closures)
		vm->MarkValue(i);
}

int FunctionObject::Size()
{
	return sizeof(FunctionObject) + sizeof(Value) * closures.size();
}

string FunctionObject::ToString()
{
	return "<function at " + to_string(begin) + ">";
}

CFunctionObject::CFunctionObject(int(&function)(VM* vm)) : function(function) { type = OT_IFUNCTION; }

void CFunctionObject::Operate(VM* vm, Operation operation)
{
	if (operation.code == OP_CALL)
	{
		Value value;
		int old_parameters_count = vm->parameters_count;
		vm->parameters_count = operation.value.as.integer;
		int ret = function(vm);
		if (ret > 0)
			value = vm->Pop();
		for (int i = 0; i < operation.value.as.integer; i++)
			vm->Pop();
		if (ret > 0)
			vm->Push(value);
		vm->frame->return_address = -1;
		vm->parameters_count = old_parameters_count;
	}
	Object::Operate(vm, operation);
}

int CFunctionObject::Size()
{
	return sizeof(CFunctionObject);
}

