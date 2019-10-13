/*
* Copyright (C) 2014  Arjun Sreedharan
* License: GPL version 2 or higher http://www.gnu.org/licenses/gpl.html
*/
#include <stdint.h>

extern char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);

extern void serial_isr(void);

void vga_puts(const char *s);
void vga_puts_nl(const char *s);


/* ---  Interrupt Table (IDT) --- */

#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_SEL 0x08

struct IDT_entry {
	unsigned short int offset_lowerbits;
	unsigned short int selector;
	unsigned char zero;
	unsigned char type_attr;
	unsigned short int offset_higherbits;
};

struct IDT_entry IDT[IDT_SIZE];


void idt_init(void)
{
	unsigned long idt_address;
	unsigned long idt_ptr[2];

	/* PIC Ports
	 *          PIC1   PIC2
	 * Command  0x20   0xA0
	 * Data     0x21   0xA1
	 */

	/* ICW1 - begin initialization */
	outb(0x20 , 0x11);
	outb(0xA0 , 0x11);

	/* ICW2 - remap offset address of IDT */
	/*
	* In x86 protected mode, we have to remap the PICs beyond 0x20 because
	* Intel have designated the first 32 interrupts as "reserved" for cpu exceptions
	*/
	outb(0x21 , 0x20);
	outb(0xA1 , 0x28);

	/* ICW3 - setup cascading */
	outb(0x21 , 0x00);
	outb(0xA1 , 0x00);

	/* ICW4 - environment info */
	outb(0x21 , 0x01);
	outb(0xA1 , 0x01);
	/* Initialization finished */

	/* mask interrupts */
	outb(0x21 , 0xff);
	outb(0xA1 , 0xff);

	/* fill the IDT descriptor */
	idt_address = (unsigned long)IDT ;
	idt_ptr[0] = (sizeof (struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
	idt_ptr[1] = idt_address >> 16 ;

	load_idt(idt_ptr);
}

void idt_set(struct IDT_entry *entry, void *handler)
{
    uint32_t offset = (uint32_t)handler;

    entry->offset_lowerbits = offset & 0xffff;
    entry->selector = KERNEL_CODE_SEGMENT_SEL;
    entry->zero = 0;
    entry->type_attr = INTERRUPT_GATE;
    entry->offset_higherbits = (offset & 0xffff0000) >> 16;
}

void idt_set_irq(int irq, void *handler)
{
    idt_set(&(IDT[0x20 + irq]), handler);
}

void pic_enable(int irq)
{
	outb(0x21 , inb(0x21) & ~(1 << irq));
}


/* ---  Serial Port  --- */

#define PORT1 0x3F8 /* Port Address Goes Here */
#define PORT1_IRQ 0x04 /* Com Port's IRQ here (Must also change PIC setting) */

/* Serial Ports Base Addresses */
/* COM1 0x3F8 */
/* COM2 0x2F8 */
/* COM3 0x3E8 */
/* COM4 0x2E8 */

void serial_init(void)
{
    outb(PORT1 + 1 , 0); /* Turn off interrupts - Port1 */

    idt_set_irq(PORT1_IRQ, serial_isr);
    pic_enable(PORT1_IRQ);

#if 0
    /* PORT 1 - Communication Settings */
    /* (for virtual hardware, this seems unnecessary) */
    outb(PORT1 + 3 , 0x80); /* SET DLAB ON */
    outb(PORT1 + 0 , 0x0C); /* Set Baud rate - Divisor Latch Low Byte */
    /* Default 0x03 = 38,400 BPS */
    /* 0x01 = 115,200 BPS */
    /* 0x02 = 57,600 BPS */
    /* 0x06 = 19,200 BPS */
    /* 0x0C = 9,600 BPS */
    /* 0x18 = 4,800 BPS */
    /* 0x30 = 2,400 BPS */
    outb(PORT1 + 1 , 0x00); /* Set Baud rate - Divisor Latch High Byte */
    outb(PORT1 + 3 , 0x03); /* 8 Bits, No Parity, 1 Stop Bit */
    outb(PORT1 + 2 , 0xC7); /* FIFO Control Register */
    outb(PORT1 + 4 , 0x0B); /* Turn on DTR, RTS, and OUT2 */
#endif

    outb(PORT1 + 1 , 0x01); /* Enable interrupt when data received */
}

asm( "serial_isr: \n"
     "call    serial_handler\n"
     "iretl");

void serial_puts(const char *s) {
    while (*s) {
        outb(PORT1, *s++);
    }
}

void serial_handler(void)
{
    /* write EOI */
	outb(0x20, 0x20);

    static char buf[256];
    static unsigned int offset = 0;

    if ((inb(PORT1 + 2)        // IIR - Interrupt Identification Register
            & 0x01) == 0)      //  bit 0 - interrupt pending (active low)
    { 
        char c;
    
        while (inb(PORT1 + 5)  // LSR - Line Status Register
                    & 0x01)    //  bit 0 - data available
        {
            c = inb(PORT1);
            if (c == '\r') c = '\n';
            outb(PORT1, c);  // echo

            /* Store in buffer (unless full), clear buffer on Enter */
            if (c == '\n') {
                if (offset < sizeof(buf)) {
                    buf[offset] = 0;
                    vga_puts("> ");
                    vga_puts_nl(buf);
                }
                for (unsigned int i = 0; i < offset; i++)
                    outb(PORT1, '-');
                outb(PORT1, '\n');
                offset = 0;
            }
            else if (offset < sizeof(buf)) {
                buf[offset++] = c;
            }
        }
    }
}


/* ---------------------- */


void vga_init();


void kmain(void)
{
	const char *msg = "\n\nminimal kernel with serial communication support\n\n";

	idt_init();
    serial_init();
    vga_init();

    serial_puts(msg);
}
