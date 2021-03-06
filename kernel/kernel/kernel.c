#include <stdio.h>
#include <kernel/tty.h>
#include <kernel/io.h>
#include <kernel/keyboard_input.h>

#include <sys/wait.h>
#include <string.h>

void reverse(char s[])
{
	int i, j;
	char c;

	 for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
}

void itoa(int n, char s[])
{
	int i, sign;

	 if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
}
 
void kernel_main(void) 
{
    NMI_disable(); //disable Non Maskable Interrupt to go to Protected mode

	terminal_initialize();

    printf("Starting ChirpOS...\n\n");

    wait(1000);

    printf(">");

    keyboard_input();
}