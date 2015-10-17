#include <stdio.h>
#include <unistd.h>
int main()	
{
	sleep(2);
	char c;
	while((c = getchar()))
	{
		if(c == EOF)
			break;			
		if(c >= 'a' && c<='z')
			putchar(c - ('a' - 'A'));
		else
			putchar(c);
	}

	return 0;
}