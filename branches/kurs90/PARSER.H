
#if !defined(rainKERNELTEXTPARSER_H_INCLUDED)
#define rainKERNELTEXTPARSER_H_INCLUDED

#pragma warning ( disable : 4786 )			// 'containers too long for debug' warning

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <fstream>
#include <ctype.h>
//#include "Globals.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////
// cParser -- generic class for parsing text data, either from file or provided string

class cParser : public std::stringstream {

public:
// parameters:
	enum buffertype { buffer_FILE, buffer_TEXT };

// constructors:
	cParser( std::string Stream, buffertype Type = buffer_TEXT, std::string Path = "" );

// destructor:
	virtual ~cParser();

// methods:
	virtual bool	getTokens( int Count = 1, bool ToLower = true, const char* Break = "\n\t ,;" );
	virtual int		getProgress() const;	// percentage of file processed.

// load traction?
        bool LoadTraction;

protected:
// methods:
	std::string readToken( bool ToLower = true, const char* Break = "\n\t ,;", std::string trtest = "niemaproblema");
	std::string readComment( const std::string Break = "\n\t ,;" );
	std::string trtest;
	bool trimComments( std::string& String );

// members:
	std::istream *mStream;					// relevant kind of buffer is attached on creation.
	std::string mPath;						// path to open stream, for relative path lookups.
	int mSize;								// size of open stream, for progress report.
	typedef std::map<std::string, std::string> commentmap;
	commentmap mComments;
	cParser *mIncludeParser;				// child class to handle include directives.
	std::vector<std::string> parameters;	// parameter list for included file.
} ;

#endif // ..!defined(rainKERNELTEXTPARSER_H_INCLUDED)
