# HeatOS, OS made only using VSC agent.

### yes even the README is ai :D

A tiny educational operating system project with a **custom kernel** and bootloader, written in x86 assembly — now with a **fully modular source structure**.

The codebase is split into focused source files:

| Layer | Files |
|---|---|
| **Boot** | `src/boot/boot.asm` |
| **Kernel entry** | `src/kernel/kernel.asm` (thin — only constants + `start:` + includes) |
| **Drivers** | `src/drivers/mouse.asm`, `pci_net.asm` |
| **Library** | `src/lib/video.asm`, `string.asm`, `input.asm`, `print.asm`, `math.asm`, `time.asm` |
| **Terminal** | `src/terminal/terminal.asm`, `commands.asm` |
| **Data** | `src/data/strings.asm`, `variables.asm` |

The project separates concerns clearly:
- **Kernel layer**: hardware-level services (boot, PCI, timing, memory, mouse, video primitives).
- **Terminal layer** (`src/terminal/`): `Heat>` shell and command set, separate from everything else.

> **Note:** The desktop environment has been removed and will be rewritten as a standalone assembly program.

It boots **directly to the terminal shell**.

Terminal commands:
- `help`
- `clear` / `cls`
- `about`
- `version` / `ver`
- `echo <text>`
- `date`
- `time`
- `uptime`
- `mem`
- `boot`
- `status`
- `history`
- `repeat`
- `net`
- `ping <host>` (loopback implemented)
- `arch`
- `apps`
- `banner`
- `beep`
- `halt` / `shutdown`
- `reboot` / `restart`

No Visual Studio 2022 is required.

## What This Project Contains

- `src/boot/boot.asm`: 512-byte boot sector that loads the kernel.
- `src/kernel/kernel.asm`: **thin entry point** — constants, `start:`, and `%include` chain only.
- `src/drivers/mouse.asm`: BIOS INT 0x33 mouse driver.
- `src/drivers/pci_net.asm`: PCI bus scan for network adapters (class 0x02).
- `src/lib/video.asm`: VGA text-mode helpers (`fill_rect`, `write_string_at`, `put_char_at`).
- `src/lib/string.asm`: zero-terminated string utilities (copy, compare, parse).
- `src/lib/input.asm`: keyboard polling, `read_line`, command history.
- `src/lib/print.asm`: BIOS teletype output (`print_char`, `print_string`, `print_newline`).
- `src/lib/math.asm`: decimal/hex conversion and formatted numeric printing.
- `src/lib/time.asm`: RTC date/time, uptime (BIOS tick counter), string builders.
- `src/terminal/terminal.asm`: `Heat>` shell session and command dispatch table.
- `src/terminal/commands.asm`: all built-in command implementations.
- `src/data/strings.asm`: zero-terminated string literals (last in binary).
- `src/data/variables.asm`: mutable state and scratch buffers (last in binary).
- `scripts/build.ps1`: assembles image, checks for C compiler, attempts ISO generation.
- `scripts/run.ps1`: builds (unless `-SkipBuild`) and runs in QEMU (floppy or ISO).
- `build.cmd` / `run.cmd`: Windows wrappers that bypass PowerShell execution-policy issues.

## Requirements (Windows)

1. Windows 10/11
2. PowerShell
3. NASM assembler
4. QEMU emulator
5. Optional for ISO builds: `xorriso` / `mkisofs` / `genisoimage`

Install dependencies with `winget`:

```powershell
winget install --id NASM.NASM -e --accept-package-agreements --accept-source-agreements
winget install --id SoftwareFreedomConservancy.QEMU -e --accept-package-agreements --accept-source-agreements
```

After install, close and reopen your terminal.

## Build and Run

From the project root (`HeatOS`):

Preferred on Windows if PowerShell scripts give you trouble:

```bat
run.cmd
```

PowerShell still works too:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\run.ps1
```

That command will:
1. Assemble `boot.asm` and `kernel.asm`
2. Create `build\Heatos.img`
3. Try to create `build\Heatos.iso` if an ISO tool is installed
4. Boot the image in QEMU

If you only want to build:

```bat
build.cmd
```

or

```powershell
.\scripts\build.ps1
```

## Using HeatOS

When QEMU starts, you will see the `Heat>` terminal prompt.

Try:
- Type `help` to see all available commands.
- Use `status` to view system information.
- Try `net` for network adapter info.
- Try `ping 127.0.0.1` for loopback ping.
- Use `arch` to see architecture details.
- Use `halt` or `reboot` to control the system.

## Troubleshooting

- `NASM was not found`:
  - Reopen terminal after installation.
  - Verify with `nasm -v`.
- `qemu-system-i386 was not found`:
  - Reopen terminal after QEMU install.
  - Verify with `qemu-system-i386 --version`.
- Script execution blocked:
  - Run `Set-ExecutionPolicy -Scope Process Bypass` in that terminal session.
  - Or use `run.cmd` / `build.cmd`, which already launch PowerShell with `-ExecutionPolicy Bypass`.

## Project Notes

- This is a real-mode (16-bit) educational kernel, intentionally minimal.
- The desktop environment is being rewritten as a standalone assembly program.
- QEMU run path now enables a NIC by default (`-nic user,model=ne2k_pci`) for network diagnostics.
- The build script auto-calculates how many sectors to load for the current kernel size.
- The current size limit is 128 sectors (64 KiB), controlled in `scripts/build.ps1` by `maxKernelSectors`.
- The bootloader now reads across floppy tracks instead of assuming the kernel fits on the first one.

## Next Steps To Evolve HeatOS

- Write the new desktop environment as a standalone assembly program.
- Add a tiny file system or ramdisk.
- Move from real mode to protected mode.
- Replace floppy image flow with an ISO + GRUB boot path.
- Add interrupt-driven keyboard and timer handling.
