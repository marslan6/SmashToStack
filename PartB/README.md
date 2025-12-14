# Part B: Stack Analysis and Overflow Exploit

## Overview

This assignment demonstrates a **stack buffer overflow attack** on an embedded ARM Cortex-M4 device (XMC4500). The goal is to exploit a vulnerable encryption function to overwrite the return address and redirect execution to the `blink` function, which turns on LEDs.

## Objective

Exploit the buffer overflow vulnerability in the `encrypt()` function to:
1. Overwrite the return address on the stack.
2. Redirect execution to the `blink` function, which configures and turns on LEDs at GPIO pins P1.0 and P1.1.
3. Resume normal program execution without crashing.

## Stack Layout Analysis

### Extracting Stack Information

#### 1. Disassemble the `main` function:
```bash
arm-none-eabi-objdump -d build/main.elf | grep -A20 "<main>"
```

**Key instructions:**
```asm
080002b4 <main>:
 80002b4:   e92d 41f0       stmdb   sp!, {r4, r5, r6, r7, r8, lr}    # Saves 6 registers (24 bytes)
 80002b8:   b08a            sub     sp, #40                          # Reserves 40 bytes for local variables
```

#### 2. Disassemble the `encrypt` function:
```bash
arm-none-eabi-objdump -d build/main.elf | grep -A20 "<encrypt>"
```

**Key instructions:**
```asm
0800077c <encrypt>:
 800077c:   b5f0            push    {r4, r5, r6, r7, lr}    # Saves 5 registers (20 bytes)
 800077e:   b0a3            sub     sp, #140                # Reserves 140 bytes for the local buffer
```

### Stack Pointer Calculations

#### Method 1: Static Calculation
- **Initial SP (from linker):** `0x10000800`
- **After `main` prologue:**
  ```
  0x10000800 - 24 - 40 = 0x100007C0
  ```
- **After `encrypt` prologue:**
  ```
  0x100007C0 - 20 - 140 = 0x10000720
  ```
- **Final Stack Address (Thumb bit):**
  ```
  0x10000720 + 1 = 0x10000721
  ```

#### Method 2: GDB Verification
```bash
# Start GDB server
JLinkGDBServer -device XMC4500 -endian little -if SWD -speed 1000 &

# Connect and break at encrypt
arm-none-eabi-gdb build/main.elf
(gdb) target remote localhost:2331
(gdb) break encrypt
(gdb) continue
# Send exploit from another terminal
gdb) print/x $sp
# Should show: 0x100007c0 (before prologue) or 0x10000720 (after)
```

### Viewing the Stack Canary Value
To see the value of the Canary in the stack, use the following command:
```bash
arm-none-eabi-objdump -d build/main.elf | grep -A20 "__stack_chk_guard"
```
Example output:
```asm
800556c:       23d58132                                2..#
```

## Debugging with GDB

### Terminal 1: Start GDB
```bash
cd /home/htmos6/Downloads/SmashToStack/assignment_2/PartB
make debug
```

### Terminal 2: Send the exploit (once GDB is ready)
```bash
cd /home/htmos6/Downloads/SmashToStack/assignment_2/PartB
python3 sendExploit.py exploit --device /dev/ttyACM0
```

## Files Overview

- **`main.c`** - Main program with vulnerable crypto
- **`crypto.c`** - Contains the vulnerable `encrypt()` function
- **`sendExploit.py`** - Sends exploit over USB (provided)
- **`create_exploit.py`** - Generates exploit binary (you create)
- **`shellcode.s`** - Assembly source for shellcode (you create)
- **`build_exploit.sh`** - Builds exploit from assembly (you create)
- **`exploit`** - Final binary payload (144 bytes)

## Expected Results

✅ **Success indicators:**
1. LEDs at P1.0 and P1.1 turn ON.
2. Board responds with encrypted data (doesn't crash).
3. Can send exploit multiple times successfully.

❌ **Failure indicators:**
1. Device timeout.
2. No LED activity.
3. Board becomes unresponsive (needs reset).

## Troubleshooting

### LEDs don't turn on
- Check stack address calculation.
- Verify shellcode is correct (disassemble it).
- Ensure pre-XOR is applied to first 128 bytes.
- Use GDB to check actual SP value.

### Device crashes
- Resume address might be wrong.
- Check Thumb bit (+1) is added to addresses.
- Verify return address is at correct offset (140).

### GDB can't connect
- Check if JLinkGDBServer is running.
- Try: `killall JLinkGDBServer && sleep 1 && JLinkGDBServer ...`
- Verify device is connected: `ls /dev/ttyACM*`.

---

**Assignment completed successfully when LEDs turn on!** 🎯💡