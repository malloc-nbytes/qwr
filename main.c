#include <forge/cmd.h>
#include <forge/arg.h>
#include <forge/str.h>
#include <forge/cstr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Flag definitions */
#define FLAG_HELP_SHORT "h"
#define FLAG_HELP_LONG "help"
#define FLAG_ISO "iso"
#define FLAG_SIZE "sz"
#define FLAG_CORES "cores"
#define FLAG_MEMORY "mem"
#define FLAG_IMAGE "img"
#define FLAG_EXTRA_DISK "extra-disk"
#define FLAG_SSH_PORT "ssh-port"

#define err(msg)                                        \
        do {                                            \
                fprintf(stderr, "[Error]: " msg "\n");  \
                exit(1);                                \
        } while (0)

#define err_wargs(msg, ...)                                             \
        do {                                                            \
                fprintf(stderr, "[Error]: " msg "\n", __VA_ARGS__);     \
                exit(1);                                                \
        } while (0)

typedef struct {
        char *mode;
        char *iso;
        char *sz;
        char *cores;
        char *mem;
        char *img;
        char *extra_disk;
        char *ssh_port;
} context;

void
create_drive(const context *ctx)
{
        if (!ctx->img) {
                err("`create-drive` requires --" FLAG_IMAGE);
        }
        if (!ctx->sz) {
                err("`create-drive` requires --" FLAG_SIZE);
        }
        cmd(forge_cstr_builder("qemu-img create -f qcow2 ", ctx->img, " ", ctx->sz, "G", NULL));
}

void
run(const context *ctx)
{
        if (!ctx->img) {
                err("`run` requires --" FLAG_IMAGE);
        }

        forge_str cmd_str = forge_str_from("sudo qemu-system-x86_64");
        forge_str_concat(&cmd_str, " -enable-kvm ");
        forge_str_concat(&cmd_str, "-m ");
        forge_str_concat(&cmd_str, ctx->mem);
        forge_str_concat(&cmd_str, "G");
        forge_str_concat(&cmd_str, " -smp ");
        forge_str_concat(&cmd_str, ctx->cores);
        forge_str_concat(&cmd_str, " -cpu host");
        forge_str_concat(&cmd_str, " -hda ");
        forge_str_concat(&cmd_str, ctx->img);
        forge_str_concat(&cmd_str, " -netdev user,id=net0");

        if (!strcmp(ctx->mode, "ssh")) {
                /* Configure SSH mode with no GUI and port forwarding */
                forge_str_concat(&cmd_str, ",hostfwd=tcp::");
                forge_str_concat(&cmd_str, ctx->ssh_port ? ctx->ssh_port : "2222");
                forge_str_concat(&cmd_str, "-:22");
                forge_str_concat(&cmd_str, " -device e1000,netdev=net0");
                forge_str_concat(&cmd_str, " -nographic");
        } else {
                /* Existing GUI configuration */
                forge_str_concat(&cmd_str, " -device e1000,netdev=net0");
                forge_str_concat(&cmd_str, " -vga virtio");
                forge_str_concat(&cmd_str, " -display sdl");
        }

        if (ctx->extra_disk) {
                forge_str_concat(&cmd_str, " -hdb ");
                forge_str_concat(&cmd_str, ctx->extra_disk);
        }
        cmd(cmd_str.data);
        forge_str_destroy(&cmd_str);
}

void
install(const context *ctx)
{
        if (!ctx->iso) {
                err("`install` requires --" FLAG_ISO);
        }
        if (!ctx->img) {
                err("`install` requires --" FLAG_IMAGE);
        }
        if (!ctx->sz) {
                err("`install` requires --" FLAG_SIZE);
        }

        cmd(forge_cstr_builder("qemu-img create -f qcow2 ", ctx->img, " ", ctx->sz, "G", NULL));
        forge_str cmd_str = forge_str_from("sudo qemu-system-x86_64");
        forge_str_concat(&cmd_str, " -enable-kvm ");
        forge_str_concat(&cmd_str, "-m ");
        forge_str_concat(&cmd_str, ctx->mem);
        forge_str_concat(&cmd_str, "G");
        forge_str_concat(&cmd_str, " -smp ");
        forge_str_concat(&cmd_str, ctx->cores);
        forge_str_concat(&cmd_str, " -cpu host");
        forge_str_concat(&cmd_str, " -cdrom ");
        forge_str_concat(&cmd_str, ctx->iso);
        forge_str_concat(&cmd_str, " -hda ");
        forge_str_concat(&cmd_str, ctx->img);
        forge_str_concat(&cmd_str, " -boot d");
        forge_str_concat(&cmd_str, " -netdev user,id=net0");
        forge_str_concat(&cmd_str, " -device e1000,netdev=net0");
        forge_str_concat(&cmd_str, " -vga virtio");
        forge_str_concat(&cmd_str, " -display sdl");
        cmd(cmd_str.data);
        forge_str_destroy(&cmd_str);
}

