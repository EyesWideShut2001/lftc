#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "lexer.h"

int iTk;	// the iterator in tokens
Token *consumed;	// the last consumed token

// same as err, but also prints the line of the current token
 void tkerr(const char *fmt,...){
	fprintf(stderr,"error in line %d: ",tokens[iTk].line);
	va_list va;
	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
	va_end(va);
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
	}

bool consume(int code){
	if(tokens[iTk].code==code){
		consumed=&tokens[iTk++];
		return true;
		}
	return false;
	}
bool baseType()
{
	if(consume(TYPE_INT) || consume(TYPE_INT)| | TYPE_STR)
	return true;
}
//defVar ::= VAR ID COLON baseType SEMICOLON
bool defVar()
{
	printf("#defVar %d\n", tokens[iTk].code);
	int start = iTk;
	if(consume(VAR))
	{
		if(consume(ID))
		{
			if(consume(COLON))
			{
				if(baseType())
				{
					if(consume(ID))
					{
						return true;
					}
				}
			}
		}
	
	}


	iTk = start; //restaurare poz initiale
	return false;

}
//defFunc ::= FUNCTION ID LPAR funcParams? RPAR COLON baseType defVar* block END
bool defFunc()
{
	printf("#defFunc %d\n", tokens[iTk].code);
	int start = iTk;
	if(consume(FUNCTION))
	{
		if(consume(ID))
		{
			if(consume(LPAR))
			{
				if(funcParams())
				{
					if(consume(RPAR))
					{
							if(consume(COLON))
							{
								if(baseType())
								{
									if(defVar())
									{
										if(block())
										{
											if(consume(END))
											{
												return true;
											}
										}
									}
								}
							}
					}
				}
			}
		}
	}

	iTk = start; //restaurare poz initiale
	return false;
}
//block ::= instr+
bool block()
{
	printf("#block %d\n", tokens[iTk].code);
	int start = iTk;
	
	if(block())
	{
		return true;
	}

	iTk = start; //restaurare poz initiale
	return false;
}


// program ::= ( defVar | defFunc | block )* FINISH
bool program(){
	for(;;){
		if(defVar()){}
		else if(defFunc()){}
		else if(block()){}
		else break;
		}
	if(consume(FINISH)){
		return true;
		}else tkerr("syntax error");
	return false;
	}

void parse(){
	iTk=0;  // iteratorul in vectorul de atomi
	program();
	}
