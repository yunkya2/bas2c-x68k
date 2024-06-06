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

#include <cstring>
#include <memory>
#include <vector>
#include <algorithm>

#include "bas2c.hpp"
#include "variable.hpp"
#include "keyword.hpp"
#include "namespace.hpp"

Bas2C::Bas2C(std::FILE *fh, int flag, int cindent) : 
fh(fh), flag(flag), cindent(cindent) {
    t = new BasTokenGen(fh, (flag & F_BASCOMMENT) ? cindent : -1, flag & F_VERBOSE);
    label.clear();
    subr.clear();
    nsp = new BasNameSpace();
    strtmp = 0;
    strtmp_max = 0;
    exfngroup.clear();
    setpass(0);
    b_exit = (flag & F_NOBINIT) ? "exit" : "b_exit";
    exitstatus = 0;
}

Bas2C::~Bas2C() {
    delete t;
    delete nsp;
}

// 変換パスを設定する
void Bas2C::setpass(int bpass) {
    this->bpass = bpass;
    updatestrtmp();
    nsp->setpass(bpass);
    nsp->setlocal("");
    initmp = 0;
    nest.clear();
    nest.push_back('M');
    indentcnt = 0;
    t->setpass(bpass);
    t->rewind();
}

// 文字列処理用一時変数の最大数を更新する
void Bas2C::updatestrtmp() {
    strtmp_max = std::max(strtmp_max, strtmp);
    strtmp = 0;
}

//////////////////////////////////////////////////////////////////////////////

// ネストのエラーメッセージを返す
static const char *nesterrmsg(char type) {
    switch (type) {
    case 'f':
        return "for - next の対応に誤りがあります";
    case 'w':
        return "while - endwhile の対応に誤りがあります";
    case 'r':
        return "repeat - until の対応に誤りがあります";
    case 's':
        return "switch - endswitch の対応に誤りがあります";
    case 'F':
        return "func - endfunc の対応に誤りがあります";
    case 'i':
    case 'I':
    case 'e':
    case 'E':
        return "if - then - else の対応に誤りがあります";
    default:
        return "ネストの対応に誤りがあります";
    }
}

// ネストを深くする
void Bas2C::nestin(char type) {
    nest.push_back(type);
}

// ネストを浅くする
void Bas2C::nestout(char type) {
    expect(nest.back() == type, nesterrmsg(type));
    nest.pop_back();
    indentcnt--;
}

// 必要なら関数末尾の括弧を閉じる
std::unique_ptr<std::string> Bas2C::nestclose() {
    std::string r;
    if (nest.size() == 1 && nest.back() == 'M') {
        // ENDを実行せずにmain関数が終了したら b_exit(0) を呼ぶ
        r = *indentout() + std::string(b_exit) + "(0);\n}\n";
        nestout('M');
    } else if (nest.size() == 1 && nest.back() == 'S') {
        // サブルーチンの括弧を閉じる
        nestout('S');
        r = "}\n";
    } else {
        // 関数定義が閉じていることを確認する
        if (nest.size() > 0) {
            expect(false, nesterrmsg(nest.back()));
            nest.clear();
        }
    }
    return std::make_unique<std::string>(r);
}

// インデントを返す
std::unique_ptr<std::string> Bas2C::indentout() {
    std::string r;
    for (auto i = 0; i < indentcnt; i++) {
        r += "\t";
    }
    return std::make_unique<std::string>(r);
}

// グローバル変数、関数の定義を出力する
std::unique_ptr<std::string> Bas2C::gendefine() {
    auto r = nsp->definition();
    for (const auto& l : subr) {           // サブルーチンのプロトタイプを出力する
        char buffer[10];
        std::snprintf(buffer, sizeof(buffer), "S%06d", l);
        r += "void " + std::string(buffer) + "(void);\n";
    }
    return std::make_unique<std::string>(r);
}

