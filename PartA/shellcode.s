.syntax unified
.thumb
.text

@ XMC4500 GPIO Shellcode - Turn on LEDs at P1.0 and P1.1
@ PORT1 Base: 0x48028100
@ IOCR0 offset: 0x10 (Pin configuration)
@ OMR offset: 0x04 (Output modification register)

@ Load PORT1 base address into r2
movw r2, #0x8100
movt r2, #0x4802

@ Load IOCR0 configuration value (0x8080 = push-pull output for P1.0 and P1.1)
movw r1, #0x8080
movt r1, #0x0000

@ Write to IOCR0 to configure pins as outputs
str.w r1, [r2, #16]

@ Load value to turn on both LEDs
movs r1, #3

@ Write to OMR to set P1.0 and P1.1 high
str r1, [r2, #4]

@ Resume normal execution at 0x08000325
movw r3, #0x0325
movt r3, #0x0800
bx r3