void
help(void)
{
        printf("Usage: %s [options] <mode>\n\n", "qwr");
        printf("A utility for managing QEMU virtual machines.\n\n");
        printf("Modes:\n");
        printf("  install         Install an OS from an ISO to a disk image\n");
        printf("  run             Run a VM with a GUI from a disk image\n");
        printf("  ssh             Run a VM in headless mode with SSH access\n");
        printf("  create-drive    Create a new disk image\n\n");
        printf("Options:\n");
        printf("  --%s=<file.iso>           * ISO file for installation (required for install)\n", FLAG_ISO);
        printf("  --%s=<size>                * Disk size in GB (required for install, create-drive)\n", FLAG_SIZE);
        printf("  --%s=<file.qcow2>         * Disk image file (required for install, run, ssh, create-drive)\n", FLAG_IMAGE);
        printf("  --%s=<number>           * Number of CPU cores (default: 1)\n", FLAG_CORES);
        printf("  --%s=<size>               * Memory size in GB (default: 1)\n", FLAG_MEMORY);
        printf("  --%s=<file.qcow2>  * Attach an additional disk image (optional for run, ssh)\n", FLAG_EXTRA_DISK);
        printf("  --%s=<port>          * SSH port for host (default: 2222, optional for ssh)\n", FLAG_SSH_PORT);
        printf("  -%s, --%s                 * Display this help message\n\n", FLAG_HELP_SHORT, FLAG_HELP_LONG);
        printf("Examples:\n");
        printf("  Create a 20GB disk image:\n");
        printf("    %s --%s=disk.qcow2 --%s=20 create-drive\n\n", "qwr", FLAG_IMAGE, FLAG_SIZE);
        printf("  Install from an ISO:\n");
        printf("    %s --%s=ubuntu.iso --%s=disk.qcow2 --%s=20 --%s=2 --%s=4 install\n\n",
               "qwr", FLAG_ISO, FLAG_IMAGE, FLAG_SIZE, FLAG_CORES, FLAG_MEMORY);
        printf("  Run a VM with GUI:\n");
        printf("    %s --%s=disk.qcow2 --%s=2 --%s=4 run\n\n", "qwr", FLAG_IMAGE, FLAG_CORES, FLAG_MEMORY);
        printf("  Run a VM with SSH access:\n");
        printf("    %s --%s=disk.qcow2 --%s=2222 ssh\n\n", "qwr", FLAG_IMAGE, FLAG_SSH_PORT);
        exit(0);
}

void
handle_args(forge_arg *arghd)
{
        context ctx = (context) {
                .mode = NULL,
                .iso = NULL,
                .sz = NULL,
                .cores = "1",
                .mem = "1",
                .img = NULL,
                .extra_disk = NULL,
                .ssh_port = "2222",
        };

        forge_arg *arg = arghd;

        if (!arg) {
                help();
        }

        while (arg) {
                if (arg->h == 1) {
                        if (!strcmp(arg->s, FLAG_HELP_SHORT)) {
                                help();
                        } else {
                                err_wargs("unknown flag -%s\n", arg->s);
                        }
                } else if (arg->h == 2) {
                        if (!strcmp(arg->s, FLAG_HELP_LONG)) {
                                help();
                        } else if (!strcmp(arg->s, FLAG_MEMORY)) {
                                if (!arg->eq) {
                                        err("option --" FLAG_MEMORY " requires `=<amt>`");
                                }
                                ctx.mem = strdup(arg->eq);
                        } else if (!strcmp(arg->s, FLAG_CORES)) {
                                if (!arg->eq) {
                                        err("option --" FLAG_CORES " requires `=<amt>`");
                                }
                                ctx.cores = strdup(arg->eq);
                        } else if (!strcmp(arg->s, FLAG_IMAGE)) {
                                if (!arg->eq) {
                                        err("option --" FLAG_IMAGE " requires `=<img.qcow2>`");
                                }
                                ctx.img = strdup(arg->eq);
                        } else if (!strcmp(arg->s, FLAG_EXTRA_DISK)) {
                                if (!arg->eq) {
                                        err("option --" FLAG_EXTRA_DISK " requires `=<img.qcow2>`");
                                }
                                ctx.extra_disk = strdup(arg->eq);
                        } else if (!strcmp(arg->s, FLAG_ISO)) {
                                if (!arg->eq) {
                                        err("option --" FLAG_ISO " requires `=<file.iso>`");
                                }
                                ctx.iso = strdup(arg->eq);
                        } else if (!strcmp(arg->s, FLAG_SIZE)) {
                                if (!arg->eq) {
                                        err("option --" FLAG_SIZE " requires `=<amt>`");
                                }
                                ctx.sz = strdup(arg->eq);
                        } else if (!strcmp(arg->s, FLAG_SSH_PORT)) {
                                if (!arg->eq) {
                                        err("option --" FLAG_SSH_PORT " requires `=<port>`");
                                }
                                ctx.ssh_port = strdup(arg->eq);
                        } else {
                                err_wargs("unknown flag --%s\n", arg->s);
                        }
                } else {
                        if (!strcmp(arg->s, "install")) {
                                ctx.mode = "install";
                        } else if (!strcmp(arg->s, "run")) {
                                ctx.mode = "run";
                        } else if (!strcmp(arg->s, "ssh")) {
                                ctx.mode = "ssh";
                        } else if (!strcmp(arg->s, "create-drive")) {
                                ctx.mode = "create-drive";
                        } else {
                                err_wargs("unknown flag %s\n", arg->s);
                        }
                }
                arg = arg->n;
        }

        if (!ctx.mode) {
                err("no mode specified");
        }
        if (!strcmp(ctx.mode, "install")) {
                install(&ctx);
        } else if (!strcmp(ctx.mode, "run")) {
                run(&ctx);
        } else if (!strcmp(ctx.mode, "ssh")) {
                run(&ctx);
        } else if (!strcmp(ctx.mode, "create-drive")) {
                create_drive(&ctx);
        }
}

int
main(int argc, char **argv)
{

        forge_arg *arghd = forge_arg_alloc(argc, argv, 1);
        handle_args(arghd);
        forge_arg_free(arghd);

        return 0;
}
