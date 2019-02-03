// Author: 	 Shounak Dey
// Filename: scoper.c
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<math.h>

/*
	Types of declarations it can evaluate: 
	int a,b;
	int a=10,b=10;
	int func(int a){}

	Assumptions:
	1. The code is properly formatted, i.e, all lines are on different lines and not in a minified format.
	2. All data types are of one words. Ex: double,float,etc. multiword data types like unsigned long are not allowed.
	3. Functions are written in the shitty format, i.e, 
				ret_type f(arglist...)
				{
					//Do something(or not).
				}

	Handles variables of the same name by adding another field called scopeID.
*/

/*Symbol table has the following fields
	1. Token ID
	2. Variable Name
	3. Data Type
	4. Scope
	5. Scope ID
	5. Arguments
	6. Argument count
	7. Return type
	8. Lifetime of the construct
*/

// Define structures and constants

typedef struct line{
	char content[100];
	int lineno;
}line;

typedef struct var{
	int id;
	char name[100];
	char type[100];
	int size;
	int entrypoint;
}var;

typedef struct funcinfo{
	int id;
	char return_type[100];
	char func_name[100];
	var args[100];
	int argc;
	int entrypoint;
	int exitpoint;
}function;

typedef struct token{
	//Visible fields
	int id;
	char name[100];
	char type[100];
	int size;
	char scope;
	int scopeID;	
	struct token *args[10];
	int argc;
	char ret_type[100];
	int lifetime;

	//Hidden fields
	int entrypoint;
	int isFunction;
	int exitpoint;
}token;


//Define general methods
int isWhitespace(char s){
	return (s==32)||(s==9);					//Checking if s is SPACE or \t	
}

int isDataType(char *s){					// Check if the parameter is a datatype
	char *types[] = {"int","double","float","char","void"};
	int size = 5,i;
	for(i=0;i<size;i++){
		if(strcmp(s,types[i])==0)
			return 1;
	}
	return 0;
}

void findFirstLexeme(char *s,char *lex){	// Finding the first word of the line.
	int pos=0;
	while(s[pos]!='\0' && s[pos]!=' '){
		lex[pos] = s[pos];
		pos++;
	}
	lex[pos]=='\0';
}

void getFirstWord(char *pcontent,char *word){	// Find the first word in a string when leading whitespace characters are possible
	int state=0,l=strlen(pcontent),i=0,pos=0;
	while(i<l){
		if(state && isWhitespace(pcontent[i]))
			break;
		if(!state && !isWhitespace(pcontent[i])){
			state = 1;
			word[pos] = pcontent[i];
			pos++;
		}
		else{
			if(!isWhitespace(pcontent[i]))
				word[pos] = pcontent[i];
			else
				break;
			pos++;
		}
		i++;
	}
	word[pos]='\0';
	return;
}


// Define methods corresponding to function data type

void getFuncName(char *pcontent,char *name){		// get function name
	int l = strlen(pcontent),pos=0,i=0;
	while(!isWhitespace(pcontent[i]))
		i++;
	i++;
	while(pcontent[i]!='('){
		name[pos] = pcontent[i];
		pos++;
		i++;
	}
	name[pos]='\0';
	return;
}

int isFunc(line tmp){								// Check if the parameter line is a function
	int start=0,brpos=0,end=0,l=strlen(tmp.content);
	// printf("%s %d\n",tmp.content,l);
	while(brpos<l && tmp.content[brpos]!='(')
		brpos++;
	start = brpos;
	while(brpos<l && tmp.content[brpos]!=')')
		brpos++;
	end = brpos;
	if(end==l || start==l)
		return 0;
	start++;end--;
	if(start > end)
		return 1;
	int pos=0;
	char pcontent[100];
	while(start<=end){
		pcontent[pos] = tmp.content[start];
		pos++;
		start++; 
	}
	pcontent[pos]='\0';
	char word[100];
	getFirstWord(pcontent,word);
	return isDataType(word);
}

void parse(line inp,function *f){					// parse parse parse
	char tmp[100];
	getFirstWord(inp.content,tmp);
	strcpy(f->return_type,tmp);
	f->entrypoint = inp.lineno;
	getFuncName(inp.content,tmp);
	strcpy(f->func_name,tmp);
	return;
}

int getExitpoint(line *inp,int start){				// find line of death
	int balance=1;
	start++;
	while(balance>0){
		if(inp[start].content[0]=='{')
			balance++;
		else if(inp[start].content[0]=='}')
			balance--;
		start++;
	}
	return start-1;
}

