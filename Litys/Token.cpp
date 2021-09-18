#include "Token.h"

Token::Token(TokenType type) : type(type), value((double)0)
{
	
}

Token::Token(TokenType type, Value value) : type(type), value(value)
{
}

string tokentype2string[] {
	"LEFT_PAREN", "RIGHT_PAREN", "BEGIN", "END", "LEFT_SCR", "RIGHT_SCR", "LEFT_BRACE", "RIGHT_BRACE",
	"COMMA", "DOT", "MINUS", "PLUS", "SEMICOLON", "SLASH", "STAR", "PERCENT", "DSLASH",

	"BANG", "BANG_EQUAL",
	"EQUAL", "EQUAL_EQUAL",
	"GREATER", "GREATER_EQUAL",
	"LESS", "LESS_EQUAL",

	"IDENTIFIER", "STRING", "NUMBER", "GLOBAL",

	"AND", "ELSE", "FALSE", "FN", "FOR", "IF", "NIL", "OR",
	"RETURN", "TRUE", "WHILE", "FROM", "LOAD", "AS", "META", "SELF",

	"END_OF_FILE"
};

string TokenTypeName(TokenType type)
{
	return tokentype2string[type];
}
