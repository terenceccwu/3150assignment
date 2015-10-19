#include <stdio.h>
#include <unistd.h>
int main()	
{
	char c;
	while((c = getchar()))
	{
		if(c == EOF)
			break;
		if(c == '\n')
		{
			putchar(c);
			fflush(stdout);
		}
		else
		{
			putchar(c);
			putchar('-');
		}
	}

	return 0;
}