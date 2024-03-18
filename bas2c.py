#!/bin/env python3

class BasToken:
    """X-BASICのトークン"""
    SYMBOL   = 0
    INT      = 1
    STR      = 2

    def __init__(self, type, value):
        self.type = type
        self.value = value

    def __repr__(self):
        return f'({self.type},{self.value})'
