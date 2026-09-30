/* AST extraction tool - parses BX source and emits binary AST cache */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#define BX_AST_MAGIC 0x42584153  /* "BXAS" */
#define BX_AST_VERSION 1

typedef enum {
    BX_AST_BOX = 1,
    BX_AST_SAY = 2,
    BX_AST_ASK = 3,
    BX_AST_MATH = 4,
    BX_AST_TEST = 5,
    BX_AST_IF = 6,
    BX_AST_JUMP = 7,
    BX_AST_JUMPIF = 8,
    BX_AST_DEL = 9,
    BX_AST_MARK = 10,
    BX_AST_PREMARK = 11,
    BX_AST_CLEAR = 12,
    BX_AST_END = 13,
    BX_AST_GFD = 14,
    BX_AST_CREQ = 15,
    BX_AST_SCAN = 16,
    BX_AST_DEFU = 17,
    BX_AST_WSTAT = 18,
    BX_AST_WDIS = 19,
    BX_AST_PULL = 20,
    BX_AST_PUSH = 21,
    BX_AST_WS = 22,
    BX_AST_WSEND = 23,
    BX_AST_WRECV = 24,
    BX_AST_WCLOSE = 25,
} bx_ast_type_t;

typedef struct {
    uint32_t type;
    uint32_t argc;
    uint32_t arg_offset;
} bx_ast_node_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t node_count;
    uint32_t string_table_size;
} bx_ast_header_t;

static char *xstrdup(const char *s) { size_t n = strlen(s) + 1; char *p = malloc(n); memcpy(p, s, n); return p; }
static char *xstrndup(const char *s, size_t n) { char *p = malloc(n + 1); memcpy(p, s, n); p[n] = 0; return p; }
static void *xrealloc(void *p, size_t n) { void *q = realloc(p, n); return q; }
static char *trim(char *s) { while (isspace(*s)) s++; char *e = s + strlen(s); while (e > s && isspace(e[-1])) *--e = 0; return s; }
static int streqi(const char *a, const char *b) { 
    while (*a && *b) { 
        char ca = *a; char cb = *b;
        if (tolower(ca) != tolower(cb)) return 0; 
        a++; b++;
    } 
    return *a == 0 && *b == 0; 
}

static const char *canonical(const char *cmd) {
    if (streqi(cmd,"b") || streqi(cmd,"box")) return "box";
    if (streqi(cmd,"s") || streqi(cmd,"say")) return "say";
    if (streqi(cmd,"a") || streqi(cmd,"ask")) return "ask";
    if (streqi(cmd,"m") || streqi(cmd,"math")) return "math";
    if (streqi(cmd,"t") || streqi(cmd,"test")) return "test";
    if (streqi(cmd,"i") || streqi(cmd,"if")) return "if";
    if (streqi(cmd,"j") || streqi(cmd,"jump")) return "jump";
    if (streqi(cmd,"ji") || streqi(cmd,"jumpif")) return "jumpif";
    if (streqi(cmd,"d") || streqi(cmd,"del")) return "del";
    if (streqi(cmd,"pm") || streqi(cmd,"premark")) return "premark";
    if (streqi(cmd,"mk") || streqi(cmd,"mark")) return "mark";
    if (streqi(cmd,"e") || streqi(cmd,"end")) return "end";
    if (streqi(cmd,"cls") || streqi(cmd,"clear")) return "clear";
    return cmd;
}

static char **split_bars(const char *s, int *out_n) {
    printf("DEBUG split_bars: input='%s'\n", s); fflush(stdout);
    int cap = 8, n = 0; char **parts = malloc(sizeof(char*) * cap);
    const char *start = s;
    for (const char *p = s;; p++) {
        if (*p == '|' || *p == 0) {
            if (n == cap) { cap *= 2; parts = xrealloc(parts, sizeof(char*) * cap); }
            parts[n++] = xstrndup(start, p - start);
            if (*p == 0) break;
            start = p + 1;
        }
    }
    *out_n = n; return parts;
}
static void free_parts(char **p, int n) { for (int i = 0; i < n; i++) free(p[i]); free(p); }

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.bx> <output.ast>\n", argv[0]);
        return 1;
    }

    FILE *in = fopen(argv[1], "r");
    if (!in) { perror(argv[1]); return 1; }
    fseek(in, 0, SEEK_END); long sz = ftell(in); rewind(in);
    char *src = malloc(sz + 1); fread(src, 1, sz, in); src[sz] = 0; fclose(in);

    /* Parse lines */
    char **lines = NULL; int line_count = 0, line_cap = 32;
    lines = malloc(sizeof(char*) * line_cap);
    for (char *p = strtok(src, "\n"); p; p = strtok(NULL, "\n")) {
        if (line_count == line_cap) { line_cap *= 2; lines = xrealloc(lines, sizeof(char*) * line_cap); }
        lines[line_count++] = xstrdup(p);
    }
    free(src);

    /* AST nodes and string table */
    bx_ast_node_t *nodes = malloc(sizeof(bx_ast_node_t) * line_cap);
    int node_count = 0;
    char *string_table = malloc(4096); int str_size = 0, str_cap = 4096;
    string_table[0] = 0; str_size = 1; /* empty string at offset 0 */

