#include <stdio.h>
#include <stdlib.h>
#include <glob.h>
#include <string.h> // Needed by strtok()
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

void signal_handle(int sig)
{
	printf("  handled\n");

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


int main(int argc, char *argv[]){
	signal(SIGCHLD, signal_handle);

	int num = 10;

	//create an array of pipes
	int pipefd[num-1][2]; //0 to (num-2) pipes
	int k;
	for(k=0;k<num-1;k++)
		pipe(pipefd[k]);

	//save a copy of stdin & stdout
	int stdin_copy = dup(0);
	int stdout_copy = dup(1);

	// char *leftSeg[2] = {"fa", NULL};
	// char *midSeg[2] = {"fa", NULL};
	// char *rightSeg[2] = {"ls", NULL};
	
	char*** L_argList = malloc(sizeof(char**) * 10);

	int y;
	for(y=0;y<10;y++)
	{
		L_argList[y] = malloc(sizeof(char*) * 3);
		L_argList[y][0] = "cat";
		L_argList[y][1] = NULL;
	}

	L_argList[0][0] = "ls";
	L_argList[0][1] = "/";
	L_argList[0][2] = NULL;
	L_argList[5][0] = "./upper";

	L_argList[7][0] = "asdf";	
	

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
