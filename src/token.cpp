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

#include "token.hpp"

// トークン同士の演算結果に与える型を得る(strはエラー)
BasToken::TokenType BasToken::resulttype(BasToken* a) {
    if (type == STR) {
        return static_cast<BasToken::TokenType>(0);
    }
    auto rty = (type != CHAR) ? type : INT;     // char->intとする
    if (a != nullptr) {
        if (a->type == STR) {
            return static_cast<BasToken::TokenType>(0);
        }
        auto aty = (a->type != CHAR) ? a->type : INT; // char->intとする
        rty = (rty == aty) ? rty : FLOAT;       // intとfloatの演算結果はfloat
    }
    return rty;
}

#if defined(DEBUG) || defined(UNITTEST)

#include <iostream>

// トークンを表示 (デバッグ用)
std::ostream& operator<<(std::ostream& os, const BasToken& token) {
    switch (token.type) {
    case BasToken::SYMBOL:
        if (std::isprint(token.ivalue)) {
            os << "(" << token.type << "," << static_cast<char>(token.ivalue) << ")";
        } else {
            os << "(" << token.type << "," << token.ivalue << ")";
        }
        break;
    case BasToken::KEYWORD:
        os << "(" << token.type << "," << token.ivalue << ")";
        break;
    default:
        os << "(" << token.type << "," << token.value <<  ")";
        break;
    }

    return os;
}

#endif

//////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG

#include <iostream>

int main()
{
    auto t1 = BasToken::makesymbol('=');
    auto t2 = BasToken::makeint("1234");

    std::cout << *t1 << std::endl;
    std::cout << *t2 << std::endl;
}

#endif