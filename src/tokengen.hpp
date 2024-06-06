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

#pragma once
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <memory>

#include "token.hpp"

#define LINELEN 256

// ソースコードからトークンを生成するクラス
class BasTokenGen {
public:
    BasTokenGen(std::FILE *fh = stdin, int cindent = -1, bool verbose = false);

    void setpass(int bpass) { this->bpass = bpass; }
    void rewind();
    int getgolineno();
    std::unique_ptr<std::string> getlineno();
    std::unique_ptr<std::string> getccode();
    std::unique_ptr<std::string> getbascmnline(const char *line);

    std::unique_ptr<BasToken> fetch();
    void unfetch(std::unique_ptr<BasToken> t);
    void skip();

    int baslineno;
    char curline[LINELEN];
    int prelen;
    bool nocomment;

private:
    bool readline();
    char *getline();
    std::unique_ptr<BasToken> get();

    int bpass;
    int cindent;
    bool verbose;

    FILE *fh;
    char *filebuf;
    int fp;

    char line[LINELEN];

    int lineno;
    int golineno;
    bool firsttoken;

    std::vector<std::unique_ptr<BasToken>> cached;
    std::string ccode;
    int curlen;
};
