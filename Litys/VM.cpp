#include <iostream>
#include <string>
#include <chrono>

#include "VM.h"
#include "Object.h"

string operatioCode2string[]{
	"PUSH", "POP",

	"ADD", "SUBTRACT", "MULTIPLY", "DIVIDE", "NOT", "NEGATE", "MOD", "DIV",

	"EQUAL", "NOT_EQUAL", "GREATER", "GREATER_EQUAL", "LESS", "LESS_EQUAL",

	"LOAD_NAME", "LOAD_FAST", "STORE_NAME", "STORE_FAST", "LOAD_ATTR", "STORE_ATTR",

	"JUMP", "JUMP_NOT_TEST",

	"CALL", "MAKE_FUNCTION", "STORE_CLOSURE", "LOAD_CLOSURE", "GET_SELF",

	"ADD_FRAME", "POP_FRAME",

	"RETURN",

	"NEW_OBJ", "SET_META",
};

string OperationCodeName(OperationCode code)
{
	return operatioCode2string[code];
}

string InlineStack(VM* vm)
{
	string r;
	for (auto i = vm->stack; i <= vm->pointer; i++) {
		r += i->ToString() + ' ';
	}
	return r;
}

string FrameS(Frame* frame)
{
	string r = "frame\n";
	for (auto i : frame->locals)
	{
		string value = i.ToString();
		r += value + "\n";
	}
	if (frame->previous != nullptr)
		r += FrameS(frame->previous);
	return r;
}

bool cstrcmp::operator()(const char* lhs, const char* rhs) const { return std::strcmp(lhs, rhs) < 0; }

VM::VM(Assembly& assembly) : assembly(assembly), stack_size(1024 * 1024 / sizeof(Value))
{
	size = static_cast<int>(assembly.operations.size());
	stack = new Value[stack_size];

	frames_pool_size = 1024;
	frames_pool = new Frame[frames_pool_size];
	frames_pool_pointer = frames_pool + frames_pool_size - 1;

	frame = PullFrame();
	frame->vm = this;

	pointer = stack;
}

VM::~VM() { delete stack; delete frames_pool; for (auto i : globals) if (i.second.type == V_OBJECT) delete i.second.as.object; }

