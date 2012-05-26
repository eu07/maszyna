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

def testLights():
    result = Lights.parseString('event sem10_light11 lights 0.0 sem10 2 0 0 1 0 endevent').asDict();

    assert(result['name'] == 'sem10_light11');
    assert(result['delay'] == Decimal('0.0'));
    assert(result['target'] == 'sem10');
    
    assert(list(result['state']) == [2, 0, 0, 1, 0]);

def testAnimation():
    result = Animation.parseString('event Testowo_A_sem_anim21 animation 0 Testowo_A rotate Ramie01 0 -45 0 40 endevent').asDict();

    assert(result['name'] == 'Testowo_A_sem_anim21');
    assert(result['delay'] == Decimal('0.0'));
    assert(result['target'] == 'Testowo_A');

    assert(result['kind'] == 'rotate');
    assert(result['submodel'] == 'Ramie01');
    assert(result['x'] == Decimal('0.0'));
    assert(result['y'] == Decimal('-45.0'));
    assert(result['z'] == Decimal('0.0'));
    assert(result['speed'] == Decimal('40.0'));

def testTrackVel():
    result = TrackVel.parseString('event zwr_1_wbok trackvel 0.0 t_zwr_1 40.0 endevent').asDict();

    assert(result['name'] == 'zwr_1_wbok');
    assert(result['delay'] == Decimal('0.0'));
    assert(result['target'] == 't_zwr_1');

    assert(result['velocity'] == Decimal('40.0'));

def testUpdateValues():    
    result = UpdateValues.parseString('event start3b updatevalues 30.0 memcell_train3 SetVelocity 50 * endevent').asDict();

    assert(result['name'] == 'start3b');
    assert(result['delay'] == Decimal('30.0'));
    assert(result['target'] == 'memcell_train3');

    assert(result['command'] == 'SetVelocity');
    assert(result['first'] == Decimal('50.0'));
    assert(result['second'] == '*');
