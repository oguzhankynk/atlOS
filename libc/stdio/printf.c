#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static void printchar(int c)
{
	putchar(c);
}

static int prints(const char *string)
{
	register int pc = 0;

	for ( ; *string ; ++string) 
	{
		printchar (*string);
		++pc;
	}

	return pc;
}

/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN 12

static int printi(int i, int b, int sg, int letbase)
{
	char print_buf[PRINT_BUF_LEN];
	register char *s;
	register int t, neg = 0, pc = 0;
	register unsigned int u = i;

	if (i == 0) 
	{
		print_buf[0] = '0';
		print_buf[1] = '\0';
		return prints (print_buf);
	}

	if (sg && b == 10 && i < 0) {
		neg = 1;
		u = -i;
	}

	s = print_buf + PRINT_BUF_LEN-1;
	*s = '\0';

	while (u) 
	{
		t = u % b;
		if( t >= 10 )
			t += letbase - '0' - 10;
		*--s = t + '0';
		u /= b;
	}

	if (neg) 
	{
		*--s = '-';
	}

	return pc + prints (s);
}

static int print(int *varg)
{
	register int pc = 0;
	register char *format = (char *)(*varg++);
	char scr[2];

	for (; *format != 0; ++format) 
	{
		if (*format == '%') 
		{
			++format;

			if (*format == '\0') 
				break;

			if (*format == '%') 
				goto out;

			if( *format == 's' ) 
			{
				register char *s = *((char **)varg++);
				pc += prints ( s?s:"(null)");
				continue;
			}

			if( *format == 'd' ) 
			{
				pc += printi (*varg++, 10, 1, 'a');
				continue;
			}		

			if( *format == 'x' ) 
			{
				pc += printi (*varg++, 16, 0, 'a');
				continue;
			}

			if( *format == 'X' )
			{
				pc += printi (*varg++, 16, 0, 'A');
				continue;
			}

			if( *format == 'u' ) 
			{
				pc += printi (*varg++, 10, 0, 'a');
				continue;
			}

			if( *format == 'c' ) 
			{
				/* char are converted to int then pushed on the stack */
				scr[0] = *varg++;
				scr[1] = '\0';
				pc += prints (scr);
				continue;
			}
		}
		else 
		{
		out:
			printchar (*format);
			++pc;
		}
	}
	return pc;
}

/* assuming sizeof(void *) == sizeof(int) */

int printf(const char *format, ...)
{
	register int *varg = (int *)(&format);
	return print(varg);
}