// 必要ならGOTO飛び先のラベル定義、GOSUB飛び先の関数定義を出力する
std::unique_ptr<std::string> Bas2C::genlabel() {
    if (auto l = t->getgolineno()) {
        if (label.find(l) != label.end()) {
            char buffer[10];
            std::snprintf(buffer, sizeof(buffer), "L%06d", l);
            return std::make_unique<std::string>(buffer + std::string(":\n"));
        } else if (subr.find(l) != subr.end()) {
            t->nocomment = false;
            std::string r = *nestclose();
            nestin('S');
            r += "\n/***************************/\n";
            indentcnt++;
            char buffer[10];
            std::snprintf(buffer, sizeof(buffer), "S%06d", l);
            r += "void " + std::string(buffer) + "(void)\n{\n";
            return std::make_unique<std::string>(r);
        }
    }
    return std::make_unique<std::string>("");
}

//////////////////////////////////////////////////////////////////////////////

#if !defined(UNITTEST) || defined(UNITTEST_BAS2C)
int Bas2C::start(std::FILE *fo, const char *finame) {
    // pass 1
    setpass(1);
    while (true) {
        try {
            auto s = statement();
            if (!s) {
                break;
            }
        } catch (const BasNameSpaceException& e) {
            error(e.what(), finame);
        } catch (const Bas2CException& e) {
        }
    }

    // pass 2
    setpass(2);
    std::fprintf(fo, "#include <basic0.h>\n");
    std::fprintf(fo, "#include <string.h>\n");
    if (flag & F_NOBINIT) {
        std::fprintf(fo, "#include <stdlib.h>\n");
    }
    for (auto e : exfngroup) {
        std::transform(e.begin(), e.end(), e.begin(), ::tolower);
        std::fprintf(fo, "#include <%s.h>\n", e.c_str());
    }
    std::fprintf(fo, "\n%s", gendefine()->c_str());
    for (auto i = 0; i < strtmp_max; i++) {
        std::fprintf(fo, "static unsigned char strtmp%d[258];\n", i);
    }
    std::fprintf(fo, "\n/******** program start ********/\n");
    std::fprintf(fo, "void main(int b_argc, char *b_argv[])\n{\n");
    if (!(flag & F_NOBINIT)) {
        std::fprintf(fo, "\tb_init();\n");
    }
    while (true) {
        try {
            indentinit();
            auto s = statement();
            std::fprintf(fo, "%s", t->getccode()->c_str());
            std::fprintf(fo, "%s", genlabel()->c_str());
            if (!s) {
                break;
            }

            auto indent = indentout();
            size_t pos = 0;
            size_t next;
            while ((next = s->find('\n', pos)) != std::string::npos) {
                auto l = *indent + s->substr(pos, next - pos) + "\n";
                std::fprintf(fo, "%s", l.c_str());
                pos = next + 1;
            }
        } catch (const Bas2CException &e) {
            error(e.what(), finame);
        }
    }
    try {
        std::fprintf(fo, "%s", nestclose()->c_str());
    } catch (const Bas2CException& e) {
        error(e.what(), finame);
    }

    return exitstatus;
}

void Bas2C::error(const char *msg, const char *finame) {
    exitstatus = 1;
    std::fprintf(stderr, "%s:%s: error: %s\n", finame, t->getlineno()->c_str(), msg);
    if (std::strlen(t->curline)) {
        std::fprintf(stderr, "%s", t->curline);
        for (auto i = 0; i < std::strlen(t->curline) - t->prelen; i++) {
            std::fputc(' ', stderr);
        }
        std::fprintf(stderr, "^\n");
    }
    t->skip();
}
#endif

//////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG

#define UNITTEST_BAS2C
#undef DEBUG
#include <iostream>
#include "tokengen.cpp"
#include "namespace.cpp"
#include "bas2cexpr.cpp"
#include "bas2cstmt.cpp"
#define DEBUG

int main() {
    auto b = Bas2C(stdin, 0, 0);
    b.start();
}

#endif
