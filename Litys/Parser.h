#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <iostream>

#include "Token.h"
#include "VM.h"

using namespace std;

string Indent(int depth);

class Node {
public:
	TokenType type;
	virtual void Compile(Assembly& assembly) {}
	virtual string ToString(int depth = 0) { return "None"; }
	virtual ~Node() {}
};

class IdentifierNode : public Node {
public:
	const char* value;
	IdentifierNode(const char* value);
	void Compile(Assembly& assembly);
	std::string ToString(int depth = 0) { return Indent(depth) + std::string(value); }; // FREE STRING MAYBE
};

class NumberNode : public Node {
public:
	double value;
	NumberNode(double value);
	void Compile(Assembly& assembly);
	string ToString(int depth = 0) { return Indent(depth) + (trunc(value) == value ? to_string((int)value) : to_string(value)); };
};

class BoolNode : public Node {
public:
	bool value;
	BoolNode(bool value);
	void Compile(Assembly& assembly);
	string ToString(int depth = 0) { return Indent(depth) + (value ? "true" : "false"); };
};

class NilNode : public Node {
public:
	void Compile(Assembly& assembly);
	string ToString(int depth = 0) { return Indent(depth) + "NIL"; };
};


class StringNode : public Node {
public:
	const char* value;
	StringNode(const char* value);
	void Compile(Assembly& assembly);
	std::string ToString(int depth = 0) { return Indent(depth) + std::string(value); }; // FREE STRING MAYBE
};

class BinOpNode : public Node {
public:
	TokenType operation;	
	Node *left, *right;
	BinOpNode(TokenType operation, Node* left, Node* right);
	~BinOpNode();
	void Compile(Assembly& assembly);
	std::string ToString(int depth = 0) { return Indent(depth) + TokenTypeName(operation) + "\n" + left->ToString(depth + 1) + "\n" + right->ToString(depth + 1); };
}; 

class UnOpNode : public Node {
public:
	TokenType operation;
	Node *node;
	UnOpNode(TokenType operation, Node* node);
	~UnOpNode();
	void Compile(Assembly& assembly);
	std::string ToString(int depth = 0) { return Indent(depth) + TokenTypeName(operation) + "\n" + node->ToString(depth + 1); };
};

class AssignNode : public Node {
public:
	bool global;
	const char* name;
	Node* node;
	AssignNode(const char* name, Node* node);
	~AssignNode();
	void Compile(Assembly& assembly);
	std::string ToString(int depth = 0) { return Indent(depth) + "ASSIGN\n" + Indent(depth + 1) + name + "\n" + node->ToString(depth + 1); };
};

class BlockNode : public Node {
public:
	bool return_table;
	vector<Node*> nodes;
	BlockNode();
	~BlockNode();
	void Compile(Assembly& assembly);
	std::string ToString(int depth = 0)
	{
		string result = Indent(depth) + "BLOCK " + to_string(return_table);
		for (auto i : nodes)
			result += "\n" + i->ToString(depth + 1);
		return result;
	}
};

class IfNode : public Node {
public:
	Node* condition;
	Node* then_branch;
	Node* else_branch;
	IfNode(Node* condition, Node* then_branch, Node* else_branch);
	~IfNode();
	void Compile(Assembly& assembly);
	std::string ToString(int depth = 0)
	{
		string result = Indent(depth) + "IF";
		result += '\n' + condition->ToString(depth + 1);
		result += '\n' + then_branch->ToString(depth + 1);
		if (else_branch != nullptr)
			result += '\n' + else_branch->ToString(depth + 1);
		return result;
	}
};

class WhileNode : public Node {
public:
	Node* condition;
	Node* branch;
	WhileNode(Node* condition, Node* branch);
	~WhileNode();
	void Compile(Assembly& assembly);
	std::string ToString(int depth = 0)
	{
		string result = Indent(depth) + "WHILE";
		result += '\n' + condition->ToString(depth + 1);
		result += '\n' + branch->ToString(depth + 1);
		return result;
	}
};

class ForNode : public Node {
public:
	Node* initializer;
	Node* condition;
	Node* increment;
	Node* branch;
	ForNode(Node* initializer, Node* condition, Node* increment, Node* branch);
	~ForNode();
	void Compile(Assembly& assembly);
	std::string ToString(int depth = 0)
	{
		string result = Indent(depth) + "FOR";
		result += '\n' + initializer->ToString(depth + 1);
		result += '\n' + condition->ToString(depth + 1);
		result += '\n' + increment->ToString(depth + 1);
		result += '\n' + branch->ToString(depth + 1);
		return result;
	}
};

