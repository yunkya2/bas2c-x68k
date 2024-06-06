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

#include "bas2c.hpp"
#include "variable.hpp"
#include "keyword.hpp"

// X-BASICの文を1つ読み込んで変換する
std::unique_ptr<std::string> Bas2C::statement() {
    std::string out;

    while (true) {
        auto s = checksymbol(':');
        if (!s) {
            break;
        }
    }

    if (checkkeyword(BasKeyword::EOF_KW)) {
        return nullptr;
    }

    updatestrtmp();

    if (auto s = checkvartype()) {
        defvar(static_cast<BasVariable::Type>(s->ivalue));

    } else if (auto s = checktype(BasToken::TokenType::KEYWORD)) {
        switch (s->ivalue) {
        case BasKeyword::EOL:
            if (nest.back() == 'i' || nest.back() == 'e') {
                nestout(nest.back());
                out = "}\n";
            }
            break;

        case BasKeyword::DIM:
        {
            auto ty = BasVariable::INT;
            if (auto t = checkvartype()) {
                ty = static_cast<BasVariable::Type>(t->ivalue);
            }
            defvar(ty);
            break;
        }

        case BasKeyword::PRINT:
        case BasKeyword::LPRINT:
        {
            auto lp = std::string((s->ivalue == BasKeyword::PRINT) ? "" : "l");
            auto crlf = true;
            if (checkkeyword(BasKeyword::USING)) {
                auto fmt = expect(expr(), "using の書式文字列がありません");
                expect(fmt->istype(BasToken::STR), "using の書式文字列がありません");
                nextsymbol(';');
                out += "b_s" + lp + "print(using(strtmp" + std::to_string(strtmp) + "," + fmt->value;
                strtmp++;
                while (true) {
                    if (auto x = expr()) {
                        if (x->istype(BasToken::STR)) {
                            out += "," + x->value;
                        } else {
                            out += ",(double)(" + x->value + ")";
                        }
                    }
                    if (!checksymbol(',')) {
                        break;
                    }
                }
                out += "));\n";
                crlf = !checksymbol(';');
            } else {
                while (true) {
                    if (auto x = expr()) {
                        if (x->istype(BasToken::STR)) {
                            out += "b_s" + lp + "print(" + x->value + ");\n";
                        } else if (x->istype(BasToken::FLOAT)) {
                            out += "b_f" + lp + "print(" + x->value + ");\n";
                        } else {
                            out += "b_i" + lp + "print(" + x->value + ");\n";
                        }
                        crlf = true;
                    } else if (checkkeyword(BasKeyword::TAB)) {
                        nextsymbol('(');
                        auto x = expect(expr());
                        nextsymbol(')');
                        out += "b_t" + lp + "print(" + x->value + ");\n";
                        crlf = true;
                    }

                    if (checksymbol(';')) {
                        crlf = false;
                    } else if (checksymbol(',')) {
                        out += "b_s" + lp + "print(STRTAB);\n";
                        crlf = false;
                    } else {
                        break;
                    }
                }
            }
            if (crlf) {
                out += "b_s" + lp + "print(STRCRLF);\n";
            }
            break;
        }

        case BasKeyword::INPUT:
        {
            std::string pstr = "\"? \"";
            if (auto p = checktype(BasToken::STR)) {
                pstr = p->value;
                if (checksymbol(';')) {
                    pstr = pstr + " \"? \"";
                } else {
                    nextsymbol(',');
                }
            }
            out += "b_input(" + pstr;

            do {
                auto a = expect(lvalue());
                if (a->isstr()) {
                    out += ", sizeof(" + a->name + "), " + a->name;
                } else {
                    const char *at;
                    switch (a->type) {
                    case BasToken::INT:
                        at = "0x204";
                        break;
                    case BasToken::CHAR:
                        at = "0x201";
                        break;
                    case BasToken::FLOAT:
                        at = "0x208";
                        break;
                    default:
                        expect(false);
                    }
                    out += ", " + std::string(at) + ", &" + a->name;
                }
            } while (checksymbol(','));
            out += ", -1);\n";
            break;
        }

        case BasKeyword::LINPUT:
        {
            if (auto p = checktype(BasToken::STR)) {
                nextsymbol(';');
                out += "b_sprint(" + p->value + ");\n";
            }
            auto a = expect(lvalue());
            expect(a->isstr());
            out += "b_linput(" + a->name + ", sizeof(" + a->name + "));\n";
            break;
        }

        case BasKeyword::IF:
        {
            auto x = expect(expr());
            nextkeyword(BasKeyword::THEN);
            nestin(checksymbol('{') ? 'I' : 'i');
            out = "if (" + x->value + ") {\n";
            break;
        }

        case BasKeyword::ELSE:
        {
            if (nest.back() == 'e') {                   // ネスト内側のelse節が終了する
                nestout('e');
                out += "}\n";
            }
            nestout('i');
            if (checkkeyword(BasKeyword::IF)) {         // else if が続く場合
                auto x = expect(expr());
                nextkeyword(BasKeyword::THEN);
                nestin(checksymbol('{') ? 'I' : 'i');
                out += "} else if (" + x->value + ") {\n";
            } else {                                    // else で終了する場合
                nestin(checksymbol('{') ? 'E' : 'e');
                out += "} else {\n";
            }
            break;
        }

        case BasKeyword::FOR:
        {
            auto v = expect(lvalue(nullptr, false, true));
            nextkeyword(BasKeyword::EQ);
            auto f = expect(expr());
            nextkeyword(BasKeyword::TO);
            auto t = expect(expr());
            nestin('f');
            out = "for (" + v->name + " = " + f->value + "; " + v->name + " <= " + t->value + "; " + v->name + "++) {\n";
            break;
        }

        case BasKeyword::NEXT:
            nestout('f');
            out = "}\n";
            break;

        case BasKeyword::WHILE:
        {
            auto x = expect(expr());
            nestin('w');
            out = "while (" + x->value + ") {\n";
            break;
        }

        case BasKeyword::ENDWHILE:
            nestout('w');
            out = "}\n";
            break;

        case BasKeyword::REPEAT:
            nestin('r');
            out = "do {\n";
            break;

        case BasKeyword::UNTIL:
        {
            auto x = expect(expr());
            nestout('r');
            out = "} while (!(" + x->value + "));\n";
            break;
        }

        case BasKeyword::SWITCH:
        {
            auto x = expect(expr());
            nestin('s');
            out = "switch (" + x->value + ") {\n";
            break;
        }

        case BasKeyword::CASE:
        {
            auto x = expect(expr());
            indentcnt--;
            out = "case " + x->value + ":\n";
            break;
        }

        case BasKeyword::DEFAULT:
            indentcnt--;
            out = "default:\n";
            break;

        case BasKeyword::ENDSWITCH:
            nestout('s');
            out = "}\n";
            break;

        case BasKeyword::GOTO:
        {
            auto l = std::stoi(nexttype(BasToken::INT));
            if (bpass == 1) {
                label.insert(l);
            }
            char buffer[10];
            std::snprintf(buffer, sizeof(buffer), "L%06d", l);
            out = "goto " + std::string(buffer) + ";\n";
            break;
        }

        case BasKeyword::GOSUB:
        {
            auto l = std::stoi(nexttype(BasToken::INT));
            if (bpass == 1) {
                subr.insert(l);
            }
            char buffer[10];
            std::snprintf(buffer, sizeof(buffer), "S%06d", l);
            out = std::string(buffer) + "();\n";
            break;
        }

        case BasKeyword::FUNC:
        {
            t->nocomment = false;
            // 関数の戻り値型を取得する(指定されていなければint型とする)
            auto fty = BasVariable::INT;
            if (auto t = checkvartype()) {
                fty = static_cast<BasVariable::Type>(t->ivalue);
            }

            // 関数名を取得する
            auto func = nexttype(BasToken::VARIABLE);

            // ローカル変数名前空間を初期化する
            nsp->setlocal(func);

            // 引数を取得する
            std::string arg;
            nextsymbol('(');
            if (checksymbol(')')) {
                arg = "void";
            } else {
                arg = "";
                while (true) {
                    // 引数名を取得する
                    auto var = nexttype(BasToken::VARIABLE);
                    // 引数の型を取得する(指定されていなければint型とする)
                    auto vty = BasVariable::INT;
                    if (checksymbol(';')) {
                        vty = static_cast<BasVariable::Type>(expect(checkvartype())->ivalue);
                    }
                    // 引数をローカル変数として登録する
                    std::string va = BasVariable::isstr(vty) ? "[32+1]" : "";
                    auto v = nsp->newVariable(var, vty, va, "", false, true);
                    arg += std::string(v->typname()) + " " + var + va;
                    if (!checksymbol(',')) {
                        break;                  // もう引数がないならループを抜ける
                    }
                    arg += ", ";
                }
                nextsymbol(')');
            }

            // 関数名はグローバルで登録する
            auto v = nsp->newVariable(func, fty, arg, "", true, false, true);

            out = *nestclose();
            nestin('F');
            out += "\n/***************************/\n";
            out += std::string(v->typname(true)) + " " + func + "(" + arg + ")\n{\n";
            if (bpass != 1) {
                // 2pass目ならローカル変数定義を出力する
                out += nsp->definition(func);
                // TBD 改行
            }
            break;
        }

        case BasKeyword::ENDFUNC:
            nsp->setlocal("");
            nestout('F');
            t->nocomment = true;
            out = "}\n";
            break;

        case BasKeyword::RETURN:
            if (checksymbol('(')) {
                auto x = expr();
                nextsymbol(')');
                if (x) {
                    out = "return " + x->value + ";\n";
                } else {
                    out = "return 0;\n";
                }
            } else {
                if (nest.back() == 'S' && nest.size() == 1) {   // サブルーチンを閉じる
                    nestout('S');
                    t->nocomment = true;
                    out = "return;\n}\n";
                } else {
                    out = "return;\n";
                }
            }
            break;

        case BasKeyword::BREAK:
            checksymbol(';');
            out = "break;\n";
            break;

        case BasKeyword::CONTINUE:
            out = "continue;\n";
            break;

        case BasKeyword::LOCATE:
            if (auto x = expr()) {
                nextsymbol(',');
                auto y = expect(expr());
                out = "locate(" + x->value + ", " + y->value + ");\n";
            } else {
                nextsymbol(',');
            }
            if (checksymbol(',')) {
                out += "b_csw(" + expect(expr())->value + ");\n";
            }
            break;

        case BasKeyword::ERROR:                             // error命令は読み飛ばして無視
            out = "/* error " + t->fetch()->value + " */\n";
            break;

        case BasKeyword::END:
            out = std::string(b_exit) + "(0);\n";
            if (nest.back() == 'M' && nest.size() == 1) {   // main関数を閉じる
                nestout('M');
                t->nocomment = true;
                out += std::string("}\n");
            }
            break;

        default:
            if (auto r = exfncall(s->ivalue)) {
                out = r->value + ";\n";
                break;
            }

            expect(false);
        }

    } else if (auto s = checktype(BasToken::SYMBOL)) {
        expect(s->ivalue == '}');               // if then/else節が終了する場合
        if (nest.back() == 'i' || nest.back() == 'e') {
            nestout(nest.back());
            out = "}\n";                        // ブロック内側のthen/else節が終了する
        }
        if (nest.back() == 'E') {               // else節が終了する
            nestout('E');
            out += "}\n";
        } else {                                // then節が終了する
            nestout('I');
            if (!checkkeyword(BasKeyword::ELSE)) {
                out += "}\n";
            } else {
                if (checkkeyword(BasKeyword::IF)) {
                    auto x = expect(expr());
                    nextkeyword(BasKeyword::THEN);
                    nestin(checksymbol('{') ? 'I' : 'i');
                    out += "} else if (" + x->value + ") {\n";
                } else {
                    nestin(checksymbol('{') ? 'E' : 'e');
                    out += "} else {\n";
                }
            }
        }

    } else if (auto s = checktype(BasToken::TokenType::COMMENT)) {
        out = s->value;

    } else  {
        auto r = t->fetch();
        if (auto s = lvalue(std::move(r), true)) {      // 左辺値
            nextkeyword(BasKeyword::EQ);
            auto x = initvar(s->type);                  // 代入する値を得る
            if (s->isarray()) {                         // 配列なら一時変数の内容をコピー
                auto v = nsp->find(s->name);
                // 一時変数を名前空間に登録する
                char buffer[20];
                std::snprintf(buffer, sizeof(buffer), "_initmp%04d", initmp++);
                nsp->newVariable(buffer, BasVariable::toconst(s->type), v->arg, x);
                out = "memcpy(" + s->name + ", " + buffer + ", sizeof(" + s->name + "));\n";
            } else if (s->isstr()) {                    // 文字列ならb_strncpy()
                out = "b_strncpy(sizeof(" + s->name + "), " + s->name + "," + x + ");\n";
            } else {                                    // 単純変数への代入
                out = s->name + " = " + x + ";\n";
            }
        } else {
            r = t->fetch();
            out = expect(fncall(std::move(r)))->value + ";\n";  // 関数呼び出し
        }
    }

    return std::make_unique<std::string>(out);
}

// 左辺値(代入可能な変数/配列)を得る
std::unique_ptr<BasVariable> Bas2C::lvalue(std::unique_ptr<BasToken> var, bool islet, bool isfor) {
    bool unfetch = (var != nullptr);
    var = var ? std::move(var) : t->fetch();
    if (!var->istype(BasToken::TokenType::VARIABLE)) {
        if (unfetch) {
            t->unfetch(std::move(var));
        }
        return nullptr;
    }
    auto v = nsp->find(var->value);
    auto x = t->fetch();
    if (x->issymbol('(')) {                 // 配列または関数呼び出し
        t->unfetch(std::move(x));
        if (!v || !v->isarray()) {
            t->unfetch(std::move(var));
            return nullptr;                 // 関数呼び出し
        }
    } else {                                // 単純変数
        t->unfetch(std::move(x));
        if (!v) {
            if (islet || isfor) {
                // 未定義変数への代入時はint型のグローバル変数として定義する
                nsp->newVariable(var->value, BasVariable::INT, "", "", false, false, true);
                v = nsp->find(var->value);
            } else {
                if (unfetch) {
                    t->unfetch(std::move(var));
                }
                return nullptr;             // 代入以外はエラー
            }
        }
    }
    auto ty = v->type;
    std::string sub = "";
    if (v->isarray()) {
        if (checksymbol('(')) {             // 配列要素
            sub = "[";
            while (true) {
                if (auto a = expr()) {
                    sub += a->value;
                }
                if (!checksymbol(',')) {
                    break;
                }
                sub += "][";
            }
            nextsymbol(')');
            sub += "]";
            ty = BasVariable::basetype(ty);
        } else {                            // 配列全体
            if (!islet) {
                if (unfetch) {
                    t->unfetch(std::move(var));
                }
                return nullptr;             // 配列全体を指定できるのは代入のみ
            }
        }
    }
    if (BasVariable::isstr(ty)) {           // 部分文字列 a[x]
        if (checksymbol('[')) {
            auto a = expect(expr());
            nextsymbol(']');
            sub += "[" + a->value + "]";
            ty = BasVariable::CHAR;
        }
    }
    return std::make_unique<BasVariable>(v->name + sub, ty);
}

// 変数/配列の定義
void Bas2C::defvar(BasVariable::Type ty) {
    do {
        // 変数名を取得する
        auto var = nexttype(BasToken::TokenType::VARIABLE);
        // 配列の最大要素番号、str変数のサイズを取得する
        std::string s;
        auto rty = ty;
        if (checksymbol('(')) {             // ()が付いていたら配列 (dimがなくても)
            rty = BasVariable::toarray(ty);
            while (true) {                  // 配列の要素数を取得する
                s += "[(" + expect(expr())->value + ")+1]";
                if (!checksymbol(',')) {
                    break;
                }
            }
            nextsymbol(')');
        }
        if (BasVariable::isstr(ty)) {       // 文字列型の場合はバッファサイズを得る
            if (checksymbol('[')) {
                s += "[" + expect(expr())->value + "+1]";
                nextsymbol(']');
            } else {
                s += "[32+1]";              // デフォルトのバッファサイズ
            }
        }
        // 初期値が指定されていたら取得する
        std::string x;
        if (checkkeyword(BasKeyword::EQ)) {
            x = initvar(rty);
        }
        // 定義された変数を名前空間に登録する
        nsp->newVariable(var, static_cast<BasVariable::Type>(rty), s, x);
    } while (checksymbol(','));             // 複数の変数をまとめて定義するなら繰り返す
}

