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
#include <cctype>
#include <algorithm>

#include "keyword.hpp"

// wordが予約語ならその値を返す
const BasKeyword::Keyword BasKeyword::find(const std::string& str) {
    auto s = str;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    auto it = keyword.find(s);
    return (it != keyword.end()) ? it->second : NONE;
}

// wordの先頭に演算子があればそれと残りの文字列を返す
const BasKeyword::Keyword BasKeyword::findop(const char* line, int& len) {
    auto s = std::string(line, 2);
    auto it = keywordop.find(s);
    if (it != keywordop.end()) {
        len = s.length();
        return it->second;
    }

    s = std::string(line, 1);
    it = keywordop.find(s);
    if (it != keywordop.end()) {
        len = s.length();
        return it->second;
    }
    return NONE;
}

// kwから予約語を得る
const char* BasKeyword::getkeyword(int kw) {
    for (const auto& it : keyword) {
        if (it.second == kw) {
            return it.first.c_str();
        }
    }
    for (const auto& it : keywordop) {
        if (it.second == kw) {
            return it.first.c_str();
        }
    }
    return nullptr;
}

// X-BASIC予約語テーブル
std::map<const std::string, BasKeyword::Keyword> BasKeyword::keyword = {
    {"print",       PRINT},
    {"for",         FOR},
    {"to",          TO},
    {"next",        NEXT},
    {"goto",        GOTO},
    {"gosub",       GOSUB},
    {"if",          IF},
    {"then",        THEN},
    {"else",        ELSE},
    {"end",         END},
    {"return",      RETURN},
    {"func",        FUNC},
    {"endfunc",     ENDFUNC},
    {"while",       WHILE},
    {"endwhile",    ENDWHILE},
    {"repeat",      REPEAT},
    {"until",       UNTIL},
    {"break",       BREAK},
    {"continue",    CONTINUE},
    {"switch",      SWITCH},
    {"case",        CASE},
    {"default",     DEFAULT},
    {"endswitch",   ENDSWITCH},
    {"input",       INPUT},
    {"linput",      LINPUT},
    {"lprint",      LPRINT},
    {"using",       USING},
    {"tab",         TAB},
    {"locate",      LOCATE},
    {"error",       ERROR},

    {"int",         INT},
    {"char",        CHAR},
    {"float",       FLOAT},
    {"str",         STR},
    {"dim",         DIM},

    {"mod",         MOD},
    {"shr",         SHR},
    {"shl",         SHL},
    {"not",         NOT},
    {"and",         AND},
    {"or",          OR},
    {"xor",         XOR}
};

// X-BASIC演算子テーブル
const std::map<const std::string, BasKeyword::Keyword> BasKeyword::keywordop = {
    {"?",           BasKeyword::PRINT},
    {"+",           BasKeyword::PLUS},
    {"-",           BasKeyword::MINUS},
    {"*",           BasKeyword::MUL},
    {"/",           BasKeyword::DIV},
    {"\\",          BasKeyword::YEN},
    {"=",           BasKeyword::EQ},
    {"<>",          BasKeyword::NE},
    {">=",          BasKeyword::GE},
    {"<=",          BasKeyword::LE},
    {">",           BasKeyword::GT},
    {"<",           BasKeyword::LT}
};

// 組込/外部関数定義情報
std::map<int, BasExFunc*> BasKeyword::exfnlist;

// 組込/外部関数定義情報をファイルから読み込む
void BasKeyword::exfninit(std::FILE *fh)
{
    std::string grp;
    auto w = 5000;
    char line[256];

    while (std::fgets(line, sizeof(line), fh) != nullptr) {
        char work[sizeof(line)];
        auto p = line;
        auto q = work;

        // グループ名 [...] を得る
        if (*p == '[') {
            p++;
            q = work;
            while (*p) {
                if (*p == ']') {
                    *q = '\0';
                    grp = std::string(work);
                    break;
                }
                *q++ = *p++;
            }
            continue;
        }

        // 組込/外部関数定義を得る

        // 戻り値の型 (あれば)
        std::string type;
        if (isalpha(*p)) {
            type = *p++;
        }
        while (isspace(*p)) {
            p++;
        }

        // X-BASIC関数名
        std::string name;
        if (isalpha(*p) || *p == '_') {
            q = work;
            while (isalnum(*p) || std::strchr("_$", *p)) {
                *q++ = *p++;
            }
            *q = '\0';
            name = std::string(work);
        } else {
            continue;
        }
        while (isspace(*p)) {
            p++;
        }

        // X-BASIC関数引数
        std::string arg;
        q = work;
        if (std::strchr("([", *p)) {
            *q++ = *p++;
        }
        while (isalnum(*p) || std::strchr(",-", *p)) {
            *q++ = *p++;
        }
        if (std::strchr(")]", *p)) {
            *q++ = *p++;
        }
        *q = '\0';
        arg = std::string(work);
        while (isspace(*p)) {
            p++;
        }

        // 区切り文字 ':'
        if (*p != ':') {
            continue;
        }
        p++;
        while (isspace(*p)) {
            p++;
        }

        // C関数名 (X-BASICの関数名と異なる場合)
        std::string cfunc;
        if (isalpha(*p) || *p == '_') {
            q = work;
            while (isalnum(*p) || *p == '_') {
                *q++ = *p++;
            }
            *q = '\0';
            cfunc = std::string(work);
        }

        // C関数引数
        if (*p != '(') {
            continue;
        }
        p++;
        std::string carg;
        q = work;
        while (std::strchr("#@&$%,", *p)) {
            *q++ = *p++;
        }
        *q = '\0';
        carg = std::string(work);
        if (*p != ')') {
            continue;
        }

        // 得られた定義情報を登録
        auto f = new BasExFunc(type, name, arg, cfunc, carg, grp);
        exfnlist[w] = f;
        keyword[name] = static_cast<BasKeyword::Keyword>(w++);
    }
}

//////////////////////////////////////////////////////////////////////////////

#include <iostream>

#ifdef DEBUG

int main() {
    std::FILE *fh = std::fopen("bas2c.def", "r");

    BasKeyword::exfninit(fh);
    for (const auto& it : BasKeyword::exfnlist) {
        auto m = it.second;
        std::cout << "[" + std::to_string(it.first) + "]"
                  << "type:" << m->type
                  << "\tname:" << m->name
                  << "\targ:" << m->arg
                  << " cfunc:" << m->cfunc
                  << " carg:" << m->carg
                  << " group:" << m->group
                  << std::endl;
    }

    std::fclose(fh);
}

#endif