#include <stdio.h>
#include <stdlib.h> // Needed by malloc()
#include <limits.h> // Needed by PATH_MAX
#include <unistd.h> // Needed by getcwd(),execvp
#include <string.h> // Needed by strtok()
#include <errno.h>

char cwd[PATH_MAX+1];

int print_header()
{
	if(getcwd(cwd,PATH_MAX+1) == NULL)
		return -1;

	printf("[3150 shell:%s]$ ",cwd);
	return 0;
}

int check_builtin(char** argList)
{
	if(strcmp(argList[0],"exit") == 0) // == 0 means two strings equal
	{
		if(argList[1] != NULL)
		{
			printf("exit: wrong number of arguments\n");
			free_mem(argList);
			return 1;
		}
		

		free_mem(argList);
		exit(0);
	}

	if(strcmp(argList[0],"cd") == 0)
	{
		if(argList[2] != NULL)
		{
			printf("cd: wrong number of arguments\n");
		}
		else
		{
			if(chdir(argList[1]) != -1)
			{
				getcwd(cwd,PATH_MAX+1);
			}
			else
			{
				printf("%s: Cannot Change Directory\n", argList[1]);
			}
		}		
		
		free_mem(argList);
		return 1;

	}

	return 0; //not a builtin function
}

char** read_input()
{
	//read the whole command
	char cmd[255];
	if(!fgets(cmd, 255, stdin))
	{
		//EOF is read
		printf("\n");
		exit(0);
	}

	if(strlen(cmd) == 1)
	{
		//empty string
		return NULL;
	}
	

	cmd[strlen(cmd)-1] = '\0';

	//tokenize
	char **argList = (char**) malloc(sizeof(char*) * 128); //255/2

	char *token = strtok(cmd," ");
	int i = 0;
	while(token != NULL)
	{
		argList[i] = (char*)malloc(sizeof(char) * strlen(token+1));
		strcpy(argList[i++],token);
		token = strtok(NULL," ");
	}

	argList[i] = NULL;

	return argList;
}

int execution(char** argList)
{
	if(fork())
	{
		//parent
		wait();
		free_mem(argList);
	}
	else
	{
		//child
		setenv("PATH","/bin:/usr/bin:.",1);
		execvp(*argList,argList);
		if(errno == ENOENT){
			printf("%s: command not found\n", argList[0]);
		} else {
			printf("%s: unknown error\n", argList[0]);
		}
		exit(0);
	}
	return 0;
}


int free_mem(char** argList)
{
	int i = 0;
	while(argList[i])
		free(argList[i++]);

	free(argList);
	return 0;
}


int main(int argc, char *argv[])
{
	
	while(1)
	{
		print_header();

		char** argList = read_input();

		if(argList) //if argList not empty
		{
			if(check_builtin(argList) == 0)
				execution(argList);
		}
		
	}

	return 0;
}