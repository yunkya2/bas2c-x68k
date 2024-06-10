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
#include <map>
#include <stdexcept>

#include "variable.hpp"

class BasNameSpaceException : public std::runtime_error {
public:
    BasNameSpaceException(const std::string& message) : std::runtime_error(message) {}
};

// グローバル/ローカル名前空間を保持するクラス
class BasNameSpace {
public:
    BasNameSpace() : bpass(0), curlocal(nullptr) {}
    void setpass(int bpass);
    void setlocal(std::string name = "");
    BasVariable* find(std::string name, bool localonly = false);
    BasVariable* newVariable(std::string name, BasVariable::Type type, std::string arg = "", std::string init = "", bool func = false, bool funcarg = false, bool forceglobl = false);
    std::string definition(std::string name = "");

private:
    std::map<std::string, BasVariable*> glist;      // グローバル名前空間
    std::map<std::string, std::map<std::string, BasVariable*>> llist;   // ローカル名前空間
    std::map<std::string, BasVariable*>* curlocal;  // 現在使用中のローカル名前空間
    int bpass;
};
