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

#include "bas2c.hpp"
#include "keyword.hpp"

// 式を解析、変換してトークンで返す
std::unique_ptr<BasToken> Bas2C::expr() {
    return opxor();
}

std::unique_ptr<BasToken> Bas2C::opxor() {
    auto r = opor();
    if (!r) {
        return nullptr;
    }
    while (checkkeyword(BasKeyword::XOR)) {
        auto a = expect(opand());
        expect(r->resulttype(a.get()));
        if (!(flag & F_BCCOMPAT)) {
            r = BasToken::makeint("((int)" + r->value + " ^ (int)" + a->value + ")");
        } else {
            r = BasToken::makeint(r->value + " ^ " + a->value);
        }
    }
    return r;
}

std::unique_ptr<BasToken> Bas2C::opor() {
    auto r = opand();
    if (!r) {
        return nullptr;
    }
    while (checkkeyword(BasKeyword::OR)) {
        auto a = expect(opand());
        expect(r->resulttype(a.get()));
        if (!(flag & F_BCCOMPAT)) {
            r = BasToken::makeint("((int)" + r->value + " | (int)" + a->value + ")");
        } else {
            r = BasToken::makeint(r->value + " | " + a->value);
        }
    }
    return r;
}

std::unique_ptr<BasToken> Bas2C::opand() {
    auto r = opnot();
    if (!r) {
        return nullptr;
    }
    while (checkkeyword(BasKeyword::AND)) {
        auto a = expect(opnot());
        expect(r->resulttype(a.get()));
        if (!(flag & F_BCCOMPAT)) {
            r = BasToken::makeint("((int)" + r->value + " & (int)" + a->value + ")");
        } else {
            r = BasToken::makeint(r->value + " & " + a->value);
        }
    }
    return r;
}

std::unique_ptr<BasToken> Bas2C::opnot() {
    auto m = t->fetch();
    const char *ms = nullptr;
    if (m->iskeyword(BasKeyword::NOT)) {
        auto r = expect(opnot());
        expect(r->resulttype());
        if (!(flag & F_BCCOMPAT)) {
            return BasToken::makeint("(~((int)" + r->value + "))");
        } else {
            return BasToken::makeint("!" + r->value);
        }
    } else {
        t->unfetch(std::move(m));
        return cmp();
    }
}

std::unique_ptr<BasToken> Bas2C::cmp() {
    auto r = shrshl();
    if (!r) {
        return nullptr;
    }
    while (true) {
        auto m = t->fetch();
        const char *ms = nullptr;
        const char *mt;
        if (m->iskeyword(BasKeyword::EQ)) {
            ms = "==";
            mt = "0x3d20";
        } else if (m->iskeyword(BasKeyword::NE)) {
            ms = "!=";
            mt = "0x3c3e";
        } else if (m->iskeyword(BasKeyword::GT)) {
            ms = ">";
            mt = "0x3e20";
        } else if (m->iskeyword(BasKeyword::LT)) {
            ms = "<";
            mt = "0x3c20";
        } else if (m->iskeyword(BasKeyword::GE)) {
            ms = ">=";
            mt = "0x3e3d";
        } else if (m->iskeyword(BasKeyword::LE)) {
            ms = "<=";
            mt = "0x3c3d";
        } else {
            t->unfetch(std::move(m));
            break;
        }
        auto a = expect(shrshl());
        std::string v;
        if (r->istype(BasToken::STR)) {
            expect(a->istype(BasToken::STR));
            v = "b_strcmp(" + r->value + ", " + std::string(mt) + ", " + a->value + ")";
            if (!(flag & F_BCCOMPAT)) {
                v = "((" + v + ")?-1:0)";
            }
        } else {
            v = r->value + " " + ms + " " + a->value;
            if (!(flag & F_BCCOMPAT)) {
                v = "-(" + v + ")";
            }
        }
        r = BasToken::makeint(v);
    }
    return r;
}

std::unique_ptr<BasToken> Bas2C::shrshl() {
    auto r = addsub();
    if (!r) {
        return nullptr;
    }
    while (true) {
        auto m = t->fetch();
        const char *ms = nullptr;
        if (m->iskeyword(BasKeyword::SHR)) {
            ms = ">>";
        } else if (m->iskeyword(BasKeyword::SHL)) {
            ms = "<<";
        } else {
            t->unfetch(std::move(m));
            break;
        }
        auto a = expect(addsub());
        expect(r->resulttype());
        if (!(flag & F_BCCOMPAT)) {
            r = BasToken::makeint("((int)" + r->value + " " + ms + " (int)" + a->value + ")");
        } else {
            r = BasToken::makeint(r->value + " " + ms + " " + a->value);
        }
    }
    return r;
}

