banner_msg db "================================", 13, 10
           db " HeatOS 0.5 + Popeye Desktop   ", 13, 10
           db "================================", 13, 10, 13, 10, 0
terminal_ready_msg db "Terminal opened from Popeye desktop.", 13, 10, 0
terminal_hint_msg db "Try: help, net, ping 127.0.0.1, desktop", 13, 10, 13, 10, 0
prompt_msg db "Heat> ", 0
unknown_msg db "Unknown command. Type 'help'.", 13, 10, 0
help_msg db "Commands:", 13, 10
         db "  help      clear/cls   about       version/ver", 13, 10
         db "  echo      banner      beep        mem", 13, 10
         db "  date      time        uptime      boot", 13, 10
         db "  status    history     repeat      apps", 13, 10
         db "  net       ping        arch", 13, 10
         db "  desktop   gui/popeye/plasma  halt/shutdown", 13, 10
         db "  reboot/restart", 13, 10
         db "Tip: press Esc while typing to clear the whole line.", 13, 10, 13, 10, 0
about_msg db "HeatOS kernel handles hardware/services; the OS layer", 13, 10
          db "provides Popeye desktop + terminal apps on top.", 13, 10
          db "Still real-mode BIOS based, but now split by role.", 13, 10, 13, 10, 0
version_msg db "HeatOS kernel v0.5", 13, 10
            db "Features: Popeye desktop, mouse hooks, net diagnostics.", 13, 10, 13, 10, 0
echo_usage_msg db "Usage: echo <text>", 13, 10, 0
date_prefix_msg db "Date: ", 0
time_prefix_msg db "Time: ", 0
uptime_prefix_msg db "Approx uptime: ", 0
mem_prefix_msg db "Conventional memory: ", 0
boot_prefix_msg db "Boot info: ", 0
status_header_msg db "System status:", 13, 10, 0
status_version_label db "  version: ", 0
version_name_msg db "HeatOS kernel v0.5", 0
status_boot_label db "  boot:    ", 0
status_mem_label db "  memory:  ", 0
status_date_label db "  date:    ", 0
status_time_label db "  time:    ", 0
status_uptime_label db "  uptime:  ", 0
status_history_label db "  history: ", 0
kb_suffix_msg db " KB", 0
entries_suffix_msg db " stored", 0
seconds_suffix_msg db " sec", 0
seconds_suffix_inline_msg db "sec", 0
unavailable_msg db "unavailable", 0
history_header_msg db "Command history:", 13, 10, 0
no_history_msg db "No history yet.", 13, 10, 0
repeat_prefix_msg db "Replaying: ", 0
no_repeat_msg db "Nothing to repeat yet.", 13, 10, 0
beep_msg db "Beep sent.", 13, 10, 0
apps_msg db "Desktop apps:", 13, 10
         db "  terminal - command shell", 13, 10
         db "  system   - live system summary", 13, 10
         db "  files    - placeholder file browser", 13, 10
         db "  notes    - roadmap and design notes", 13, 10
         db "  clock    - RTC date/time view", 13, 10
         db "  network  - PCI NIC diagnostics + ping stub", 13, 10
         db "  power    - halt or reboot", 13, 10, 13, 10, 0
returning_to_desktop_msg db "Returning to the desktop...", 13, 10, 0
halt_msg db "System halted.", 13, 10, 0
reboot_msg db "Rebooting...", 13, 10, 0
boot_summary_a db "Boot drive 0x", 0
boot_summary_b db ", kernel ", 0
boot_summary_c db " sectors loaded.", 13, 10, 0
boot_inline_a db "drive 0x", 0
boot_inline_b db ", kernel ", 0
boot_inline_c db " sectors / ", 0
boot_inline_d db " bytes", 0

