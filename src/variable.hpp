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
#include <string>

#include "keyword.hpp"

// 変数/関数の型と名前を保持するクラス
class BasVariable {
public:
    // 変数型 (関数の場合は戻り値の型)
    enum Type {
        INT             = BasKeyword::INT,      // 1
        CHAR            = BasKeyword::CHAR,     // 2 
        FLOAT           = BasKeyword::FLOAT,    // 3
        STR             = BasKeyword::STR,      // 4

        DIM             = 0x10,
        DIM_INT         = DIM + INT,            // 0x11
        DIM_CHAR        = DIM + CHAR,           // 0x12
        DIM_FLOAT       = DIM + FLOAT,          // 0x13 
        DIM_STR         = DIM + STR,            // 0x14

        STATICCONST     = 0x20
    };

    BasVariable(std::string name, Type type, std::string arg = "", std::string init = "", bool func = false, bool funcarg = false)
        : name(name), type(type), arg(arg), init(init), func(func), funcarg(funcarg) {}

    bool isstr()   { return type == STR; }
    bool isarray() { return type >= DIM; }

    static bool isstr(BasVariable::Type t)   { return t == STR; }
    static bool isarray(BasVariable::Type t) { return t >= DIM; }
    static BasVariable::Type toarray(BasVariable::Type t) { return static_cast<BasVariable::Type>(t | DIM); }
    static BasVariable::Type toconst(BasVariable::Type t) { return static_cast<BasVariable::Type>(t | STATICCONST); }
    static BasVariable::Type basetype(BasVariable::Type t) { return static_cast<BasVariable::Type>(t & 0xf); }

    const char *typname(bool fnres = false);
    const char *typqual(bool globl = false);
    std::string definition(bool globl = false);

    std::string name;       // 変数名/関数名
    Type type;              // 型 (関数の場合は戻り値の型)
    std::string arg;        // 関数の引数 (配列の場合は要素数)
    std::string init;       // 初期値
    bool func;              // 関数ならtrue
    bool funcarg;           // 関数の仮引数ならtrue
private:
};
