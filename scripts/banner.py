def color(data_input_output):
    color_codes = {
        "GREEN": '\033[32m',
        "LIGHTGREEN_EX": '\033[92m',
        "YELLOW": '\033[33m',
        "LIGHTYELLOW_EX": '\033[93m',
        "CYAN": '\033[36m',
        "LIGHTCYAN_EX": '\033[96m',
        "BLUE": '\033[34m',
        "LIGHTBLUE_EX": '\033[94m',
        "MAGENTA": '\033[35m',
        "LIGHTMAGENTA_EX": '\033[95m',
        "RED": '\033[31m',
        "LIGHTRED_EX": '\033[91m',
        "BLSYN": '\033[30m',
        "LIGHTBLSYN_EX": '\033[90m',
        "WHITE": '\033[37m',
        "LIGHTWHITE_EX": '\033[97m',
        "GRAY": '\033[90m',
        "C": '\033[36m'
    }
    return color_codes.get(data_input_output, "")

def gradient(start_color, end_color, steps):
    start_r, start_g, start_b = int(start_color[1:3], 16), int(start_color[3:5], 16), int(start_color[5:7], 16)
    end_r, end_g, end_b = int(end_color[1:3], 16), int(end_color[3:5], 16), int(end_color[5:7], 16)
    gradient_colors = []
    for step in range(steps):
        r = start_r + (end_r - start_r) * step // steps
        g = start_g + (end_g - start_g) * step // steps
        b = start_b + (end_b - start_b) * step // steps
        gradient_colors.append(f"\033[38;2;{r};{g};{b}m")
    return gradient_colors

start_color = "#FF33CC"
end_color = "#66FFFF"
gradient_colors = gradient(start_color, end_color, 13)

banner_lines = [
    f"{gradient_colors[5]}                           {gradient_colors[5]}    '---''(_/--'  `-'\_) ",
    f"{gradient_colors[6]}                          {gradient_colors[6]}══╦════════════════════════╦══",
    f"{gradient_colors[7]}                    {gradient_colors[7]}╔═══════╩════════════════════════╩═══════╗",
    f"{gradient_colors[8]}                    {gradient_colors[8]}║               Welcome Back             ║",
    f"{gradient_colors[9]}                    {gradient_colors[9]}║      ”I mean we kinda lost the src”    ║",
    f"{gradient_colors[10]}                  {gradient_colors[10]}╔╗╚═════════════════════════════════════════╗",
    f"{gradient_colors[11]}                  {gradient_colors[11]}║╚═══════════════════════════════════════════╝",
    f"{gradient_colors[12]}                 {gradient_colors[12]}╔╩═══════════════════════════════════════════╩╗",
    f"{gradient_colors[12]}                 {gradient_colors[12]}║   ~ ~ ~ Type H̲E̲L̲P̲ To See Commands ~ ~ ~     ║",
    f"{gradient_colors[11]}                 {gradient_colors[11]}║ Copyright © 2025 Catnet All Rights Reserved ║",
    f"{gradient_colors[10]}                 {gradient_colors[10]}╚═════════════════════════════════════════════╝"
]

banner = '\n'.join(banner_lines)
print(banner)