void VM::Run()	
{
	//map<OperationCode, long long> benchmark;
	//auto b1 = chrono::high_resolution_clock::now();

	while (!End()) {
		//int c = current;
		Operation operation = Advance();
		
		// cout << c << " " << OperationCodeName(operation.code) << " " << (operation.value.ToString()) << ' ' << InlineStack(this) << endl;
		// cout << FrameS(frame) << endl;

		//auto t1 = chrono::high_resolution_clock::now();
		switch (operation.code)
		{
		case OP_GET_SELF:
		{
			Push(callee->self);
		}
		break;
		case OP_LOAD_CLOSURE:
		{
			Push(callee->closures[operation.value.as.integer]);
		}
		break;
		case OP_STORE_CLOSURE:
		{
			auto value = Pop();
			static_cast<FunctionObject*>(Peek().as.object)->closures.push_back(value);
		}
		break;
		case OP_MAKE_FUNCTION:
		{
			auto f = new FunctionObject();
			f->begin = operation.value.as.integer;
			NewObject(f);
			Push(f);
		}
		break;
		case OP_SET_META:
		{
			ValueTableObject* meta = static_cast<ValueTableObject*>(Pop().as.object);
			ValueTableObject* table = static_cast<ValueTableObject*>(Peek().as.object);
			table->meta = meta;
		}
		break;
		case OP_NEW_OBJ:
		{
			Object* object;
			switch (operation.value.as.integer)
			{
			case 0: // vector
				object = new ValueVectorObject();
				break;
			case 1: // table
				object = new ValueTableObject();
				break;
			case 2: // string
				object = new StringObject(Pop().as.c_str);
				break;
			default:
				object = new Object();
				break;
			}
			NewObject(object);
			Push(object);
		}
			break;
		case OP_RETURN:
		{
			while (frame->return_address == -1 && frame->previous != nullptr) {
				Frame* previous = frame->previous;
				ReturnFrame(frame);
				frame = previous;
			}
			current = frame->return_address;
			frame->return_address = -1;
			if (exit_on_return)
				return;
		}
			break;
		case OP_ADD_FRAME:
		{
			Frame* old = frame;
			frame = PullFrame();
			frame->vm = this;
			frame->previous = old;
		}
			break;
		case OP_POP_FRAME:
		{
			if (frame->previous != nullptr) {
				Frame* previous = frame->previous;
				ReturnFrame(frame);
				frame = previous;
			}
		}
			break;
		case OP_LOAD_ATTR:
		{
			Value result;

			if (operation.value.type == V_CSTRING)
			{
				Value v = Pop();
				ValueTableObject* obj = static_cast<ValueTableObject*>(v.as.object);
				result = obj->GetValue(operation.value.as.c_str);

				if (result.type == V_OBJECT && result.as.object->type == OT_FUNCTION) {
					auto f = (static_cast<FunctionObject*>(result.as.object));
					f->self = obj;
				}
			}
			else
			{
				auto obj = (static_cast<ValueVectorObject*>(Pop().as.object));
				auto at = ((int)Pop().as.number);
				result = obj->vector.at(at);
			} 

			Push(result);
		}
			break;
		case OP_STORE_ATTR:
		{
			if (operation.value.type == V_CSTRING)
			{
				auto index = operation.value.as.c_str;
				auto value = Pop();

				auto obj = static_cast<ValueTableObject*>(Peek().as.object);

				obj->table[index] = value;
			} else if (operation.value.type == V_NUMBER)
			{
				auto index = (int)Pop().as.number;
				auto value = Pop();
				auto obj = static_cast<ValueVectorObject*>(Peek().as.object);

				obj->vector[index] = value;
			} else {
				auto value = Pop();
				auto obj = static_cast<ValueVectorObject*>(Peek().as.object);

				obj->vector.push_back(value);
			}
		}
			break;
		case OP_LOAD_NAME:
			Push(globals[operation.value.as.c_str]);
			break;
		case OP_STORE_NAME:
			globals[operation.value.as.c_str] = Pop();
			break;
		case OP_LOAD_FAST:
			Push(frame->GetPrevious(operation.value.as.double16.b)->GetLocal(operation.value.as.double16.a));
			break;
		case OP_STORE_FAST:
		{
			StoreLocal(operation.value.as.double16.a, operation.value.as.double16.b, Pop());
		}
			break;
		case OP_PUSH:
			Push(operation.value);
			break;
		case OP_POP:
			for (int j = 0; j < operation.value.as.integer; j++)
				Pop();
			break;
		case OP_JUMP_NOT_TEST:
			if (!Pop().as.boolean)
				current = operation.value.as.integer;
			break;
		case OP_JUMP:
			current = operation.value.as.integer;
			break;
		default:
			Value value = Pop();

			if (value.type == V_OBJECT)
				value.as.object->Operate(this, operation);

			if (value.type == V_NUMBER)
			{
				switch (operation.code)
				{
				case OP_ADD:
					Push(Value(value.as.number + Pop().as.number));
					break;
				case OP_SUBTRACT:
					Push(Value(value.as.number - Pop().as.number));
					break;
				case OP_DIVIDE:
					Push(Value(value.as.number / Pop().as.number));
					break;
				case OP_MULTIPLY:
					Push(Value(value.as.number * Pop().as.number));
					break;
				case OP_NOT:
					Push(Value(!value.as.number));
					break;
				case OP_NEGATE:
					Push(Value(-value.as.number));
					break;
				case OP_EQUAL:
					Push(Value(value.as.number == Pop().as.number));
					break;
				case OP_NOT_EQUAL:
					Push(Value(value.as.number != Pop().as.number));
					break;
				case OP_GREATER:
					Push(Value(value.as.number > Pop().as.number));
					break;
				case OP_GREATER_EQUAL:
					Push(Value(value.as.number >= Pop().as.number));
					break;
				case OP_LESS:
					Push(Value(value.as.number < Pop().as.number));
					break;
				case OP_LESS_EQUAL:
					Push(Value(value.as.number <= Pop().as.number));
					break;
				case OP_MOD:
					Push(Value(div((int)value.as.number, (int)Pop().as.number).rem));
					break;
				case OP_DIV:
					Push(Value(div((int)value.as.number, (int)Pop().as.number).quot));
					break;
				default:
					break;
				}
			}
			break;
		}

		//auto t2 = chrono::high_resolution_clock::now();
		//benchmark[operation.code] += chrono::duration_cast<chrono::microseconds>(t2 - t1).count();
	}
	//auto b2 = chrono::high_resolution_clock::now();
	//
	//cout << chrono::duration_cast<chrono::microseconds>(b2 - b1).count() << endl;
	//
	//for (auto i : benchmark)
	//{
	//	cout << OperationCodeName(i.first) << ": " << i.second << endl;
	//}

	CollectGarbage();
}

