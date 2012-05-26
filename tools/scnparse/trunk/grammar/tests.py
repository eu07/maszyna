import unittest;
from decimal import Decimal;

import event;

class TestEventGrammar(unittest.TestCase):

    def testLights(self):
        result = event.Lights.parseString('event sem10_light11 lights 0.0 sem10 2 0 0 1 0 endevent').asDict()

        # normalize state list
        result['state'] = list(result['state'])

        self.assertDictEqual(result, {
            # common
            'name': 'sem10_light11',
            'delay': Decimal('0.0'),
            'target': 'sem10',
            # lights
            'state': [2, 0, 0, 1, 0]
        })

    def testAnimation(self):
        result = event.Animation.parseString('event Testowo_A_sem_anim21 animation 0 Testowo_A rotate Ramie01 0 -45 0 40 endevent').asDict()

        self.assertDictEqual(result, {
            # common 
            'name': 'Testowo_A_sem_anim21',
            'delay': Decimal('0.0'),
            'target': 'Testowo_A',
            # animation 
            'kind': 'rotate',
            'submodel': 'Ramie01',
            'x': Decimal('0.0'),
            'y': Decimal('-45.0'),
            'z': Decimal('0.0'),
            'speed': Decimal('40.0')
        })    

    def testTrackVel(self):
        result = event.TrackVel.parseString('event zwr_1_wbok trackvel 0.0 t_zwr_1 40.0 endevent').asDict()

        self.assertDictEqual(result, {
            # common 
            'name': 'zwr_1_wbok',
            'delay': Decimal('0.0'),
            'target': 't_zwr_1',
            # trackvel
            'velocity': Decimal('40.0')
        })    

    def testUpdateValues(self):    
        result = event.UpdateValues.parseString('event start3b updatevalues 30.0 memcell_train3 SetVelocity 50 * endevent').asDict();

        self.assertDictEqual(result, {
            # common
            'name': 'start3b',
            'delay': Decimal('30.0'),
            'target': 'memcell_train3',
            # updatevalues
            'command': 'SetVelocity',
            'first': Decimal('50.0'),
            'second': '*'
        })
