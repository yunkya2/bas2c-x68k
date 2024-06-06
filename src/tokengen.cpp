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

#include <cctype>

#include "tokengen.hpp"

BasTokenGen::BasTokenGen(std::FILE *fh, int cindent, bool verbose) {
    this->cindent = cindent;
    this->verbose = verbose;
    if (fh == stdin) {          // 標準入力は巻き戻せないので一度すべてを読み込む
        this->filebuf = static_cast<char *>(malloc(1));
        this->filebuf[0] = '\0';
        int filelen = 1;
        char line[LINELEN];
        while (std::fgets(line, sizeof(line), fh)) {
            if (line[0] == '\x1a') {
                break;
            }
            int linelen = std::strlen(line);
            this->filebuf = static_cast<char *>(realloc(this->filebuf, filelen + linelen));
            std::strcat(this->filebuf, line);
            filelen += linelen;
        }
        this->fh = nullptr;
    } else {
        this->fh = fh;
    }
    rewind();
}

// ファイルを巻き戻す
void BasTokenGen::rewind() {
    if (this->fh) {
        std::fseek(this->fh, 0, SEEK_SET);
    } else {
        this->fp = 0;
    }
    std::strcpy(this->line, "");
    std::strcpy(this->curline, "");
    this->lineno = 0;
    this->baslineno = 0;
    this->golineno = 0;
    this->firsttoken = true;
    this->cached.clear();
    this->nocomment = false;
    this->ccode = "";
    this->prelen = 0;
    this->curlen = 0;
}

// 1行読み込む
bool BasTokenGen::readline() {
    char *buf = this->line;
    size_t size = sizeof(this->line);
    char *res;
    if (this->fh) {
        res = std::fgets(buf, size, this->fh);
    } else {
        int i = 0;
        size_t len = std::strlen(this->filebuf);
        res = buf;
        while (i < size - 1) {
            if (this->fp >= len) {
                res = nullptr;
                break;
            }
            char c = this->filebuf[this->fp++];
            buf[i++] = c;
            if (c == '\n') {
                break;
            }
        }
        buf[i++] = '\0';
    }
    if (buf[0] == '\x1a') {
        buf[0] = '\0';
    }

    std::strcpy(this->curline, this->line);
    this->golineno = 0;
    this->firsttoken = true;
    if (!res) {
        return false;
    }

    this->lineno++;
    this->baslineno++;
    if (this->cindent >= 0 && std::strlen(this->line) > 0) {
        for (auto i = 0; i < cindent; i++) {
            this->ccode += std::string("\t");
        }
        this->ccode += std::string("/*===") + *getbascmnline(this->line) + "===*/\n";
    }
    if (this->verbose && this->bpass == 2) {
        std::printf("%s", this->line);
    }

    // 行番号があれば取得する
    char *p = this->line;
    long num = std::strtol(this->line, &p, 10);
    if (this->line != p) {
        this->golineno = num;
        this->baslineno = num;
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        std::memcpy(this->line, p, std::strlen(p) + 1);
    }

    return true;
}

// 必要があれば1行読み込む
char *BasTokenGen::getline() {
    if (std::strlen(this->line) == 0) {
        readline();
        // #c～#endcの間を取り込む
        if (std::strncmp(this->line, "#c", 2) == 0) {
            while (readline()) {
                if (std::strncmp(this->line, "#endc", 5) == 0) {
                    break;
                }
                this->ccode += this->line;
            }
            readline();
        }
    }
    // 行頭の空白などは読み飛ばす
    char *p = this->line;
    while (*p == ' ' || *p == '\t' || *p == '\r') {
        p++;
    }
    std::memcpy(this->line, p, std::strlen(p) + 1);
    prelen = curlen = std::strlen(line);
    return this->line;
}

// GOTO/GOSUB用の行番号を取得する
int BasTokenGen::getgolineno() {
    int r = golineno;
    golineno = 0;           // 取得できるのは一度だけ
    return r;
}

