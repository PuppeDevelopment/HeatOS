# HeatOS, OS made only using VSC agent.

### yes even the README is ai :D

A tiny educational operating system project with a **custom kernel** and bootloader, written in x86 assembly — now with a **fully modular source structure**.

The codebase is no longer a single monolithic file. It is split into 24 focused source files:

| Layer | Files |
|---|---|
| **Boot** | `src/boot/boot.asm` |
| **Kernel entry** | `src/kernel/kernel.asm` (thin — only constants + `start:` + includes) |
| **Drivers** | `src/drivers/mouse.asm`, `pci_net.asm` |
| **Library** | `src/lib/video.asm`, `string.asm`, `input.asm`, `print.asm`, `math.asm`, `time.asm` |
| **Desktop (Popeye Plasma)** | `src/desktop/desktop.asm`, `kickoff.asm`, `run_dialog.asm`, `renderer.asm` |
| **Apps** | `src/apps/system.asm`, `files.asm`, `notes.asm`, `clock.asm`, `network.asm`, `power.asm` |
| **Terminal** | `src/terminal/terminal.asm`, `commands.asm` |
| **Data** | `src/data/strings.asm`, `variables.asm` |

The project now separates concerns clearly:
- **Kernel layer**: hardware-level services (boot, PCI, timing, memory, mouse, video primitives).
- **Desktop layer** (`src/desktop/`): Popeye Plasma-style desktop environment — completely separate from the kernel.
- **App layer** (`src/apps/`): each built-in app is its own file.
- **Terminal layer** (`src/terminal/`): `Heat>` shell and command set, separate from everything else.

It boots **directly to the desktop** — KDE Plasma-like text-mode shell with built-in apps and an integrated terminal.

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
- `desktop` / `gui` / `popeye` / `plasma`
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
- `src/desktop/desktop.asm`: Popeye Plasma event loop and app dispatcher.
- `src/desktop/kickoff.asm`: kickoff overlay menu (`M` key).
- `src/desktop/run_dialog.asm`: run-by-name dialog (`R` key).
- `src/desktop/renderer.asm`: desktop home screen, launcher rail, app frame rendering.
- `src/apps/system.asm`: System Center app.
- `src/apps/files.asm`: Files app (virtual layout / ramdisk placeholder).
- `src/apps/notes.asm`: Notes / roadmap app.
- `src/apps/clock.asm`: RTC clock app.
- `src/apps/network.asm`: Network diagnostics app.
- `src/apps/power.asm`: Power / halt / reboot app.
- `src/terminal/terminal.asm`: `Heat>` shell session and command dispatch table.
- `src/terminal/commands.asm`: all built-in command implementations.
- `src/data/strings.asm`: zero-terminated string literals (last in binary).
- `src/data/variables.asm`: mutable state and scratch buffers (last in binary).
- `scripts/build.ps1`: assembles image, checks for C compiler, attempts ISO generation.
- `scripts/run.ps1`: builds (unless `-SkipBuild`) and runs in QEMU (floppy or ISO).
- `scripts/split_kernel.ps1`: utility — re-extracts module files from the backup monolith.
- `build.cmd` / `run.cmd`: Windows wrappers that bypass PowerShell execution-policy issues.

## C Language Support

The project is currently **assembly-only** because 16-bit real-mode C requires a special
cross-compiler (`ia16-elf-gcc` or OpenWatcom `wcc`). The build script detects these
automatically if installed — otherwise it proceeds with NASM-only assembly.

To add C support later:

```powershell
# Option A: ia16-elf-gcc (build from source — Linux/WSL recommended)
# https://github.com/tkchia/gcc-ia16

# Option B: OpenWatcom (Windows-native 16-bit C compiler)
# https://github.com/open-watcom/open-watcom-v2/releases
```

Once installed, drop `.c` files anywhere under `src/` — `build.ps1` will pick them up.

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
Popeye Plasma Desktop
```

Try:
- Use `Up` / `Down` to move between desktop apps.
- Press `Enter` to launch the selected app.
- Press `1` through `7` for quick launch.
- Press `M` for the kickoff-style app menu.
- Press `R` for the run dialog (`terminal`, `system`, `files`, etc.).
- Press `F2` for an instant terminal.
- Use mouse click on app rows when mouse services are available.
- Press `F1` for desktop help.
- Open `Terminal`, then use `desktop` / `gui` / `popeye` / `plasma` to return to desktop.
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
