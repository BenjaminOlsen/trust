#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

static void print_usage(const char *program) {
    fprintf(stderr,
            "usage: %s [-c columns] [-n startline,endline] -f filename\n",
            program);
}

int main(int argc, char** argv) {
    size_t colwidth = 80;
    size_t startline = 0;
    size_t endline = 0;
    const char *fn = NULL;

    for (size_t argi = 1; argi < argc; ++argi) {
        if (strcmp(argv[argi], "-c") == 0) {
            if (argi + 1 >= (size_t)argc) {
                fprintf(stderr, "-c requires a column width\n");
                return 1;
            }
            char *end = NULL;
            long width = strtol(argv[++argi], &end, 10);
            if (end == argv[argi] || *end != '\0' || width <= 0) {
                fprintf(stderr, "column width must be a positive integer\n");
                return 1;
            }
            colwidth = (size_t)width;
        } else if (strcmp(argv[argi], "-n") == 0) {
            if (argi + 1 >= (size_t)argc) {
                fprintf(stderr, "-n requires startline,endline\n");
                return 1;
            }

            const char *range = argv[++argi];
            char *separator = NULL;
            char *end = NULL;
            long first = strtol(range, &separator, 10);

            if (separator == range || *separator != ',') {
                fprintf(stderr, "line range must have the form startline,endline\n");
                return 1;
            }

            long last = strtol(separator + 1, &end, 10);
            if (end == separator + 1 || *end != '\0' || first <= 0 ||
                last <= 0 || first > last) {
                fprintf(stderr,
                        "line range must contain positive integers with "
                        "startline <= endline\n");
                return 1;
            }

            startline = (size_t)first;
            endline = (size_t)last;
        } else if (strcmp(argv[argi], "-f") == 0) {
            if (argi + 1 >= (size_t)argc) {
                fprintf(stderr, "-f requires a filename\n");
                return 1;
            }
            fn = argv[++argi];
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

    if (fn == NULL) {
        print_usage(argv[0]);
        return 1;
    }

    FILE *f = fopen(fn, "rb");
    if (f == NULL) {
        perror(fn);
        return 1;
    }

    size_t capacity = 4096;
    size_t length = 0;
    char *s = malloc(capacity + 1);
    if (s == NULL) {
        perror("malloc");
        fclose(f);
        return 1;
    }

    for (;;) {
        if (length == capacity) {
            if (capacity > SIZE_MAX / 2) {
                fprintf(stderr, "file is too large to read\n");
                free(s);
                fclose(f);
                return 1;
            }
            capacity *= 2;
            char *grown = realloc(s, capacity + 1);
            if (grown == NULL) {
                perror("realloc");
                free(s);
                fclose(f);
                return 1;
            }
            s = grown;
        }

        size_t count = fread(s + length, 1, capacity - length, f);
        length += count;
        if (count == 0) {
            if (ferror(f)) {
                perror(fn);
                free(s);
                fclose(f);
                return 1;
            }
            break;
        }
    }
    fclose(f);
    s[length] = '\0';

    size_t first_byte = 0;
    size_t one_past_last_byte = length;

    if (startline != 0) {
        size_t line_count = length == 0 ? 0 : 1;
        for (size_t i = 0; i + 1 < length; ++i) {
            if (s[i] == '\n')
                ++line_count;
        }

        if (endline > line_count) {
            fprintf(stderr,
                    "line range %zu,%zu is outside the file's %zu lines\n",
                    startline, endline, line_count);
            free(s);
            return 1;
        }

        size_t current_line = 1;
        for (size_t i = 0; i < length && current_line < startline; ++i) {
            if (s[i] == '\n') {
                ++current_line;
                first_byte = i + 1;
            }
        }

        current_line = startline;
        one_past_last_byte = length;
        for (size_t i = first_byte; i < length; ++i) {
            if (s[i] == '\n' && current_line == endline) {
                one_past_last_byte = i + 1;
                break;
            }
            if (s[i] == '\n')
                ++current_line;
        }
    }

    for (size_t i = first_byte; i < one_past_last_byte; ++i) {
        size_t output_index = i - first_byte;
        if (output_index > 0) {
            if (output_index % colwidth == 0)
                printf(",\n");
            else
                printf(", ");
        }
        printf("%u", (unsigned char)s[i]);
    }
    printf("\n");
    free(s);
    return 0;
}
