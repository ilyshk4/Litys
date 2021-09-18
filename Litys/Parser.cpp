#include "Parser.h"
#include "Object.h"

string Indent(int depth)
{
	string r;
	for (int i = 0; i < depth; i++)
		r += " ";
	return r;
}

IdentifierNode::IdentifierNode(const char* value) : value(value) { type = T_IDENTIFIER; }

void IdentifierNode::Compile(Assembly& assembly)
{
	Value i = assembly.compiler->GetLocal(value);
	Value j = assembly.compiler->GetClosure(value);

	if (j.type != V_NIL)
		assembly.Put(Operation(OP_LOAD_CLOSURE, j));
	else if (i.type != V_NIL)
		assembly.Put(Operation(OP_LOAD_FAST, i));
	else
		assembly.Put(Operation(OP_LOAD_NAME, value));
}

NumberNode::NumberNode(double value) : value(value) { type = T_NUMBER; }

void NumberNode::Compile(Assembly& assembly)
{
	assembly.Put(Operation(OP_PUSH, value));
}

BoolNode::BoolNode(bool value) : value(value) { type = T_TRUE; }

void BoolNode::Compile(Assembly& assembly)
{
	assembly.Put(Operation(OP_PUSH, Value(value)));
}

void NilNode::Compile(Assembly& assembly)
{
	assembly.Put(Operation(OP_PUSH, Value()));
}

StringNode::StringNode(const char* value) : value(value) { type = T_STRING; }

void StringNode::Compile(Assembly& assembly)
{
	assembly.Put(Operation(OP_PUSH, value));
	assembly.Put(Operation(OP_NEW_OBJ, 2));
}

BinOpNode::BinOpNode(TokenType operation, Node* left, Node* right) : operation(operation), left(left), right(right) {}
BinOpNode::~BinOpNode() { delete left; delete right; }

void BinOpNode::Compile(Assembly& assembly)
{
	OperationCode code;
	switch (operation)
	{
	case T_OR:
	case T_PLUS:
		code = OP_ADD;
		break;
	case T_MINUS:
		code = OP_SUBTRACT;
		break;
	case T_AND:
	case T_STAR:
		code = OP_MULTIPLY;
		break;
	case T_SLASH:
		code = OP_DIVIDE;
		break;
	case T_BANG_EQUAL:
		code = OP_NOT_EQUAL;
		break;
	case T_EQUAL_EQUAL:
		code = OP_EQUAL;
		break;
	case T_GREATER:
		code = OP_GREATER;
		break;
	case T_GREATER_EQUAL:
		code = OP_GREATER_EQUAL;
		break;
	case T_LESS:
		code = OP_LESS;
		break;
	case T_LESS_EQUAL:
		code = OP_LESS_EQUAL;
		break;
	case T_PERCENT:
		code = OP_MOD;
		break;
	case T_DSLASH:
		code = OP_DIV;
		break;
	default:
		break;
	}
	right->Compile(assembly);
	left->Compile(assembly);
	assembly.Put(Operation(code));
}

UnOpNode::UnOpNode(TokenType operation, Node* node) : operation(operation), node(node) {}
UnOpNode::~UnOpNode() { delete node; }

void UnOpNode::Compile(Assembly& assembly)
{
	node->Compile(assembly);
	assembly.Put(Operation(operation == T_BANG ? OP_NOT : OP_NEGATE));
}

AssignNode::AssignNode(const char* name, Node* node) : name(name), node(node) {}

AssignNode::~AssignNode() { delete node; }

void AssignNode::Compile(Assembly& assembly)
{
	node->Compile(assembly);

	if (assembly.compiler->previous->global) {
		assembly.Put(Operation(OP_STORE_NAME, name));
	}
	else {
		Value i = assembly.compiler->GetLocal(name);
		if (i.type == V_NIL)
		{
			i.type = V_DOUBLE16;
			i.as.integer = assembly.compiler->locals.size();
			assembly.compiler->locals.push_back(name);
		}
		assembly.Put(Operation(OP_STORE_FAST, i));
	}
}

BlockNode::BlockNode() : return_table(false) { type = T_BEGIN; }
BlockNode::~BlockNode() { for (auto i : nodes) delete i; }

void BlockNode::Compile(Assembly& assembly)
{
	auto previous_compiler = assembly.compiler;

	assembly.compiler = new Compiler();
	assembly.compiler->previous = previous_compiler;

	assembly.Put(Operation(OP_ADD_FRAME));
	for (auto i : nodes)
		i->Compile(assembly);
	assembly.Put(Operation(OP_POP_FRAME, return_table));

	delete assembly.compiler;
	assembly.compiler = previous_compiler;
}

