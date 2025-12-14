#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ad.h"
#include "utils.h"

Ret ret; //folosim pentru a transmite tipul
Domain *symTable; //lista de domenii
Symbol *crtFn; //functia curenta

Domain *addDomain(){
	puts("creates a new domain");
	Domain *d=(Domain*)safeAlloc(sizeof(Domain));
	d->parent=symTable; //conectam parintele la ultimul domeniu creat
	d->symbols=NULL;
	symTable=d; //noul domeniu creat devine varful stivei 
	return d;
	}

void delSymbols(Symbol *list);

void delSymbol(Symbol *s){
	printf("\tdeletes the symbol %s\n",s->name);
	if(s->kind==KIND_FN){
		delSymbols(s->args);
		}
	free(s);
	}

void delSymbols(Symbol *list){
	for(Symbol *s1=list,*s2;s1;s1=s2){
		s2=s1->next; //salvam urmatorul simbol
		delSymbol(s1); //stergem cel curent
		}
	}

void delDomain(){
	puts("deletes the current domain");
	Domain *parent=symTable->parent; //salvam domeniul parinte
	delSymbols(symTable->symbols); //sterge simbolurile din domeniul curent
	free(symTable); 
	symTable=parent; //ne intoarcem la domeniul parinte
	puts("returns to the parent domain");
	}

Symbol *searchInList(Symbol *list,const char *name){
	for(Symbol *s=list;s;s=s->next){
		if(!strcmp(s->name,name))return s;
		}
	return NULL;
	}

Symbol *searchInCurrentDomain(const char *name){
	return searchInList(symTable->symbols,name);
	}

Symbol *searchSymbol(const char *name){ //cauta in toate domeniile
	for(Domain *d=symTable;d;d=d->parent){
		Symbol *s=searchInList(d->symbols,name);
		if(s)return s;
		}
	return NULL;
	}

Symbol *createSymbol(const char *name,int kind){
	Symbol *s=(Symbol*)safeAlloc(sizeof(Symbol));
	s->name=name;
	s->kind=kind; // functie/variabila/argument
	return s;
	}

Symbol *addSymbol(const char *name,int kind){
	printf("\tadds symbol %s\n",name);
	Symbol *s=createSymbol(name,kind);
	s->next=symTable->symbols; //inseram pe prima pozitie din lista
	symTable->symbols=s; //actualizam head
	return s;
	}

Symbol *addFnArg(Symbol *fn,const char *argName){ //adaugare simboluri in lista de arg a fs
	printf("\tadds symbol %s as argument\n",argName);
	Symbol *s=createSymbol(argName,KIND_ARG);
	s->next=NULL;
	if(fn->args){
		Symbol *p;
		for(p=fn->args;p->next;p=p->next){}
		p->next=s;
		}else{
		fn->args=s;
		}
	return s;
	}

