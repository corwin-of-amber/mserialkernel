; Copyright (C) 2014  Arjun Sreedharan
; License: GPL version 2 or higher http://www.gnu.org/licenses/gpl.html

bits 32
section .text
        ;multiboot spec
        align 4
        dd 0x1BADB002              ;magic
        dd 0x00                    ;flags
        dd - (0x1BADB002 + 0x00)   ;checksum. m+f+c should be zero

global start
global keyboard_handler
global serial_handler
global inb
global outb
global load_idt

extern kmain 		;this is defined in the c file
extern keyboard_handler_main
extern serial_handler_main

inb:
	mov edx, [esp + 4]
	            ;al is the lower 8 bits of eax
	in al, dx	;dx is the lower 16 bits of edx
	ret

outb:
	mov   edx, [esp + 4]    
	mov   al, [esp + 4 + 4]  
	out   dx, al  
	ret

load_idt:
	mov edx, [esp + 4]
	lidt [edx]
	sti 				;turn on interrupts
	ret

start:
	cli 				;block interrupts
	mov esp, stack_space
	call kmain
idle:
	hlt 				;halt the CPU
    jmp idle

section .bss
resb 8192; 8KB for stack
stack_space:
