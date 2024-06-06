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

#include "namespace.hpp"

// 変換パスを設定する
void BasNameSpace::setpass(int bpass) {
    this->bpass = bpass;
    this->curlocal = nullptr;
}

// ローカル名前空間を設定する
void BasNameSpace::setlocal(std::string name) {
    if (!name.empty()) {
        if (bpass == 1) {       // 新たな名前空間を作るのはpass1のみ
            llist[name] = std::map<std::string, BasVariable*>();
        }
        curlocal = &llist[name];
    } else {
        curlocal = nullptr;
    }
}

// 名前がグローバルorローカル名前空間に定義されているかを調べる
BasVariable* BasNameSpace::find(std::string name) {
    if (curlocal != nullptr) {
        auto it = curlocal->find(name);
        if (it != curlocal->end()) {
            return it->second;
        }
    }
    auto it = glist.find(name);
    if (it != glist.end()) {
        return it->second;
    }
    return nullptr;
}

// 変数を名前空間に定義する
BasVariable* BasNameSpace::newVariable(std::string name, BasVariable::Type type, std::string arg, std::string init, bool func, bool funcarg, bool forceglobl) {
    // 現在有効な名前空間を取得する
    std::map<std::string, BasVariable*>* gls = forceglobl || curlocal == nullptr ? &glist : curlocal;
    if (bpass == 1) {       // 変数定義を行うのはpass1のみ
        if (gls->find(name) != gls->end()) {
            throw BasNameSpaceException("変数 " + name + " が多重定義されています");
        }
        (*gls)[name] = new BasVariable(name, type, arg, init, func, funcarg);
    }
    return (*gls)[name];
}

// グローバル/ローカル名前空間に定義されている変数の定義リストを出力する
std::string BasNameSpace::definition(std::string name) {
    std::map<std::string, BasVariable*>* gls = name.empty() ? &glist : &llist[name];
    std::string r = "";
    std::string tab = name.empty() ? "" : "\t";
    for (const auto& kv : *gls) {
        auto a = kv.second->definition(name.empty());
        if (!a.empty()) {
            r += tab + a;
        }
    }
    return r;
}

//////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG

#undef DEBUG
#include <iostream>
#include "variable.cpp"
#define DEBUG

int main() {
    BasNameSpace ns;
    ns.setpass(1);

    ns.setlocal();
    ns.newVariable("a", BasVariable::INT);
    ns.newVariable("a1", BasVariable::INT);
    ns.newVariable("func", BasVariable::INT, "int a, int b", "", true);

    ns.setlocal("test1");
    ns.newVariable("a2", BasVariable::FLOAT, "", "1234");
    ns.newVariable("a3", BasVariable::DIM_CHAR, "[10]");

    ns.setlocal("test2");
    ns.newVariable("a1", BasVariable::INT);
    ns.newVariable("a2", BasVariable::FLOAT, "", "5678");

    std::cout << "global namespace" << std::endl;
    std::cout << ns.definition() << std::endl;
    std::cout << "test1 namespace" << std::endl;
    std::cout << ns.definition("test1") << std::endl;
    std::cout << "test2 namespace" << std::endl;
    std::cout << ns.definition("test2") << std::endl;
}

#endif