IfNode::IfNode(Node* condition, Node* then_branch, Node* else_branch) : condition(condition), then_branch(then_branch), else_branch(else_branch) {}

IfNode::~IfNode() { delete condition; delete then_branch; if (else_branch != nullptr) delete else_branch; }

void IfNode::Compile(Assembly& assembly)
{
	condition->Compile(assembly);
	int jump_test_index = assembly.Size();
	assembly.Put(Operation(OP_JUMP_NOT_TEST, 0));
	then_branch->Compile(assembly);
	int jump_skip_ondex = assembly.Size();
	assembly.Put(Operation(OP_JUMP));
	assembly.Set(jump_test_index, assembly.Size());
	if (else_branch != nullptr)
		else_branch->Compile(assembly);
	assembly.Set(jump_skip_ondex, assembly.Size());
}

WhileNode::WhileNode(Node* condition, Node* branch) : condition(condition), branch(branch) {}
WhileNode::~WhileNode() { delete condition; delete branch; }

void WhileNode::Compile(Assembly& assembly)
{
	int begin = assembly.Size();
	condition->Compile(assembly);
	int jump_test_index = assembly.Size();
	assembly.Put(Operation(OP_JUMP_NOT_TEST, 0));
	branch->Compile(assembly);
	assembly.Put(Operation(OP_JUMP, begin));
	assembly.Set(jump_test_index, assembly.Size());
}

ForNode::ForNode(Node* initializer, Node* condition, Node* increment, Node* branch) : initializer(initializer), condition(condition), increment(increment), branch(branch) {}
ForNode::~ForNode() { delete initializer; delete condition; delete increment; delete branch; }

void ForNode::Compile(Assembly& assembly)
{
	initializer->Compile(assembly);
	int condition_begin = assembly.Size();
	condition->Compile(assembly);
	int jump_test_index = assembly.Size();
	assembly.Put(Operation(OP_JUMP_NOT_TEST, 0));
	increment->Compile(assembly);
	branch->Compile(assembly);
	assembly.Put(Operation(OP_JUMP, condition_begin));
	assembly.Set(jump_test_index, assembly.Size());
}


CallNode::CallNode(Node* callee) : callee(callee) {}
CallNode::~CallNode() { delete callee;  for (auto i : arguments) delete i; }

void CallNode::Compile(Assembly& assembly)
{
	for (auto i : arguments)
		i->Compile(assembly);
	callee->Compile(assembly);
	assembly.Put(Operation(OP_CALL, static_cast<int>(arguments.size())));
}

FnDefNode::FnDefNode(bool closure, string name, Node* branch, vector<string> parameters, vector<IdentifierNode*> closures) : closure(closure), name(name), branch(branch), parameters(parameters), closures(closures) {}
FnDefNode::~FnDefNode() { delete branch; for (auto i : closures) delete i; }

void FnDefNode::Compile(Assembly& assembly)
{
	auto previous_compiler = assembly.compiler;

	assembly.compiler = new Compiler();
	assembly.compiler->previous = previous_compiler;

	int make_func_index = assembly.Size();
	assembly.Put(Operation(OP_MAKE_FUNCTION));

	auto f_compiler = assembly.compiler;
	assembly.compiler = previous_compiler;

	for (auto i : closures) {
		i->Compile(assembly);
		assembly.Put(Operation(OP_STORE_CLOSURE));
		assembly.compiler->closures.push_back(i->value);
	}

	assembly.compiler = f_compiler;

	if (!closure)
	{
		Value i;
		i.type = V_DOUBLE16;
		i.as.integer = previous_compiler->locals.size();
		assembly.Put(Operation(OP_STORE_FAST, i));
		previous_compiler->locals.push_back(name.c_str());
	}

	int jump_index = assembly.Size();
	assembly.Put(Operation(OP_JUMP, 0));
	assembly.Set(make_func_index, assembly.Size());
	assembly.Put(Operation(OP_ADD_FRAME));

	int j = 0;
	for (int i = parameters.size() - 1; i >= 0; i--)
	{
		assembly.compiler->locals.push_back(parameters[i].c_str());
		assembly.Put(Operation(OP_STORE_FAST, Value(j, 0)));
		j++;
	}

	branch->Compile(assembly);
	assembly.Put(Operation(OP_POP_FRAME, 0));
	assembly.Put(Operation(OP_RETURN, false));

	assembly.Set(jump_index, assembly.Size());
	
	delete assembly.compiler;
	assembly.compiler = previous_compiler;
}

