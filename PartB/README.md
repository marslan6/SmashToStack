# Part B: Stack Analysis and Overflow Exploit

## Overview
This assignment demonstrates a **stack buffer overflow attack** on an embedded ARM Cortex-M4 device (XMC4500). The goal is to exploit a vulnerable encryption function to overwrite the return address and redirect execution to the `blink` function, which turns on LEDs.

## Stack Layout & Register Storage
The vulnerable `encrypt()` function allocates a local buffer and saves registers, creating a specific stack layout that we exploit.

### Stack Map
Because we overflow the buffer, our payload must match this exact structure to successfully restore registers and hijack control flow.

| Offset (Bytes) | Size | Content | Description |
| :--- | :--- | :--- | :--- |
| **0 - 127** | 128 | **Shellcode + Padding** | buffer allocated by `encrypt`. Filled with Shellcode + `0x8e` (encrypted `0xcc`). |
| **128 - 131** | 4 | **Canary** | `0x23d58132` (Must match to bypass check) |
| **132 - 135** | 4 | **Padding** | Alignment/Local variables (Filled with `0x8e`) |
| **136 - 139** | 4 | **R4** | Saved Register R4 (`0x08000290`) |
| **140 - 143** | 4 | **R5** | Saved Register R5 (`0x100007c8`) |
| **144 - 147** | 4 | **R6** | Saved Register R6 (`0x100007dc`) |
| **148 - 151** | 4 | **R7** | Saved Register R7 (`0x00000000`) |
| **152 - 155** | 4 | **LR** | **Return Address** (`0x0800034b`) - Redirects to `main` resume point. |

### Technical Details
- **Buffer:** The `encrypt` function XORs the first 128 bytes. Our exploit script pre-XORs the payload so that `0xcc` ends up in memory.
- **Canary:** A security check compares the value at `SP+128`. We overwrite it with the known correct value.
- **Registers:** The function ending `pop {r4-r7, pc}` restores these values. We inject specific addresses to maintain program stability (e.g. keeping R4-R6 pointing to valid stack/data areas).
- **Return Address (LR):** Overwritten to jump to the `blink` function logic or resume execution cleanly.

## Analysis Commands
These commands were used to determine the stack layout and addresses. Use them to verify your target:

### 1. Inspect `encrypt` function (Stack Frame & Logic)
```bash
arm-none-eabi-objdump -d build/main.elf | grep -A 30 "<encrypt>"
```
*Look for `sub sp, #140` (buffer allocation) and `pop` instructions.*

### 2. Inspect `main` function (Stack Depth & Calls)
```bash
arm-none-eabi-objdump -d build/main.elf | grep -A 30 "<main>"
```

### 3. Find Global Addresses (e.g., Canary)
```bash
arm-none-eabi-objdump -d build/main.elf | grep -A 20 "__stack_chk_guard"
```

### 4. Find Gadget/Function Addresses
To find the address of `blink` or resume points:
```bash
arm-none-eabi-objdump -d build/main.elf | grep "blink"
```

## Instructions

### 1. Build the Exploit
Generate the `exploit` binary file using the python script. This pre-calculates the offsets and XOR encryption.
```bash
python3 create_exploit.py
```

### 2. Verify with GDB (Optional but Recommended)
Use the provided GDB commands to verify the stack memory before the function returns.
```bash
# Terminal 1
make debug
# Type 'c' to continue and wait for exploit
```

### 3. Send the Exploit
Send the binary payload to the victim device over USB.
```bash
# Terminal 2 (or new pane)
ls /dev/ttyACM*                             # Find your device (e.g., /dev/ttyACM0)
python3 sendExploit.py exploit --device /dev/ttyACM0
```

## Expected Results
- **Console:** The `sendExploit.py` script should report "Exploit sent".
- **GDB:** Should show "PASS: Canary matches" and verify `0xcc` padding.
- **Device:** The LEDs (P1.0 and P1.1) should turn **ON**.

## Troubleshooting
- **No Device Found:** Ensure the board is connected. Try `ls /dev/ttyACM*`.
- **Canary Mismatch:** Check if `create_exploit.py` uses the correct canary `0x23d58132`.
- **Wrong Return Address:** Ensure `val_lr` is set to `0x0800034b` (+1 for Thumb mode).