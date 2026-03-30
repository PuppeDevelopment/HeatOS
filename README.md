## I made the decicion to archive this project. I don't have much motivation and this os is in a state, where AI cannot improve it more unless everything get rewritten or we get some super AI.
 
 # HeatOS

### Remember that HeatOS is fully made by Artificial Intelligence.

HeatOS is an ultra-minimalist 32-bit x86 protected-mode operating system written entirely from scratch in C and Assembly. It focuses on clean, comprehensible kernel primitives without any bloat or unnecessary dependencies.

## Architecture & Features

The project is structured to implement core OS subsystems functionally and sequentially:

- **Bootloader & Initialization:** Custom 16-bit real-mode to 32-bit protected-mode boot sequence.
- **Hardware Interrupts:** Custom IDT, GDT, and ISR architectures to handle CPU exceptions gracefully and control hardware interactions (VGA, Keyboard).
- **Memory Management:** 
  - Physical Memory Management (PMM) using bitmap block tracking mapping available RAM.
  - Virtual Memory / Paging (VMM - planned).
  - Basic kernel heap allocator (`kmalloc` / `kfree`).
- **Process Scheduler:** Foundational Ring-0 preemptive round-robin process scheduler setup.
- **Storage:** ATA Block driver stubs and FAT32 file system parser framework.
- **Networking Core:** Bare-metal clean-room TCP/IP Stack (Ethernet, ARP, IPv4, ICMP, UDP, TCP).
- **Security & Web:** Scaffolding for abstract Web Application interfaces (DNS, HTTP Client, mbedTLS dummy wrappers).

## Building

HeatOS requires `nasm` (for assembly) and `LLVM/clang` for compiling the C components. It relies strictly on LLVM's `ld.lld` and `llvm-objcopy` for linking and binary conversion.

To build the kernel and generate the OS image on Windows:
```bat
build.cmd
```
This script will output `Heatos.img` (a bootable floppy image) and optionally `Heatos.iso` in the `build/` directory without requiring Linux cross-compilers.

## Running

You can easily run the compiled OS using QEMU:
```bat
run.cmd
```

## Contributing
To add features or improve subsystems, explore the `src/kernel` and `src/drivers` directories where everything is strictly compartmentalized. 