// エラー表示用の行番号を取得する
std::unique_ptr<std::string> BasTokenGen::getlineno() {
    auto s = std::to_string(lineno) + " (" + std::to_string(baslineno) + ")";
    return std::make_unique<std::string>(s);
}

// #c～#endcのコードを取得する
std::unique_ptr<std::string> BasTokenGen::getccode() {
    auto r = ccode;
    ccode = "";
    return std::make_unique<std::string>(r);
}

// コメント挿入用にBASICの行を取得する
std::unique_ptr<std::string> BasTokenGen::getbascmnline(const char *line) {
    auto r = std::string(line);
    size_t pos;
    while ((pos = r.find("/*")) != std::string::npos) {
        r.erase(pos, 2);
    }
    while ((pos = r.find("*/")) != std::string::npos) {
        r.erase(pos, 2);
    }
    while ((pos = r.find("\r")) != std::string::npos) {
        r.erase(pos, 1);
    }
    while ((pos = r.find("\n")) != std::string::npos) {
        r.erase(pos, 1);
    }
    return std::make_unique<std::string>(r);
}

// トークンを取得する
std::unique_ptr<BasToken> BasTokenGen::get() {
    // ファイル終了
    if (std::strlen(getline()) == 0) {
        return BasToken::makekeyword(BasKeyword::EOF_KW);
    }
    // CRは削除
    if (this->line[0] == '\r') {
        std::memcpy(this->line, this->line + 1, std::strlen(this->line + 1) + 1);
    }
    // 行末
    if (std::strcmp(this->line, "\n") == 0) {
        std::strcpy(this->line, "");
        return BasToken::makekeyword(BasKeyword::EOL);
    }
    // コメント
    if (std::strncmp(this->line, "/*", 2) == 0) {
        if (this->firsttoken && !this->nocomment) {
            std::string comment = std::string("/*") + *getbascmnline(line) + "*/\n";
            std::strcpy(this->line, "\n");
            return BasToken::makecomment(comment);
        } else {
            std::strcpy(this->line, "");
            return BasToken::makekeyword(BasKeyword::EOL);
        }
    }

    this->firsttoken = false;

    auto *p = this->line;
    auto c = *p++;
    auto s = std::string() + c;
    switch (c) {
    case '\"':              // 文字列 "~"
        do {
            c = *p;
            if (c == '\0' || c == '\n') {
                s += '\"';                  // 引用符を閉じずに行が終わっていたら補う
                break;
            }
            p++;
            s += c;
        } while (c != '\"');
        std::memcpy(this->line, p, std::strlen(p) + 1);
        return BasToken::makestr(s);
    case '\'':              // 文字 'x'
        c = *p++;
        if (c == '\0' || c == '\n' || c =='\'') {
            break;
        }
        if (*p++ != '\'') {
            break;
        }
        s += c;
        s += '\'';
        std::memcpy(this->line, p, std::strlen(p) + 1);
        return BasToken::makeint(s);
    case '&':
        c = *p++;
        switch (std::tolower(c)) {
        case 'h':           // 16進数 &Hxxxx
            s = "0x";
            while (true) {
                c = *p;
                if (!isxdigit(c)) {
                    break;
                }
                p++;
                s += c;
            }
            std::memcpy(this->line, p, std::strlen(p) + 1);
            return BasToken::makeint(s);
        case 'o':           // 8進数 &Oxxxx
            s = "0";
            while (true) {
                c = *p;
                if (!(c >= '0' && c <= '7')) {
                    break;
                }
                p++;
                s += c;
            }
            std::memcpy(this->line, p, std::strlen(p) + 1);
            return BasToken::makeint(s);
        case 'b':           // 2進数 &Bxxxx
            s = "0b";
            while (true) {
                c = *p;
                if (!(c >= '0' && c <= '1')) {
                    break;
                }
                p++;
                s += c;
            }
            std::memcpy(this->line, p, std::strlen(p) + 1);
            return BasToken::makeint(s);
        default:
            break;
        }
        break;
    case '0'...'9':         // 整数 or 実数
    case '.':
        while (true) {
            c = *p;
            if (!isdigit(c)) {
                break;
            }
            p++;
            s += c;
        }
        if (c == '.' || c == 'e' || c == 'E' || c == '#') {
            if (c == '.') {
                p++;
                s += c;
                while (true) {
                    c = *p;
                    if (!isdigit(c)) {
                        break;
                    }
                    p++;
                    s += c;
                }
            }
            if (c == 'e' || c == 'E') {
                p++;
                s += c;
                if (*p == '-' || *p == '+') {
                    s += *p++;
                }
                while (true) {
                    c = *p;
                    if (!isdigit(c)) {
                        break;
                    }
                    p++;
                    s += c;
                }
            }
            if (c == '#') {
                p++;
                s += c;
            }
            std::memcpy(this->line, p, std::strlen(p) + 1);
            return BasToken::makefloat(s);
        } else {
            while (s.length() > 1 && s[0] == '0') {
                s.erase(0, 1);                  // 冒頭の0は8進数に解釈されてしまうので取り除く
            }
            std::memcpy(this->line, p, std::strlen(p) + 1);
            return BasToken::makeint(s);
        }
        break;
    case 'a'...'z':         // 変数名または予約語
    case 'A'...'Z':
    case '_':
    {
        while (true) {
            c = *p;
            if (!(isalnum(c) || (c == '_') || (c == '$'))) {
                break;
            }
            p++;
            s += c;
        }
        std::memcpy(this->line, p, std::strlen(p) + 1);
        auto k = BasKeyword::find(s);
        if (k != BasKeyword::NONE) {
            return BasToken::makekeyword(k);
        } else {
            size_t found = s.find('$');
            while (found != std::string::npos) {    // 変数名の '$' は 'S' に置き換える
                s.replace(found, 1, "S");
                found = s.find('$', found + 1);
            }
            return BasToken::makevariable(s);
        }
        break;
    }
    default:
        int l;
        auto w = BasKeyword::findop(line, l);       // 演算子
        if (w != BasKeyword::NONE) {
            std::memcpy(line, line + l, std::strlen(line + l) + 1);
            return BasToken::makekeyword(w);
        }
        break;
    }
    std::memcpy(this->line, this->line + 1, std::strlen(this->line + 1) + 1);
    return BasToken::makesymbol(c);                 // その他の文字は記号
}

// トークンを取得する (先読みされていたものがあればそれを返す)
std::unique_ptr<BasToken> BasTokenGen::fetch() {
    prelen = curlen;
    curlen = std::strlen(line);
    if (cached.empty()) {
        return get();
    } else {
        auto t = std::move(cached.back());
        cached.pop_back();
        return t;
    }
}

// 一度読んだトークンを戻す
void BasTokenGen::unfetch(std::unique_ptr<BasToken> t) {
    cached.push_back(std::move(t));
    curlen = prelen;
}

void BasTokenGen::skip() {
    while (true) {
        auto t = fetch();
        if (t->issymbol(':') || t->iskeyword(BasKeyword::EOL) || t->iskeyword(BasKeyword::EOF_KW)) {
            return;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG

#define UNITTEST
#undef DEBUG
#include "token.cpp"
#include "keyword.cpp"
#define DEBUG

int main() {
    BasTokenGen t;
    t.setpass(1);

    while (true) {
        auto a = t.fetch();
        if (a->type == BasToken::KEYWORD && a->ivalue == BasKeyword::EOF_KW) {
            break;
        }
        int l;
        if ((l = t.getgolineno()) != 0)
            std::cout << l;
        std::cout << *a << std::endl;
    }
}

#endif