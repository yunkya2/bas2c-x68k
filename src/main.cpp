/*
 * X-BASIC to C converter bas2c
 * Copyright (c) 2024 Yuichi Nakamura (@yunkya2)
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <cstdio>
#include <cstdlib>

#include "variable.hpp"
#include "namespace.hpp"
#include "tokengen.hpp"
#include "bas2c.hpp"
#include "keyword.hpp"

#ifndef GIT_REPO_VERSION
#define GIT_REPO_VERSION ""
#endif

static void readdef(const char *cmd) {
    std::FILE *fh;
    char bas2cdef[256] = "\0";
    char *p = bas2cdef;

#ifdef X68k
    extern const char *_MCB;
    std::strcpy(bas2cdef, &_MCB[128]);
    p = &bas2cdef[std::strlen(bas2cdef)];
#endif

    std::strcpy(p, "bas2c.def");
    fh = std::fopen(bas2cdef, "r");
    if (fh == nullptr) {
        std::strcpy(p, "BC.DEF");
        fh = std::fopen(bas2cdef, "r");
        if (fh == nullptr) {
            std::fprintf(stderr, "%s: cannot load bas2c.def\n", cmd);
            return;
        }
    }
    BasKeyword::exfninit(fh);
    std::fclose(fh);
}

void usage(const char *cmd) {
    std::fprintf(stderr, "usage: %s [-Dunbv][-c[tabs]][-o output.c] input.bas\n", cmd);
    std::exit(1);
}

int main(int argc, char **argv) {
    int flag = 0;
    int cindent = 0;
    std::FILE *fh = nullptr;
    std::FILE *fo = nullptr;
    char *finame = nullptr;
    char *foname = nullptr;

#ifdef X68k
    std::printf("X-BASIC to C Converter bas2c " GIT_REPO_VERSION " Copyright 2024 Y.Nakamura\n");
#endif

    for (int i = 1; i < argc; i++) {
        if ((argv[i][0] == '-' || argv[i][0] == '/') && argv[i][1] != '\0') {
            switch (argv[i][1]) {
            case 'D':
                flag |= Bas2C::F_DEBUG;
                break;
            case 'u':
                flag |= Bas2C::F_UNDEFERR;
                break;
            case 'n':
                flag |= Bas2C::F_NOBINIT;
                break;
            case 'v':
                flag |= Bas2C::F_VERBOSE;
                break;
            case 'b':
                flag |= Bas2C::F_BCCOMPAT;
                break;
            case 'c':
                flag |= Bas2C::F_BASCOMMENT;
                {
                    char *p;
                    long num = std::strtol(&argv[i][2], &p, 10);
                    if (p != &argv[i][2]) {
                        cindent = num;
                    } else {
                        cindent = 7;
                    }
                }
                break;
            case 'o':
                foname = argv[++i];
                break;
            default:
                usage(argv[0]);
            }
        } else {
            if (!finame) {
                finame = argv[i];
            } else if (!foname) {
                foname = argv[i];
            }
        }
    }

    if (!finame) {
        usage(argv[0]);
    }

    char name[256];
    if ((std::strcmp(finame, "-") != 0) && !foname) {
        std::strcpy(name, finame);
        auto p = std::strrchr(name, '.');
        if (p) {
            *p = '\0';
        }
        std::strcat(name, ".c");
        foname = name;
    }

    fh = (finame && std::strcmp(finame, "-")) ? std::fopen(finame, "r") : stdin;
    if (fh == nullptr) {
        std::fprintf(stderr, "%s: %s file not found\n", argv[0], finame);
        std::exit(1);
    }

    fo = (foname && std::strcmp(foname, "-")) ? std::fopen(foname, "w") : stdout;
    if (fo == nullptr) {
        std::fprintf(stderr, "%s: cannot create output file %s\n", argv[0], foname);
        std::exit(1);
    }

    readdef(argv[0]);
    auto b = Bas2C(fh, flag, cindent);
    auto status = b.start(fo, finame ? finame : "<stdin>");

    std::fclose(fh);
    std::fclose(fo);
    std::exit(status);
}
