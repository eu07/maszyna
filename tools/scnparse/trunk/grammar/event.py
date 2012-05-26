from pyparsing import *
from decimal import Decimal

from common import DecimalNum, Identifier

# events
EventTag = CaselessKeyword('event');
EndEventTag = CaselessKeyword('endevent');

# lights event
LightState = oneOf('0 1 2');
LightState.setParseAction(lambda tokens: int(tokens[0]));

Lights = \
    EventTag + \
    Identifier('name') + \
    CaselessKeyword('lights') + \
    DecimalNum('delay') + \
    Identifier('target') + \
    OneOrMore(LightState)('state') + \
    EndEventTag;

# animation event
Animation = \
    EventTag + \
    Identifier('name') + \
    CaselessKeyword('animation') + \
    DecimalNum('delay') + \
    Identifier('target') + \
    oneOf('rotate translate', caseless=True)('kind') + \
    Identifier('submodel') + \
    DecimalNum('x') + \
    DecimalNum('y') + \
    DecimalNum('z') + \
    DecimalNum('speed') + \
    EndEventTag;

# track velocity event
TrackVel = \
    EventTag + \
    Identifier('name') + \
    CaselessKeyword('trackvel') + \
    DecimalNum('delay') + \
    Identifier('target') + \
    DecimalNum('velocity') + \
    EndEventTag;

# update values event
CommandValue = DecimalNum | Keyword('*');

UpdateValues = \
    EventTag + \
    Identifier('name') + \
    CaselessKeyword('updatevalues') + \
    DecimalNum('delay') + \
    Identifier('target') + \
    Identifier('command') + \
    CommandValue('first') + \
    CommandValue('second') + \
    EndEventTag;

# get values event
GetValues = \
    EventTag + \
    Identifier('name') + \
    CaselessKeyword('getvalues') + \
    DecimalNum('delay') + \
    Identifier('target') + \
    EndEventTag;

# put values
# putValuesEvent = \

