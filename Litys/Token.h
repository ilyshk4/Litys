#ifndef TOKEN_H
#define TOKEN_H

#include <string>

#include "Value.h"

using namespace std;

enum TokenType {
	T_LEFT_PAREN, T_RIGHT_PAREN, T_BEGIN, T_END, T_LEFT_SCR, T_RIGHT_SCR, T_LEFT_BRACE, T_RIGHT_BRACE,
	T_COMMA, T_DOT, T_MINUS, T_PLUS, T_SEMICOLON, T_SLASH, T_STAR, T_PERCENT, T_DSLASH,

	T_BANG, T_BANG_EQUAL,
	T_EQUAL, T_EQUAL_EQUAL,
	T_GREATER, T_GREATER_EQUAL,
	T_LESS, T_LESS_EQUAL,

	T_IDENTIFIER, T_STRING, T_NUMBER, T_GLOBAL,

	T_AND, T_ELSE, T_FALSE, T_FN, T_FOR, T_IF, T_NIL, T_OR,
	T_RETURN, T_TRUE, T_WHILE, T_FROM, T_LOAD, T_AS, T_META, T_SELF,

	T_END_OF_FILE
};

string TokenTypeName(TokenType type);

struct Token
{
public:
	TokenType type;
	Value value;
	Token(TokenType type);
	Token(TokenType type, Value value);
};

#endif