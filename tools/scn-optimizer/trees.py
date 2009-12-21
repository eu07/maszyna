import math
import re
import sys

from include import Include, findIncludes

class Tree(Include):
	def __init__(self, line, source, parameters):
		Include.__init__(self, line, source, parameters)
		self.__texture = parameters[0]
		self.__x = float(parameters[1])
		self.__y = float(parameters[2])
		self.__z = float(parameters[3])
		self.__width = float(parameters[6])
		self.__height = float(parameters[5])

	def getTexture(self): return self.__texture	

	def getX(self): return self.__x 
	def getY(self): return self.__y
	def getZ(self): return self.__z
	
	def getWidth(self): return self.__width
	def getHeight(self): return self.__height

def groupTrees(trees):
	result = dict()

	for tree in trees:
		texture = tree.getTexture()
		if texture in result:
			result[texture].append(tree)
		else:
			result[texture] = [tree]

	return result

def gridTrees(trees):
	grid = dict()

	for tree in trees:
		position = (int(tree.getX()) / 200, int(tree.getY()) / 200)
		if position in grid:
			grid[position].append(tree)
		else:
			grid[position] = [tree]	

	return grid

def getTreeCoordinates(tree):
	x, y, z, w, h = (tree.getX(), tree.getY(), tree.getZ(), tree.getWidth(), tree.getHeight())
	return {'x': x, 'y': y, 'z': z, 'xmw': x-w, 'xw': x+w, 'yh': x+h, 'zmw': z-w, 'zw': z+w}

if __name__ == "__main__":
	input = file(sys.argv[1])
#	output = file(sys.argv[2] if len(sys.argv) == 3 else "trees.scm", "w+")
	output = str()
	template = file("tree-triangle.inc").read()

	trees = findIncludes(input, "tree.inc", Tree)
	groups = groupTrees(trees)

	for group in groups.iteritems():
		texture = group[0]
		output += "// Texture: %s\n" % texture
		for cell in gridTrees(group[1]).iteritems():
			coords = (cell[0][0] * 400.0, cell[0][1] * 400.0)
			output += "// Cell %s containing %d trees\n" % (repr(coords), len(cell[1]))
			output += "node 1000 0 none triangles %s\n" % texture
			for tree in cell[1]:
				output += template % getTreeCoordinates(tree)
			output = output[:-5] + "\nendtri\n\n"

	file("trees.scm", "w+").write(output)
					
#			print str(cell[0]) + ": " + str(len(cell[1]))
		
#	occurences = dict()

#	for include in includes:
#		source = include.getSource()
#		if source in occurences:
#			occurences[source] += 1
#		else:
#			occurences[source] = 1

#	occurences = occurences.items()
#	occurences.sort(lambda left, right: cmp(right[1], left[1]))

#	for occurence in occurences:
#		print "%s: %d" % occurence
