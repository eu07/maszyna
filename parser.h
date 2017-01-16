/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>

/////////////////////////////////////////////////////////////////////////////////////////////////////
// cParser -- generic class for parsing text data, either from file or provided string

class cParser : public std::stringstream
{
  public:
    // parameters:
    enum buffertype
    {
        buffer_FILE,
        buffer_TEXT
    };
    // constructors:
    cParser(std::string Stream, buffertype Type = buffer_TEXT, std::string Path = "",
            bool tr = true);
    // destructor:
    virtual ~cParser();
    // methods:
    template <typename _Output>
	_Output
		getToken( bool const ToLower = true )
    {
        getTokens( 1, ToLower );
		_Output output;
        *this >> output;
		return output;
    };
	template <>
	bool
		getToken<bool>( bool const ToLower ) {

		return ( getToken<std::string>() == "true" );
	}
    inline void ignoreToken()
    {
        readToken();
    };
    inline void ignoreTokens(int count)
    {
        for (int i = 0; i < count; i++)
            readToken();
    };
    inline bool expectToken(std::string value)
    {
        return readToken() == value;
    };
    bool eof()
    {
        return mStream->eof();
    };
    bool ok()
    {
        return !mStream->fail();
    };
    bool getTokens(int Count = 1, bool ToLower = true, const char *Break = "\n\t ;");
    int getProgress() const; // percentage of file processed.
    // load traction?
    bool LoadTraction;

  protected:
    // methods:
    std::string readToken(bool ToLower = true, const char *Break = "\n\t ;");
    std::string readComment(const std::string Break = "\n\t ;");
    std::string trtest;
    bool trimComments(std::string &String);
    // members:
    std::istream *mStream; // relevant kind of buffer is attached on creation.
    std::string mPath; // path to open stream, for relative path lookups.
    std::streamoff mSize; // size of open stream, for progress report.
    typedef std::map<std::string, std::string> commentmap;
    commentmap mComments;
    cParser *mIncludeParser; // child class to handle include directives.
    std::vector<std::string> parameters; // parameter list for included file.
};

inline
cParser&
operator>>( cParser &Parser, bool &Right ) {

	std::istream::sentry const streamokay( Parser );
	if( streamokay ) {

		std::string value;
		Parser >> value;
		Right = ( value == "true" );
	}

	return Parser;
}