// 変数/配列の初期値を得る
std::string Bas2C::initvar(BasVariable::Type ty) {
    if (BasVariable::isarray(ty)) {
        nextsymbol('{');                    // 配列の場合
        std::string n = "{";
        auto nest = 1;
        while (nest > 0) {
            if (checksymbol('{')) {
                n += "{";
                nest++;
            } else if (checksymbol('}')) {
                n += "}";
                nest--;
            } else if (auto a = checktype(BasToken::SYMBOL)) {
                n += static_cast<char>(a->ivalue);
            } else if (checkkeyword(BasKeyword::EOL)) {
                n += "\n";
            } else if (auto a = checktype(BasToken::COMMENT)) {
                n += a->value;
            } else {
                n += expect(expr())->value;
            }
        }
        return n;
    } else {
        return expect(expr())->value;
    }
}

// 関数呼び出しを生成する
std::unique_ptr<BasToken> Bas2C::fncall(std::unique_ptr<BasToken> var) {
    bool unfetch = (var == nullptr);
    var = var ? std::move(var) : t->fetch();
    if (!var->istype(BasToken::TokenType::VARIABLE)) {
        if (unfetch) {
            t->unfetch(std::move(var));
        }
        return nullptr;
    }
    auto v = nsp->find(var->value);     // 関数が定義済みか調べる
    if (flag & F_UNDEFERR) {
        expect(v || bpass == 1);        // (パス1なら未定義でもよい)
    }
    std::string arg;                    // 引数を得る
    nextsymbol('(');
    while (true) {
        if (auto a = expr()) {          // (型チェック)
            arg += a->value;
        }
        if (!checksymbol(',')) {
            break;
        }
        arg += ", ";
    }
    nextsymbol(')');
    BasToken::TokenType rty;
    rty = v ? static_cast<BasToken::TokenType>(v->type) : BasToken::TokenType::FUNCTION;
    return BasToken::make(rty, var->value + "(" + arg + ")");
}

