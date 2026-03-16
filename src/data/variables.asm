boot_drive db 0
kernel_sectors db 0
desktop_selection db 0
kickoff_selection db 0
boot_mode db 0
event_type db 0
history_next db 0
history_used db 0
boot_ticks_hi dw 0
boot_ticks_lo dw 0
args_ptr dw 0
number_work_hi dw 0
number_work_lo dw 0

mouse_available db 0
mouse_visible db 0
mouse_prev_buttons db 0
mouse_col db 0
mouse_row db 0
mouse_click_app db 0

net_present db 0
net_device_slot db 0
net_class db 0
net_subclass db 0
net_vendor_id dw 0
net_device_id dw 0

rect_char db 0
rect_attr db 0
rect_top db 0
rect_left db 0
rect_height db 0
rect_width db 0

input_buffer times HISTORY_ENTRY_SIZE db 0
command_buffer times COMMAND_MAX + 1 db 0
last_runnable_buffer times HISTORY_ENTRY_SIZE db 0
history_buffer times HISTORY_COUNT * HISTORY_ENTRY_SIZE db 0
decimal_buffer times 11 db 0
date_buffer times 12 db 0
time_buffer times 12 db 0
memory_buffer times 8 db 0
kernel_sector_buffer times 8 db 0
boot_drive_buffer times 4 db 0
uptime_buffer times 12 db 0
network_status_buffer times 16 db 0
net_slot_buffer times 8 db 0
net_vendor_buffer times 8 db 0
net_device_buffer times 8 db 0
net_class_buffer times 4 db 0
net_subclass_buffer times 4 db 0

fs_cwd db 0
fs_data_free dw 0

fs_token_buf times 13 db 0
fs_path_buf times 64 db 0
fs_pwd_stack times 64 db 0

fs_node_type times 64 db 0
fs_node_parent times 64 db 0
fs_node_first_child times 64 db 0
fs_node_next_sibling times 64 db 0
fs_node_size times 64 dw 0
fs_node_data_off times 64 dw 0
fs_node_name times 64 * 12 db 0

fs_data_pool times 8192 db 0