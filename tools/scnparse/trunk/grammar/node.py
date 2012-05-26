from pyparsing import *
from common import Identifier, UIntNum, DecimalNum, Position, FileName

# common
NodeTag = CaselessKeyword('node')
Preamble = \
    NodeTag + \
    DecimalNum('max_dist') + \
    DecimalNum('min_dist') + \
    Identifier('Name')

# track
TrackType = oneOf('normal switch road river')
Environment = oneOf('flat mountains canyon tunnel')

TrackMaterial = \
    Group(
        FileName('tex') + \
        DecimalNum('scale'))('ballast') + \
    Group(    
        FileName('tex') + \
        DecimalNum('scale'))('rail') + \
    Group(    
        DecimalNum('height') + \
        DecimalNum('width') + \
        DecimalNum('slope'))('ballast');

TrackGeometry = \
    Position('point_1') + \
    DecimalNum('roll_1') + \
    Position('control_1') + \
    Position('control_2') + \
    Position('point_2') + \
    DecimalNum('roll_2') + \
    DecimalNum('radius')

SwitchGeometry = \
    TrackGeometry + \
    Position('point_3') + \
    DecimalNum('roll_3') + \
    Position('control_3') + \
    Position('control_4') + \
    DecimalNum('roll_4') + \
    DecimalNum('radius_2')

TrackParameters = \
    Preamble + \
    CaselessKeyword('track') + \
    TrackType('kind') + \
    DecimalNum('length') + \
    DecimalNum('width') + \
    DecimalNum('friction') + \
    DecimalNum('sound_dist') + \
    UIntNum('quality') + \
    UIntNum('damage_flag') + \
    Environment('environment')
