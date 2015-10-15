#include <stdio.h>
#include <stdlib.h>
#include <glob.h>
#include <string.h> // Needed by strtok()
#include <errno.h>

int print(glob_t* myresult)
{
	int i;
	for(i=0;i<myresult->gl_pathc;i++)
		printf("%s\n", myresult->gl_pathv[i]);
	return 0;
}

int main()
{
	const char* pa1 = "./*";
	const char* pa2 = "asdf";

	glob_t* result = malloc(sizeof(glob_t));
	glob(pa1, GLOB_NOCHECK, NULL, result); // first call -> don;t use GLOB_APPEND
	glob(pa2, GLOB_NOCHECK | GLOB_APPEND, NULL, result);
	
	print(result);
	
	globfree(result);
	return 0;
}