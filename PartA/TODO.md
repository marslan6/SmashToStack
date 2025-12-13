# Part A: Stack Buffer Overflow Exploit - TODO List

## Phase 1: Understanding the Vulnerability
- [ ] Read `main.c` and understand the program flow
- [ ] Identify the vulnerable function (`encrypt` in crypto.c)
- [ ] Understand why `memcpy(buf, plaintext, plaintext_len)` is vulnerable
- [ ] Note the buffer size (128 bytes)
- [ ] Understand the XOR encryption that happens after memcpy

## Phase 2: Analyzing the Binary
- [ ] Build the project: `make`
- [ ] Disassemble the `encrypt` function: `arm-none-eabi-objdump -d build/main.elf | grep -A 30 "<encrypt>"`
- [ ] Note the function prologue:
  - [ ] `push {r4, r5, r6, lr}` - saves 16 bytes
  - [ ] `sub sp, #128` - allocates 128 bytes for buffer
- [ ] Calculate padding needed: 128 (buffer) + 12 (r4, r5, r6) = 140 bytes
- [ ] Identify the return address location on stack

## Phase 3: Finding Critical Addresses
- [ ] Find the call to `encrypt` in main:
  - [ ] `arm-none-eabi-objdump -d build/main.elf | grep -B5 -A5 "bl.*encrypt"`
  - [ ] Note the address of the instruction AFTER the call (resume address)
  - [ ] Add 1 for Thumb mode (e.g., 0x08000324 → 0x08000325)
- [ ] Calculate stack address using GDB or static analysis:
  - [ ] Initial SP = 0x10000800 (from linker script)
  - [ ] SP after main's prologue
  - [ ] SP after encrypt's prologue = buffer start address
  - [ ] Add 1 for Thumb bit
- [ ] Note the XOR key from main.c: `key[8] = {0x42, 0x42, ...}`

## Phase 4: Writing the Shellcode
- [ ] Determine target GPIO registers:
  - [ ] PORT1 Base: 0x48028100
  - [ ] IOCR0 (Pin config): offset 0x10
  - [ ] OMR (Output): offset 0x04
- [ ] Write assembly code (`shellcode.s`):
  - [ ] Load PORT1 base into register
  - [ ] Configure P1.0 and P1.1 as push-pull outputs (write 0x8080 to IOCR0)
  - [ ] Turn on LEDs (write 0x03 to OMR)
  - [ ] Jump back to resume address
- [ ] Assemble and extract binary:
  ```bash
  arm-none-eabi-as -o shellcode.o -mthumb -mcpu=cortex-m4 shellcode.s
  arm-none-eabi-objcopy -O binary shellcode.o shellcode.bin
  ```
- [ ] Verify shellcode disassembles correctly

## Phase 5: Building the Exploit
- [ ] Create exploit structure:
  - [ ] Shellcode (34 bytes)
  - [ ] Padding to reach offset 140
  - [ ] Return address (4 bytes, little-endian)
- [ ] **IMPORTANT**: Pre-XOR first 128 bytes with 0x42
  - [ ] This counteracts the XOR in encrypt() function
  - [ ] Bytes 128-143 are NOT XORed (saved registers + return address)
- [ ] Write `create_exploit.py` or use `build_exploit.sh`
- [ ] Generate `exploit` file (144 bytes total)

## Phase 6: Testing with GDB
- [ ] Start GDB server: `JLinkGDBServer -device XMC4500 -endian little -if SWD -speed 1000 &`
- [ ] Connect with GDB and set breakpoint at encrypt
- [ ] Send exploit: `python3 sendExploit.py exploit --device /dev/ttyACM0`
- [ ] Verify:
  - [ ] SP value matches calculation
  - [ ] Stack contents show shellcode (after XOR)
  - [ ] Return address is overwritten correctly
  - [ ] PC jumps to shellcode address after return

## Phase 7: Final Verification
- [ ] Reset board: `echo -e "h\nloadfile build/main.hex\nr\ng\nq" | JLinkExe -device XMC4500 -if SWD -speed 1000 -autoconnect 1`
- [ ] Send exploit: `python3 sendExploit.py exploit --device /dev/ttyACM0`
- [ ] Verify LEDs at P1.0 and P1.1 turn ON
- [ ] Verify board responds (doesn't crash)
- [ ] Test multiple times to ensure reliability

## Phase 8: Documentation
- [ ] Document all addresses found
- [ ] Document stack layout
- [ ] Explain the XOR pre-processing requirement
- [ ] Create README.md with complete explanation
- [ ] Add comments to exploit code

## Deliverables
- [ ] `exploit` - Binary exploit file (144 bytes)
- [ ] `create_exploit.py` - Python script to generate exploit
- [ ] `shellcode.s` - Assembly source for shellcode
- [ ] `build_exploit.sh` - Shell script to build from assembly
- [ ] `README.md` - Complete documentation
- [ ] `TODO.md` - This checklist
