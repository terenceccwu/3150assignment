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
	printf("handled\n");

}

int main(int argc, char *argv[]){
	signal(SIGCHLD, signal_handle);

	int pipefd1[2];
	int pipefd2[2];
	pipe(pipefd1);
	pipe(pipefd2);

	char *leftSeg[2] = {"ls", NULL};
	char *midSeg[2] = {"cat", NULL};
	char *rightSeg[2] = {"./upper", NULL};
	

	int num = 3;
	int i;
	int result[num];
	for(i=0;i<num;i++)
	{
		printf("-%d\n", i);
		if(!(result[i] = fork()))
		{
			//child
			printf("child: %d is created\n", getpid());
			if(i==0) //first process
			{
				close(pipefd1[0]);
				close(pipefd2[0]);
				close(pipefd2[1]);
				dup2(pipefd1[1], STDOUT_FILENO);
				execvp(*leftSeg, leftSeg);
			}
			else if(i == num - 1) //last process
			{
				close(pipefd2[1]);
				close(pipefd1[0]);
				close(pipefd1[1]);
				dup2(pipefd2[0], STDIN_FILENO);
				execvp(*rightSeg, rightSeg);
			}
			else //middle process
			{
				close(pipefd1[1]);
				close(pipefd2[0]);
				dup2(pipefd1[0], STDIN_FILENO);
				dup2(pipefd2[1], STDOUT_FILENO);
				sleep(2);
				execvp(*midSeg, midSeg);
			}

			break; //child does not loop
		}
		else
		{
			//parent
			
			if(i == 0) // only run for first time
				printf("parent: %d\n", getpid());	

			else if(i == num - 1)
			{
				close(pipefd1[0]);
				close(pipefd1[1]);
				close(pipefd2[0]);
				close(pipefd2[1]);

				int j;
				for(j=0;j<num;j++)
				{
					waitpid(-1,NULL,0);
					printf("process %d terminated\n", j);
				}
    		}
		}
	}

	return 0;
}
