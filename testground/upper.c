#include <stdio.h>
#include <unistd.h>
int main()	
{
	char c;
	while((c = getchar()))
	{
		if(c == EOF)
			break;			
		if(c >= 'a' && c<='z')
			putchar(c - ('a' - 'A'));
		else if(c == '\n')
		{
			putchar(c);
			fflush(stdout);
		}
		else
			putchar(c);
	}

	return 0;
}