#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
 
#include <kernel/tty.h>
#include <kernel/io.h>
 
#include "vga.h"

#define PIC1 0x20
#define PIC2 0xA0

#define ICW1 0x11
#define ICW4 0x01
 
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;
 
static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

size_t current_pos = 2;

volatile unsigned char ScanCode;
 
void terminal_initialize(void) 
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = VGA_MEMORY;
	for (size_t y = 0; y < VGA_HEIGHT; y++) 
    {
		for (size_t x = 0; x < VGA_WIDTH; x++) 
        {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void terminal_set_cursor(int x, int y)
{
	uint16_t pos = y * VGA_WIDTH + x;
	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

uint16_t get_cursor_position(void)
{
    uint16_t pos = 0;
    outb(0x3D4, 0x0F);
    pos |= inb(0x3D5);
    outb(0x3D4, 0x0E);
    pos |= ((uint16_t)inb(0x3D5)) << 8;
    return pos;
}

void enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);
 
	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

void disable_cursor()
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
}

void NMI_enable()
{
	outb(0x70, inb(0x70) & 0x7F);
}

void NMI_disable()
{
	outb(0x70, inb(0x70) | 0x80);
}

void init_pics(int pic1, int pic2)
{
	/* send ICW1 */
	outb(PIC1, ICW1);
	outb(PIC2, ICW1);

	/* send ICW2 */
	outb(PIC1 + 1, pic1);
	outb(PIC2 + 1, pic2);

	/* send ICW3 */
    outb(PIC1 + 1, 4);   
    outb(PIC2 + 1, 2);

   	/* send ICW4 */
    outb(PIC1 + 1, ICW4);
    outb(PIC2 + 1, ICW4);

	/* disable all IRQs */
    outb(PIC1 + 1, 0xFF);
}

void terminal_scroll()
{
    for(size_t i = 0; i < VGA_HEIGHT; i++)
	{
        for (size_t m = 0; m < VGA_WIDTH; m++)
		{
            terminal_buffer[i * VGA_WIDTH + m] = terminal_buffer[(i + 1) * VGA_WIDTH + m];
        }
    }
}
 
void terminal_setcolor(uint8_t color) 
{
	terminal_color = color;
}
 
void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y) 
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}
 
void terminal_putchar(char c) 
{
	switch(c)
	{
		case '\n':
			terminal_row++;
			terminal_column = 0;
			break;
		case '\t':
			terminal_column += 4;
			break;
		default:
			terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
			terminal_column++;
	}

	if(terminal_column >= VGA_WIDTH) 
	{
        terminal_column = 0;
        terminal_row++;
    }

	if(terminal_row >= VGA_HEIGHT) 
	{
        terminal_scroll();

        terminal_row = VGA_HEIGHT - 1;
    }
}

void change_current_pos()
{
	current_pos += 2;
}

void terminal_removechar()
{
	if (terminal_row > current_pos)
	{
		const size_t index = terminal_row * VGA_WIDTH + terminal_column;
		terminal_buffer[index-1] = vga_entry(' ', terminal_color);

		terminal_column -= 1;

		terminal_set_cursor(terminal_column, terminal_row);
	}
	else
	{
		if (terminal_column != 1)
		{
			const size_t index = terminal_row * VGA_WIDTH + terminal_column;
			terminal_buffer[index-1] = vga_entry(' ', terminal_color);

			terminal_column -= 1;

			terminal_set_cursor(terminal_column, terminal_row);
		}
	}

	if (terminal_column == 0 && terminal_row != 0)
	{
		terminal_column = VGA_WIDTH;
		terminal_row -= 1;

		terminal_set_cursor(terminal_column, terminal_row);
	}
}
 
void terminal_write(const char* data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
	{
		terminal_putchar(data[i]);
	}
	terminal_set_cursor(terminal_column, terminal_row);
}
 
void terminal_writestring(const char* data) 
{
	terminal_write(data, strlen(data));
}