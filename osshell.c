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

int free_mem(char*** L_argList)
{
	int i = 0;
	while(L_argList[i])
	{
		int j = 0;
		while(L_argList[i][j])
			free(L_argList[i][j++]);
		free(L_argList[i++]);
	}
	free(L_argList);
	return 0;
}

int print_header()
{
	if(getcwd(cwd,PATH_MAX+1) == NULL)
		return -1;

	printf("[3150 shell:%s]$ ",cwd);
	return 0;
}

int check_builtin(char*** L_argList)
{
	char** argList = L_argList[0];

	if(strcmp(argList[0],"exit") == 0) // == 0 means two strings equal
	{
		if(argList[1] != NULL)
		{
			printf("exit: wrong number of arguments\n");
			free_mem(L_argList);
			return 1;
		}
		

		free_mem(L_argList);
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
		
		free_mem(L_argList);
		return 1;

	}

	return 0; //not a builtin function
}

char*** read_input()
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
	char*** L_argList = malloc(sizeof(char**) * (max_arg+1)); // +1 for the final NULL char

	int i = 0;
	L_argList[i] = malloc(sizeof(char*) * (max_arg+1));


	char *token = strtok(cmd," ");
	int arg = 0;
	while(token != NULL)
	{
		if((token[0] == '|'))
		{
			printf("pipe!\n");
			L_argList[i][arg] = NULL;
			arg = 0;
			i++;
			L_argList[i] = malloc(sizeof(char*) * (max_arg+1));
			token = strtok(NULL," ");
			continue;
		}


		//wildcard expansion
		if(strstr(token,"*")) //if token contains *
		{
			glob_t result;
			glob(token, GLOB_NOCHECK, NULL, &result);
			int j;
			for(j = 0; j < result.gl_pathc; j++)
			{
				char* next = result.gl_pathv[j];
				L_argList[i][arg] = (char*)malloc(sizeof(char) * (strlen(next) + 1));
				strcpy(L_argList[i][arg++], next);
			}
			globfree(&result);

		}
		else //normal copying
		{
			L_argList[i][arg] = (char*)malloc(sizeof(char) * (strlen(token) + 1));
			strcpy(L_argList[i][arg++],token);
		}

		token = strtok(NULL," ");
	}


	L_argList[i++][arg] = NULL;
	L_argList[i] = NULL;
	return L_argList;
}

int print_L_argList(char*** L_argList)
{
	printf("print L_argList\n");
	int i = 0;
	while(L_argList[i])
	{
		printf("program %d\n", i);
		int j = 0;
		while(L_argList[i][j])
			printf("  arg[%d] = %s\n", j, L_argList[i][j++]);
		i++;
	}
	return 0;
}

int execution(char*** L_argList, int p_num)
{
	char** argList = L_argList[p_num];

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
	free_mem(L_argList);
	exit(0);
}

int single_cmd(char*** L_argList)
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

		free_mem(L_argList);
	}
	else
	{
		//child
		execution(L_argList,0);
	}
	return 0;
}

int manage_pipes(int fd[][2], int p, int num)
{
	if(p==0) //first process
	{
		dup2(fd[0][1], STDOUT_FILENO);
		close(fd[0][0]);
	}
	else if(p==num-1) //last process
	{
		dup2(fd[p-1][0], STDIN_FILENO);
		close(fd[p-1][1]);
	}
	else if(p>0 && p < num-1) //middle process
	{
		dup2(fd[p-1][0], STDIN_FILENO);
		dup2(fd[p][1], STDOUT_FILENO);

		close(fd[p-1][1]);
		close(fd[p][0]);
	}

	int i;
	for(i=0;i<num-1;i++) //if p == num, it means close all pipes fd
	{
		if(i==p-1 || i==p)
			continue;
		else
		{
			close(fd[i][0]);
			close(fd[i][1]);
		}
			
	}

	return 0;
}

int pipe_cmd(char*** L_argList)
{
	int num = 0;
	while(L_argList[num]) //calculate no. of programs
	{
		num++;
	}
	printf("++%d\n", num);

	//create an array of pipes
	int pipefd[num-1][2]; //0 to (num-2) pipes
	int k;
	for(k=0;k<num-1;k++)
		pipe(pipefd[k]);

	//save a copy of stdin & stdout
	int stdin_copy = dup(0);
	int stdout_copy = dup(1);

	int i;
	int result[num];
	for(i=0;i<num;i++)
	{		
		if(!(result[i] = fork()))
		{
			//child
			printf("  child: %d is created\n", getpid());
			manage_pipes(pipefd,i,num); //all children will be handled

			setenv("PATH","/bin:/usr/bin:.",1);
			execvp(*L_argList[i], L_argList[i]);
			//if error
			dup2(stdout_copy,1); //reset stdout to screen

			if(errno == ENOENT){
				printf("%s: command not found\n", L_argList[i][0]);
			} else {
				printf("%s: unknown error\n", L_argList[i][0]);
			}
			exit(0);

			break; //child does not loop
		}
		else
		{
			//parent
			
			if(i == 0) // only run for first time
				printf("  parent: %d\n", getpid());	

			else if(i == num - 1)
			{
				//close all pipes
				manage_pipes(pipefd,num,num);

				int j;
				for(j=0;j<num;j++)
				{
					waitpid(-1,NULL,0);
					printf("  process %d terminated\n", j);
				}
    		}
		}
	}
	return 0;
}


int main(int argc, char *argv[])
{
	//signal(SIGINT, SIG_IGN); // ctrl-c
	signal(SIGTERM, SIG_IGN); // kill
	signal(SIGQUIT, SIG_IGN); // ctrl- backslash
	signal(SIGTSTP, SIG_IGN); // ctrl-z

	while(1)
	{
		print_header();

		char*** L_argList = read_input();
		print_L_argList(L_argList);

		if(L_argList)
		{
			if(L_argList[1] == NULL) //single_cmd
			{
				if(check_builtin(L_argList) == 0)
					single_cmd(L_argList);
			}
			else //pipe
			{
				printf("pipe!\n");
				pipe_cmd(L_argList);
			}
		}
		
		//free_mem(L_argList);
	}

	return 0;
}
