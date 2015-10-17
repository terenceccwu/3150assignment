#include <stdio.h>
#include <stdlib.h> // Needed by malloc()
#include <limits.h> // Needed by PATH_MAX
#include <unistd.h> // Needed by getcwd(),execvp
#include <string.h> // Needed by strtok()
#include <errno.h>
#include <sys/wait.h> // Needed by waitpid()
#include <sys/types.h> // Needed by waitpid()
#include <glob.h>
#include <signal.h>

char cwd[PATH_MAX+1];

int free_mem(char** argList)
{
	int i = 0;
	while(argList[i])
		free(argList[i++]);

	free(argList);
	return 0;
}

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
		if(argList[2] != NULL || argList[1] == NULL)
		{
			//only 2 args (<2 or >2)
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
	char cmd[256];
	if(!fgets(cmd, 256, stdin))
	{
		//EOF is read
		printf("\n");
		exit(0);
	}

	if(strlen(cmd) == 1 || cmd[0] == ' ')
	{
		//empty string OR a space is read
		return NULL;
	}
	

	cmd[strlen(cmd)-1] = '\0';

	//tokenize
	int max_arg = 128; //255/2
	char **argList = (char**) malloc(sizeof(char*) * (max_arg + 1)); // +1 for the final NULL char

	char *token = strtok(cmd," ");
	int i = 0;
	while(token != NULL)
	{
		//wildcard expansion
		if(strstr(token,"*")) //if token contains *
		{
			glob_t result;
			glob(token, GLOB_NOCHECK, NULL, &result);
			int j;
			for(j = 0; j < result.gl_pathc; j++)
			{
				char* next = result.gl_pathv[j];
				argList[i] = (char*)malloc(sizeof(char) * (strlen(next) + 1));
				strcpy(argList[i++], next);
			}
			globfree(&result);

		}
		else //normal copying
		{
			argList[i] = (char*)malloc(sizeof(char) * (strlen(token) + 1));
			strcpy(argList[i++],token);
		}

		token = strtok(NULL," ");
	}

	argList[i] = NULL;

	return argList;
}

int execution(char** argList)
{
	signal(SIGINT, SIG_DFL); // ctrl-c
	signal(SIGTERM, SIG_DFL); // kill
	signal(SIGQUIT, SIG_DFL); // ctrl- backlash
	signal(SIGTSTP, SIG_DFL); // ctrl-z

	setenv("PATH","/bin:/usr/bin:.",1);
	execvp(*argList,argList);
	if(errno == ENOENT){
		printf("%s: command not found\n", argList[0]);
	} else {
		printf("%s: unknown error\n", argList[0]);
	}
	free_mem(argList);
	exit(0);
}

int single_cmd(char** argList)
{
	pid_t child_pid;
	if((child_pid = fork()))
	{
		//parent
		int child_status;
		waitpid(child_pid, &child_status,WUNTRACED);

		//detect child signal that causes termination
		if(WIFSIGNALED(child_status) || WIFSTOPPED(child_status))
			printf("\n");

		free_mem(argList);
	}
	else
	{
		//child
		execution(argList);
	}
	return 0;
}




int main(int argc, char *argv[])
{
	signal(SIGINT, SIG_IGN); // ctrl-c
	signal(SIGTERM, SIG_IGN); // kill
	signal(SIGQUIT, SIG_IGN); // ctrl- backslash
	signal(SIGTSTP, SIG_IGN); // ctrl-z

	while(1)
	{
		print_header();

		char** argList = read_input();

		if(argList) //if argList not empty
		{
			if(check_builtin(argList) == 0)
				single_cmd(argList);
		}
		
	}

	return 0;
}
