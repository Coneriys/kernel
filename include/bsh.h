#ifndef BSH_H
#define BSH_H

#include <stdint.h>
#include <stddef.h>

#define BSH_MAX_CMD_LEN 256
#define BSH_MAX_ARGS 16
#define BSH_MAX_ARG_LEN 64
#define BSH_HISTORY_SIZE 32

typedef struct {
    char name[BSH_MAX_ARG_LEN];
    int arg_count;
    char args[BSH_MAX_ARGS][BSH_MAX_ARG_LEN];
} bsh_command_t;

typedef struct {
    char name[32];
    const char* description;
    int (*handler)(bsh_command_t* cmd);
} bsh_builtin_t;

typedef struct {
    char commands[BSH_HISTORY_SIZE][BSH_MAX_CMD_LEN];
    int count;
    int current;
} bsh_history_t;

void bsh_init(void);
void bsh_run(void);
void bsh_print_prompt(void);
int bsh_parse_command(const char* input, bsh_command_t* cmd);
int bsh_execute_command(bsh_command_t* cmd);

// Built-in commands
int bsh_cmd_help(bsh_command_t* cmd);
int bsh_cmd_clear(bsh_command_t* cmd);
int bsh_cmd_echo(bsh_command_t* cmd);
int bsh_cmd_ps(bsh_command_t* cmd);
int bsh_cmd_version(bsh_command_t* cmd);
int bsh_cmd_mem(bsh_command_t* cmd);
int bsh_cmd_exit(bsh_command_t* cmd);

// File system commands
int bsh_cmd_ls(bsh_command_t* cmd);
int bsh_cmd_cd(bsh_command_t* cmd);
int bsh_cmd_mkdir(bsh_command_t* cmd);
int bsh_cmd_rmdir(bsh_command_t* cmd);
int bsh_cmd_rm(bsh_command_t* cmd);
int bsh_cmd_pwd(bsh_command_t* cmd);
int bsh_cmd_touch(bsh_command_t* cmd);
int bsh_cmd_history(bsh_command_t* cmd);
int bsh_cmd_hypr(bsh_command_t* cmd);
int bsh_cmd_man(bsh_command_t* cmd);
int bsh_cmd_ifconfig(bsh_command_t* cmd);
int bsh_cmd_ping(bsh_command_t* cmd);
int bsh_cmd_cat(bsh_command_t* cmd);
int bsh_cmd_grep(bsh_command_t* cmd);
int bsh_cmd_find(bsh_command_t* cmd);
int bsh_cmd_wc(bsh_command_t* cmd);
int bsh_cmd_sort(bsh_command_t* cmd);
int bsh_cmd_dhcp(bsh_command_t* cmd);
int bsh_cmd_mouse(bsh_command_t* cmd);
int bsh_cmd_video(bsh_command_t* cmd);
int bsh_cmd_kbd(bsh_command_t* cmd);
int bsh_cmd_wget(bsh_command_t* cmd);
int bsh_cmd_netstat(bsh_command_t* cmd);
int bsh_cmd_top(bsh_command_t* cmd);
int bsh_cmd_df(bsh_command_t* cmd);
int bsh_cmd_whoami(bsh_command_t* cmd);
int bsh_cmd_uptime(bsh_command_t* cmd);
int bsh_cmd_free(bsh_command_t* cmd);
int bsh_cmd_date(bsh_command_t* cmd);
int bsh_cmd_uname(bsh_command_t* cmd);
int bsh_cmd_lscpu(bsh_command_t* cmd);
int bsh_cmd_amdgpu(bsh_command_t* cmd);
int bsh_cmd_usb(bsh_command_t* cmd);
int bsh_cmd_gui(bsh_command_t* cmd);
int bsh_cmd_app(bsh_command_t* cmd);
int bsh_cmd_pkg(bsh_command_t* cmd);
int bsh_cmd_perf(bsh_command_t* cmd);
int bsh_cmd_theme(bsh_command_t* cmd);
int bsh_cmd_cursor(bsh_command_t* cmd);
int bsh_cmd_workspace(bsh_command_t* cmd);
int bsh_cmd_notify(bsh_command_t* cmd);

#endif