void getFargs(char *decl,char *arglist){			// generate argument string from func declaration
	int start=0;
	while(decl[start]!='(')
		start++;
	start++;
	int end=start;
	while(decl[end]!=')')
		end++;
	end--;
	if(start>end){
		arglist[0]='\0';
		return;
	}
	int pos=0;
	while(start<=end){
		arglist[pos] = decl[start];
		pos++;
		start++;
	}
	arglist[pos] = '\0';
	return;
}

void getArg(char *decl,int typelen,char *arglist){		// Get arg name when there's only one variable
	int start=typelen+1,l=strlen(decl),pos=0;
	while(start<l){
		arglist[pos] = decl[start];
		start++;
		pos++;
	}
	arglist[pos] = '\0';
	return;
}

// Define functions for the var data type
void generateArgs(char *decl,int typelen,char *arglist){	// the 3rd function that generates arguments. Consider modularization
	int start=typelen+1,l=strlen(decl),pos=0;	
	while(start<l){
		arglist[pos] = decl[start];
		start++;
		pos++;
	}
	arglist[pos-1] = '\0';
	return;
}

void create(var* tmp,char *name,char *type,int id,int lineno){		// create a variable
	strcpy(tmp->name,name);
	strcpy(tmp->type,type);
	tmp->id = id;
	tmp->entrypoint = lineno;
	return;
}

void strip(char *tmp){										
	int pos=0;
	char ret[100];
	while(tmp[pos]!='\0' && tmp[pos]!='=' && tmp[pos]!=' '){
		ret[pos] = tmp[pos];
		pos++;
	}
	ret[pos] = '\0';
	strcpy(tmp,ret);
	return;
}

int getSize(char *s){										
	if(strcmp(s,"char")==0) return 1;
	if(strcmp(s,"int")==0 || strcmp(s,"float")==0) return 2;
	return 4;
}


// Define functions for the token data type
void printToken(token* tmp){
	// printf("Name-12Type-12Size-12Scope-12Return Type-12Number of Arguments-12Arguments-12\n");
	if(!tmp->isFunction)
		printf("%-12d%-12s%-12s%-12d%-12c%-12d%-12s%-12s%-12d%-12s\n",tmp->id,tmp->name,tmp->type,tmp->size,tmp->scope,tmp->scopeID,"NA","NA",tmp->lifetime,"NA");
	else{
		printf("%-12d%-12s%-12s%-12s%-12c%-12d%-12s%-12d%-12d",tmp->id,tmp->name,tmp->type,"NA",tmp->scope,tmp->scopeID,tmp->ret_type,tmp->argc,tmp->lifetime);
		int i;
		if(tmp->argc == 0){
			printf("%-12s\n","None");
			return;
		}
		for(i=0;i<(tmp->argc - 1);i++)
			printf("%s,",tmp->args[i]->name);
		printf("%s\n",tmp->args[tmp->argc - 1]->name);
	}
	return;
}

int cmp(const void *A,const void *B){
	token *a = (token *)A;
	token *b = (token *)B;
	if(a->entrypoint == b->entrypoint)
		return a->isFunction < b->isFunction;
	return a->entrypoint > b->entrypoint;
}

// Main function

