#!/bin/env python3
import sys
import re

class BasKeyword:
    EOF         = 0
    EOL         = 9999

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

    def __repr__(self):
        return f'({self.type},{self.value})'

class BasTokenGen:
    """ソースコードからトークンを生成するクラス"""
    def __init__(self, fh=sys.stdin):
        self.fh = fh
        self.line = ''

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
        # 整数
        elif m := ismatch(r'(\d+)'):
            return BasToken.int(m.group(0))
        # 変数名または予約語
        elif m := ismatch(r'[a-zA-Z_][a-zA-Z0-9_$]*'):
            return BasToken.variable(m.group(0))
        # その他の文字は記号
        else:
            c = self.line[0]
            self.line = self.line[1:]
            return BasToken.symbol(c)


if __name__ == '__main__':
    # 入力をトークンに分割して表示
    t = BasTokenGen()
    while True:
        tok = t.get()
        print(tok)
        if tok.type == BasToken.KEYWORD and tok.value == BasKeyword.EOF:
            break
    print(tok)
