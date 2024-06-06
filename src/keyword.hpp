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
#include <map>
#include <string>

// 組込/外部関数定義情報を保持するクラス
class BasExFunc {
public:
    BasExFunc(std::string type, std::string name, std::string arg, std::string cfunc, std::string carg, std::string group)
        : type(type), name(name), arg(arg), cfunc(cfunc), carg(carg), group(group) {}

    std::string type;       // 戻り値の型
    std::string name;       // 関数名
    std::string arg;        // 引数の型
    std::string cfunc;      // C関数名
    std::string carg;       // C引数の型
    std::string group;      // グループ名 (BASIC/MOUSE/STICK/..)
};

// X-BASICの予約語
class BasKeyword {
public:
    enum Keyword {
        EOF_KW      = 0,

        INT         = 1,
        CHAR        = 2,
        FLOAT       = 3,
        STR         = 4,
        DIM         = 5,

        PLUS        = 1001,
        MINUS       = 1002,
        MUL         = 1003,
        DIV         = 1004,
        YEN         = 1005,
        MOD         = 1006,
        SHR         = 1007,
        SHL         = 1008,
        EQ          = 1009,
        NE          = 1010,
        GT          = 1011,
        LT          = 1012,
        GE          = 1013,
        LE          = 1014,
        NOT         = 1015,
        AND         = 1016,
        OR          = 1017,
        XOR         = 1018,

        PRINT       = 2001,
        FOR         = 2002,
        TO          = 2003,
        NEXT        = 2004,
        GOTO        = 2005,
        GOSUB       = 2006,
        IF          = 2007,
        THEN        = 2008,
        ELSE        = 2009,
        END         = 2010,
        RETURN      = 2011,
        FUNC        = 2012,
        ENDFUNC     = 2013,
        WHILE       = 2014,
        ENDWHILE    = 2015,
        REPEAT      = 2016,
        UNTIL       = 2017,
        BREAK       = 2018,
        CONTINUE    = 2019,
        SWITCH      = 2020,
        CASE        = 2021,
        DEFAULT     = 2022,
        ENDSWITCH   = 2023,
        INPUT       = 2024,
        LINPUT      = 2025,
        LPRINT      = 2026,
        USING       = 2027,
        TAB         = 2028,
        LOCATE      = 2029,
        ERROR       = 2030,

        EOL         = 9999,

        NONE        = -1
    };

    static const Keyword find(const std::string& str);
    static const Keyword findop(const char* line, int& len);
    static const char* getkeyword(int kw);
    static void exfninit(std::FILE *fh);
    static std::map<int, BasExFunc*> exfnlist;

private:
    static std::map<const std::string, Keyword> keyword;
    static const std::map<const std::string, Keyword> keywordop;
};
