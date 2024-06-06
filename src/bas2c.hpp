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
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <stdexcept>

#include "namespace.hpp"
#include "tokengen.hpp"

class Bas2CException : public std::runtime_error {
public:
    Bas2CException(const std::string& message) : std::runtime_error(message) {}
};

class Bas2C {
public:
    enum Flag {
        F_DEBUG       = (1 << 0),   // デバッグモード
        F_UNDEFERR    = (1 << 1),   // 未定義関数呼び出しをエラーにする
        F_NOBINIT     = (1 << 2),   // プログラム開始/終了時にb_init(),b_exit()を呼ばない
        F_BASCOMMENT  = (1 << 3),   // BASICの各行をコメント行として挿入する
        F_VERBOSE     = (1 << 4),   // 変換中の行を表示する
        F_BCCOMPAT    = (1 << 5),   // 演算子の優先順位や論理演算の結果を変換しない(BC.Xコンパチ)
    };

    Bas2C(std::FILE *fh, int flag, int cindent = -1);
    ~Bas2C();

    int start(std::FILE *fo = stdout, const char *finame = "<stdin>");
    void error(const char *msg, const char *finame);

    std::unique_ptr<BasToken> expr();

private:
    void setpass(int bpass);
    void updatestrtmp();

    void nestin(char type);
    void nestout(char type);
    std::unique_ptr<std::string> nestclose();
    void indentinit() { indentcnt = nest.size(); }
    std::unique_ptr<std::string> indentout();
    std::unique_ptr<std::string> gendefine();
    std::unique_ptr<std::string> genlabel();

    std::unique_ptr<std::string> statement();
    std::unique_ptr<BasVariable> lvalue(std::unique_ptr<BasToken> var = nullptr, bool islet = false, bool isfor = false);
    void defvar(BasVariable::Type ty);
    std::string initvar(BasVariable::Type ty);
    std::unique_ptr<BasToken> fncall(std::unique_ptr<BasToken> var = nullptr);
    std::unique_ptr<BasToken> exfncall(int kw, bool isexpr = false);

    // 実行結果がNoneでないことを確認する
    template <typename T>
    T expect(T v, const char *err = "構文に誤りがあります") {
        if (!v) {
            throw Bas2CException(err);
        }
        return v;
    }

    // 次のトークンが型tであることを確認して値を返す
    std::string nexttype(BasToken::TokenType t) {
        auto x = this->t->fetch();
        expect(x->istype(t));
        return x->value;
    }
    int nexttypei(BasToken::TokenType t) {
        auto x = this->t->fetch();
        expect(x->istype(t));
        return x->ivalue;
    }

    // 次のトークンが予約語kであることを確認する
    void nextkeyword(int k) {
        expect(t->fetch()->iskeyword(k), (std::string(BasKeyword::getkeyword(k)) + " がありません").c_str());
    }

    // 次のトークンがシンボルsであることを確認する
    void nextsymbol(int s) {
        expect(t->fetch()->issymbol(s), (std::string(1, s) + " がありません").c_str());
    }

    // 型tが出たら読み進む
    std::unique_ptr<BasToken> checktype(BasToken::TokenType t) {
        auto x = this->t->fetch();
        if (x->istype(t)) {
            return x;
        }
        this->t->unfetch(std::move(x));
        return nullptr;
    }

    // 予約語kが出たら読み進む
    std::unique_ptr<BasToken> checkkeyword(int k) {
        auto x = t->fetch();
        if (x->iskeyword(k)) {
            return x;
        }
        t->unfetch(std::move(x));
        return nullptr;
    }

    // シンボルsが出たら読み進む
    std::unique_ptr<BasToken> checksymbol(int s) {
        auto x = t->fetch();
        if (x->issymbol(s)) {
            return x;
        }
        t->unfetch(std::move(x));
        return nullptr;
    }

    // 変数型を表すトークンが出たら読み進む
    std::unique_ptr<BasToken> checkvartype() {
        auto x = t->fetch();
        if (x->isvartype()) {
            return x;
        }
        t->unfetch(std::move(x));
        return nullptr;
    }

    std::FILE *fh;
    int flag;
    int cindent;
    int bpass;

    BasTokenGen *t;
    std::set<int> label;
    std::set<int> subr;
    BasNameSpace *nsp;

    int strtmp;
    int strtmp_max;
    int initmp;
    std::set<std::string> exfngroup;
    const char *b_exit;

    std::vector<char> nest;
    int indentcnt;

    int exitstatus;

    std::unique_ptr<BasToken> opxor();
    std::unique_ptr<BasToken> opor();
    std::unique_ptr<BasToken> opand();
    std::unique_ptr<BasToken> opnot();
    std::unique_ptr<BasToken> cmp();
    std::unique_ptr<BasToken> shrshl();
    std::unique_ptr<BasToken> addsub();
    std::unique_ptr<BasToken> mod();
    std::unique_ptr<BasToken> yen();
    std::unique_ptr<BasToken> muldiv();
    std::unique_ptr<BasToken> posneg();
    std::unique_ptr<BasToken> paren();
    std::unique_ptr<BasToken> atom();
};

