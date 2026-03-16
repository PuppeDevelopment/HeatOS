banner_msg db "================================", 13, 10
           db "        HeatOS 0.5              ", 13, 10
           db "================================", 13, 10, 13, 10, 0
terminal_ready_msg db "Terminal ready.", 13, 10, 0
terminal_hint_msg db "Try: help, net, ping 127.0.0.1", 13, 10, 13, 10, 0
prompt_msg db "Heat> ", 0
unknown_msg db "Unknown command. Type 'help'.", 13, 10, 0
help_msg db "Commands:", 13, 10
         db "  help      clear/cls   about       version/ver", 13, 10
         db "  echo      banner      beep        mem", 13, 10
         db "  date      time        uptime      boot", 13, 10
         db "  status    history     repeat      apps", 13, 10
         db "  ls        cd          pwd         mkdir", 13, 10
         db "  net       ping        arch", 13, 10
         db "  halt/shutdown  reboot/restart", 13, 10
         db "Tip: press Esc while typing to clear the whole line.", 13, 10, 13, 10, 0
about_msg db "HeatOS kernel handles hardware and low-level services.", 13, 10
          db "The desktop environment is being rewritten in standalone assembly.", 13, 10
          db "Still real-mode BIOS based, but now split by role.", 13, 10, 13, 10, 0
version_msg db "HeatOS kernel v0.5", 13, 10
            db "Features: terminal shell, mouse hooks, net diagnostics.", 13, 10, 13, 10, 0
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
apps_msg db "Terminal commands:", 13, 10
         db "  help, clear, about, version, echo, banner, beep", 13, 10
         db "  date, time, uptime, mem, boot, status, history", 13, 10
         db "  net, ping, arch, ls, cd, pwd, mkdir", 13, 10
         db "  halt, shutdown, reboot, restart", 13, 10, 13, 10, 0
halt_msg db "System halted.", 13, 10, 0
reboot_msg db "Rebooting...", 13, 10, 0
boot_summary_a db "Boot drive 0x", 0
boot_summary_b db ", kernel ", 0
boot_summary_c db " sectors loaded.", 13, 10, 0
boot_inline_a db "drive 0x", 0
boot_inline_b db ", kernel ", 0
boot_inline_c db " sectors / ", 0
boot_inline_d db " bytes", 0

arch_msg db "Architecture:", 13, 10
         db "  kernel: hardware init, timing, memory, pci, services", 13, 10
         db "  desktop: standalone assembly (being rewritten)", 13, 10
         db "This separation keeps low-level and user-facing logic cleaner.", 13, 10, 13, 10, 0

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
cmd_halt db "halt", 0
cmd_shutdown db "shutdown", 0
cmd_reboot db "reboot", 0
cmd_restart db "restart", 0
cmd_ls db "ls", 0
cmd_cd db "cd", 0
cmd_pwd db "pwd", 0
cmd_mkdir db "mkdir", 0
