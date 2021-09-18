#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <map>

#include "Token.h"

using namespace std;

class Lexer {
private:
	vector<Token> tokens;
	string source;
	string error_message;
	bool error;
	int error_line;
	unsigned int start = 0;
	unsigned int current = 0;
	unsigned int line = 0;
public:
	Lexer(string source);
	~Lexer();
	void Tokenize();
	bool Error(string& message);
	vector<Token> Get();
private:
	bool End();
	void ParseToken();
	char Advance();
	bool Match(char expected);
	char Peek(int offset = 0);
	void String();
	void Number();
	void Identifier();
	void ThrowError(string message);
};

#endif