import re
import sys

class Include(object):
	def __init__(self, line, source, parameters):
		self.__line = int(line)
		self.__source = source
		self.__parameters = parameters

	def getLine(self): return self.__line
	def getSource(self): return self.__source
	def getParameters(self): return self.__parameters

def findIncludes(input, fileName = None, type = Include):
	delim = re.compile("[ ,;]+")
	result = list()
	count = 1

	for line in input:
		if line.startswith("include"):
			line = delim.sub(' ', line[:-1])
			matches = line.split(" ")
			if not fileName or matches[1] == fileName:
				result.append(type(count, matches[1], matches[2:]))

	return result

if __name__ == "__main__":
	input = file(sys.argv[1])

	includes = findIncludes(input)

	occurences = dict()

	for include in includes:
		source = include.getSource()
		if source in occurences:
			occurences[source] += 1
		else:
			occurences[source] = 1

	occurences = occurences.items()
	occurences.sort(lambda left, right: cmp(right[1], left[1]))

	for occurence in occurences:
		print "%s: %d" % occurence