desktop_top_title_msg db "Popeye Plasma Desktop", 0
desktop_apps_header_msg db "Apps", 0
desktop_workspace_header_msg db "Workspace", 0
desktop_app_terminal_msg db "1  Terminal", 0
desktop_app_system_msg db "2  System", 0
desktop_app_files_msg db "3  Files", 0
desktop_app_notes_msg db "4  Notes", 0
desktop_app_clock_msg db "5  Clock", 0
desktop_app_network_msg db "6  Network", 0
desktop_app_power_msg db "7  Power", 0
desktop_footer_line1_msg db "F1 help  F2 terminal  M menu  R run  Enter launch", 0
desktop_footer_line2_msg db "1-7 quick launch  T/S/F/N/C/W/P jump  Esc power", 0
desktop_footer_line3_msg db "Menu: click app rows with mouse or use arrows + Enter", 0
desktop_quick_header_msg db "Quick status", 0
desktop_memory_label_msg db "Memory", 0
desktop_kernel_label_msg db "Kernel sectors", 0
desktop_boot_label_msg db "Boot drive", 0
desktop_clock_label_msg db "Clock", 0
desktop_network_label_msg db "Network", 0
desktop_launch_hint_msg db "Press Enter to launch the selected app.", 0
desktop_kb_msg db "KB", 0
desktop_mouse_on_msg db "Mouse ON", 0
desktop_mouse_off_msg db "Mouse OFF", 0

preview_terminal_title_msg db "Terminal", 0
preview_terminal_line1_msg db "Open the integrated shell for commands, status, history, and uptime.", 0
preview_terminal_line2_msg db "Type 'desktop' from the shell to return here instantly.", 0
preview_terminal_line3_msg db "This is the fastest path to inspect live kernel state.", 0
preview_system_title_msg db "System", 0
preview_system_line1_msg db "View core operating-system metrics and current runtime state.", 0
preview_system_line2_msg db "The screen refreshes on demand and stays BIOS-only for now.", 0
preview_system_line3_msg db "Good place to validate boot, memory, time, and uptime signals.", 0
preview_files_title_msg db "Files", 0
preview_files_line1_msg db "This is the starting point for a future ramdisk or tiny filesystem.", 0
preview_files_line2_msg db "Right now it documents the planned layout instead of mounting disks.", 0
preview_files_line3_msg db "Next step is making these paths real and writable.", 0
preview_notes_title_msg db "Notes", 0
preview_notes_line1_msg db "Roadmap and desktop design notes live here until storage exists.", 0
preview_notes_line2_msg db "Use it as the design board for the next subsystems.", 0
preview_notes_line3_msg db "Protected mode, interrupts, storage, and actual graphics come next.", 0
preview_clock_title_msg db "Clock", 0
preview_clock_line1_msg db "Shows RTC date/time plus uptime from BIOS timer ticks.", 0
preview_clock_line2_msg db "Useful for checking that the kernel is keeping sane runtime state.", 0
preview_clock_line3_msg db "Refresh the screen whenever you want a fresh timestamp.", 0
preview_network_title_msg db "Network", 0
preview_network_line1_msg db "Scans PCI for NIC class devices and reports adapter identity.", 0
preview_network_line2_msg db "Run with QEMU -nic user,model=ne2k_pci for predictable testing.", 0
preview_network_line3_msg db "Terminal also includes net + ping diagnostics commands.", 0
preview_power_title_msg db "Power", 0
preview_power_line1_msg db "Shut the machine down or reboot directly from the desktop.", 0
preview_power_line2_msg db "This is still a BIOS reboot and CPU halt path, not ACPI power-off.", 0
preview_power_line3_msg db "Use it when you want fast control without dropping into the shell.", 0

app_system_title_msg db "HeatOS System Center", 0
app_files_title_msg db "HeatOS Files", 0
app_notes_title_msg db "HeatOS Notes", 0
app_clock_title_msg db "HeatOS Clock", 0
app_network_title_msg db "HeatOS Network", 0
app_power_title_msg db "HeatOS Power", 0
app_help_title_msg db "HeatOS Desktop Help", 0
app_footer_line1_msg db "Esc desktop  Enter desktop  T terminal", 0
app_footer_line2_msg db "Press R to refresh where it makes sense", 0
app_footer_line3_msg db "M opens kickoff menu from desktop  F2 jumps to terminal", 0

