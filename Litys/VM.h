#ifndef VM_H
#define VM_H

#include <vector>
#include <stack>
#include <map>

#include "Token.h"
#include "Value.h"

struct FnDefNode;
enum ObjectType;

using namespace std;

struct cstrcmp {
	bool operator()(const char* lhs, const char* rhs) const;
};

struct Entry {
	const char* name;
	Value value;
};

enum OperationCode {
	OP_PUSH, OP_POP,

	OP_ADD, OP_SUBTRACT, OP_MULTIPLY, OP_DIVIDE, OP_NOT, OP_NEGATE, OP_MOD, OP_DIV,

	OP_EQUAL, OP_NOT_EQUAL, OP_GREATER, OP_GREATER_EQUAL, OP_LESS, OP_LESS_EQUAL,

	OP_LOAD_NAME, OP_LOAD_FAST, OP_STORE_NAME, OP_STORE_FAST, OP_LOAD_ATTR, OP_STORE_ATTR,

	OP_JUMP, OP_JUMP_NOT_TEST,

	OP_CALL, OP_MAKE_FUNCTION, OP_STORE_CLOSURE, OP_LOAD_CLOSURE, OP_GET_SELF,

	OP_ADD_FRAME, OP_POP_FRAME,

	OP_RETURN,

	OP_NEW_OBJ, OP_SET_META,
};

string OperationCodeName(OperationCode code);

struct Operation {
	OperationCode code;
	Value value;
	Operation(OperationCode code);
	Operation(OperationCode code, Value value);
};


struct Compiler
{
	bool global = false;
	Compiler* previous = nullptr;
	vector<const char*> locals;
	vector<const char*> closures;
	Value GetLocal(const char* name, int depth = 0);
	Value GetClosure(const char* name, int depth = 0);
};

struct Assembly {
	Compiler* compiler;
	vector<Operation> operations;
	Assembly();
	~Assembly();
	void Save();
	void Put(Operation operation);
	void Set(int index, Value value);
	int Size();
};

class VM;

struct Frame {
	VM* vm;
	int return_address;
	Frame* previous;
	Value locals[256];
	int locals_count = 0;
	Frame();
	Value GetLocal(int index);
	Frame* GetPrevious(int depth);
};

struct Class {
	struct Member {
		const char* name;
		Value value;
	};
	vector<Member> members;
};

class FunctionObject;

class VM {
public:
	map<const char*, Value, cstrcmp> globals;
	Frame* frames_pool;
	int frames_pool_size;
	Frame* frames_pool_pointer;
	Frame* frame;
	Assembly& assembly;
	Value* stack;
	size_t stack_size;
	int size;
	Value* pointer;
	int current;
	Object* objects = nullptr;
	Object* objects_to_mark = nullptr;
	int parameters_count;
	int bytes_allocated = 0;
	FunctionObject* callee;
	bool exit_on_return = false;
	VM(Assembly& assembly);
	~VM();
	void Run();
	void Add(const char* name, Value value);
	void AddClassConstuctor(ObjectType type);
	void Push(Value value);
	void StoreLocal(int index, int depth, Value value);
	Value Pop();
	Value Peek();
	Frame* PullFrame();
	void ReturnFrame(Frame* frame);
	void CollectGarbage();
	void MarkValue(Value value);
	void MarkObject(Object* object);
	Value GetParameter(int index);
	int GetParametersCount();
	void NewObject(Object* object);
private:
	bool End();
	Operation Advance();
};

#endif