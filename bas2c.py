#!/bin/env python3
import sys
import re

class BasKeyword:
    EOF         = 0

    PLUS        = 1001
    MINUS       = 1002
    MUL         = 1003
    DIV         = 1004
    YEN         = 1005
    MOD         = 1006
    SHR         = 1007
    SHL         = 1008
    EQ          = 1009
    NE          = 1010
    GT          = 1011
    LT          = 1012
    GE          = 1013
    LE          = 1014
    NOT         = 1015
    AND         = 1016
    OR          = 1017
    XOR         = 1018

    PRINT       = 2001
    FOR         = 2002
    TO          = 2003
    NEXT        = 2004

    EOL         = 9999

    keyword = {
        'print'     : PRINT,
        'for'       : FOR,
        'to'        : TO,
        'next'      : NEXT,

        'mod'       : MOD,
        'shr'       : SHR,
        'shl'       : SHL,
        'not'       : NOT,
        'and'       : AND,
        'or'        : OR,
        'xor'       : XOR,
    }

    keywordop = {
        '+'         : PLUS,
        '-'         : MINUS,
        '*'         : MUL,
        '/'         : DIV,
        '\\'        : YEN,
        '='         : EQ,
        '<>'        : NE,
        '>='        : GE,
        '<='        : LE,
        '>'         : GT,
        '<'         : LT,
    }

    @classmethod
    def find(cls, word):
        """wordが予約語ならその値を返す"""
        return cls.keyword.get(word, None)

    @classmethod
    def findop(cls, word):
        """wordの先頭に演算子があればそれと残りの文字列を返す"""
        if w := cls.keywordop.get(word[:2], None):
            return (w, word[2:])
        if w := cls.keywordop.get(word[:1], None):
            return (w, word[1:])
        return None        