int main(){
	//Definitions
	line inp[100];
	char c;
	int lines=0;
	int i,j;
	line loi[100];
	function flist[100];
	int loicount=0;
	int fcount=0;
	int linescope[100];
	memset(linescope,-1,sizeof(linescope));
	var vlist[100];
	int vcount=0;

	token tlist[100];
	int tcount=0;

	// Take input
	while(scanf("%[^\n]",inp[lines].content)!=EOF){
		// printf("%s\n",inp[lines]);
		inp[lines].lineno = lines;
		lines++;
		c = getc(stdin);
	}

	// Remove indentations blocks - Tested only with tabspaces
	for(i=0;i<lines;i++){
		int state=0,pos=0;				//State variable is 1 if it has passed indentation stage.
		char tmp[100];
		for(j=0;j<strlen(inp[i].content);j++){
			if(!isWhitespace(inp[i].content[j]) || state){
				state = 1;
				tmp[pos++] = inp[i].content[j];
			}
		}
		tmp[pos] = '\0';
		strcpy(inp[i].content,tmp);
	}

	// Find lines of interest. 
	i=0;
	while(i<lines){
		char str[100];
		memset(str,0,sizeof(str));
		findFirstLexeme(inp[i].content,str);
		// printf("%s %s\n",str,inp[i].content);
		if(isDataType(str)){
			strcpy(loi[loicount].content,inp[i].content);
			loi[loicount].lineno = inp[i].lineno;
			loicount++;
		}
		i++;
	}

	// Identify functions first.
	printf("Generating Functions...\n");
	i=0;	
	while(i<loicount){
		if(isFunc(loi[i])){
			parse(loi[i],&flist[fcount]);
			flist[fcount].id = fcount;
			flist[fcount].exitpoint = getExitpoint(inp,loi[i].lineno+1);
			flist[fcount].argc = 0;
			printf("ID:%-12dReturn Type:%-12s\tFunction Name:%-12s\tEntry Point:%-12d\tExit Point:%-12d\n",flist[fcount].id,flist[fcount].return_type,flist[fcount].func_name,flist[fcount].entrypoint,flist[fcount].exitpoint);
			for(j=flist[fcount].entrypoint;j<=flist[fcount].exitpoint;j++) linescope[j]=flist[fcount].id;
			fcount++;
		}
		i++;
	}
	printf("\n");

	//Identify variables now.
	printf("Identifying Variables...\n");
	i=0;
	while(i<loicount){
		char arglist[100];
		if(isFunc(loi[i])){
			getFargs(loi[i].content,arglist);
			char *v,type[100],name[100];
			v = strtok(arglist,",");
			while(v != NULL){
				char type[100];
				getFirstWord(v,type);
				getArg(v,strlen(type),name);
				create(&(vlist[vcount]),name,type,vcount,loi[i].lineno);
				vcount++;
				v = strtok(NULL,",");
			}
		}
		else{
			char type[100];
			getFirstWord(loi[i].content,type);
			//Parsing the variables from the arglist.
			generateArgs(loi[i].content,strlen(type),arglist);
			char *v;
			v = strtok(arglist,",");
			while(v != NULL){
				strip(v);
				create(&(vlist[vcount]),v,type,vcount,loi[i].lineno);
				vcount++;
				v = strtok(NULL,",");
			}
		}
		i++;
	}


	//Build Symbol table
	printf("Building Symbol Table...\n\n");
	//Tokenize the variables first.
	for(i=0;i<vcount;i++){
		strcpy(tlist[tcount].name,vlist[i].name);
		strcpy(tlist[tcount].type,vlist[i].type);
		tlist[tcount].size = getSize(tlist[tcount].type);
		tlist[tcount].scope = linescope[vlist[i].entrypoint]>=0?'L':'G';
		tlist[tcount].entrypoint = vlist[i].entrypoint;
		tlist[tcount].isFunction = 0;
		tlist[tcount].scopeID = -1;
		tlist[tcount].lifetime = -1;
		tcount++;
	}
	// Tokenize the functions now.
	for(i=0;i<fcount;i++){
		strcpy(tlist[tcount].name,flist[i].func_name);
		strcpy(tlist[tcount].ret_type,flist[i].return_type);
		strcpy(tlist[tcount].type,"FUNC");
		tlist[tcount].size = -1;
		if(i==0 || (strcmp(tlist[tcount].name,"main")==0))
			tlist[tcount].scope = 'G';
		else
			tlist[tcount].scope = 'L';
		tlist[tcount].entrypoint = flist[i].entrypoint;
		tlist[tcount].exitpoint = flist[i].exitpoint;
		tlist[tcount].isFunction = 1;
		tlist[tcount].scopeID = linescope[tlist[tcount].entrypoint];
		tlist[tcount].lifetime = tlist[tcount].exitpoint - tlist[tcount].entrypoint;
		tcount++;
	}


	printf("%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s\n\n","Token ID","Name","Type","Size","Scope","ScopeID","Ret Type","Argc","Lifetime","Arguments");
	// Sort all tokens by entry time.
	qsort(tlist,tcount,sizeof(token),cmp);
	for(i=0;i<tcount;i++){
		tlist[i].id = i+1;
		if(!tlist[i].isFunction)
			continue;
		tlist[i].argc = 0;
		for(j=0;j<tcount;j++){
			if(tlist[j].entrypoint==tlist[i].entrypoint && !tlist[j].isFunction){
				tlist[i].args[tlist[i].argc] = &(tlist[j]);
				tlist[i].argc++;
			}
			if(!tlist[j].isFunction && tlist[j].entrypoint>=tlist[i].entrypoint && tlist[j].entrypoint<=tlist[i].exitpoint){
				tlist[j].scopeID = tlist[i].scopeID;
				tlist[j].lifetime = tlist[i].exitpoint - tlist[j].entrypoint;	
			}
		}
	}
	for(i=0;i<tcount;i++)
		printToken(&(tlist[i]));
	return 0;
}