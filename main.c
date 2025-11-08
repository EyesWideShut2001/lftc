#include "utils.h"
#include "lexer.h"
#include "parser.h"

#include <stdio.h>

int main()
{
	char* inbuf = loadFile("test/1.q");
	puts(inbuf);
	tokenize(inbuf);
	showTokens();
	
	 parse();
	return 0;
}