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

class cParser //: public std::stringstream
{
  public:
    // parameters:
    enum buffertype
    {
        buffer_FILE,
        buffer_TEXT
    };
    // constructors:
    cParser(std::string const &Stream, buffertype const Type = buffer_TEXT, std::string Path = "", bool const Loadtraction = true );
    // destructor:
    virtual ~cParser();
    // methods:
    template <typename _Type>
    cParser&
        operator>>( _Type &Right );
    template <>
    cParser&
        operator>>( std::string &Right );
    template <>
    cParser&
        operator>>( bool &Right );
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
    inline bool expectToken(std::string const &value)
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
    bool getTokens(unsigned int Count = 1, bool ToLower = true, const char *Break = "\n\t ;");
    // returns percentage of file processed so far
    int getProgress() const;
    // add custom definition of text which should be ignored when retrieving tokens
    void addCommentStyle( std::string const &Commentstart, std::string const &Commentend );

  private:
    // methods:
    std::string readToken(bool ToLower = true, const char *Break = "\n\t ;");
    std::string readQuotes( char const Quote = '\"' );
    std::string readComment( std::string const &Break = "\n\t ;" );
//    std::string trtest;
    bool findQuotes( std::string &String );
    bool trimComments( std::string &String );
    // members:
    bool LoadTraction; // load traction?
    std::istream *mStream; // relevant kind of buffer is attached on creation.
    std::string mPath; // path to open stream, for relative path lookups.
    std::streamoff mSize; // size of open stream, for progress report.
    typedef std::map<std::string, std::string> commentmap;
    commentmap mComments;
    cParser *mIncludeParser; // child class to handle include directives.
    std::vector<std::string> parameters; // parameter list for included file.
    std::deque<std::string> tokens;
};

template<typename _Type>
cParser&
cParser::operator>>( _Type &Right ) {

    if( true == this->tokens.empty() ) { return *this; }

    std::stringstream converter( this->tokens.front() );
    converter >> Right;
    this->tokens.pop_front();

    return *this;
}

template<>
cParser&
cParser::operator>>( std::string &Right ) {

    if( true == this->tokens.empty() ) { return *this; }

    Right = this->tokens.front();
    this->tokens.pop_front();

    return *this;
}

template<>
cParser&
cParser::operator>>( bool &Right ) {

    if( true == this->tokens.empty() ) { return *this; }

    Right = ( ( this->tokens.front() == "true" )
           || ( this->tokens.front() == "yes" )
           || ( this->tokens.front() == "1" ) );
    this->tokens.pop_front();

    return *this;
}