ReturnNode::ReturnNode(Node* node) : node(node) {}
ReturnNode::~ReturnNode() { delete node; }

void ReturnNode::Compile(Assembly& assembly)
{
	if (node != nullptr) 
		node->Compile(assembly);
	assembly.Put(Operation(OP_RETURN, node != nullptr));
}

GetNode::GetNode(Node* node, string name) : node(node), name(name) { type = T_DOT; }
GetNode::~GetNode() { delete node; }

void GetNode::Compile(Assembly& assembly)
{
	node->Compile(assembly);
	assembly.compiler->locals.push_back("self");
	assembly.Put(Operation(OP_LOAD_ATTR, name.c_str()));
}

IndexNode::IndexNode(Node* node, Node* index) : node(node), index(index) { type = T_LEFT_SCR; }
IndexNode::~IndexNode() { delete node; delete index; }

void IndexNode::Compile(Assembly& assembly)
{
	index->Compile(assembly);
	node->Compile(assembly);
	assembly.Put(Operation(OP_LOAD_ATTR));
}

SetNode::SetNode(GetNode* get, Node* value) : get(get), value(value) {}
SetNode::SetNode(IndexNode* index, Node* value) : index(index), value(value) {}
SetNode::~SetNode() { delete get; delete index; delete value; }

void SetNode::Compile(Assembly& assembly)
{
	if (get != nullptr) {
		get->node->Compile(assembly);

		value->Compile(assembly);
		assembly.Put(Operation(OP_STORE_ATTR, get->name.c_str()));
	}
	else if (index != nullptr) {
		index->node->Compile(assembly);

		value->Compile(assembly);
		index->index->Compile(assembly);
		assembly.Put(Operation(OP_STORE_ATTR, 0));
	}
}


TableNode::~TableNode() { for (auto i : assigns) delete i; }

void TableNode::Compile(Assembly& assembly)
{
	assembly.Put(Operation(OP_NEW_OBJ, 1));

	if (meta != nullptr) {
		meta->Compile(assembly);
		assembly.Put(Operation(OP_SET_META));
	}

	for (auto i : assigns) {
		i->node->Compile(assembly);
		assembly.Put(Operation(OP_STORE_ATTR, i->name));
	}

}

ArrayNode::~ArrayNode() { for (auto i : values) delete i; }

void ArrayNode::Compile(Assembly& assembly)
{
	assembly.Put(Operation(OP_NEW_OBJ, 0));

	for (auto i : values) {
		i->Compile(assembly);
		assembly.Put(Operation(OP_STORE_ATTR));
	}
}

void GetSelfNode::Compile(Assembly& assembly)
{
	assembly.Put(OP_GET_SELF);
}

Parser::Parser(vector<Token> tokens) : tokens(tokens) {}

Node *Parser::Parse()
{
	return Block();
}

bool Parser::Check(TokenType type)
{
	if (End())
		return false;
	return Peek().type == type;
}

Token Parser::Advance()
{
	if (!End())
		current++;
	return Previous();
}

Token Parser::Consume(TokenType type, const char* error_msg)
{
	if (Check(type)) {
		return Advance();
	}
	cout << "Error at token (" << TokenTypeName(type) << ", " << TokenTypeName(Peek().type) << ") " << current << ": " << error_msg << endl;
}

bool Parser::End()
{
	return Peek().type == T_END_OF_FILE;
}

Token Parser::Peek()
{
	return tokens.at(current);
}

Token Parser::Previous()
{
	return tokens.at(current - 1);
}

Node* Parser::Block()
{
	auto r = new BlockNode();

	while (!Check(T_END) && !End())
	{
		r->nodes.push_back(Statement());
	}

	return r;
}

Node* Parser::Statement()
{
	if (Check(T_RETURN)) {
		Advance();
		return ReturnStatement();
	}

	if (Check(T_FN)) {
		Advance();
		return FnStatement(false);
	}

	if (Check(T_IF)) {
		Advance();
		return IfStatement();
	}

	if (Check(T_WHILE)) {
		Advance();
		return WhileStatement();
	}

	if (Check(T_FOR)) {
		Advance();
		return ForStatement();
	}

	if (Check(T_LOAD)) {
		Advance();
		return LoadStatement();
	}

	auto expr = Expression();
	Consume(T_SEMICOLON, "Expected semicolon after expression statement.");
	return expr;
}

