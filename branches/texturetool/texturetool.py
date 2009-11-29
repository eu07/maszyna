import os
import os.path
import subprocess
import time
import optparse

import struct

class ImageUnsupportedException(Exception):
    def __init__(self, filename):
        self.__filename = filename

    def __str__(self):
        return "Image '%s' is not supported" % self.__filename

class ImageParseErrorException(Exception):
    def __init__(self, filename):
        self.__filename = filename

    def __str__(self):
        return "Error parsing '%s'" % self.__filename 

class ImageTraits(object):
    def __init__(self, filename):

        methodName = 'parse' + filename[filename.rfind('.')+1:].upper()

        if not hasattr(self, methodName):
            raise ImageUnsupportedException(filename)

        self.__parseImpl = getattr(self, methodName)
        self.__input = file(filename)
        self.parse()

    def hasAlpha(self): return self.__hasAlpha
    def getWidth(self): return self.__width
    def getHeight(self): return self.__height 

    def parse(self):
        self.__parseImpl(self.__input)
        self.__input.close()

    def parseTGA(self, input):
        compressed = input.read(12)
        self.__compressed = compressed[2] == '\x02';

        header = input.read(5)
        (self.__width, self.__height, bpp) = struct.unpack("HHB", header)
        self.__hasAlpha = bpp == 32

    def parseBMP(self, input):
        input.read(18) # discard first 18 characters (BITMAPHEADER and size information)
        
        header = input.read(8)
        (width, height) = struct.unpack("ii", header)

        self.__width = abs(width)
        self.__height = abs(height)
        self.__hasAlpha = False

    def parseTEX(self, input):
        format = input.read(4)
        self.__hasAlpha = format == "RGBA"

        header = input.read(8)
        (self.__width, self.__height) = struct.unpack("ii", header)

def locate_files(path, extensions):

    result = []
    
    def ext_matches(filename):
        filename = filename.lower()
        for ext in extensions:
            if filename.endswith(ext):
                return True
        return False

    for path, dirs, files in os.walk(path):
        result += [os.path.join(path, filename) for filename in files if ext_matches(filename)] 

    return result
    
def compress_textures(path, remove_original):
    
    files = locate_files(path, ['tga', 'bmp'])
    
    size = float(len(files))
    index = 1
    
    for path in files:
        print '[' + str(int(index / size * 100)) + '%] ' + path
        temp = path
        pos = temp.rfind('.')
        temp = temp[:pos] + '.tmp.tga'
        cmd = "nconvert -xflip -o %s %s" % (temp, path)
        
        #print cmd
        subprocess.Popen(["nconvert", "-yflip", "-out", "tga", "-o", temp, path], stdout=log, stderr=log).wait()
        #os.system(cmd)
        
        output = path
        pos = output.rfind('.')
        output = output[:pos] + '.dds'
            
        traits = ImageTraits(temp)
        cmd = ["nvcompress", temp, output]
        if traits.hasAlpha():
            cmd.insert(1, "-bc3")
        
        #print cmd
        subprocess.Popen(cmd, stdout=log).wait();
        #os.system(cmd) 
        
        os.remove(temp)
        if remove_original:
            os.remove(path)
            
        index += 1
        
def replace_entries(path):
    
    for path in locate_files(path, ['scn', 'scm', 'inc', 't3d']):
        
        print path
        f = file(path, 'r+')
        content = f.read()
        
        content = content.replace('.tga', '.dds')
        content = content.replace('.bmp', '.dds')
        
        f.truncate(0)
        f.write(content)

if __name__ == "__main__":
    parser = optparse.OptionParser("Usage: %prog [options] path")
    parser.add_option('--convert', help = 'Convert TGA textures to DDS', action = "store_true", dest="convert", default = False)
    parser.add_option('--update', help = 'Update SCN, SCN, INC and T3D files', action="store_true", dest="update", default = False)
    parser.add_option('--remove-temporary', help = 'Remove temporary files', action="store_true",  dest="remove_temporary", default = False)
    parser.add_option('--remove-originals', help = 'Remove original files', action="store_true", dest="remove_original", default = False)
    
    (options, path) = parser.parse_args()
    
    if len(path):
        path = path[0]
    else:
        path = '.'
    
    start = time.clock()
    log = file('tt-log.txt', 'w+');
    
    if options.remove_temporary:
        print "*** Removing temporary files:"
        for path in locate_files(path, ['.tmp.tga', '.tmp.bmp']):
            print path
            os.remove(path);
    
    if options.convert:
        print "*** Compressing textures:"
        compress_textures(path, options.remove_original); 
        
    if options.update:
        print "*** Replacing entries:"
        replace_entries(path);
    
    print "*** Total time: %1.2f s" % (time.clock() - start)
    
    log.close();