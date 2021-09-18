#include <iostream>

#include "Lexer.h"

using namespace std;

map<string, TokenType> keywords = {
    { "and",    T_AND },
    { "begin",    T_BEGIN },
    { "end",    T_END },
    { "meta",  T_META },
    { "self",  T_SELF },
    { "global",  T_GLOBAL },
    { "else",   T_ELSE },
    { "false",  T_FALSE },
    { "for",    T_FOR },
    { "fn",     T_FN },
    { "if",     T_IF },
    { "nil",    T_NIL },
    { "or",     T_OR },
    { "return", T_RETURN },
    { "true",   T_TRUE },
    { "while",  T_WHILE },
    { "from",  T_FROM },
    { "load",  T_LOAD },
    { "as",  T_AS },
};


Lexer::Lexer(string source) : source(source) {

}

void Lexer::Tokenize() {
    while (!End() && !error) {
        start = current;
        ParseToken();
    }
    tokens.push_back(Token(T_END_OF_FILE));
}

bool Lexer::Error(string& message)
{
    if (error)
        message = error_message;
    return error;
}

vector<Token> Lexer::Get()
{
    return tokens;
}

Lexer::~Lexer()
{

}

bool Lexer::End() {
    return current >= source.length();
}

void Lexer::ParseToken() {
    char c = Advance();
    switch (c) {
    case '(':
        tokens.push_back(T_LEFT_PAREN);
        break;
    case ')':
        tokens.push_back(T_RIGHT_PAREN);
        break;
    case '[':
        tokens.push_back(T_LEFT_SCR);
        break;
    case ']':
        tokens.push_back(T_RIGHT_SCR);
        break;
    case '{':
        tokens.push_back(T_LEFT_BRACE);
        break;
    case '}':
        tokens.push_back(T_RIGHT_BRACE);
        break;
    case ',':
        tokens.push_back(T_COMMA);
        break;
    case '.':
        tokens.push_back(T_DOT);
        break;
    case '-':
        tokens.push_back(T_MINUS);
        break;
    case '+':
        tokens.push_back(T_PLUS);
        break;
    case ';':
        tokens.push_back(T_SEMICOLON);
        break;
    case '*':
        tokens.push_back(T_STAR);
        break;
    case '/':
        tokens.push_back(Match('/') ? T_DSLASH : T_SLASH);
        break;
    case '%':
        tokens.push_back(T_PERCENT);
        break;
    case '!':
        tokens.push_back(Match('=') ? T_BANG_EQUAL : T_BANG);
        break;
    case '=':
        tokens.push_back(Match('=') ? T_EQUAL_EQUAL : T_EQUAL);
        break;
    case '<':
        tokens.push_back(Match('=') ? T_LESS_EQUAL : T_LESS);
        break;
    case '>':
        tokens.push_back(Match('=') ? T_GREATER_EQUAL : T_GREATER);
        break;
    case ' ':
    case '\r':
    case '\t':
        break;
    case '\n':
        line++;
        break;
    case '"':
        String();
        break;
    default:    
        if (isdigit(c))
            Number();
        else if (isalpha(c) || c == '_')
            Identifier();
        else
            ThrowError("Unexpected character");
        break;
    }
}

char Lexer::Advance() {
    return source.at(current++);
}

bool Lexer::Match(char expected)
{
    if (End() || source.at(current) != expected)
        return false;
    else {
        current++;
        return true;
    }
}

char Lexer::Peek(int offset)
{
    if (End())
        return '\0';
    else return source.at(current + offset);
}

void Lexer::String()
{
    int beginPosition = current;
    int beginLine = line;

    while (Peek() != '"' && !End()) {
        if (Peek() == '\n')
            line++;
        Advance();
    }

    if (End()) {
        ThrowError("String not terminated");
        return;
    }

    Advance();


    string* copy = new string(source.substr(start + 1, current - start - 2).c_str());
    tokens.push_back(Token(T_STRING, copy->c_str()));
}

void Lexer::Number()
{ 
    while (isdigit(Peek()) || isalpha(Peek()))
        Advance();

    if (Peek() == '.' && isdigit(Peek(1))) {
        Advance();
        while (isdigit(Peek()))
            Advance();
    }

    tokens.push_back(Token(T_NUMBER, stod(source.substr(start, current - start))));
}

void Lexer::Identifier()
{
    while (isalpha(Peek()) || isdigit(Peek()) || Peek() == '_')
    {
        Advance();
    }

    string value = source.substr(start, current - start);

    if (keywords.find(value) != keywords.end())
        tokens.push_back(Token(keywords[value]));
    else {
        string* copy = new string(value.c_str());
        tokens.push_back(Token(T_IDENTIFIER, copy->c_str()));
    }
}

void Lexer::ThrowError(string message)
{
    error_message = "Error at line " + to_string(line) + ": " + message;
    error = true;
}