std::unique_ptr<BasToken> Bas2C::addsub() {
    auto r = mod();
    if (!r) {
        return nullptr;
    }
    if (r->istype(BasToken::STR)) {                 // 文字列の連結
        if (!checkkeyword(BasKeyword::PLUS)) {
            return r;
        }
        r = BasToken::makestr("b_stradd(strtmp" + std::to_string(strtmp) + ", " + r->value + ", ");
        strtmp++;
        while (true) {
            auto a = expect(mod());
            expect(a->istype(BasToken::STR));
            r->value += a->value + ", ";
            if (!checkkeyword(BasKeyword::PLUS)) {
                break;
            }
        }
        r->value += "-1)";
        return r;
    } else {
        while (true) {
            auto m = t->fetch();
            const char *ms = nullptr;
            if (m->iskeyword(BasKeyword::PLUS)) {
                ms = "+";
            } else if (m->iskeyword(BasKeyword::MINUS)) {
                ms = "-";
            } else {
                t->unfetch(std::move(m));
                break;
            }
            auto a = expect(mod());
            auto rty = static_cast<BasToken::TokenType>(r->resulttype(a.get()));
            auto v = r->value + " " + ms + " " + a->value;
            if (!(flag & F_BCCOMPAT)) {
                v = "(" + v + ")";
            }
            r = BasToken::make(rty, v);
        }
    }
    return r;
}

std::unique_ptr<BasToken> Bas2C::mod() {
    auto r = yen();
    if (!r) {
        return nullptr;
    }
    while (checkkeyword(BasKeyword::MOD)) {
        auto a = expect(yen());
        expect(r->resulttype());
        if (!(flag & F_BCCOMPAT)) {
            r = BasToken::makeint("((int)" + r->value + " % (int)" + a->value + ")");
        } else {
            r = BasToken::makeint(r->value + " % " + a->value);
        }
    }
    return r;
}

std::unique_ptr<BasToken> Bas2C::yen() {
    auto r = muldiv();
    if (!r) {
        return nullptr;
    }
    while (checkkeyword(BasKeyword::YEN)) {
        auto a = expect(muldiv());
        expect(r->resulttype());
        if (!(flag & F_BCCOMPAT)) {
            r = BasToken::makeint("((int)" + r->value + " / (int)" + a->value + ")");
        } else {
            r = BasToken::makeint(r->value + " / " + a->value);
        }
    }
    return r;
}

std::unique_ptr<BasToken> Bas2C::muldiv() {
    auto r = posneg();
    if (!r) {
        return nullptr;
    }
    while (true) {
        auto m = t->fetch();
        const char *ms = nullptr;
        if (m->iskeyword(BasKeyword::MUL)) {
            ms = "*";
        } else if (m->iskeyword(BasKeyword::DIV)) {
            ms = "/";
        } else {
            t->unfetch(std::move(m));
            break;
        }
        auto a = expect(posneg());
        auto rty = static_cast<BasToken::TokenType>(r->resulttype(a.get()));
        auto v = r->value + " " + ms + " " + a->value;
        if (!(flag & F_BCCOMPAT)) {
            v = "(" + v + ")";
        }
        r = BasToken::make(rty, v);
    }
    return r;
}

std::unique_ptr<BasToken> Bas2C::posneg() {
    auto m = t->fetch();
    const char *ms = nullptr;
    if (m->iskeyword(BasKeyword::PLUS)) {
        ms = "+";
    } else if (m->iskeyword(BasKeyword::MINUS)) {
        ms = "-";
    } else {
        t->unfetch(std::move(m));
        return paren();
    }
    auto r = expect(posneg());
    auto rty = static_cast<BasToken::TokenType>(r->resulttype());
    return BasToken::make(rty, ms + r->value);
}

std::unique_ptr<BasToken> Bas2C::paren() {
    if (checksymbol('(')) {
        auto r = expect(expr());
        nextsymbol(')');
        return BasToken::make(r->type, "(" + r->value + ")");
    }
    return atom();
}

std::unique_ptr<BasToken> Bas2C::atom() {
    auto r = t->fetch();

    if (r->isconst()) {                             // 定数
        return r;
    }
#ifndef DEBUG
    else if (r->istype(BasToken::KEYWORD)) {
        if (auto v = exfncall(r->ivalue, true)) {   // 組込関数/外部関数
            return v;
        }
        t->unfetch(std::move(r));
    } else if (auto v = lvalue(std::move(r))) {     // 左辺値
        return BasToken::make(static_cast<BasToken::TokenType>(v->type), v->name);
    } else if (auto v = fncall()) {                 // 関数呼び出し
        return v;
    }
#endif
    return nullptr;                                 // 該当なし(トークンを戻す)
}

//////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG

#define UNITTEST
#undef DEBUG
#include <iostream>
#include "tokengen.cpp"
#include "token.cpp"
#include "namespace.cpp"
#include "keyword.cpp"
#include "variable.cpp"
#include "bas2c.cpp"
#define DEBUG

int main() {
    auto b = Bas2C(stdin, 0, 0);

    auto v = b.expr();
    std::cout << *v << std::endl;
}

#endif