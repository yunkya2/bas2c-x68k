#!/bin/env python3
import sys
import re

class BasKeyword:
    EOF         = 0

    INT         = 1
    CHAR        = 2
    FLOAT       = 3
    STR         = 4
    DIM         = 5

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
    GOTO        = 2005
    GOSUB       = 2006
    IF          = 2007
    THEN        = 2008
    ELSE        = 2009
    END         = 2010
    RETURN      = 2011
    FUNC        = 2012
    ENDFUNC     = 2013
    WHILE       = 2014
    ENDWHILE    = 2015
    REPEAT      = 2016
    UNTIL       = 2017
    BREAK       = 2018
    CONTINUE    = 2019
    SWITCH      = 2020
    CASE        = 2021
    DEFAULT     = 2022
    ENDSWITCH   = 2023
    INPUT       = 2024
    LINPUT      = 2025
    LPRINT      = 2026
    USING       = 2027
    TAB         = 2028
    LOCATE      = 2029
    ERROR       = 2030

    EOL         = 9999

    keyword = {
        'print'     : PRINT,
        'for'       : FOR,
        'to'        : TO,
        'next'      : NEXT,
        'goto'      : GOTO,
        'gosub'     : GOSUB,
        'if'        : IF,
        'then'      : THEN,
        'else'      : ELSE,
        'end'       : END,
        'return'    : RETURN,
        'func'      : FUNC,
        'endfunc'   : ENDFUNC,
        'while'     : WHILE,
        'endwhile'  : ENDWHILE,
        'repeat'    : REPEAT,
        'until'     : UNTIL,
        'break'     : BREAK,
        'continue'  : CONTINUE,
        'switch'    : SWITCH,
        'case'      : CASE,
        'default'   : DEFAULT,
        'endswitch' : ENDSWITCH,
        'input'     : INPUT,
        'linput'    : LINPUT,
        'lprint'    : LPRINT,
        'using'     : USING,
        'tab'       : TAB,
        'locate'    : LOCATE,
        'error'     : ERROR,

        'int'       : INT,
        'char'      : CHAR,
        'float'     : FLOAT,
        'str'       : STR,
        'dim'       : DIM,

        'mod'       : MOD,
        'shr'       : SHR,
        'shl'       : SHL,
        'not'       : NOT,
        'and'       : AND,
        'or'        : OR,
        'xor'       : XOR,
    }
    keywordop = {
        '?'         : PRINT,
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

class BasVariable:
    """変数/関数の型と名前を保持するクラス"""
    INT         = BasKeyword.INT    # 1
    CHAR        = BasKeyword.CHAR   # 2
    FLOAT       = BasKeyword.FLOAT  # 3
    STR         = BasKeyword.STR    # 4

    DIM         = 10
    DIM_INT     = DIM + INT         # 11
    DIM_CHAR    = DIM + CHAR        # 12
    DIM_FLOAT   = DIM + FLOAT       # 13
    DIM_STR     = DIM + STR         # 14

    def __init__(self, name, type, arg='', init='', func=False):
        self.name = name
        self.type = type
        self.arg = arg
        self.init = init
        self.func = func

    def isarray(self):
        """配列であればTrue"""
        return self.type >= self.DIM

    def typename(self, fnres=False):
        map = { self.INT:   'int', \
                self.CHAR:  'unsigned char', \
                self.FLOAT: 'double', \
                self.STR:   'unsigned char' }
        if fnres and self.type == self.STR:
            return 'unsigned char *'        # strを返す関数の戻り値型
        return map[self.type] if self.type in map else map[self.type - self.DIM]

    def definition(self, globl=False):
        if self.func:
            return f'{self.typename()} {self.name}({self.arg});\n'
        else:
            r = 'static ' if globl else ''
            r += f'{self.typename()} {self.name}{self.arg}'
            if self.init:
                r += f' = {self.init}'
            return r + ';\n'

    def __repr__(self):
        return f'({self.typename()},{self.name},{self.type},{self.arg},{self.init},{self.func})'

class BasNameSpace:
    """グローバル/ローカル名前空間を保持するクラス"""
    def __init__(self):
        self.glist = {}
        self.llist = None
        self.bpass = 0

    def setlocal(self, enable):
        """ローカル名前空間を有効にする/無効にする"""
        self.llist = {} if enable else None

    def setpass(self, bpass):
        self.bpass = bpass

    def find(self, name, lonly=False):
        """名前がグローバルorローカル名前空間に定義されているかを調べる"""
        if self.llist != None:
            if v := self.llist.get(name, None):
                return v
            if lonly:
                return None
        return self.glist.get(name, None)

    def new(self, name, type, arg='', init='', func=False, forceglobl=False):
        """変数を名前空間に定義する"""
        if forceglobl or (self.llist == None):  # グローバル変数定義
            if self.bpass == 1:                 # 変数定義を行うのはpass1のみ
                if self.glist.get(name, None):
                    raise Exception('変数の多重定義')
                self.glist[name] = BasVariable(name, type, arg, init, func)
            return self.glist[name]
        else:                                   # ローカル変数定義
            if self.llist.get(name, None):
                raise Exception('変数の多重定義')
            self.llist[name] = BasVariable(name, type, arg, init, func)
            return self.llist[name]

    def definition(self):
        """グローバル名前空間に定義されている変数の定義リストを出力する"""
        r = ''
        for k in self.glist:
            r += self.glist[k].definition(True)
        return r

class BasToken:
    """X-BASICのトークン"""
    SYMBOL   = 0
    INT      = BasKeyword.INT       # 1
    CHAR     = BasKeyword.CHAR      # 2
    FLOAT    = BasKeyword.FLOAT     # 3
    STR      = BasKeyword.STR       # 4
    KEYWORD  = 5
    VARIABLE = 6
    FUNCTION = 7

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
    @classmethod
    def function(cls, value):
        return cls(cls.FUNCTION, value)

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
    def isvartype(self):
        """変数型を表すトークンならTrue"""
        return self.type == self.KEYWORD and \
           (self.value >= self.INT and self.value <= self.STR)

    def resulttype(self, a=None):
        """トークン同士の演算結果に与える型を得る(strはエラー)"""
        if self.type == self.STR:
            return None
        rty = self.type if self.type != self.CHAR else self.INT # char->intとする
        if a != None:
            if a.type == self.STR:
               return None
            aty = a.type if a.type != self.CHAR else self.INT   # char->intとする
            rty = rty if rty == aty else self.FLOAT     # intとfloatの演算結果はfloat
        return rty

    def __repr__(self):
        return f'({self.type},{self.value})'

class BasTokenGen:
    """ソースコードからトークンを生成するクラス"""
    def __init__(self, fh=sys.stdin):
        if fh == sys.stdin:
            # 標準入力は巻き戻せないので一度すべてを読み込む
            self.filebuf = ''
            while l := fh.readline():
                if l[0] == '\x1a':
                    break
                self.filebuf += l
            self.fh = None
        else:
            self.fh = fh
        self.rewind()

    def rewind(self):
        """ファイルを巻き戻す"""
        if self.fh:
            self.fh.seek(0)
        else:
            self.fp = 0
        self.line = ''
        self.lineno = 0
        self.baslineno = 0
        self.cached = []

    def getline(self):
        """必要があれば1行読み込む"""
        if len(self.line) == 0:
            if self.fh:
                self.line = self.fh.readline()
            else:
                if self.fp < len(self.filebuf):
                    n = self.filebuf.find('\n', self.fp)
                    if n < 0:
                        self.line = self.filebuf[self.fp:]
                        self.fp = len(self.filebuf)
                    else:
                        self.line = self.filebuf[self.fp:n + 1]
                        self.fp = n + 1
            self.lineno += 1
            self.baslineno = 0
            # 行番号があれば取得する
            if m := re.match(r'[ \t]*(\d+)', self.line):
                self.baslineno = int(m.group(1))
                self.line = self.line[m.end():]
        # 行頭の空白などは読み飛ばす
        self.line = self.line.lstrip(' \t\r\x1a')
        return self.line

    def getbaslineno(self):
        """BASICの行番号を取得する"""
        r = self.baslineno
        self.baslineno = 0      # 取得できるのは一度だけ
        return r

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

##############################################################################

class Bas2C:
    def __init__(self, fh):
        self.fh = fh
        self.t = BasTokenGen(fh)
        self.label = []
        self.subr = []
        self.g = BasNameSpace()
        self.strtmp = 0
        self.strtmp_max = 0
        self.setpass(0)

    def setpass(self, bpass):
        """変換パスを設定する"""
        self.bpass = bpass
        self.mainend = False
        self.updatestrtmp()
        self.g.setpass(bpass)
        self.g.setlocal(False)
        self.initmp = 0
        self.t.rewind()

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

    def peek(self):
        """次のトークンを先読みする"""
        t = self.expect(self.t.fetch())
        self.t.unfetch(t)
        return t

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
    def checkvartype(self):
        """変数型を表すトークンが出たら読み進む"""
        if (x := self.t.fetch()).isvartype():
            return x
        return self.t.unfetch(x)

    def gendefine(self):
        """グローバル変数、関数の定義を出力する"""
        r = self.g.definition()
        for l in self.subr:         # サブルーチンのプロトタイプを出力する
            r += f'void S{l:05d}(void);\n'
        return r

    def genlabel(self):
        """必要ならGOTO飛び先のラベル定義、GOSUB飛び先の関数定義を出力する"""
        if l := self.t.getbaslineno():
            if l in self.label:
                return f'L{l:06d}:\n'
            elif l in self.subr:
                r = ''
                if not self.mainend:
                    r = '}\n'
                    self.mainend = True
                return r + f'void S{l:06d}(void)\n' + '{\n'
        return ''

    def updatestrtmp(self):
        """文字列処理用一時変数の最大数を更新する"""
        self.strtmp_max = max(self.strtmp, self.strtmp_max)
        self.strtmp = 0

    def statement(self):
        """X-BASICの文を1つ読み込んで変換する"""
        while self.checksymbol(':'):
            pass
        if self.checkkeyword(BasKeyword.EOF):
            return None

        self.updatestrtmp()

        # 変数定義 (int/char/float/str)
        if s := self.checkvartype():
            return self.defvar(s.value)

        elif s := self.checktype(BasToken.KEYWORD):
            if s.value == BasKeyword.EOL:
                return ''

            elif s.value == BasKeyword.DIM:
                ty = BasVariable.INT
                if t := self.checkvartype():
                    ty = t.value
                return self.defvar(ty)

            elif s.value == BasKeyword.PRINT or s.value == BasKeyword.LPRINT:
                lp = '' if s.value == BasKeyword.PRINT else 'l'
                r = ''
                crlf = True
                if self.checkkeyword(BasKeyword.USING):
                    fmt = self.expect(self.expr())
                    self.expect(fmt.istype(BasToken.STR))
                    self.nextsymbol(';')
                    r = f'b_s{lp}print(using(strtmp{self.strtmp},{fmt.value}'
                    self.strtmp += 1
                    while True:
                        if x := self.expr():
                            if x.istype(BasToken.STR):
                                r += f',{x.value}'
                            else:
                                r += f',(double)({x.value})'
                        if not self.checksymbol(','):
                            break
                    r += '));\n'
                    crlf = not self.checksymbol(';')
                else:
                    while True:
                        if x := self.expr():
                            if x.istype(BasToken.STR):
                                r += f'b_s{lp}print({x.value});\n'
                            elif x.istype(BasToken.FLOAT):
                                r += f'b_f{lp}print({x.value});\n'
                            else:
                                r += f'b_i{lp}print({x.value});\n'
                            crlf = True
                        elif self.checkkeyword(BasKeyword.TAB):
                            self.nextsymbol('(')
                            x = self.expect(self.expr())
                            self.nextsymbol(')')
                            r += f'b_t{lp}print({x.value});\n'
                            crlf = True

                        if self.checksymbol(';'):
                            crlf = False
                        elif self.checksymbol(','):
                            r += f'b_s{lp}print(STRTAB);\n'
                            crlf = False
                        else:
                            break
                if crlf:
                    r += f'b_s{lp}print(STRCRLF);\n'
                return r

            elif s.value == BasKeyword.INPUT:
                pstr = '"? "'
                if p := self.checktype(BasToken.STR):
                    pstr = p.value
                    if self.checksymbol(';'):
                        pstr += ' "? "'
                    else:
                        self.nextsymbol(',')
                r = f'b_input({pstr:s}'

                while True:
                    a = self.expect(self.lvalue())
                    if a.istype(BasToken.STR):
                        at = f'sizeof({a.value})'
                        av = a.value
                    else:
                        map = {BasToken.INT:0x204, BasToken.CHAR:0x201, BasToken.FLOAT:0x208}
                        at = f'0x{map[a.type]:x}'
                        av = '&'+a.value
                    r += f',{at},{av}'
                    if not self.checksymbol(','):
                        break
                r += ',-1);\n'
                return r

            elif s.value == BasKeyword.LINPUT:
                r = ''
                if p := self.checktype(BasToken.STR):
                    self.nextsymbol(';')
                    r += f'b_sprint({p.value});\n'
                a = self.expect(self.lvalue())
                self.expect(a.istype(BasToken.STR))
                return r + f'b_linput({a.value},sizeof({a.value}));\n'

            elif s.value == BasKeyword.IF:
                r = ''
                while True:
                    x = self.expect(self.expr())
                    self.nextkeyword(BasKeyword.THEN)
                    r += f'if ({x.value}) ' + '{\n'
                    # then節の命令取り込み
                    if self.checksymbol('{'):           # { } で囲まれている場合
                        while not self.checksymbol('}'):
                            r += self.statement()
                        if not self.checkkeyword(BasKeyword.ELSE):
                            return r + '}\n'                        # elseなしで終わり
                    else:                               # { } で囲まれていない場合
                        while not self.checkkeyword(BasKeyword.ELSE):
                            r += self.statement()
                            if self.checkkeyword(BasKeyword.EOL):   # 行末が来たら
                                return r + '}\n'                    # elseなしで終わり
                            elif self.peek().issymbol('}'):
                                return r + '}\n'                    # elseなしで終わり
                    r += '} else '
                    if not self.checkkeyword(BasKeyword.IF):
                        r += '{\n'
                        # else節の命令取り込み
                        if self.checksymbol('{'):           # { } で囲まれている場合
                            while not self.checksymbol('}'):
                                r += self.statement()
                        else:                               # { } で囲まれていない場合
                            while not self.checkkeyword(BasKeyword.EOL):
                                if self.peek().issymbol('}'):
                                    break
                                r += self.statement()
                        return r + '}\n'

            elif s.value == BasKeyword.FOR:
                v = self.expect(self.lvalue())
                self.nextkeyword(BasKeyword.EQ)
                f = self.expect(self.expr())
                self.nextkeyword(BasKeyword.TO)
                t = self.expect(self.expr())
                return f'for ({v.value} = {f.value}; {v.value} <= {t.value}; {v.value}++) ' + '{\n'

            elif s.value == BasKeyword.NEXT:
                return '}\n'

            elif s.value == BasKeyword.WHILE:
                x = self.expect(self.expr())
                return f'while ({x.value}) ' + '{\n'

            elif s.value == BasKeyword.ENDWHILE:
                return '}\n'

            elif s.value == BasKeyword.REPEAT:
                return 'do {\n'

            elif s.value == BasKeyword.UNTIL:
                x = self.expect(self.expr())
                return '} ' + f'while (!({x.value}));\n'

            elif s.value == BasKeyword.SWITCH:
                x = self.expect(self.expr())
                return f'switch ({x.value}) ' + '{\n'

            elif s.value == BasKeyword.CASE:
                x = self.expect(self.expr())
                return f'case {x.value}:\n'

            elif s.value == BasKeyword.DEFAULT:
                return 'default:\n'

            elif s.value == BasKeyword.ENDSWITCH:
                return '}\n'

            elif s.value == BasKeyword.GOTO:
                l = int(self.nexttype(BasToken.INT))
                if self.bpass == 1:
                    self.label.append(l)
                return f'goto L{l:06d};\n'

            elif s.value == BasKeyword.GOSUB:
                l = int(self.nexttype(BasToken.INT))
                if self.bpass == 1:
                    self.subr.append(l)
                return f'S{l:06d}();\n'

            elif s.value == BasKeyword.FUNC:
                # 関数の戻り値型を取得する(指定されていなければint型とする)
                fty = BasVariable.INT
                if t := self.checkvartype():
                    fty = t.value

                # 関数名を取得する
                func = self.nexttype(BasToken.VARIABLE)

                # ローカル変数を初期化する
                self.g.setlocal(True)

                # 引数を取得する
                self.nextsymbol('(')
                if self.checksymbol(')'):
                    arg = 'void'
                else:
                    arg = ''
                    while True:
                        # 引数名を取得する
                        var = self.nexttype(BasToken.VARIABLE)
                        # 引数の型を取得する(指定されていなければint型とする)
                        vty = BasVariable.INT
                        if self.checksymbol(';'):
                            vty = self.expect(self.checkvartype()).value
                        # 引数をローカル変数として登録する
                        va = '[32+1]' if vty == BasVariable.STR else ''
                        v = self.g.new(var, vty, va)
                        arg += f'{v.typename()} {var}{va}'
                        # もう引数がないならループを抜ける
                        if not self.checksymbol(','):
                            break
                        arg += ','
                    self.nextsymbol(')')

                # 関数名はグローバルで登録する
                v = self.g.new(func, fty, arg, func=True, forceglobl=True)

                r = ''
                if not self.mainend:
                    r = '}\n'
                    self.mainend = True
                return r + f'\n{v.typename(True)} {func}({arg})\n' + '{\n'

            elif s.value == BasKeyword.ENDFUNC:
                self.g.setlocal(False)
                return '}\n'

            elif s.value == BasKeyword.RETURN:
                if self.checksymbol('('):
                    r = self.expr()
                    self.nextsymbol(')')
                    if r:
                        return f'return {r.value};\n'
                    else:
                        return 'return 0;\n'
                else:
                    return '}\n'

            elif s.value == BasKeyword.BREAK:
                self.checksymbol(';')
                return 'break;\n'

            elif s.value == BasKeyword.CONTINUE:
                return 'continue;\n'

            elif s.value == BasKeyword.LOCATE:
                r = ''
                x = self.expr()
                if x != None:
                    self.nextsymbol(',')
                    y = self.expect(self.expr())
                    r = f'locate({x.value},{y.value});\n'
                else:
                    self.nextsymbol(',')
                if self.checksymbol(','):
                    a = self.expect(self.expr())
                    r += f'b_csw({a.value});\n'
                return r

            elif s.value == BasKeyword.ERROR:
                x = self.expect(self.t.fetch())     # error命令は読み飛ばして無視
                return f'/* error {x.value} */\n'

            elif s.value == BasKeyword.END:
                return 'return;\n'

            else:
                raise Exception('構文エラー')

        elif s := self.lvalue(None, True):                  # 左辺値 or 関数名の場合
            if s.istype(BasToken.FUNCTION):
                return self.fncall(s.value).value + ';\n'   # 関数呼び出し
            self.nextkeyword(BasKeyword.EQ)
            x = self.initvar(s.type)                        # 代入する値を得る
            if s.type >= BasVariable.DIM:                   # 配列なら一時変数の内容をコピー
                v = self.g.find(s.value)
                r = f'static const {v.typename()} _initmp{self.initmp:04d}{v.arg} = {x};\n'
                r += f'memcpy({s.value}, _initmp{self.initmp:04d}, sizeof({s.value}));\n'
                self.initmp += 1
                return r
            if s.istype(BasToken.STR):                      # 文字列ならb_strncpy()
                return f'b_strncpy(sizeof({s.value}),{s.value},{x});\n'
            else:                                           # 単純変数への代入
                return f'{s.value} = {x};\n'

        else:
            return None

##############################################################################

    def lvalue(self, var=None, arrayok=False):
        """左辺値(代入可能な変数/配列)を得る"""
        var = self.expect(self.t.fetch() if not var else var)
        if not var.istype(BasToken.VARIABLE):
            return None
        v = self.g.find(var.value)
        if self.peek().issymbol('('):               # 配列または関数呼び出し
            if (not v) or (not v.isarray()):
                return BasToken.function(var.value) # 関数呼び出し
        else:                                       # 単純変数
            if not v:
                # 未定義変数への代入時はint型のグローバル変数として定義する
                self.g.new(var.value, BasVariable.INT, forceglobl=True)
                v = self.g.find(var.value)
        ty = v.type
        sub = ''
        if v.isarray():
            if self.checksymbol('('):
                sub = '['
                while True:
                    if a := self.expr():
                        sub += a.value
                    if not self.checksymbol(','):
                        break
                    sub += ']['
                self.nextsymbol(')')
                sub += ']'
                ty -= BasVariable.DIM
        if ty >= BasVariable.DIM and not arrayok:
            return None         # 配列全体は返せない
        return BasToken(ty, f'{v.name}{sub}')

    def defvar(self, ty):
        """変数/配列の定義"""
        r = ''
        while True:
            # 変数名を取得する
            var = self.nexttype(BasToken.VARIABLE)
            # 配列の最大要素番号、str変数のサイズを取得する
            s = ''
            rty = ty
            if self.checksymbol('('):   # ()が付いていたら配列 (dimがなくても)
                rty = ty + BasVariable.DIM
                while True:             # 配列の要素数を取得する
                    s += '[(' + self.expect(self.expr()).value + ')+1]'
                    if not self.checksymbol(','):
                        break
                self.nextsymbol(')')
            if ty == BasVariable.STR:   # 文字列型の場合はバッファサイズを得る
                if self.checksymbol('['):
                    s += '[' + self.expect(self.expr()).value + '+1]'
                    self.nextsymbol(']')
                else:
                    s += '[32+1]'       # デフォルトのバッファサイズ
            # 初期値が指定されていたら取得する
            x = self.initvar(rty) if self.checkkeyword(BasKeyword.EQ) else ''
            # 変数を登録する
            v = self.g.new(var, rty, s, x)
            # ローカル変数であればその場に定義を出力する(グローバル変数はパス2冒頭でまとめて出力)
            if self.g.llist != None:
                r += v.definition()
            # 複数の変数をまとめて定義するなら繰り返す
            if not self.checksymbol(','):
                break
        return r

    def initvar(self, ty):
        """変数/配列の初期値を得る"""
        if ty >= BasVariable.DIM:
            self.nextsymbol('{')            # 配列の場合
            n = '{'
            nest = 1
            while nest > 0:
                if self.checksymbol('{'):
                    n += '{'
                    nest += 1
                elif self.checksymbol('}'):
                    n += '}'
                    nest -= 1
                elif a := self.checktype(BasToken.SYMBOL):
                    n += a.value
                elif a := self.checkkeyword(BasKeyword.EOL):
                    n += '\n'
                else:
                    n += self.expect(self.expr()).value
                    self.checksymbol(',')
                    n += ','
            return n
        else:
            return self.expect(self.expr()).value

    def fncall(self, fn):
        """関数呼び出しを生成する"""
        v = self.g.find(fn)                 # 関数が定義済みか調べる
        self.expect(v or (self.bpass == 1)) # (パス1なら未定義でもよい)
        arg = ''                            # 引数を得る
        self.nextsymbol('(')
        while True:
            if a := self.expr():            # TBD 型チェック
                arg += a.value
            if not self.checksymbol(','):
                break
            arg += ', '
        self.nextsymbol(')')
        if not v:
            return BasToken.function(fn)    # 未定義関数だったらFUNCTION型を返す
        return BasToken(v.type, f'{fn}({arg})')

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
                r = BasToken.str(f'b_stradd(strtmp{self.strtmp},{r.value},')
                self.strtmp += 1
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
            elif v := self.lvalue(r):                   # 左辺値 or 関数名
                if v.istype(BasToken.FUNCTION):
                    return self.fncall(v.value)         # 関数呼び出し
                return v
            self.t.unfetch(r)                           # 該当なしなのでトークンを戻す
            return None

        return opxor(self)

##############################################################################

    def start(self):
        self.setpass(1)     # pass 1
        while True:
            if self.statement() == None:
                break

        self.setpass(2)     # pass 2
        print(self.gendefine(), end='')
        for _ in range(self.strtmp_max):
            print(f'static unsigned char strtmp{_}[258];')
        print('void main(int b_argc, char *b_argv[])\n{')
        while True:
            s = self.statement()
            print(self.genlabel(), end='')
            if s == None:
                break
            if s:
                print(s, end='')
        if not self.mainend:
            print('}')

##############################################################################

if __name__ == '__main__':
    if len(sys.argv) < 2:
        fh = sys.stdin
    else:
        fh = open(sys.argv[1], 'r', encoding='cp932')
    b = Bas2C(fh)
    b.start()

    sys.exit(0)
