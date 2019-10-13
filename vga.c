#include <stdint.h>

extern char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char data);


struct {
    uint16_t width;
    uint16_t height;
    uint16_t *vidmem;

    uint16_t current_loc;
}
VGA = {80, 25, (uint16_t*)0xb8000, 0};



void vga_update_cursor(uint16_t pos)
{
	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) pos);
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) (pos >> 8));
}

void vga_update_cursor_xy(int x, int y)
{
	vga_update_cursor(y * VGA.width + x);
}

void vga_clear()
{
    for (uint16_t ofs = 0; ofs < VGA.width * VGA.height; ofs++) {
        VGA.vidmem[ofs] = (0x07 << 8) | ' ';
    }
    vga_update_cursor(0);
}

void vga_init()
{
    vga_clear();
}