kickoff_title_msg db "Popeye Menu", 0
kickoff_terminal_msg db "1  Terminal", 0
kickoff_system_msg db "2  System", 0
kickoff_files_msg db "3  Files", 0
kickoff_notes_msg db "4  Notes", 0
kickoff_clock_msg db "5  Clock", 0
kickoff_network_msg db "6  Network", 0
kickoff_power_msg db "7  Power", 0
kickoff_hint_msg db "Up/Down select  Enter launch  Esc close", 0

run_dialog_title_msg db "Popeye Run Dialog", 13, 10, 0
run_dialog_help_msg db "Type app name: terminal, system, files, notes, clock, network, power", 13, 10, 0
run_dialog_prompt_msg db "run> ", 0
run_dialog_unknown_msg db "Unknown app. Press any key to continue.", 13, 10, 0

system_heading_msg db "Live system summary", 0
system_version_label_msg db "Kernel version", 0
system_boot_label_msg db "Boot drive", 0
system_kernel_label_msg db "Kernel sectors", 0
system_memory_label_msg db "Conventional memory", 0
system_date_label_msg db "RTC date", 0
system_time_label_msg db "RTC time", 0
system_uptime_label_msg db "Approx uptime", 0
system_note_line1_msg db "This remains a single-task real-mode kernel with BIOS services.", 0
system_note_line2_msg db "Kernel core and OS shell are now separated by responsibility.", 0
system_note_line3_msg db "Next milestone is storage + protected mode + graphics pipeline.", 0

files_heading_msg db "Virtual file layout", 0
files_line1_msg db "/", 0
files_line2_msg db "  apps/     built-in programs and launchers", 0
files_line3_msg db "  docs/     notes, help, and system references", 0
files_line4_msg db "  home/     future per-user space", 0
files_line5_msg db "  system/   kernel data, drivers, and config", 0
files_note_line1_msg db "No persistent filesystem is mounted yet.", 0
files_note_line2_msg db "This screen is the contract for what the first ramdisk should expose.", 0
files_note_line3_msg db "Once storage exists, the desktop can become more than a launcher.", 0

notes_heading_msg db "Roadmap", 0
notes_line1_msg db "1. Add a tiny ramdisk and path lookup.", 0
notes_line2_msg db "2. Move keyboard and timer logic off direct BIOS polling.", 0
notes_line3_msg db "3. Add a real graphics mode and window compositor.", 0
notes_line4_msg db "4. Enter protected mode and build proper memory management.", 0
notes_line5_msg db "5. Replace placeholders with actual apps and saved data.", 0
notes_footer_line1_msg db "This screen is effectively the OS design notebook for now.", 0
notes_footer_line2_msg db "The terminal command set is still the quickest way to inspect behavior.", 0

clock_heading_msg db "Clock and uptime", 0
clock_date_label_msg db "RTC date", 0
clock_time_label_msg db "RTC time", 0
clock_uptime_label_msg db "Approx uptime", 0
clock_note_line1_msg db "Press R to redraw and fetch fresh BIOS clock values.", 0
clock_note_line2_msg db "Current uptime is derived from the BIOS tick counter since boot.", 0

network_heading_msg db "Network diagnostics", 0
network_status_label_msg db "Adapter status", 0
network_slot_label_msg db "PCI slot (bus0)", 0
network_vendor_label_msg db "Vendor ID", 0
network_device_label_msg db "Device ID", 0
network_class_label_msg db "Class/Subclass", 0
network_note_line1_msg db "This is an early networking layer: detect + inspect + ping stub.", 0
network_note_line2_msg db "Use run.cmd default NIC config to expose ne2k_pci in QEMU.", 0
network_note_line3_msg db "Press R to rescan PCI network class devices.", 0
network_note_line4_msg db "Next step: bring up a minimal packet TX/RX driver.", 0

power_heading_msg db "Power control", 0
power_line1_msg db "H  Halt the CPU", 0
power_line2_msg db "R  Reboot through BIOS", 0
power_line3_msg db "T  Open terminal instead", 0
power_line4_msg db "Esc or Enter returns to the desktop", 0
power_line5_msg db "This is an early OS control panel, not an ACPI shutdown path.", 0
power_line6_msg db "It is enough to control the VM cleanly while the platform grows.", 0

