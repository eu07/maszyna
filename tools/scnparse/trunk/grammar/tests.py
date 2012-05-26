import unittest
from decimal import Decimal

import event

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
        result['position'] = dict(result['position'])

        self.assertDictEqual(result, {
            # common 
            'name': 'Testowo_A_sem_anim21',
            'delay': Decimal('0.0'),
            'target': 'Testowo_A',
            # animation 
            'kind': 'rotate',
            'submodel': 'Ramie01',
            'position': {
                'x': Decimal('0.0'),
                'y': Decimal('-45.0'),
                'z': Decimal('0.0'),
            },    
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
        result = event.UpdateValues.parseString('event start3b updatevalues 30.0 memcell_train3 SetVelocity 50 * endevent').asDict()

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

    def testMultiple(self):
        result = event.Multiple.parseString('event semA_S13 multiple 0 none semA_light13 semA_S13set endevent').asDict()
        result['events'] = list(result['events'])

        self.assertDictEqual(result, {
            # common
            'name': 'semA_S13',
            'delay': Decimal('0.0'),
            'target': 'none',
            # multiple
            'events': ['semA_light13', 'semA_S13set']
        })

        result = event.Multiple.parseString('event przejazd_otwieraj multiple 2.0 tornaprzejezdzie przejazd_1_sygn1 przejazd_1_sygn2 condition trackfree endevent').asDict();
        result['events'] = list(result['events'])

        self.assertDictEqual(result, {
            # common
            'name': 'przejazd_otwieraj',
            'delay': Decimal('2.0'),
            'target': 'tornaprzejezdzie',
            # multiple
            'events': ['przejazd_1_sygn1', 'przejazd_1_sygn2'],
            'condition': 'trackfree'
        })  
       
        result = event.Multiple.parseString('event Zaczynek-Testowo1 multiple 3.0 Testowo-status Testowo-Zatwierdz Testowo-zwr1+ Testowo_ToA_os2 Testowo_A_S5 Testowo_D_S1 condition memcompare Rozwiazany * * endevent').asDict();
        result['events'] = list(result['events'])

        self.assertDictEqual(result, {
            # common
            'name': 'Zaczynek-Testowo1',
            'delay': Decimal('3.0'),
            'target': 'Testowo-status',
            # multiple
            'events': ['Testowo-Zatwierdz', 'Testowo-zwr1+', 'Testowo_ToA_os2', 'Testowo_A_S5', 'Testowo_D_S1'],
            'condition': 'memcompare',
            'command': 'Rozwiazany',
            'first': '*',
            'second': '*'
        })    
 
    def testSwitch(self):
        result = event.Switch.parseString('event Testowo_zwr1+ switch 0.0 Testowo_zwr1 1 endevent').asDict()

        self.assertDictEqual(result, {
            # common
            'name': 'Testowo_zwr1+',
            'delay': Decimal('0.0'),
            'target': 'Testowo_zwr1',
            # switch
            'state': 1
        })  
        
    def testSound(self):
        result = event.Sound.parseString('event wroclaw_wjezdza_zapowiedz sound 4.0 wroclaw_wjezdza_zapowiedz 1 endevent').asDict()
        
        self.assertDictEqual(result, {
            # common
            'name': 'wroclaw_wjezdza_zapowiedz',
            'delay': Decimal('4.0'),
            'target': 'wroclaw_wjezdza_zapowiedz',
            # switch
            'state': 1
        })        
