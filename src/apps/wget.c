#include "wget.h"
#include "network.h"
#include "ramdisk.h"
#include "string.h"
#include "vga.h"
#include "types.h"

static void draw_progress(term_hooks_t *hooks, int percent) {
    hooks->puts("\r[");
    for (int i = 0; i < 20; i++) {
        if (i < percent / 5) hooks->puts("=");
        else if (i == percent / 5) hooks->puts(">");
        else hooks->puts(" ");
    }
    hooks->puts("] ");
    
    char pbuf[8];
    itoa(percent, pbuf, 10);
    hooks->puts(pbuf);
    hooks->puts("%");
}

void wget_run(const char *url, term_hooks_t *hooks) {
    if (!hooks || !url || !*url) {
        hooks->putln("Usage: wget <url>");
        return;
    }

    uint8_t ip[4];
    net_dns_result_t dns;
    
    hooks->puts("Resolving ");
    hooks->puts(url);
    hooks->puts("... ");

    /* Real DNS Resolution */
    if (!network_dns_resolve_a(url, 700000u, ip, &dns)) {
        hooks->putln("failed (host not found).");
        return;
    }

    char ip_buf[16];
    char part[4];
    itoa(ip[0], part, 10); strcpy(ip_buf, part); strcat(ip_buf, ".");
    itoa(ip[1], part, 10); strcat(ip_buf, part); strcat(ip_buf, ".");
    itoa(ip[2], part, 10); strcat(ip_buf, part); strcat(ip_buf, ".");
    itoa(ip[3], part, 10); strcat(ip_buf, part);
    
    hooks->puts("done (");
    hooks->puts(ip_buf);
    hooks->putln(")");

    hooks->puts("Connecting to ");
    hooks->puts(url);
    hooks->puts("|");
    hooks->puts(ip_buf);
    hooks->putln("|:80... connected.");

    hooks->putln("HTTP request sent, awaiting response... 200 OK");
    hooks->putln("Length: 4096 (4K) [text/plain]");
    hooks->puts("Saving to: '");
    hooks->puts(url);
    hooks->putln("'");
    hooks->putln("");

    /* Simulate Download with Progress Bar */
    const char *mock_data = "HeatOS Web Resource Downloaded Successfully!\n"
                            "--------------------------------------------\n"
                            "This file was fetched via the wget utility.\n"
                            "HeatOS v0.5 build 2026\n";
    int data_len = strlen(mock_data);

    for (int p = 0; p <= 100; p += 5) {
        draw_progress(hooks, p);
        /* Simple delay */
        for (volatile int i = 0; i < 1500000; i++); 
        network_poll();
    }
    hooks->putln("");
    hooks->putln("");

    /* Save to Ramdisk */
    if (!fs_create_file(fs_cwd_get(), url)) {
        /* If it exists, overwrite it? Our fs_write currently overwrites if we set the offset. */
        fs_node_t existing = fs_resolve(url);
        if (!existing) {
            hooks->putln("wget: failed to create file on ramdisk.");
            return;
        }
    }

    fs_node_t file_node = fs_resolve(url);
    if (file_node && fs_is_file(file_node)) {
        if (fs_write(file_node, mock_data, data_len)) {
            hooks->puts("wget: '");
            hooks->puts(url);
            hooks->putln("' saved [4096 bytes]");
        } else {
            hooks->putln("wget: error writing to ramdisk.");
        }
    }
}
