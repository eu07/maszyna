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
    cParser(std::string const &Stream, buffertype const Type = buffer_TEXT, std::string Path = "", bool const Loadtraction = true, std::vector<std::string> Parameters = std::vector<std::string>() );
    // destructor:
    virtual ~cParser();
    // methods:
    template <typename Type_>
    cParser &
        operator>>( Type_ &Right );
    template <typename Output_>
	Output_
		getToken( bool const ToLower = true, char const *Break = "\n\r\t ;" ) {
            getTokens( 1, ToLower, Break );
		    Output_ output;
            *this >> output;
		    return output; };
    inline
    void
        ignoreToken() {
            readToken(); };
    inline
    bool
        expectToken( std::string const &Value ) {
            return readToken() == Value; };
    inline
    bool
        eof() {
            return mStream->eof(); };
    inline
    bool
        ok() {
            return ( !mStream->fail() ); };
    cParser &
        autoclear( bool const Autoclear );
    inline
    bool
        autoclear() const {
            return m_autoclear; }
    bool
        getTokens( unsigned int Count = 1, bool ToLower = true, char const *Break = "\n\r\t ;" );
    // returns next incoming token, if any, without removing it from the set
    inline
    std::string
        peek() const {
            return (
                false == tokens.empty() ?
                    tokens.front() :
                    "" ); }
	// inject string as internal include
	void injectString(const std::string &str);

    // returns percentage of file processed so far
    int getProgress() const;
    int getFullProgress() const;
    //
    static std::size_t countTokens( std::string const &Stream, std::string Path = "" );
    // add custom definition of text which should be ignored when retrieving tokens
    void addCommentStyle( std::string const &Commentstart, std::string const &Commentend );
    // returns name of currently open file, or empty string for text type stream
    std::string Name() const;
    // returns number of currently processed line
    std::size_t Line() const;
	// returns number of currently processed line in main file, -1 if inside include
	int LineMain() const;
	bool expandIncludes = true;
	bool allowRandomIncludes = false;

  private:
    // methods:
    std::string readToken(bool ToLower = true, const char *Break = "\n\r\t ;");
    std::vector<std::string> readParameters( cParser &Input );
    std::string readQuotes( char const Quote = '\"' );
    void skipComment( std::string const &Endmark );
    bool findQuotes( std::string &String );
    bool trimComments( std::string &String );
    std::size_t count();
    // members:
    bool m_autoclear { true }; // unretrieved tokens are discarded when another read command is issued (legacy behaviour)
    bool LoadTraction { true }; // load traction?
    std::shared_ptr<std::istream> mStream; // relevant kind of buffer is attached on creation.
    std::string mFile; // name of the open file, if any
    std::string mPath; // path to open stream, for relative path lookups.
    std::streamoff mSize { 0 }; // size of open stream, for progress report.
    std::size_t mLine { 0 }; // currently processed line
    bool mIncFile { false }; // the parser is processing an *.inc file
    bool mFirstToken { true }; // processing first token in the current file; helper used when checking for utf bom
    typedef std::map<std::string, std::string> commentmap;
    commentmap mComments {
        commentmap::value_type( "/*", "*/" ),
        commentmap::value_type( "//", "\n" ) };
    std::shared_ptr<cParser> mIncludeParser; // child class to handle include directives.
    std::vector<std::string> parameters; // parameter list for included file.
    std::deque<std::string> tokens;
};


template <>
glm::vec3
cParser::getToken( bool const ToLower, const char *Break );


template<typename Type_>
cParser&
cParser::operator>>( Type_ &Right ) {

    if( true == this->tokens.empty() ) { return *this; }

    std::stringstream converter( this->tokens.front() );
    converter >> Right;
    this->tokens.pop_front();

    return *this;
}

template<>
cParser&
cParser::operator>>( std::string &Right );

template<>
cParser&
cParser::operator>>( bool &Right );

template<>
bool
cParser::getToken<bool>( bool const ToLower, const char *Break );
