/* JIT Runner - loads AST cache and executes directly with optional detailed mode */
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <strings.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>

#define BX_AST_MAGIC 0x42584153
#define BX_AST_VERSION 1

typedef enum {
    BX_AST_BOX = 1, BX_AST_SAY = 2, BX_AST_ASK = 3, BX_AST_MATH = 4,
    BX_AST_TEST = 5, BX_AST_IF = 6, BX_AST_JUMP = 7, BX_AST_JUMPIF = 8,
    BX_AST_DEL = 9, BX_AST_MARK = 10, BX_AST_PREMARK = 11, BX_AST_CLEAR = 12,
    BX_AST_END = 13, BX_AST_GFD = 14, BX_AST_CREQ = 15, BX_AST_SCAN = 16,
    BX_AST_DEFU = 17, BX_AST_WSTAT = 18, BX_AST_WDIS = 19, BX_AST_PULL = 20,
    BX_AST_PUSH = 21, BX_AST_WS = 22, BX_AST_WSEND = 23, BX_AST_WRECV = 24,
    BX_AST_WCLOSE = 25,
} bx_ast_type_t;

typedef struct {
    uint32_t type; uint32_t argc; uint32_t arg_offset;
} bx_ast_node_t;

typedef struct {
    uint32_t magic; uint32_t version; uint32_t node_count; uint32_t string_table_size;
} bx_ast_header_t;

typedef struct {
    char *name;
    long value;
} Box;

static Box *boxes = NULL;
static int box_count = 0, box_cap = 0;

static long box_get(const char *name) {
    for (int i = 0; i < box_count; i++) if (!strcmp(boxes[i].name, name)) return boxes[i].value;
    return 0;
}

static void box_set(const char *name, long value) {
    for (int i = 0; i < box_count; i++) {
        if (!strcmp(boxes[i].name, name)) { boxes[i].value = value; return; }
    }
    if (box_count == box_cap) { box_cap = box_cap ? box_cap * 2 : 16; boxes = realloc(boxes, box_cap * sizeof(Box)); }
    boxes[box_count].name = strdup(name); boxes[box_count].value = value; box_count++;
}

static int is_num(const char *s) { if (*s=='-'||*s=='+') s++; if (!*s) return 0; while(*s) if (!isdigit(*s++)) return 0; return 1; }
static long resolve(const char *s) { if (*s == '$') return box_get(s+1); return atol(s); }

static void ast_dump_node(bx_ast_node_t *node, char *strtab, int indent) {
    const char *type_names[] = {"","BOX","SAY","ASK","MATH","TEST","IF","JUMP","JUMPIF","DEL","MARK","PREMARK","CLEAR","END","GFD","CREQ","SCAN","DEFU","WSTAT","WDIS","PULL","PUSH","WS","WSEND","WRECV","WCLOSE"};
    printf("%*s%s [argc=%d]", indent, "", type_names[node->type], node->argc);
    char *p = strtab + node->arg_offset;
    for (int i = 0; i < node->argc; i++) {
        printf(" '%s'", p);
        p += strlen(p) + 1;
    }
    printf("\n");
}

