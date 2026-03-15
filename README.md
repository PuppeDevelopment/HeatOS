# HeatOS, OS made only using VSC agent.

### yes even the README is ai :D

A tiny educational operating system project with a **custom kernel** and bootloader, written in x86 assembly.

The project now separates concerns more clearly:
- **Kernel layer**: hardware-level services (boot/runtime state, PCI scan, timing, memory stats).
- **OS layer**: the **Popeye** desktop environment plus terminal apps/commands.

It now boots into a **desktop-style text-mode environment** with built-in apps and an integrated terminal.

Desktop apps:
- `Terminal`
- `System`
- `Files`
- `Notes`
- `Clock`
- `Network`
- `Power`

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
- `desktop` / `gui` / `popeye`
- `banner`
- `beep`
- `halt` / `shutdown`
- `reboot` / `restart`

No Visual Studio 2022 is required.

## What This Project Contains

- `src/boot/boot.asm`: 512-byte boot sector that loads your kernel.
- `src/kernel/kernel.asm`: kernel services + Popeye desktop + integrated terminal.
- `scripts/build.ps1`: assembles image and attempts ISO generation when an ISO tool exists.
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

Optional ISO tool example:

```powershell
winget search xorriso
```

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

If you already built once and only want to run:

```powershell
.\scripts\run.ps1 -SkipBuild
```

Boot as ISO (if `build\Heatos.iso` exists):

```powershell
.\scripts\run.ps1 -BootIso
```

## Using HeatOS

When QEMU starts, you should see a desktop heading like:

```text
Popeye Desktop Environment
```

Try:
- Use `Up` / `Down` to move between desktop apps.
- Press `Enter` to launch the selected app.
- Press `1` through `7` for quick launch.
- Use mouse click on app rows when mouse services are available.
- Press `F1` for desktop help.
- Open `Terminal`, then use `desktop` / `gui` / `popeye` to return to desktop.
- In terminal, try `status`, `net`, `ping 127.0.0.1`, and `arch`.

## Troubleshooting

- `NASM was not found`:
  - Reopen terminal after installation.
  - Verify with `nasm -v`.
- `qemu-system-i386 was not found`:
  - Reopen terminal after QEMU install.
  - Verify with `qemu-system-i386 --version`.
- `Heatos.iso` missing when using `-BootIso`:
  - Install `xorriso` / `mkisofs` / `genisoimage`.
  - Run `build.cmd` again.
- Script execution blocked:
  - Run `Set-ExecutionPolicy -Scope Process Bypass` in that terminal session.
  - Or use `run.cmd` / `build.cmd`, which already launch PowerShell with `-ExecutionPolicy Bypass`.

## Project Notes

- This is a real-mode (16-bit) educational kernel, intentionally minimal.
- Popeye desktop is an early text-mode desktop shell, not a full graphical compositor yet.
- QEMU run path now enables a NIC by default (`-nic user,model=ne2k_pci`) for network diagnostics.
- The build script auto-calculates how many sectors to load for the current kernel size.
- The current size limit is 128 sectors (64 KiB), controlled in `scripts/build.ps1` by `maxKernelSectors`.
- The bootloader now reads across floppy tracks instead of assuming the kernel fits on the first one.

## Next Steps To Evolve HeatOS

- Add a tiny file system or ramdisk so the Files app becomes real.
- Move the desktop from text mode to a proper graphics mode.
- Move from real mode to protected mode.
- Replace floppy image flow with an ISO + GRUB boot path.
- Add interrupt-driven keyboard and timer handling.