for (int i = 0; i < line_count; i++) {
        char *l = lines[i];
        char *s = trim(l);
        if (!*s || (s[0]=='/' && s[1]=='/')) { continue; }
        char *sp = s; while (*sp && !isspace(*sp)) sp++;
        char save = *sp; *sp = 0; 
        const char *cmd = canonical(s);
        *sp = save;
        char *args = save ? trim(sp + 1) : sp;
        
        printf("DEBUG: line %d: cmd='%s'\n", i, cmd);
        
        bx_ast_type_t type = 0;
        if (streqi(cmd, "box")) type = BX_AST_BOX;
        else if (streqi(cmd, "say")) type = BX_AST_SAY;
        else if (streqi(cmd, "ask")) type = BX_AST_ASK;
        else if (streqi(cmd, "math")) type = BX_AST_MATH;
        else if (streqi(cmd, "test")) type = BX_AST_TEST;
        else if (streqi(cmd, "if")) type = BX_AST_IF;
        else if (streqi(cmd, "jump")) type = BX_AST_JUMP;
        else if (streqi(cmd, "jumpif")) type = BX_AST_JUMPIF;
        else if (streqi(cmd, "del")) type = BX_AST_DEL;
        else if (streqi(cmd, "mark")) type = BX_AST_MARK;
        else if (streqi(cmd, "premark")) type = BX_AST_PREMARK;
        else if (streqi(cmd, "clear") || streqi(cmd, "cls")) type = BX_AST_CLEAR;
        else if (streqi(cmd, "end")) type = BX_AST_END;
        else if (streqi(cmd, "gfd")) type = BX_AST_GFD;
        else if (streqi(cmd, "creq")) type = BX_AST_CREQ;
        else if (streqi(cmd, "scan")) type = BX_AST_SCAN;
        else if (streqi(cmd, "defu")) type = BX_AST_DEFU;
        else if (streqi(cmd, "wstat")) type = BX_AST_WSTAT;
        else if (streqi(cmd, "wdis")) type = BX_AST_WDIS;
        else if (streqi(cmd, "pull")) type = BX_AST_PULL;
        else if (streqi(cmd, "push")) type = BX_AST_PUSH;
        else if (streqi(cmd, "ws")) type = BX_AST_WS;
        else if (streqi(cmd, "wsend")) type = BX_AST_WSEND;
        else if (streqi(cmd, "wrecv")) type = BX_AST_WRECV;
        else if (streqi(cmd, "wclose")) type = BX_AST_WCLOSE;

        printf("DEBUG: type=%u\n", type); fflush(stdout);
        if (!type) { continue; }

        printf("DEBUG: args_ptr=%p args='%s'\n", (void*)args, args); fflush(stdout);
        int n; char **p = split_bars(args, &n);
        printf("DEBUG: after split_bars, n=%d\n", n); fflush(stdout);

        for (int j = 0; j < n; j++) {
            int len = strlen(p[j]);
            if (str_size + len + 1 > str_cap) { str_cap *= 2; string_table = xrealloc(string_table, str_cap); }
            memcpy(string_table + str_size, p[j], len);
            str_size += len;
            string_table[str_size++] = 0;
        }
        free_parts(p, n);
        node_count++;
        printf("DEBUG: node_count now %d\n", node_count);
    }
    printf("DEBUG: Final node_count = %d\n", node_count); fflush(stdout);

    /* Write AST file */
    FILE *out = fopen(argv[2], "wb");
    if (!out) { perror(argv[2]); return 1; }

    bx_ast_header_t hdr = { BX_AST_MAGIC, BX_AST_VERSION, node_count, str_size };
    fwrite(&hdr, sizeof(hdr), 1, out);
    fwrite(nodes, sizeof(bx_ast_node_t), node_count, out);
    fwrite(string_table, 1, str_size, out);
    fclose(out);

    /* Cleanup */
    for (int i = 0; i < line_count; i++) free(lines[i]);
    free(lines);
    free(nodes);
    free(string_table);

    return 0;
}