help_heading_msg db "Desktop controls", 0
help_line1_msg db "Up/Down moves between apps in the launcher rail.", 0
help_line2_msg db "Enter launches the selected app.", 0
help_line3_msg db "1-7 or T/S/F/N/C/W/P open apps directly.", 0
help_line4_msg db "M opens kickoff menu and R opens a run dialog.", 0
help_line5_msg db "F2 opens terminal instantly from the desktop.", 0
help_line6_msg db "Esc from home jumps to the power panel.", 0
help_line7_msg db "Mouse can select and click app rows when available.", 0
help_line8_msg db "Type desktop/gui/popeye/plasma in terminal to return.", 0

boot_menu_title_msg db "Boot mode", 13, 10, 0
boot_menu_line1_msg db "Kernel and OS shell are separate layers now.", 13, 10, 0
boot_menu_line2_msg db "D/Enter = Popeye desktop   C = console first", 13, 10, 0
boot_menu_line3_msg db "You can switch later using 'desktop' from terminal.", 13, 10, 13, 10, 0
boot_menu_prompt_msg db "Select boot mode: ", 0
boot_menu_invalid_msg db "Invalid choice. Press D, C, or Enter.", 13, 10, 0

net_console_header_msg db "Network status:", 13, 10, 0
net_console_state_label_msg db "  state: ", 0
net_console_slot_label_msg db "  slot:  ", 0
net_console_vendor_label_msg db "  id:    vendor 0x", 0
net_console_device_mid_msg db "  device 0x", 0
net_console_class_label_msg db "  class: 0x", 0
net_console_hint_msg db "  qemu: run with NIC (run.cmd does this by default).", 13, 10, 13, 10, 0
net_console_missing_msg db "  no PCI network adapter found.", 13, 10
                      db "  try QEMU with -nic user,model=ne2k_pci", 13, 10, 13, 10, 0

ping_usage_msg db "Usage: ping <host>. Try: ping 127.0.0.1", 13, 10, 0
ping_stub_msg db "Only loopback ping is implemented in this stage.", 13, 10, 0
ping_reply_msg db "Reply from 127.0.0.1: bytes=32 time<1ms ttl=255", 13, 10, 0
ping_loopback_target_msg db "127.0.0.1", 0
ping_localhost_target_msg db "localhost", 0

arch_msg db "Architecture split:", 13, 10
         db "  kernel: hardware init, timing, memory, pci, services", 13, 10
         db "  os:     popeye desktop, apps, terminal command layer", 13, 10
         db "This separation keeps low-level and user-facing logic cleaner.", 13, 10, 13, 10, 0

run_cmd_terminal db "terminal", 0
run_cmd_console db "console", 0
run_cmd_system db "system", 0
run_cmd_files db "files", 0
run_cmd_notes db "notes", 0
run_cmd_clock db "clock", 0
run_cmd_network db "network", 0
run_cmd_power db "power", 0

slash_msg db "/", 0
net_state_online_msg db "detected", 0
net_state_offline_msg db "not-detected", 0
net_none_short_msg db "n/a", 0
net_none_word_msg db "----", 0
net_none_byte_msg db "--", 0

cmd_help db "help", 0
cmd_clear db "clear", 0
cmd_cls db "cls", 0
cmd_about db "about", 0
cmd_version db "version", 0
cmd_ver db "ver", 0
cmd_echo db "echo", 0
cmd_banner db "banner", 0
cmd_beep db "beep", 0
cmd_date db "date", 0
cmd_time db "time", 0
cmd_uptime db "uptime", 0
cmd_mem db "mem", 0
cmd_boot db "boot", 0
cmd_status db "status", 0
cmd_history db "history", 0
cmd_repeat db "repeat", 0
cmd_net db "net", 0
cmd_ping db "ping", 0
cmd_arch db "arch", 0
cmd_apps db "apps", 0
cmd_desktop db "desktop", 0
cmd_gui db "gui", 0
cmd_popeye db "popeye", 0
cmd_plasma db "plasma", 0
cmd_halt db "halt", 0
cmd_shutdown db "shutdown", 0
cmd_reboot db "reboot", 0
cmd_restart db "restart", 0