class BasToken:
    """X-BASICのトークン"""
    SYMBOL   = 0
    INT      = 1
    FLOAT    = 2
    STR      = 3
    KEYWORD  = 4
    VARIABLE = 5

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
    def float(cls, value):
        return cls(cls.FLOAT, value)
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

    def resulttype(self, a=None):
        """トークン同士の演算結果に与える型を得る(strはエラー)"""
        if self.type == self.STR:
            return None
        if a != None:
            if a.type == self.STR:
               return None
            rty = self.type if self.type == a.type else self.FLOAT     # intとfloatの演算結果はfloat
        return rty

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
        # 実数 0. or .0 or 0#
        elif (m := ismatch(r'(\d+\.\d*([eE]\d+)?)#?')) or \
             (m := ismatch(r'(\d*\.\d+([eE]\d+)?)#?')) or \
             (m := ismatch(r'(\d+)#')):
            return BasToken.float('(double)' + m.group(1))
        # 整数
        elif m := ismatch(r'(\d+)'):
            # 冒頭の0は8進数に解釈されてしまうので取り除く
            return BasToken.int(re.sub(r'^0*(.)',r'\1',m.group(0)))
        # 変数名または予約語
        elif m := ismatch(r'[a-zA-Z_][a-zA-Z0-9_$]*'):
            if k := BasKeyword.find(m.group(0)):
                return BasToken.keyword(k)
            else:           # 変数名の '$' は 'S' に置き換える
                return BasToken.variable(m.group(0).replace('$','S'))
        # 演算子
        elif w := BasKeyword.findop(self.line):
            self.line = w[1]
            return BasToken.keyword(w[0])
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

    def nexttype(self, t):
        """次のトークンが型tであることを確認して値を返す"""
        return self.expect(self.t.fetch().istype(t))
    def nextkeyword(self, k):
        """次のトークンが予約語kであることを確認する"""
        return self.expect(self.t.fetch().iskeyword(k))
    def nextsymbol(self, s):
        """次のトークンがシンボルsであることを確認する"""
        return self.expect(self.t.fetch().issymbol(s))

    def checktype(self, t):
        """型tが出たら読み進む"""
        if (x := self.t.fetch()).istype(t):
            return x
        return self.t.unfetch(x)
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

    def statement(self):
        """X-BASICの文を1つ読み込んで変換する"""
        while self.checksymbol(':'):
            pass
        if self.checkkeyword(BasKeyword.EOF):
            return None

        elif s := self.checktype(BasToken.KEYWORD):
            if s.value == BasKeyword.EOL:
                return ''

            elif s.value == BasKeyword.PRINT:
                r = ''
                crlf = True
                while True:
                    if x := self.expr():
                        if x.istype(BasToken.STR):
                            r += f'b_sprint({x.value});\n'
                        elif x.istype(BasToken.FLOAT):
                            r += f'b_fprint({x.value});\n'
                        else:
                            r += f'b_iprint({x.value});\n'
                        crlf = True
                    if self.checksymbol(';'):
                        crlf = False
                    elif self.checksymbol(','):
                        r += f'b_sprint(STRTAB);\n'
                        crlf = False
                    else:
                        break
                if crlf:
                    r += f'b_sprint(STRCRLF);\n'
                return r

            elif s.value == BasKeyword.FOR:
                v = self.expect(self.nexttype(BasToken.VARIABLE))
                self.nextkeyword(BasKeyword.EQ)
                f = self.expect(self.expr())
                self.nextkeyword(BasKeyword.TO)
                t = self.expect(self.expr())
                return f'for ({v} = {f.value}; {v} <= {t.value}; {v}++) ' + '{\n'

            elif s.value == BasKeyword.NEXT:
                return '}\n'

        else:
            return None

    def expr(self):
        """"式を解析、変換してトークンで返す"""

        def checkops(self, map):
            """次のトークンが予約語でmap内にあるなら読み進む"""
            t = self.expect(self.t.fetch())
            if t.istype(BasToken.KEYWORD) and t.value in map:
                return t
            return self.t.unfetch(t)

        def opxor(self):
            if not (r := opor(self)):
                return None
            while self.checkkeyword(BasKeyword.XOR):
                a = self.expect(opor(self))
                self.expect(r.resulttype(a))
                r = BasToken.int(f'((int){r.value} ^ (int){a.value})')
            return r

        def opor(self):
            if not (r := opand(self)):
                return None
            while self.checkkeyword(BasKeyword.OR):
                a = self.expect(opand(self))
                self.expect(r.resulttype(a))
                r = BasToken.int(f'((int){r.value} | (int){a.value})')
            return r

        def opand(self):
            if not (r := opnot(self)):
                return None
            while self.checkkeyword(BasKeyword.AND):
                a = self.expect(opnot(self))
                self.expect(r.resulttype(a))
                r = BasToken.int(f'((int){r.value} & (int){a.value})')
            return r

        def opnot(self):
            if self.checkkeyword(BasKeyword.NOT):
                r = self.expect(opnot(self))
                self.expect(r.resulttype())
                return BasToken.int(f'(~((int){r.value}))')
            return cmp(self)

        def cmp(self):
            if not (r := shrshl(self)):
                return None
            map = {BasKeyword.EQ: ('==', 0x3d20),\
                   BasKeyword.NE: ('!=', 0x3c3e),\
                   BasKeyword.GT: ('>',  0x3e20),\
                   BasKeyword.LT: ('<' , 0x3c20),\
                   BasKeyword.GE: ('>=', 0x3e3d),\
                   BasKeyword.LE: ('<=', 0x3c3d)}
            while p := checkops(self, map):
                a = self.expect(shrshl(self))
                if r.istype(BasToken.STR):
                    self.expect(a.istype(BasToken.STR))
                    r = BasToken.int(f'(b_strcmp({r.value},0x{map[p.value][1]:x},{a.value})?-1:0)')
                else:
                    r = BasToken.int(f'-({r.value} {map[p.value][0]} {a.value})')
            return r

        def shrshl(self):
            if not (r := addsub(self)):
                return None
            map = {BasKeyword.SHR: '>>', BasKeyword.SHL: '<<'}
            while p := checkops(self, map):
                a = self.expect(addsub(self))
                self.expect(r.resulttype(a))
                r = BasToken.int(f'((int){r.value} {map[p.value]} (int){a.value})')
            return r

        def addsub(self):
            if not (r := mod(self)):
                return None
            if r.istype(BasToken.STR):        # 文字列の連結
                if not self.checkkeyword(BasKeyword.PLUS):
                    return r
                r = BasToken.str(f'b_stradd(strtmp,{r.value},')
                while True:
                    a = self.expect(mod(self))
                    self.expect(a.istype(BasToken.STR))
                    r = BasToken.str(f'{r.value}{a.value},')
                    if not self.checkkeyword(BasKeyword.PLUS):
                        break
                return BasToken.str(f'{r.value}-1)')
            else:
                map = {BasKeyword.PLUS: '+', BasKeyword.MINUS: '-'}
                while p := checkops(self, map):
                    a = self.expect(mod(self))
                    rty = self.expect(r.resulttype(a))
                    r = BasToken(rty, f'({r.value} {map[p.value]} {a.value})')
                return r

        def mod(self):
            if not (r := yen(self)):
                return None
            while self.checkkeyword(BasKeyword.MOD):
                a = self.expect(yen(self))
                self.expect(r.resulttype(a))
                r = BasToken.int(f'((int){r.value} % (int){a.value})')
            return r

        def yen(self):
            if not (r := muldiv(self)):
                return None
            while self.checkkeyword(BasKeyword.YEN):
                a = self.expect(muldiv(self))
                self.expect(r.resulttype(a))
                r = BasToken.int(f'((int){r.value} / (int){a.value})')
            return r

        def muldiv(self):
            if not (r := posneg(self)):
                return None
            map = {BasKeyword.MUL: '*', BasKeyword.DIV: '/'}
            while p := checkops(self, map):
                a = self.expect(posneg(self))
                rty = self.expect(r.resulttype(a))
                r = BasToken(rty, f'({r.value} {map[p.value]} {a.value})')
            return r

        def posneg(self):
            map = {BasKeyword.PLUS: '+', BasKeyword.MINUS: '-'}
            if p := checkops(self, map):
                r = self.expect(posneg(self))
                rty = self.expect(r.resulttype())
                return BasToken(rty, map[p.value] + r.value)
            return paren(self)

        def paren(self):
            if self.checksymbol('('):
                r = self.expect(self.expr())
                self.nextsymbol(')')
                return BasToken(r.type, f'({r.value})')
            return atom(self)

        def atom(self):
            r = self.t.fetch()
            if r.isconst():                             # 定数
                return r
            elif r.istype(BasToken.VARIABLE):
                return BasToken.int(r.value)
            return None

        return opxor(self)

##############################################################################

    def start(self):
        while True:
            s = self.statement()
            if s == None:
                break
            if s:
                print(s, end='')

##############################################################################

if __name__ == '__main__':
    if len(sys.argv) < 2:
        fh = sys.stdin
    else:
        fh = open(sys.argv[1], 'r', encoding='cp932')
    b = Bas2C(fh)
    b.start()

    sys.exit(0)
