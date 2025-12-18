#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define BUF_SIZE 8192

typedef struct {
    int min_len;
    char offset_format; // 0 (none), 'd', 'o', 'x'
    bool print_filename;
} Config;

// Dynamic string buffer
typedef struct {
    char *data;
    size_t len;
    size_t cap;
    unsigned long long start_offset;
} StrBuf;

void init_buf(StrBuf *b) {
    b->cap = 128;
    b->data = malloc(b->cap);
    if (!b->data) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    b->len = 0;
    b->data[0] = '\0';
    b->start_offset = 0;
}

void append_buf(StrBuf *b, char c) {
    if (b->len + 1 >= b->cap) {
        b->cap *= 2;
        char *new_data = realloc(b->data, b->cap);
        if (!new_data) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        b->data = new_data;
    }
    b->data[b->len++] = c;
    b->data[b->len] = '\0';
}

void reset_buf(StrBuf *b) {
    b->len = 0;
    b->data[0] = '\0';
}

void free_buf(StrBuf *b) {
    free(b->data);
}

bool is_printable(unsigned char c) {
    return (c >= 0x20 && c <= 0x7E);
}

void print_match(const Config *cfg, const char *filename, const StrBuf *b) {
    if (cfg->print_filename) {
        printf("%s: ", filename);
    }
    if (cfg->offset_format) {
        switch (cfg->offset_format) {
            case 'd': printf("%7llu ", b->start_offset); break;
            case 'o': printf("%7llo ", b->start_offset); break;
            case 'x': printf("%7llx ", b->start_offset); break;
        }
    }
    printf("%s\n", b->data);
}

void process_file(const char *filename, const Config *cfg) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        return;
    }

    unsigned char buffer[BUF_SIZE];
    size_t n;
    unsigned long long global_offset = 0;

    // ASCII State
    StrBuf ascii_buf;
    init_buf(&ascii_buf);

    // UTF-16LE State
    StrBuf utf16_buf;
    init_buf(&utf16_buf);
    int utf16_state = 0; // 0: expect char, 1: expect null
    char utf16_pending_char = 0;
    unsigned long long utf16_pending_start = 0;

    while ((n = fread(buffer, 1, BUF_SIZE, fp)) > 0) {
        for (size_t i = 0; i < n; i++) {
            unsigned char b = buffer[i];
            unsigned long long current_pos = global_offset + i;

            // --- ASCII Logic ---
            if (is_printable(b)) {
                if (ascii_buf.len == 0) {
                    ascii_buf.start_offset = current_pos;
                }
                append_buf(&ascii_buf, (char)b);
            } else {
                if (ascii_buf.len >= cfg->min_len) {
                    print_match(cfg, filename, &ascii_buf);
                }
                reset_buf(&ascii_buf);
            }

            // --- UTF-16LE Logic ---
            // Pattern: printable + 0x00
            if (utf16_state == 0) {
                if (is_printable(b)) {
                    utf16_pending_char = (char)b;
                    utf16_pending_start = current_pos;
                    utf16_state = 1;
                } else {
                    // Not a printable char, so sequence breaks
                    if (utf16_buf.len >= cfg->min_len) {
                        print_match(cfg, filename, &utf16_buf);
                    }
                    reset_buf(&utf16_buf);
                }
            } else { // utf16_state == 1
                if (b == 0x00) {
                    // Valid pair
                    if (utf16_buf.len == 0) {
                        utf16_buf.start_offset = utf16_pending_start;
                    }
                    append_buf(&utf16_buf, utf16_pending_char);
                    utf16_state = 0;
                } else {
                    // Found something else instead of 0x00.
                    // Sequence ends.
                    if (utf16_buf.len >= cfg->min_len) {
                        print_match(cfg, filename, &utf16_buf);
                    }
                    reset_buf(&utf16_buf);
                    
                    // Re-evaluate this byte as a potential start of new char
                    utf16_state = 0;
                    if (is_printable(b)) {
                        utf16_pending_char = (char)b;
                        utf16_pending_start = current_pos;
                        utf16_state = 1;
                    }
                }
            }
        }
        global_offset += n;
    }

    // End of file cleanup
    if (ascii_buf.len >= cfg->min_len) {
        print_match(cfg, filename, &ascii_buf);
    }
    if (utf16_buf.len >= cfg->min_len) {
        print_match(cfg, filename, &utf16_buf);
    }

    free_buf(&ascii_buf);
    free_buf(&utf16_buf);
    fclose(fp);
}

void print_help(const char *prog_name) {
    printf("Usage: %s [options] <file...>\n", prog_name);
    printf("Options:\n");
    printf("  -n <number>       Specify minimum string length (default 4)\n");
    printf("  -t {o,d,x}        Print the location of the string in base 8, 10 or 16\n");
    printf("  -f                Print the name of the file before each string\n");
    printf("  -h                Display this help\n");
}

int main(int argc, char *argv[]) {
    Config cfg;
    cfg.min_len = 4;
    cfg.offset_format = 0;
    cfg.print_filename = false;

    int file_count = 0;
    char **files = malloc(argc * sizeof(char*));
    
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-n") == 0) {
                if (i + 1 < argc) {
                    cfg.min_len = atoi(argv[++i]);
                    if (cfg.min_len < 1) cfg.min_len = 1;
                } else {
                    fprintf(stderr, "Error: -n requires an argument\n");
                    return 1;
                }
            } else if (strcmp(argv[i], "-t") == 0) {
                if (i + 1 < argc) {
                    char fmt = argv[++i][0];
                    if (fmt == 'o' || fmt == 'd' || fmt == 'x') {
                        cfg.offset_format = fmt;
                    } else {
                        fprintf(stderr, "Error: Invalid radix for -t. Use o, d, or x.\n");
                        return 1;
                    }
                } else {
                    fprintf(stderr, "Error: -t requires an argument\n");
                    return 1;
                }
            } else if (strcmp(argv[i], "-f") == 0) {
                cfg.print_filename = true;
            } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                print_help(argv[0]);
                return 0;
            } else {
                // Assume it's a file if it starts with - but isn't a known flag? 
                // Standard strings treats unknown flags as errors usually.
                // But let's be safe and treat as file if it looks like a path, 
                // or error. For now, error on unknown flag.
                fprintf(stderr, "Unknown option: %s\n", argv[i]);
                print_help(argv[0]);
                return 1;
            }
        } else {
            files[file_count++] = argv[i];
        }
    }

    if (file_count == 0) {
        fprintf(stderr, "No input files specified.\n");
        print_help(argv[0]);
        return 1;
    }

    for (int i = 0; i < file_count; i++) {
        process_file(files[i], &cfg);
    }

    free(files);
    return 0;
}
