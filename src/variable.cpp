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

#include "variable.hpp"

// 変数型を文字列で返す
const char *BasVariable::typname(bool fnres) {
    Type ty = basetype(type);
    if (fnres && type == STR) {
        return "unsigned char *";       // strを返す関数の戻り値型
    }
    switch (ty) {
    case INT:
        return "int";
    case CHAR:
        return "unsigned char";
    case FLOAT:
        return "double";
    case STR:
        return "unsigned char";
    default:
        return "";
    }
}

// 変数型修飾子を文字列で返す
const char *BasVariable::typqual(bool globl) {
    if (type >= STATICCONST) {
        return "static const ";
    }
    if (globl) {
        return "static ";
    }
    return "";
}

// 変数定義/関数宣言を得る
std::string BasVariable::definition(bool globl) {
    if (funcarg) {
        return "";              // 関数の仮引数の定義は出力しない
    }
    if (func) {
        return std::string(typname(true)) + " " + name + "(" + arg + ");\n";
    } else {
        auto r = std::string(typqual(globl)) + typname() + " " + name + arg;
        if (init != "") {
            r += " = " + init;
        }
        return r + ";\n";
    }
}

//////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG

#undef DEBUG
#include <iostream>
#define DEBUG

int main() {
    BasVariable v1("a1", BasVariable::INT);
    BasVariable v2("a2", BasVariable::FLOAT, "", "1234");
    BasVariable v3("a3", BasVariable::DIM_CHAR, "[10]");
    BasVariable f("func", BasVariable::INT, "int a, int b", "", true);

    std::cout << v1.definition();
    std::cout << v2.definition();
    std::cout << v3.definition();
    std::cout << f.definition();
    std::cout << "v1.isarray=" << v1.isarray() << std::endl;
    std::cout << "v3.isarray=" << v3.isarray() << std::endl;
}

#endif