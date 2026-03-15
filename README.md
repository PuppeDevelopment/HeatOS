# RushOS, OS made only using VSC agent.

### yes even the README is ai :D

A tiny educational operating system project with a **custom kernel** and bootloader, written in x86 assembly.

This starter boots into a simple kernel shell with these commands:
- `help`
- `clear`
- `about`
- `halt`
- `reboot`

No Visual Studio 2022 is required.

## What This Project Contains

- `src/boot/boot.asm`: 512-byte boot sector that loads your kernel.
- `src/kernel/kernel.asm`: your custom kernel entry + tiny shell.
- `scripts/build.ps1`: assembles and creates a bootable floppy image.
- `scripts/run.ps1`: builds (unless `-SkipBuild`) and runs in QEMU.

## Requirements (Windows)

1. Windows 10/11
2. PowerShell
3. NASM assembler
4. QEMU emulator

Install dependencies with `winget`:

```powershell
winget install --id NASM.NASM -e --accept-package-agreements --accept-source-agreements
winget install --id SoftwareFreedomConservancy.QEMU -e --accept-package-agreements --accept-source-agreements
```

After install, close and reopen your terminal.

## Build and Run

From the project root (`RushOS`):

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\run.ps1
```

That command will:
1. Assemble `boot.asm` and `kernel.asm`
2. Create `build\rushos.img`
3. Boot the image in QEMU

If you only want to build:

```powershell
.\scripts\build.ps1
```

If you already built once and only want to run:

```powershell
.\scripts\run.ps1 -SkipBuild
```

## Using The Kernel Shell

When QEMU starts, you should see a prompt like:

```text
rush>
```

Try:
- `help` for available commands
- `about` for kernel info
- `clear` to clear the text screen

## Troubleshooting

- `NASM was not found`:
  - Reopen terminal after installation.
  - Verify with `nasm -v`.
- `qemu-system-i386 was not found`:
  - Reopen terminal after QEMU install.
  - Verify with `qemu-system-i386 --version`.
- Script execution blocked:
  - Run `Set-ExecutionPolicy -Scope Process Bypass` in that terminal session.

## Project Notes

- This is a real-mode (16-bit) educational kernel, intentionally minimal.
- The build script auto-calculates how many sectors to load for the current kernel size.
- The current size limit is 16 sectors (8 KiB), controlled in `scripts/build.ps1` by `maxKernelSectors`.

## Next Steps To Evolve RushOS

- Add your own command handlers in `src/kernel/kernel.asm`.
- Add memory map and hardware probing.
- Move from real mode to protected mode.
- Replace floppy image flow with an ISO + GRUB boot path.
