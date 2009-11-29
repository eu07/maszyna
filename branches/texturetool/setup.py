from distutils.core import setup

import sys
import py2exe

sys.argv.append('py2exe')
 
setup(console=['texturetool.py'], options = {'py2exe': {'bundle_files': 1}}, zipfile=None)
