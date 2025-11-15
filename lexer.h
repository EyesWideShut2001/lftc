#pragma once

enum{
	ID
	// keywords
	,TYPE_INT,TYPE_REAL,TYPE_STR,VAR,FUNCTION,IF,ELSE,WHILE,END,RETURN
	//Constants
	,INT,REAL,STR
	// delimiters
	,COMMA,FINISH,COLON,SEMICOLON,LPAR,RPAR
	// operators
	,ASSIGN,EQUAL,ADD,SUB,MUL,DIV,AND,OR,NOT,NOTEQ,LESS,GREATER,GREATERQ,SPACE,COMMENT
	};

#define MAX_STR		127

typedef struct{
	int code;		
	int line;		
	union{
		char text[MAX_STR+1];		
		int i;		
		double r;		
		};
	}Token;

#define MAX_TOKENS		4096
extern Token tokens[];
extern int nTokens;

void tokenize(const char *pch);
void showTokens();