static int run_ast(const char *ast_file, int detailed) {
    FILE *f = fopen(ast_file, "rb");
    if (!f) { perror(ast_file); return 1; }

    bx_ast_header_t hdr;
    fread(&hdr, sizeof(hdr), 1, f);
    if (hdr.magic != BX_AST_MAGIC) { fprintf(stderr, "Invalid AST magic\n"); return 1; }

    bx_ast_node_t *nodes = malloc(hdr.node_count * sizeof(bx_ast_node_t));
    fread(nodes, sizeof(bx_ast_node_t), hdr.node_count, f);

    char *strtab = malloc(hdr.string_table_size);
    fread(strtab, 1, hdr.string_table_size, f);
    fclose(f);

    if (detailed) {
        printf("=== AST DUMP (%d nodes) ===\n", hdr.node_count);
        for (uint32_t i = 0; i < hdr.node_count; i++) {
            printf("L%u: ", i);
            ast_dump_node(&nodes[i], strtab, 4);
        }
        printf("=== EXECUTION ===\n");
    }

    /* First pass: build mark table from PREMARK nodes */
    int *mark_table = calloc(hdr.node_count, sizeof(int));
    int mark_count = 0;
    for (uint32_t i = 0; i < hdr.node_count; i++) {
        bx_ast_node_t *n = &nodes[i];
        if (n->type == BX_AST_PREMARK && n->argc >= 1) {
            char *mark_name = strtab + n->arg_offset;
            mark_table[mark_count] = i;
            mark_count++;
            if (detailed) printf("PREMARK '%s' at line %d\n", mark_name, i);
        }
    }
    if (detailed) printf("Mark table has %d entries\n", mark_count);

    int pc = 0;
    while (pc < hdr.node_count) {
        bx_ast_node_t *n = &nodes[pc];
        char *p = strtab + n->arg_offset;

        if (detailed) {
            printf("PC=%d: ", pc);
            const char *type_names[] = {"","BOX","SAY","ASK","MATH","TEST","IF","JUMP","JUMPIF","DEL","MARK","PREMARK","CLEAR","END"};
            printf("%s", type_names[n->type]);
            for (int i = 0; i < n->argc; i++) { printf(" '%s'", p); p += strlen(p) + 1; }
            printf("\n");
        }

        switch (n->type) {
            case BX_AST_BOX:
                if (n->argc >= 2) { box_set(p, atol(p + strlen(p) + 1)); }
                break;
            case BX_AST_SAY:
                if (n->argc >= 1) { printf("%ld\n", resolve(p)); }
                else { printf("\n"); }
                fflush(stdout);
                break;
            case BX_AST_MATH:
                if (n->argc >= 4) {
                    long a = resolve(p), b = resolve(p + strlen(p) + 1);
                    char *op = p + strlen(p) + 1 + strlen(p + strlen(p) + 1) + 1;
                    long v = 0;
                    if (!strcmp(op,"+")) v = a + b;
                    else if (!strcmp(op,"-")) v = a - b;
                    else if (!strcmp(op,"*") || !strcasecmp(op,"x")) v = a * b;
                    else if (!strcmp(op,"/")) v = b ? a / b : 0;
                    else if (!strcmp(op,"%")) v = b ? a % b : 0;
                    box_set(p, v);
                }
                break;
            case BX_AST_TEST:
                if (n->argc >= 4) {
                    long a = resolve(p), b = resolve(p + strlen(p) + 1 + strlen(p + strlen(p) + 1) + 1);
                    char *op = p + strlen(p) + 1;
                    int ok = 0;
                    if (!strcmp(op,"==")) ok = a == b;
                    else if (!strcmp(op,"!=")) ok = a != b;
                    else if (!strcmp(op,">")) ok = a > b;
                    else if (!strcmp(op,"<")) ok = a < b;
                    else if (!strcmp(op,">=")) ok = a >= b;
                    else if (!strcmp(op,"<=")) ok = a <= b;
                    char *truev = n->argc >= 5 ? p + strlen(p) + 1 + strlen(p + strlen(p) + 1) + 1 + strlen(op) + 1 : "1";
                    char *falsev = n->argc >= 6 ? truev + strlen(truev) + 1 : "0";
                    box_set(p, ok ? atol(truev) : atol(falsev));
                }
                break;
            case BX_AST_JUMP:
                if (n->argc >= 1) {
                    if (is_num(p)) { 
                        if (detailed) printf("JUMP: jumping to line %d\n", atoi(p));
                        pc = atoi(p); continue; 
                    }
                    /* Mark jump */
                    if (detailed) printf("JUMP: looking for mark '%s'\n", p);
                    for (int i = 0; i < mark_count; i++) {
                        bx_ast_node_t *m = &nodes[mark_table[i]];
                        char *mname = strtab + m->arg_offset;
                        if (detailed) printf("JUMP: checking mark '%s' at line %d\n", mname, mark_table[i]);
                        if (!strcmp(p, mname)) { 
                            if (detailed) printf("JUMP: jumping to mark '%s' at line %d\n", mname, mark_table[i]);
                            pc = mark_table[i]; goto continue_exec; 
                        }
                    }
                }
                break;
            case BX_AST_JUMPIF:
                if (n->argc >= 4) {
                    /* Parse args by iterating */
                    char *args[10];
                    int argc = 0;
                    char *arg_ptr = p;
                    if (detailed) printf("JUMPIF: raw p='%s'\n", p);
                    for (int i = 0; i < n->argc && argc < 10; i++) {
                        args[argc++] = arg_ptr;
                        if (detailed) printf("JUMPIF: arg[%d]='%s'\n", argc-1, arg_ptr);
                        arg_ptr += strlen(arg_ptr) + 1;
                    }
                    if (detailed) printf("JUMPIF: parsed %d args\n", argc);
                    if (argc >= 4) {
                        long a = resolve(args[0]);
                        char *op = args[1];
                        long b = resolve(args[2]);
                        char *target = args[3];
                        if (detailed) printf("JUMPIF: a=%ld op=%s b=%ld target='%s'\n", a, op, b, target);
                        int ok = 0;
                        if (!strcmp(op,"==")) ok = a == b;
                        else if (!strcmp(op,"!=")) ok = a != b;
                        else if (!strcmp(op,">")) ok = a > b;
                        else if (!strcmp(op,"<")) ok = a < b;
                        else if (!strcmp(op,">=")) ok = a >= b;
                        else if (!strcmp(op,"<=")) ok = a <= b;
                        if (detailed) printf("JUMPIF: ok=%d\n", ok);
                        if (ok) {
                            if (is_num(target)) { 
                                if (detailed) printf("JUMPIF: jumping to line %d\n", atoi(target));
                                pc = atoi(target); continue; 
                            }
                            /* Mark jump */
                            if (detailed) printf("JUMPIF: looking for mark '%s'\n", target);
                            for (int i = 0; i < mark_count; i++) {
                                bx_ast_node_t *m = &nodes[mark_table[i]];
                                char *mname = strtab + m->arg_offset;
                                if (detailed) printf("JUMPIF: checking mark '%s' at line %d\n", mname, mark_table[i]);
                                if (!strcmp(target, mname)) { 
                                    if (detailed) printf("JUMPIF: jumping to mark '%s' at line %d\n", mname, mark_table[i]);
                                    pc = mark_table[i]; goto continue_exec; 
                                }
                            }
                        }
                    }
                }
                break;
            case BX_AST_MARK:
                /* Runtime mark - update mark table */
                if (n->argc >= 1) {
                    char *mark_name = p;
                    for (int i = 0; i < mark_count; i++) {
                        bx_ast_node_t *m = &nodes[mark_table[i]];
                        char *mname = strtab + m->arg_offset;
                        if (!strcmp(mark_name, mname)) {
                            mark_table[i] = pc + 1;  /* mark points to next line */
                            if (detailed) printf("MARK '%s' updated to line %d\n", mark_name, pc + 1);
                            break;
                        }
                    }
                }
                break;
            case BX_AST_END:
                goto done;
            case BX_AST_CLEAR:
                printf("\033[2J\033[H");
                break;
        }
        pc++;
    continue_exec:
        ;
    }
done:
    free(mark_table);
    free(nodes);
    free(strtab);
    free(boxes);
    return 0;
}

static int build_ast(const char *src_file, const char *ast_file) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), ".build/bx_ast %s %s", src_file, ast_file);
    return system(cmd);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [--detailed] <file.bx> [--build]\n", argv[0]);
        return 1;
    }

    int detailed = 0;
    const char *src_file = NULL;
    int build_only = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--detailed")) detailed = 1;
        else if (!strcmp(argv[i], "--build")) build_only = 1;
        else if (!src_file) src_file = argv[i];
    }

    if (!src_file) return 1;

    char ast_file[512];
    const char *base = strrchr(src_file, '/');
    base = base ? base + 1 : src_file;
    snprintf(ast_file, sizeof(ast_file), ".build/ast_cache/%s.ast", base);

    struct stat st_src, st_ast;
    int need_build = 1;
    if (!stat(ast_file, &st_ast) && !stat(src_file, &st_src)) {
        if (st_ast.st_mtime >= st_src.st_mtime) need_build = 0;
    }

    if (need_build || build_only) {
        if (build_ast(src_file, ast_file)) return 1;
        if (build_only) return 0;
    }

    return run_ast(ast_file, detailed);
}