Node* Parser::ReturnStatement()
{
	ReturnNode* node = new ReturnNode(nullptr);
	if (Check(T_SEMICOLON))
		Advance();
	else
		node->node = Or();
	Consume(T_SEMICOLON, "Expected semicolon after return statement.");
	return node;
}

Node* Parser::FnStatement(bool closure)
{
	string name;
	if (!closure) {
		name = Peek().value.as.c_str;
		Advance();
	}
	vector<string> parameters;
	vector<IdentifierNode*> closures;
	Consume(T_LEFT_PAREN, "Expected left parenthesis after identifier.");
	if (!Check(T_RIGHT_PAREN))
	{
		bool comma;
		do {
			parameters.push_back(Consume(T_IDENTIFIER, "Expected function parameter.").value.as.c_str);
			comma = Check(T_COMMA);
			if (comma)
				Advance();
		} while (comma);
	}
	Consume(T_RIGHT_PAREN, "Expected right parenthesis after parameters.");

	if (closure && Check(T_LEFT_SCR)) {
		Advance();
		if (!Check(T_RIGHT_SCR))
		{
			bool comma;
			do {
				closures.push_back(static_cast<IdentifierNode*>(Primary()));
				comma = Check(T_COMMA);
				if (comma)
					Advance();
			} while (comma);
		}
		Consume(T_RIGHT_SCR, "Expected right square bracket after closures.");
	}
	Node* node = Expression();
	return new FnDefNode(closure, name, node, parameters, closures);
}

Node* Parser::IfStatement()
{
	Consume(T_LEFT_PAREN, "Expected left parenthesis before condition.");
	Node* condition = Or();
	Consume(T_RIGHT_PAREN, "Expected right parenthesis after condition.");
	Node* then_branch = Expression();
	Node* else_branch = nullptr;
	if (Check(T_ELSE))
	{
		Advance();
		else_branch = Expression();
	}

	return new IfNode(condition, then_branch, else_branch);
}

Node* Parser::WhileStatement()
{
	Consume(T_LEFT_PAREN, "Expected left parenthesis before condition.");
	Node* condition = Or();
	Consume(T_RIGHT_PAREN, "Expected right parenthesis after condition.");
	Node* branch = Expression();
	return new WhileNode(condition, branch);
}

Node* Parser::ForStatement()
{
	Consume(T_LEFT_PAREN, "Expected left parenthesis after for statement.");
	Node* initializer = Expression();
	Consume(T_SEMICOLON, "Expected semicolon after for initializer.");
	Node* condition = Or();
	Consume(T_SEMICOLON, "Expected semicolon after for condition.");
	Node* increment = Expression();
	Consume(T_RIGHT_PAREN, "Expected right parenthesis after for statement parameters.");
	Node* branch = Expression();
	return new ForNode(initializer, condition, increment, branch);
}

Node* Parser::LoadStatement()
{
	return nullptr;
}

Node* Parser::Expression()
{
	return Assignment();
}

Node* Parser::Assignment()
{
	Node *node = Or();

	while (Check(T_EQUAL)) {
		Advance();

		Node* value = Or();

		if (node->type == T_IDENTIFIER) {
			const char* name = static_cast<IdentifierNode*>(node)->value;
			return new AssignNode(name, value);
		}

		if (node->type == T_DOT) {
			return new SetNode(static_cast<GetNode*>(node), value);
		}

		if (node->type == T_LEFT_SCR) {
			return new SetNode(static_cast<IndexNode*>(node), value);
		}
	}

	return node;
}

Node* Parser::Or()
{
	Node* node = And();

	while (Check(T_OR)) {
		Advance();
		Token op = Previous();
		Node* right = And();
		node = new BinOpNode(op.type, node, right);
	}

	return node;
}

Node* Parser::And()
{
	Node* node = Equality();

	while (Check(T_AND)) {
		Advance();
		Token op = Previous();
		Node* right = Equality();
		node = new BinOpNode(op.type, node, right);
	}

	return node;
}

Node* Parser::Equality()
{
	Node *node = Comparison();

	while (Check(T_BANG_EQUAL) || Check(T_EQUAL_EQUAL)) {
		Advance();
		Token op = Previous();
		Node *right = Comparison();
		node = new BinOpNode(op.type, node, right);
	}

	return node;
}

Node *Parser::Comparison()
{
	Node *node = Term();

	while (Check(T_GREATER) || Check(T_GREATER_EQUAL) || Check(T_LESS) || Check(T_LESS_EQUAL)) {
		Advance();
		Token op = Previous();
		Node *right = Term();
		node = new BinOpNode(op.type, node, right);
	}

	return node;
}

