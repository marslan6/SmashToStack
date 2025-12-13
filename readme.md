# Debugging and Building (XMC4500) — VS Code

## Prerequisites
- Visual Studio Code
- Install the Cortex-Debug extension: https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug
- SEGGER J-Link tools (JLinkGDBServer, JLinkSWOViewerExe) and arm-none-eabi toolchain installed and in PATH as needed.

## Project Structure

Put .c, Makefile, student.mk files under new folder named as `"PROJECT.NAME"`

```
|------- "NAME" 
|   
|------- /.vscode  
|  
|------- /"PROJECT.NAME"  
```

Example for PartC:
```
SmashToStack/
├── .vscode/
│   ├── settings.json
│   ├── c_cpp_properties.json
│   ├── tasks.json
│   └── launch.json
└── PartC/
    ├── main.c
    ├── Makefile
    ├── student.mk
    └── [other source files]
```

## VSCode Configuration

All settings are parametrized for easy reuse across different parts (PartA, PartB, PartC).

### .vscode/settings.json
Set the parameter `"PROJECT.NAME": "PartC"` to your folder name where .c and makefiles are listed.

#### File Associations
The `files.associations` section only lists `.h` header files, not `.c` source files. This is intentional:
- **`.c` files** are automatically recognized by VSCode as C language files - no explicit association needed.
- **`.h` files** must be explicitly marked as `"c"` to ensure IntelliSense treats them as C headers (not C++).
- Only header files that exist in your project should be listed (e.g., `clockSetup.c` and `stack_protector.c` have no corresponding `.h` files, so they don't appear in the associations).

Example:
```json
{
    "PROJECT.NAME": "PartC",
    "files.associations": {
        "xmc_gpio.h": "c",
        "VirtualSerial.h": "c",
        "Descriptors.h": "c"
    }
}
```

### .vscode/c_cpp_properties.json
Configures IntelliSense for ARM-GCC and XMC4500. Update include paths and defines to match your installation:

```json
{
    "configurations": [
        {
            "name": "ARM-GCC",
            "includePath": [
                "${workspaceFolder}/${config:PROJECT.NAME}",
                "/opt/XMClib/XMC_Peripheral_Library_v2.1.16/XMCLib/inc",
                "/opt/XMClib/XMC_Peripheral_Library_v2.1.16/CMSIS/Include",
                "/opt/XMClib/XMC_Peripheral_Library_v2.1.16/CMSIS/Infineon/XMC4500_series/Include",
                "/opt/XMClib/XMC_Peripheral_Library_v2.1.16/ThirdPartyLibraries/**"
            ],
            "defines": [
                "XMC4500_F100x1024"
            ],
            "compilerPath": "/usr/bin/arm-none-eabi-gcc",
            "cStandard": "gnu99",
            "cppStandard": "gnu++17",
            "intelliSenseMode": "linux-gcc-arm"
        }
    ],
    "version": 4
}
```

### .vscode/tasks.json
Defines build, clean, and debug tasks. All settings are parametrized. Copy and paste file directly to use.

Available tasks:
- **make (build)** - Default build task (Ctrl+Shift+B)
- **make clean** - Clean build artifacts
- **make debug** - Start GDB debugging session

Run tasks via:
- `Ctrl+Shift+B` (default build task)
- `Ctrl+Shift+P` → "Tasks: Run Task"
- Ctrl+Shift+P → Task:Run Task

### .vscode/launch.json
Configures the debugger for J-Link and XMC4500. Adjust `runToEntryPoint` to match your project layout:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "J-Link (XMC4500)",
            "type": "cortex-debug",
            "request": "launch",
            "servertype": "jlink",
            "device": "XMC4500",
            "interface": "swd",
            "runToEntryPoint": "main",
            "cwd": "${workspaceFolder}",
            "executable": "${workspaceFolder}/${config:PROJECT.NAME}/build/main.elf",
            "gdbPath": "/usr/bin/gdb-multiarch",
            "serverpath": "JLinkGDBServer"
        }
    ]
}
```

## student.mk Configuration
To ensure correct stepping during debug mode, add the following line to your `student.mk`:

```makefile
SCFLAGS += -Og -fno-omit-frame-pointer -fno-inline
```

Additional settings to configure in `student.mk`:
- Set `LD_NAME` to your project name
- Adjust `SRCS`, `HDRS`, `LIBSRCS` according to project structure

## Build
Run these from your workspace root (adjust path if workspace is the `PartC` folder):

```bash
cd PartC
make clean
make
```

Or use VSCode:
- Press `Ctrl+Shift+B` to build
- `Ctrl+Shift+P` → "Tasks: Run Task" → "make clean"

## Flash
Flash the compiled binary to the XMC4500 board:

```bash
cd PartC
make program    # flash to Cortex-M4 (non-debug)
```

## Debugging

### Start a Debug Session
1. Open `main.c` (or your entry source file) in VS Code.
2. Set breakpoints as needed.
3. Press `F5` to start the Cortex-Debug session.

### Debug Notes
- In the `settings.json` set parameter `${config:PROJECT.NAME}` to your project name.
- Ensure `executable` in `launch.json` points to the built ELF.
- If debugging does not start, verify `gdbPath` and `serverpath` are correct and that J-Link is connected.
- Adjust `runToEntryPoint` if your project uses a different entry function.

## View printf Output via ITM/SWO
The project uses **ITM (Instrumentation Trace Macrocell)** to route `printf` over SWD/SWO instead of UART—no extra wiring needed, just your J-Link debug probe.

### Quick Start:
1. Flash your board: `make program`
2. Open JLinkSWOViewer:
   ```bash
   JLinkSWOViewerExe
   ```
3. In JLinkSWOViewer:
   - Device: XMC4500-1024
   - Target Interface: SWD
   - CPU Clock: CLICK MEASURE
   - SWO Clock: CLICK MEASURE
   - Click **OK**, then **Start** to see printf output in the terminal window.

4. Just complete steps once more: `Edit > Configure > Measure > OK`

## Additional Notes

### Quick Setup Checklist
- Copy `.vscode`, `Makefile`, and `student.mk` to your project
- Update `PROJECT.NAME` in `.vscode/settings.json`
- Inside `.vscode/settings.json` update section `files.associations`based on your files
- Update include paths in `.vscode/c_cpp_properties.json` if XMClib is installed elsewhere
- Set `LD_NAME` in `student.mk` to your project name
- Adjust `SRCS`, `HDRS`, `LIBSRCS` in `student.mk` according to project structure

### Disable GitHub Copilot (Optional)
To disable Copilot auto-pilot code suggestions:
`File > Preferences > Settings > Github Copilot Enable > (Adjust) Item * = False`