void VM::Add(const char* name, Value value)
{
	globals[name] = value;
}

Value VM::GetParameter(int index)
{
	return *(pointer - parameters_count + index + 1);
}

int VM::GetParametersCount()
{
	return parameters_count;
}

bool VM::End()
{
	return current >= size;
}

void VM::NewObject(Object* object)
{
	object->vm = this;
	if (bytes_allocated > 1024 * 1024)
		CollectGarbage();
	bytes_allocated += object->Size();

	object->gc_info.previous = objects;
	objects = object;
}

Frame* VM::PullFrame()
{
	Frame* f = frames_pool_pointer--;

	f->locals_count = 0;
	f->return_address = -1;
	f->previous = nullptr;

	return f;
}

void VM::ReturnFrame(Frame* frame)
{
	++frames_pool_pointer = frame;
}

void VM::CollectGarbage()
{
	bytes_allocated = 0;

	if (objects != nullptr) {
		Object* end = objects;
		do
		{
			end->gc_info.marked = false;
			end = end->gc_info.previous;
		} while (end != nullptr);
	}

	for (auto& i : globals)
		MarkValue(i.second);

	for (auto i = stack; i <= pointer; i++)
		MarkValue(*i);

	if (objects_to_mark == nullptr)
	{
		do {
			Object* unmarked = objects_to_mark;
			objects_to_mark = objects_to_mark->gc_info.previous_mark;
			MarkObject(unmarked);
		} while (objects_to_mark != nullptr);
	}

	int deleted_count = 0;
	Object* last = nullptr;
	Object* end = objects;
	if (end != nullptr) {
		do
		{
			if (!end->gc_info.marked) {
				Object* next_end = end->gc_info.previous;
				if (last != nullptr)
					last->gc_info.previous = end->gc_info.previous;
				else
					objects = next_end;
				delete end;
				deleted_count++;
				end = next_end;
			}
			else
			{
				last = end;
				end = end->gc_info.previous;
			}
		} while (end != nullptr);
	}
}

void VM::MarkValue(Value value)
{
	if (value.type == V_OBJECT)
		MarkObject(value.as.object);
}

void VM::MarkObject(Object* object)
{
	if (!object->gc_info.marked) {
		object->gc_info.marked = true;
		object->gc_info.previous_mark = objects_to_mark;
		objects_to_mark = object;

		object->MarkObjects(this);
	}
}

Operation VM::Advance()
{
	return assembly.operations.at(current++);
}

int CreateClassInstance(VM* vm) {
	return 1;
}

void VM::AddClassConstuctor(ObjectType type)
{
	Add("---", new CFunctionObject(CreateClassInstance));
}

void VM::Push(Value value)
{
	*++pointer = value;
}

void VM::StoreLocal(int index, int depth, Value value)
{
	Frame* f = frame->GetPrevious(depth);
	if (index < f->locals_count)
		f->locals[index] = value;
	else
		f->locals[f->locals_count++] = value;
}

Value VM::Pop()
{
	return *pointer--;
}

Value VM::Peek()
{
	return *pointer;
}

Operation::Operation(OperationCode code) : code(code) {}
Operation::Operation(OperationCode code, Value value) : code(code), value(value) {}

Assembly::Assembly()
{
	compiler = new Compiler();
	compiler->global = true;
}

Assembly::~Assembly()
{
	delete compiler;
}

void Assembly::Save()
{

}

void Assembly::Put(Operation operation)
{
	operations.push_back(operation);
}

void Assembly::Set(int index, Value value)
{
	operations.at(index).value = value;
}

int Assembly::Size()
{
	return operations.size();
}

Frame::Frame() : previous(nullptr), return_address(-1) { }

Value Frame::GetLocal(int index)
{
	return locals[index];
}

Frame* Frame::GetPrevious(int depth)
{
	if (depth > 0)
		return previous->GetPrevious(depth - 1);
	return this;
}

Value Compiler::GetLocal(const char* name, int depth)
{
	for (int i = 0; i < locals.size(); i++)
	{
		if (strcmp(locals[i], name) == 0)
			return Value(i, depth);
	}
	if (previous != nullptr)
		return previous->GetLocal(name, depth + 1);
	return Value();
}

Value Compiler::GetClosure(const char* name, int depth)
{
	for (int i = 0; i < closures.size(); i++)
	{
		if (strcmp(closures[i], name) == 0)
			return Value(i);
	}
	if (previous != nullptr)
		return previous->GetClosure(name, depth + 1);
	return Value();
}
