from pyparsing import *
from decimal import Decimal

# common
UIntNum = Word(nums)
UIntNum.setParseAction(lambda tokens: int(tokens[0]))

DecimalNum = Combine(Optional('-') + Word(nums) + Optional('.' + Word(nums)))
DecimalNum.setParseAction(lambda tokens: Decimal(tokens[0]))

Identifier = Word(alphanums + '_+-').setParseAction(lambda tokens: str(tokens[0]))

Position = Group(
    DecimalNum('x') + \
    DecimalNum('y') + \
    DecimalNum('z') \
)

FileName = Word(alphanums + '!#$%&()+,-./;=?[\\]^_`{}~')
