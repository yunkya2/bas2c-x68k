#!/bin/env python3
import sys
import re

class BasKeyword:
    EOF         = 0

    PLUS        = 1001
    MINUS       = 1002
    MUL         = 1003
    DIV         = 1004

    EOL         = 9999

    keywordop = {
        '+'         : PLUS,
        '-'         : MINUS,
        '*'         : MUL,
        '/'         : DIV,
    }

class BasToken:
    """X-BASICのトークン"""
    SYMBOL   = 0
    INT      = 1
    STR      = 2
    KEYWORD  = 3
    VARIABLE = 4

    def __init__(self, type, value):
        self.type = type
        self.value = value

    @classmethod
    def symbol(cls, value):
        return cls(cls.SYMBOL, value)
    @classmethod
    def int(cls, value):
        return cls(cls.INT, value)
    @classmethod
    def str(cls, value):
        return cls(cls.STR, value)
    @classmethod
    def keyword(cls, value):
        return cls(cls.KEYWORD, value)
    @classmethod
    def variable(cls, value):
        return cls(cls.VARIABLE, value)

    def isconst(self):
        """定数であればTrue"""
        return self.type >= self.INT and self.type <= self.STR
    def istype(self, type):
        """与えたtypeであればvalueを、異なればNoneを返す"""
        return self.value if self.type == type else None
    def issymbol(self, value):
        """シンボルがvalueであればTrue"""
        return self.type == self.SYMBOL and self.value == value
    def iskeyword(self, value):
        """予約語がvalueであればTrue"""
        return self.type == self.KEYWORD and self.value == value

    def __repr__(self):
        return f'({self.type},{self.value})'

class BasTokenGen:
    """ソースコードからトークンを生成するクラス"""
    def __init__(self, fh=sys.stdin):
        self.fh = fh
        self.line = ''
        self.cached = []

    def getline(self):
        """必要があれば1行読み込む"""
        if len(self.line) == 0:
            self.line = self.fh.readline()
        # 行頭の空白などは読み飛ばす
        self.line = self.line.lstrip(' \t\r\x1a')
        return self.line

    def get(self):
        """トークンを取得する"""
        def ismatch(r):
            """入力行と正規表現がマッチする部分を取り除く"""
            if m := re.match(r, self.line):
                self.line = self.line[m.end(0):]
                return m
            return None

        # ファイル終了
        if not self.getline():
            return BasToken.keyword(BasKeyword.EOF)
        # 行末
        elif self.line == '\n':
            self.line = ''
            return BasToken.keyword(BasKeyword.EOL)
        # コメント
        elif self.line[:2] == '/*':
            self.line = ''
            return BasToken.keyword(BasKeyword.EOL)
        # 文字列 "~"
        elif m := ismatch(r'"[^"\n]*("?)'):
            r = m.group(0)
            # 引用符を閉じずに行が終わっていたら補う
            r += '"' if m.group(1) != '"' else ''
            return BasToken.str(re.sub(r'\\',r'\\\\',r))
        # 文字 'x'
        elif m := ismatch(r'\'[^\']?\''):
            return BasToken.int(m.group(0))
        # 16進数 &Hxxxx
        elif m := ismatch(r'&[hH]([0-9a-fA-F]+)'):
            return BasToken.int('0x' + m.group(1))  # 0xXXXX に変換
        # 8進数 &Oxxxx
        elif m := ismatch(r'&[oO]([0-7]+)'):
            return BasToken.int('0' + m.group(1))   # 0XXXX に変換
        # 2進数 &Bxxxx
        elif m := ismatch(r'&[bB]([01]+)'):
            return BasToken.int('0b' + m.group(1))  # 0XXXX に変換
        # 整数
        elif m := ismatch(r'(\d+)'):
            # 冒頭の0は8進数に解釈されてしまうので取り除く
            return BasToken.int(re.sub(r'^0*(.)',r'\1',m.group(0)))
        # 変数名または予約語
        elif m := ismatch(r'[a-zA-Z_][a-zA-Z0-9_$]*'):
            return BasToken.variable(m.group(0))
        # 演算子
        elif w := BasKeyword.keywordop.get(self.line[:1], None):
            self.line = self.line[1:]
            return BasToken.keyword(w)
        # その他の文字は記号
        else:
            c = self.line[0]
            self.line = self.line[1:]
            return BasToken.symbol(c)

    def fetch(self):
        """トークンを取得する (先読みされていたものがあればそれを返す)"""
        if self.cached:
            return self.cached.pop()
        return self.get()

    def unfetch(self, t):
        """一度読んだトークンを戻す"""
        self.cached.append(t)
        return None

class Bas2C:
    def __init__(self, fh):
        self.fh = fh
        self.t = BasTokenGen(fh)

    def expect(self, v):
        """実行結果がNoneでないことを確認する"""
        if not v:
            raise Exception('構文エラー')
        return v

    def nextsymbol(self, s):
        """次のトークンがシンボルsであることを確認する"""
        return self.expect(self.t.fetch().issymbol(s))

    def checkkeyword(self, k):
        """予約語kが出たら読み進む"""
        if (x := self.t.fetch()).iskeyword(k):
            return x
        return self.t.unfetch(x)
    def checksymbol(self, s):
        """シンボルsが出たら読み進む"""
        if (x := self.t.fetch()).issymbol(s):
            return x
        return self.t.unfetch(x)

    def expr(self):
        """"式を解析、変換してトークンで返す"""

        def checkops(self, map):
            """次のトークンが予約語でmap内にあるなら読み進む"""
            t = self.expect(self.t.fetch())
            if t.istype(BasToken.KEYWORD) and t.value in map:
                return t
            return self.t.unfetch(t)

        def addsub(self):
            if not (r := muldiv(self)):
                return None
            if r.istype(BasToken.STR):        # 文字列の連結
                if not self.checkkeyword(BasKeyword.PLUS):
                    return r
                r = BasToken.str(f'b_stradd(strtmp,{r.value},')
                while True:
                    a = self.expect(muldiv(self))
                    self.expect(a.istype(BasToken.STR))
                    r = BasToken.str(f'{r.value}{a.value},')
                    if not self.checkkeyword(BasKeyword.PLUS):
                        break
                return BasToken.str(f'{r.value}-1)')
            else:
                map = {BasKeyword.PLUS: '+', BasKeyword.MINUS: '-'}
                while p := checkops(self, map):
                    a = self.expect(muldiv(self))
                    r = BasToken.int(f'({r.value} {map[p.value]} {a.value})')
                return r

        def muldiv(self):
            if not (r := posneg(self)):
                return None
            map = {BasKeyword.MUL: '*', BasKeyword.DIV: '/'}
            while p := checkops(self, map):
                a = self.expect(posneg(self))
                r = BasToken.int(f'({r.value} {map[p.value]} {a.value})')
            return r

        def posneg(self):
            map = {BasKeyword.PLUS: '+', BasKeyword.MINUS: '-'}
            if p := checkops(self, map):
                r = self.expect(posneg(self))
                return BasToken.int(map[p.value] + r.value)
            return paren(self)

        def paren(self):
            if self.checksymbol('('):
                r = self.expect(self.expr())
                self.nextsymbol(')')
                return BasToken.int(f'({r.value})')
            return atom(self)

        def atom(self):
            r = self.t.fetch()
            if r.isconst():                             # 定数
                return r
            return None

        return addsub(self)


if __name__ == '__main__':
    # 入力式を解析する
    b = Bas2C(sys.stdin)
    while True:
        print(b.expr())
