#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "utils.h"

Token tokens[MAX_TOKENS];
int nTokens;

int line=1;		


Token *addTk(int code){
	if(nTokens==MAX_TOKENS)err("too many tokens");
	Token *tk=&tokens[nTokens];
	tk->code=code;
	tk->line=line;
	nTokens++;
	return tk;
	}


char *copyn(char *dst,const char *begin,const char *end){
	char *p=dst;
	if(end-begin>MAX_STR)err("string too long");
	while(begin!=end)*p++=*begin++;
	*p='\0';
	return dst;
	}

void tokenize(const char* pch) {
	const char* start;
	Token* tk;
	char buf[MAX_STR + 1];

	for (;;) {
		switch (*pch) {

		case ' ': case '\t':
			pch++;
			break;

		case '\r': 
			if (pch[1] == '\n') pch++;
			
		case '\n':
			line++;
			pch++;
			break;

		case '\0':
			addTk(FINISH);
			return;

			
		case ',':
			addTk(COMMA); pch++; break;
		case ':':
			addTk(COLON); pch++; break;
		case ';':
			addTk(SEMICOLON); pch++; break;
		case '(':
			addTk(LPAR); pch++; break;
		case ')':
			addTk(RPAR); pch++; break;

			
		case '+':
			addTk(ADD); pch++; break;
		case '-':
			addTk(SUB); pch++; break;
		case '*':
			addTk(MUL); pch++; break;
		case '/':
			addTk(DIV); pch++; break;
		case '=':
			if (pch[1] == '=') 
				{ addTk(EQUAL); pch += 2; }
			else { addTk(ASSIGN); pch++; }
			break;

		case '!':
			if (pch[1] == '=') { addTk(NOTEQ); pch += 2; }
			else { addTk(NOT); pch++; }
			break;
		case '<':
			addTk(LESS); pch++; break;
		case '>':
			if (pch[1] == '=') { addTk(GREATERQ); pch += 2; }
			else { addTk(GREATER); pch++; }
			break;
		case '&':
			if (pch[1] == '&') { addTk(AND); pch += 2; }
			else err("invalid char: %c (%d)", *pch, *pch);
			break;
		case '|':
			if (pch[1] == '|') { addTk(OR); pch += 2; }
			else err("invalid char: %c (%d)", *pch, *pch);
			break;

		default:
			if (isalpha(*pch) || *pch == '_') {
				for (start = pch++; isalnum(*pch) || *pch == '_'; pch++);
				char* text = copyn(buf, start, pch);

				
				if (strcmp(text, "int") == 0) addTk(TYPE_INT);
				else if (strcmp(text, "real") == 0) addTk(TYPE_REAL);
				else if (strcmp(text, "str") == 0) addTk(TYPE_STR);
				else if (strcmp(text, "var") == 0) addTk(VAR);
				else if (strcmp(text, "function") == 0) addTk(FUNCTION);
				else if (strcmp(text, "if") == 0) addTk(IF);
				else if (strcmp(text, "else") == 0) addTk(ELSE);
				else if (strcmp(text, "while") == 0) addTk(WHILE);
				else if (strcmp(text, "end") == 0) addTk(END);
				else if (strcmp(text, "return") == 0) addTk(RETURN);
				else {
					tk = addTk(ID);
					strcpy(tk->text, text);
				}
			}
			else if (isdigit(*pch)) {
				for (start = pch++; isdigit(*pch) || *pch == '.'; pch++);
				char* num = copyn(buf, start, pch);
				if (strchr(num, '.')) {
					tk = addTk(REAL);
					tk->r = atof(buf);
				}
				else {
					tk = addTk(INT);
					tk->i = atoi(buf);
				}
			}
			else if (*pch == '"') {
				pch++;
				for (start = pch; *pch && *pch != '"'; pch++);
				if (*pch != '"') err("unterminated string");
				char* str = copyn(buf, start, pch);
				tk = addTk(STR);
				strcpy(tk->text, str);
				pch++;
			}
			else err("invalid char: %c (%d)", *pch, *pch);
			break;
		}
	}
}

void showTokens() {
	for (int i = 0; i < nTokens; i++) {
		Token* tk = &tokens[i];
		printf("%d ", tk->line);

		switch (tk->code) {
			// identifiers and strings
		case ID:        printf("ID:%s", tk->text); break;
		case STR:       printf("STR:%s", tk->text); break;

			// constants
		case INT:       printf("INT:%d",  tk->i); break;
		case REAL:      printf("REAL:%g", tk->r); break;

			// keywords
		case TYPE_INT:  printf("TYPE_INT"); break;
		case TYPE_REAL: printf("TYPE_REAL"); break;
		case TYPE_STR:  printf("TYPE_STR"); break;
		case VAR:       printf("VAR"); break;
		case FUNCTION:  printf("FUNCTION"); break;
		case IF:        printf("IF"); break;
		case ELSE:      printf("ELSE"); break;
		case WHILE:     printf("WHILE"); break;
		case END:       printf("END"); break;
		case RETURN:    printf("RETURN"); break;

			// delimiters
		case COMMA:     printf("COMMA"); break;
		case COLON:     printf("COLON"); break;
		case SEMICOLON: printf("SEMICOLON"); break;
		case LPAR:      printf("LPAR"); break;
		case RPAR:      printf("RPAR"); break;
		case FINISH:    printf("FINISH"); break;

			// operators
		case ASSIGN:    printf("ASSIGN"); break;
		case EQUAL:     printf("EQUAL"); break;
		case ADD:       printf("ADD"); break;
		case SUB:       printf("SUB"); break;
		case MUL:       printf("MUL"); break;
		case DIV:       printf("DIV"); break;
		case AND:       printf("AND"); break;
		case OR:        printf("OR"); break;
		case NOT:       printf("NOT"); break;
		case NOTEQ:     printf("NOTEQ"); break;
		case LESS:      printf("LESS"); break;
		case GREATER:   printf("GREATER"); break;
		case GREATERQ:  printf("GREATERQ"); break;
		default:        printf("UNKNOWN(%d)", tk->code); break;
		}
		printf("\n");
	}
}