Node *Parser::Term()
{
	Node *node = Factor();

	while (Check(T_PLUS) || Check(T_MINUS)) {
		Advance();
		Token op = Previous();
		Node *right = Factor();
		node = new BinOpNode(op.type, node, right);
	}

	return node;
}

Node *Parser::Factor()
{
	Node *node = Unary();

	while (Check(T_STAR) || Check(T_SLASH) || Check(T_PERCENT) || Check(T_DSLASH)) {
		Advance();
		Token op = Previous();
		Node *right = Unary();
		node = new BinOpNode(op.type, node, right);
	}

	return node;
}

Node *Parser::Unary()
{
	if (Check(T_BANG) || Check(T_MINUS)) {
		Advance();
		Token op = Previous();
		Node *right = Unary();
		return new UnOpNode(op.type, right);
	}

	return Call();
}

Node* Parser::Call()
{
	Node* node = Primary();
	while (true) {
		if (Check(T_LEFT_PAREN))
		{
			Advance();
			node = FinishCall(node);
		}
		else if (Check(T_DOT)) {
			Advance();
			Token name = Consume(T_IDENTIFIER, "Expected attribute name.");
			node = new GetNode(node, name.value.as.c_str);
		}
		else if (Check(T_LEFT_SCR)) {
			Advance();
			Node* index = Term();
			Consume(T_RIGHT_SCR, "Expected right square bracket.");
			node = new IndexNode(node, index);
		}
		else break;
	}

	return node;
}

Node* Parser::FinishCall(Node* node)
{
	CallNode* call_node = new CallNode(node);

	if (!Check(T_RIGHT_PAREN))
	{
		bool comma;
		do {
			call_node->arguments.push_back(Or());
			comma = Check(T_COMMA);
			if (comma)
				Advance();
		} while (comma);
	}

	Consume(T_RIGHT_PAREN, "Expected right parenthesis after call identifier.");

	return call_node;
}

Node *Parser::Primary()
{
	if (Check(T_FALSE)) {
		Advance();
		return new BoolNode(false);
	}

	if (Check(T_TRUE)) {
		Advance();
		return new BoolNode(true);
	}

	if (Check(T_NIL)) {
		Advance();
		return new NilNode();
	}

	if (Check(T_IDENTIFIER)) {
		Advance();
		return new IdentifierNode(Previous().value.as.c_str);
	}

	if (Check(T_NUMBER)) {
		Advance();
		return new NumberNode(Previous().value.as.number);
	}

	if (Check(T_STRING)) {
		Advance();
		return new StringNode(Previous().value.as.c_str);
	}

	if (Check(T_SELF)) {
		Advance();
		return new GetSelfNode();
	}

	if (Check(T_LEFT_PAREN)) {
		Advance();
		Node *node = Expression();
		Consume(T_RIGHT_PAREN, "Expected group right parenthesis.");
		return node;
	}

	if (Check(T_BEGIN)) {
		Advance();
		Node* node = Block();
		Consume(T_END, "Expected end.");
		return node;
	}

	if (Check(T_LEFT_BRACE)) {
		Advance();
		TableNode* node = static_cast<TableNode*>(Table());
		Consume(T_RIGHT_BRACE, "Expected right brace.");

		if (Check(T_META)) {
			Advance();
			node->meta = Primary();
		}

		return node;
	}

	if (Check(T_LEFT_SCR)) {
		Advance();
		Node* node = Array();
		Consume(T_RIGHT_SCR, "Expected right square brace.");
		return node;
	}

	if (Check(T_FN)) {
		Advance();
		return FnStatement(true);
	}
}

Node* Parser::Table()
{
	TableNode* node = new TableNode();
	if (!Check(T_RIGHT_BRACE) || !Check(T_COMMA))
	{
		bool comma;
		do {
			if (!Check(T_RIGHT_BRACE))
				node->assigns.push_back(static_cast<AssignNode*>(Assignment()));
			comma = Check(T_COMMA);
			if (comma)
				Advance();
		} while (comma);
	}
	return node;
}

Node* Parser::Array()
{
	ArrayNode* node = new ArrayNode();
	if (!Check(T_RIGHT_SCR))
	{
		bool comma;
		do {
			node->values.push_back(Or());
			comma = Check(T_COMMA);
			if (comma)
				Advance();
		} while (comma);
	}
	return node;
}