#define NASI        "0x4e415349"        // 外部関数の引数省略時に使われる値

// kwが組込関数/外部関数であれば引数を与えて呼び出す
std::unique_ptr<BasToken> Bas2C::exfncall(int kw, bool isexpr) {
    auto nt = t->fetch();               // 次のトークンを先読みする

    // 組込関数の特殊ケース (intは通常の予約語でもあるため先にチェックする)
    if (kw == BasKeyword::INT && nt->issymbol('(')) {
        kw = BasKeyword::find("int$$");
    }

    auto it = BasKeyword::exfnlist.find(kw);
    if (it == BasKeyword::exfnlist.end()) {
        t->unfetch(std::move(nt));
        return nullptr;                 // キーワードだが組込関数/外部関数ではない
    }
    auto ex = it->second;

    // 組込関数の特殊ケース
    if (ex->name == "date$" && nt->iskeyword(BasKeyword::EQ)) {         // date$= -> date$$
        ex = BasKeyword::exfnlist[BasKeyword::find("date$$")];
    } else if (ex->name == "time$" && nt->iskeyword(BasKeyword::EQ)) {  // time$= -> time$$
        ex = BasKeyword::exfnlist[BasKeyword::find("time$$")];
    } else {
        if (ex->name == "inkey$" && nt->issymbol('(')) {                // inkey$(0) -> inkey$$(0)
            ex = BasKeyword::exfnlist[BasKeyword::find("inkey$$")];
        } else if (ex->name == "color" && nt->issymbol('[')) {          // color[..] -> color$$(..)
            ex = BasKeyword::exfnlist[BasKeyword::find("color$$")];
        }
        t->unfetch(std::move(nt));      // 先読みした文字を戻す
    }

    // 使われた関数グループを記録する(#includeに使用するため)
    if (ex->group != "") {
        exfngroup.insert(ex->group);
    }

    // # 戻り値型
    auto rty = BasToken::INT;
    switch (ex->type[0]) {
    case 'I':
        rty = BasToken::INT;
        break;
    case 'C':
        rty = BasToken::CHAR;
        break;
    case 'F':
        rty = BasToken::FLOAT;
        break;
    case 'S':
        rty = BasToken::STR;
        break;
    default:
        expect(!isexpr);            // (式なら必須)
    }

    auto fn = ex->cfunc.empty() ? ex->name : ex->cfunc;     // C関数名
    std::vector<std::string> av;
    auto a = ex->arg;                           // X-BASIC引数の型
    while (a.size() > 0) {
        if (std::strchr("([])", a[0])) {
            nextsymbol(a[0]);
        } else if (a[0] == ',') {
            if (!checksymbol(',')) {
                a = a.substr(1);                // 残りの引数がすべて省略された
                while (a != "") {
                    if (std::strchr("ISCF", a[0]) && a[1] == '-') {
                        av.push_back(NASI);
                        a = a.substr(2);
                    } else if (a[0] == ',') {
                        a = a.substr(1);
                    } else if (std::strchr("([])", a[0])) {
                        nextsymbol(a[0]);
                        a = a.substr(1);
                    } else {
                        expect(false);
                    }
                }
                break;
            }
        } else if (std::strchr("ISCFN", a[0])) {
            if (a.size() > 1 && a[1] == 'A') {  // 配列
                a = a.substr(1);
                auto vn = nexttype(BasToken::VARIABLE);     // 配列変数名
                auto va = expect(nsp->find(vn));            // 定義済みであることを確認
                expect(va->isarray());          // (型の確認)
                av.push_back(vn);
            } else {
                auto x = expr();
                if (!x and a[1] == '-') {                   // 引数が省略された
                    if (ex->name == "exit") {               // 特殊ケース exit() -> exit(0)
                        av.push_back("0");
                    } else if (ex->name == "pi") {          // 特殊ケース pi() -> pi(0)
                        fn = "pi";
                        av.push_back("");
                    } else {
                        av.push_back(NASI);
                    }
                    a = a.substr(1);
                } else {
                    if (ex->name == "str$" and x->istype(BasToken::FLOAT)) {
                        fn = "b_strfS";                     // 特殊ケース str$(float) -> b_strfS(float)
                    } else if (ex->name == "abs" and x->istype(BasToken::FLOAT)) {
                        fn = "fabs";                        // 特殊ケース abs(float) -> fabs(float)
                        rty = BasToken::FLOAT;              // 戻り値もfloatになる
                    }
                    av.push_back(x->value);     // (型の確認)
                }
            }
        }
        a = a.substr(1);
    }

    std::string arg;
    a = ex->carg;           // C引数の型
    int i = 0;
    while (a.size() > 0) {
        if (a[0] == ',') {
            arg += ",";
            a = a.substr(1);
            continue;
        }
        if (a[0] == '#') {                          // 1つ前の引数のサイズ
            arg += "sizeof(" + av[i - 1] + ")";
        } else if (a[0] == '@') {                   // 1つ前の引数の要素のサイズ
            arg += "sizeof(" + av[i - 1] + "[0])";
        } else if (a[0] == '&') {                   // 引数へのポインタ
            arg += "&" + av[i];
            i++;
        } else if (a[0] == '%') {                   // 引数
            if (i < av.size()) {
                arg += av[i];
            }
            i++;
        } else if (a[0] == '$') {                   // 文字列作業用ワーク
            arg += "strtmp" + std::to_string(strtmp++);
        } else if (a[0] == ',') {
            arg += ",";
        }
        a = a.substr(1);
    }
    return BasToken::make(rty, fn + "(" + arg + ")");
}