class CallNode : public Node {
public:
	Node* callee;
	vector<Node*> arguments;
	CallNode(Node* callee);
	~CallNode();
	void Compile(Assembly& assembly);
	std::string ToString(int depth = 0)
	{
		string result = Indent(depth) + "CALL";
		result += '\n' + callee->ToString(depth + 1);
		for (auto i : arguments)
			result += '\n' + i->ToString(depth + 1);
		return result;
	}
};

class FnDefNode : public Node {
public:
	bool closure;
	string name;
	vector<string> parameters;
	vector<IdentifierNode*> closures;
	Node* branch;
	FnDefNode(bool closure, string name, Node* branch, vector<string> parameters, vector<IdentifierNode*> closures);
	~FnDefNode();
	void Compile(Assembly& assembly);
	std::string ToString(int depth = 0)
	{
		string result = Indent(depth) + "FN DEF " + (closure ? "closure" : "");
		result += ' ' + name;
		for (auto i : parameters)
			result += ' ' + i;
		result += '\n' + branch->ToString(depth + 1);
		return result;
	}
};


class ReturnNode : public Node {
public:
	Node* node;
	ReturnNode(Node* node);
	~ReturnNode();
	void Compile(Assembly& assembly);
	std::string ToString(int depth = 0)
	{
		if (node != nullptr)
			return Indent(depth) + "RETURN\n" + node->ToString(depth + 1);
		else 
			return Indent(depth) + "RETURN void";
	}
};

class GetNode : public Node {
public:
	Node* node;
	string name;
	GetNode(Node* node, string name);
	~GetNode();
	void Compile(Assembly& assembly);
	std::string ToString(int depth = 0) { return Indent(depth) + "GET " + name + "\n" + node->ToString(depth + 1); };
};

class IndexNode : public Node {
public:
	Node* node;
	Node* index;
	IndexNode(Node* node, Node* index);
	~IndexNode();
	void Compile(Assembly& assembly);
	std::string ToString(int depth = 0) { return Indent(depth) + "INDEX\n" + index->ToString(depth + 1) + "\n" + node->ToString(depth + 1); };
};

class SetNode : public Node {
public:
	GetNode* get;
	IndexNode* index;
	Node* value;
	SetNode(GetNode* get, Node* value);
	SetNode(IndexNode* index, Node* value);
	~SetNode();
	void Compile(Assembly& assembly);
	std::string ToString(int depth = 0) { return Indent(depth) + "SET\n" + (get != nullptr ? get->ToString(depth + 1) : index->ToString(depth + 1)) + "\n" + value->ToString(depth + 1); };
};

class TableNode : public Node {
public:
	Node* meta = nullptr;
	vector<AssignNode*> assigns;
	~TableNode();
	void Compile(Assembly& assembly);
	std::string ToString(int depth = 0)
	{
		string result = Indent(depth) + "TABLE ";
		if (meta != nullptr)
			result += '\n' + meta->ToString(depth + 1);
		for (auto i : assigns)
			result += '\n' + Indent(depth + 1) + string(i->name) + ": \n" + i->ToString(depth + 1);
		return result;
	}
};

class ArrayNode : public Node {
public:
	vector<Node*> values;
	~ArrayNode();
	void Compile(Assembly& assembly);
	std::string ToString(int depth = 0)
	{
		string result = Indent(depth) + "ARRAY";
		for (auto i : values)
			result += '\n' + i->ToString(depth + 1);
		return result;
	}
};

class GetSelfNode : public Node {
public:
	void Compile(Assembly& assembly);
	std::string ToString(int depth = 0) { return Indent(depth) + "SELF"; };
};

class Parser {
private:
	unsigned int current;
	vector<Token> tokens;
public:
	Parser(vector<Token> tokens);
	Node* Parse();
private:
	bool Check(TokenType type);
	Token Advance();
	Token Consume(TokenType type, const char* error_msg);
	bool End();
	Token Peek();
	Token Previous();
	Node* Block();
	Node* Statement();
	Node* ReturnStatement();
	Node* FnStatement(bool closure);
	Node* IfStatement();
	Node* WhileStatement();
	Node* ForStatement();
	Node* LoadStatement();
	Node* Expression();
	Node* Assignment();
	Node* Or();
	Node* And();
	Node* Equality();
	Node* Comparison();
	Node* Term();
	Node* Factor();
	Node* Unary();
	Node* Call();
	Node* FinishCall(Node* node);
	Node* Primary();
	Node* Table();
	Node* Array();
};

#endif