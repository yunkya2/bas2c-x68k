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
#include <memory>

#include "keyword.hpp"

// X-BASICのトークン
class BasToken {
public:
    enum TokenType {
        SYMBOL          = 0,
        INT             = BasKeyword::INT,      // 1
        CHAR            = BasKeyword::CHAR,     // 2 
        FLOAT           = BasKeyword::FLOAT,    // 3
        STR             = BasKeyword::STR,      // 4
        KEYWORD         = 5,
        VARIABLE        = 6,
        FUNCTION        = 7,
        COMMENT         = 8,
        ERROR           = -1
    };

    BasToken(TokenType type, std::string value) : type(type), value(value) {}
    BasToken(TokenType type, int ivalue) : type(type), ivalue(ivalue) {}
    ~BasToken() {}

    static std::unique_ptr<BasToken> make(TokenType type, std::string value) { return std::make_unique<BasToken>(type, value); }
    static std::unique_ptr<BasToken> make(TokenType type, int ivalue) { return std::make_unique<BasToken>(type, ivalue); }

    static std::unique_ptr<BasToken> makesymbol(int ivalue) { return std::make_unique<BasToken>(SYMBOL, ivalue); }
    static std::unique_ptr<BasToken> makeint(std::string value) { return std::make_unique<BasToken>(INT, value); }
    static std::unique_ptr<BasToken> makefloat(std::string value) { return std::make_unique<BasToken>(FLOAT, value); }
    static std::unique_ptr<BasToken> makestr(std::string value) { return std::make_unique<BasToken>(STR, value); }
    static std::unique_ptr<BasToken> makekeyword(int ivalue) { return std::make_unique<BasToken>(KEYWORD, ivalue); }
    static std::unique_ptr<BasToken> makevariable(std::string value) { return std::make_unique<BasToken>(VARIABLE, value); }
    static std::unique_ptr<BasToken> makefunction(std::string value) { return std::make_unique<BasToken>(FUNCTION, value); }
    static std::unique_ptr<BasToken> makecomment(std::string value) { return std::make_unique<BasToken>(COMMENT, value); }

    bool isconst()            { return type >= INT && type <= STR; }
    bool istype(TokenType ty) { return type == ty; }
    bool issymbol(int c)      { return type == SYMBOL && this->ivalue == c; }
    bool iskeyword(int kw)    { return type == KEYWORD && this->ivalue == kw; }
    bool isvartype()          { return type == KEYWORD && (ivalue >= BasKeyword::INT && ivalue <= BasKeyword::STR); }

    TokenType resulttype(BasToken* a = nullptr);

    friend std::ostream& operator<<(std::ostream& os, const BasToken& token);

    TokenType type;         // トークンのタイプ
    union {
        std::string value;  // トークン値の文字列
        int ivalue;         // (SYMBOL,KEYWORDの場合)トークン値の整数値
    };
private:
};
