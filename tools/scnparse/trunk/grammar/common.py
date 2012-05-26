from pyparsing import *
from decimal import Decimal

# common
DecimalNum = Optional('-') + Word(nums) + Optional('.' + Word(nums));
DecimalNum.setParseAction(lambda tokens: Decimal(''.join(tokens)));

Identifier = Word(alphanums + '_+-').setParseAction(lambda tokens: str(tokens[0]));
