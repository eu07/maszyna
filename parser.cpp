/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "parser.h"
#include "utilities.h"
#include "Logs.h"

#include "scenenodegroups.h"

/*
    MaSzyna EU07 locomotive simulator parser
    Copyright (C) 2003  TOLARIS

*/

/////////////////////////////////////////////////////////////////////////////////////////////////////
// cParser -- generic class for parsing text data.

// constructors
cParser::cParser( std::string const &Stream, buffertype const Type, std::string Path, bool const Loadtraction, std::vector<std::string> Parameters ) :
    mPath(Path),
    LoadTraction( Loadtraction ) {
    // store to calculate sub-sequent includes from relative path
    if( Type == buffertype::buffer_FILE ) {
        mFile = Stream;
    }
    // reset pointers and attach proper type of buffer
    switch (Type) {
        case buffer_FILE: {
            Path.append( Stream );
			mStream = std::make_shared<std::ifstream>( Path, std::ios_base::binary );
            // content of *.inc files is potentially grouped together
            if( ( Stream.size() >= 4 )
             && ( ToLower( Stream.substr( Stream.size() - 4 ) ) == ".inc" ) ) {
                mIncFile = true;
                scene::Groups.create();
            }
            break;
        }
        case buffer_TEXT: {
            mStream = std::make_shared<std::istringstream>( Stream );
            break;
        }
        default: {
            break;
        }
    }
    // calculate stream size
    if (mStream)
    {
        if( true == mStream->fail() ) {
            ErrorLog( "Failed to open file \"" + Path + "\"" );
        }
        else {
            mSize = mStream->rdbuf()->pubseekoff( 0, std::ios_base::end );
            mStream->rdbuf()->pubseekoff( 0, std::ios_base::beg );
            mLine = 1;
        }
    }
    // set parameter set if one was provided
    if( false == Parameters.empty() ) {
        parameters.swap( Parameters );
    }
}

// destructor
cParser::~cParser() {

    if( true == mIncFile ) {
        // wrap up the node group holding content of processed file
        scene::Groups.close();
    }
}

template <>
glm::vec3
cParser::getToken( bool const ToLower, char const *Break ) {
    // NOTE: this specialization ignores default arguments
    getTokens( 3, false, "\n\r\t  ,;[]" );
    glm::vec3 output;
    *this
        >> output.x
        >> output.y
        >> output.z;
    return output;
};

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

template <>
bool
cParser::getToken<bool>( bool const ToLower, const char *Break ) {

    auto const token = getToken<std::string>( true, Break );
    return ( ( token == "true" )
          || ( token == "yes" )
          || ( token == "1" ) );
}

// methods
cParser &
cParser::autoclear( bool const Autoclear ) {

    m_autoclear = Autoclear;
    if( mIncludeParser ) { mIncludeParser->autoclear( Autoclear ); }

    return *this;
}

bool cParser::getTokens(unsigned int Count, bool ToLower, const char *Break)
{
    if( true == m_autoclear ) {
        // legacy parser behaviour
        tokens.clear();
    }
    /*
     if (LoadTraction==true)
      trtest="niemaproblema"; //wczytywać
     else
      trtest="x"; //nie wczytywać
    */
/*
    int i;
    this->str("");
    this->clear();
*/
    for (unsigned int i = tokens.size(); i < Count; ++i)
    {
        std::string token = readToken(ToLower, Break);
        if( true == token.empty() ) {
            // no more tokens
            break;
        }
        // collect parameters
        tokens.emplace_back( token );
/*
        if (i == 0)
            this->str(token);
        else
        {
            std::string temp = this->str();
            temp.append("\n");
            temp.append(token);
            this->str(temp);
        }
*/
    }
    if (tokens.size() < Count)
        return false;
    else
        return true;
}

std::string cParser::readToken( bool ToLower, const char *Break ) {

    std::string token;
    if( mIncludeParser ) {
        // see if there's include parsing going on. clean up when it's done.
        token = mIncludeParser->readToken( ToLower, Break );
        if( true == token.empty() ) {
            mIncludeParser = nullptr;
        }
    }
    if( true == token.empty() ) {
        // get the token yourself if the delegation attempt failed
        char c { 0 };
        do {
            while( mStream->peek() != EOF && strchr( Break, c = mStream->get() ) == NULL ) {
                if( ToLower )
                    c = tolower( c );
                token += c;
                if( findQuotes( token ) ) // do glue together words enclosed in quotes
                    continue;
                if( trimComments( token ) ) // don't glue together words separated with comment
                    break;
            }
            if( c == '\n' ) {
                // update line counter
                ++mLine;
            }
        } while( token == "" && mStream->peek() != EOF ); // double check in case of consecutive separators
    }
    // check the first token for potential presence of utf bom
    if( mFirstToken ) {
        mFirstToken = false;
        if( token.rfind( "\xef\xbb\xbf", 0 ) == 0 ) {
            token.erase( 0, 3 );
        }
        if( true == token.empty() ) {
            // potentially possible if our first token was standalone utf bom
            token = readToken( ToLower, Break );
        }
    }

    if( false == parameters.empty() ) {
        // if there's parameter list, check the token for potential parameters to replace
        size_t pos; // początek podmienianego ciągu
        while( ( pos = token.find( "(p" ) ) != std::string::npos ) {
            // check if the token is a parameter which should be replaced with stored true value
            auto const parameter{ token.substr( pos + 2, token.find( ")", pos ) - ( pos + 2 ) ) }; // numer parametru
            token.erase( pos, token.find( ")", pos ) - pos + 1 ); // najpierw usunięcie "(pN)"
            size_t nr = atoi( parameter.c_str() ) - 1;
            if( nr < parameters.size() ) {
                token.insert( pos, parameters.at( nr ) ); // wklejenie wartości parametru
                if( ToLower )
                    for( ; pos < parameters.at( nr ).size(); ++pos )
                        token[ pos ] = tolower( token[ pos ] );
            }
            else
                token.insert( pos, "none" ); // zabezpieczenie przed brakiem parametru
        }
    }

    // launch child parser if include directive found.
    // NOTE: parameter collecting uses default set of token separators.
	if( expandIncludes && token == "include" ) {
		std::string includefile = allowRandomIncludes ? deserialize_random_set(*this) : readToken(ToLower); // nazwa pliku
		replace_slashes(includefile);
        if( ( true == LoadTraction )
         || ( ( false == contains( includefile, "tr/" ) )
           && ( false == contains( includefile, "tra/" ) ) ) ) {
            if (Global.ParserLogIncludes)
                WriteLog("including: " + includefile);
            mIncludeParser = std::make_shared<cParser>( includefile, buffer_FILE, mPath, LoadTraction, readParameters( *this ) );
			mIncludeParser->allowRandomIncludes = allowRandomIncludes;
            mIncludeParser->autoclear( m_autoclear );
            if( mIncludeParser->mSize <= 0 ) {
                ErrorLog( "Bad include: can't open file \"" + includefile + "\"" );
            }
        }
        else {
            while( token != "end" ) {
                token = readToken( true ); // minimize risk of case mismatch on comparison
            }
        }
        token = readToken(ToLower, Break);
    }
    else if( ( std::strcmp( Break, "\n\r" ) == 0 ) && ( token.compare( 0, 7, "include" ) == 0 ) ) {
        // HACK: if the parser reads full lines we expect this line to contain entire include directive, to make parsing easier
        cParser includeparser( token.substr( 7 ) );
		includeparser.allowRandomIncludes = allowRandomIncludes;
		std::string includefile = allowRandomIncludes ? deserialize_random_set( includeparser ) : includeparser.readToken( ToLower ); // nazwa pliku
        replace_slashes(includefile);
        if( ( true == LoadTraction )
         || ( ( false == contains( includefile, "tr/" ) )
           && ( false == contains( includefile, "tra/" ) ) ) ) {
            if (Global.ParserLogIncludes)
                WriteLog("including: " + includefile);
            mIncludeParser = std::make_shared<cParser>( includefile, buffer_FILE, mPath, LoadTraction, readParameters( includeparser ) );
			mIncludeParser->allowRandomIncludes = allowRandomIncludes;
            mIncludeParser->autoclear( m_autoclear );
            if( mIncludeParser->mSize <= 0 ) {
                ErrorLog( "Bad include: can't open file \"" + includefile + "\"" );
            }
        }
        token = readToken( ToLower, Break );
    }
    // all done
    return token;
}

std::vector<std::string> cParser::readParameters( cParser &Input ) {

    std::vector<std::string> includeparameters;
    std::string parameter = Input.readToken( false ); // w parametrach nie zmniejszamy
    while( ( parameter.empty() == false )
        && ( parameter != "end" ) ) {
        includeparameters.emplace_back( parameter );
        parameter = Input.readToken( false );
    }
    return includeparameters;
}

std::string cParser::readQuotes(char const Quote) { // read the stream until specified char or stream end
    std::string token = "";
    char c { 0 };
	bool escaped = false;
	while( mStream->peek() != EOF ) { // get all chars until the quote mark
		c = mStream->get();

		if (escaped) {
			escaped = false;
		}
		else {
			if (c == '\\') {
				escaped = true;
				continue;
			}
			else if (c == Quote)
				break;
		}

		if (c == '\n')
			++mLine; // update line counter
		token += c;
    }

	return token;
}

void cParser::skipComment( std::string const &Endmark ) { // pobieranie znaków aż do znalezienia znacznika końca
    std::string input = "";
    char c { 0 };
    auto const endmarksize = Endmark.size();
    while( mStream->peek() != EOF ) {
        // o ile nie koniec pliku
        c = mStream->get(); // pobranie znaku
        if( c == '\n' ) {
            // update line counter
            ++mLine;
        }
        input += c;
        if( input == Endmark ) // szukanie znacznika końca
            break;
        if( input.size() >= endmarksize ) {
            // keep the read text short, to avoid pointless string re-allocations on longer comments
            input = input.substr( 1 );
        }
    }
    return;
}

bool cParser::findQuotes( std::string &String ) {

    if( String.back() == '\"' ) {

        String.pop_back();
        String += readQuotes();
        return true;
    }
    return false;
}

bool cParser::trimComments(std::string &String)
{
    for (auto const &comment : mComments)
    {
        if( String.size() < comment.first.size() ) { continue; }

        if (String.compare( String.size() - comment.first.size(), comment.first.size(), comment.first ) == 0)
        {
            skipComment(comment.second);
            String.resize(String.rfind(comment.first));
            return true;
        }
    }
    return false;
}

void cParser::injectString(const std::string &str)
{
	if (mIncludeParser) {
		mIncludeParser->injectString(str);
	}
	else {
		mIncludeParser = std::make_shared<cParser>( str, buffer_TEXT, "", LoadTraction );
		mIncludeParser->allowRandomIncludes = allowRandomIncludes;
		mIncludeParser->autoclear( m_autoclear );
	}
}

int cParser::getProgress() const
{
    return static_cast<int>( mStream->rdbuf()->pubseekoff(0, std::ios_base::cur) * 100 / mSize );
}

int cParser::getFullProgress() const {

    int progress = getProgress();
    if( mIncludeParser )	return progress + ( ( 100 - progress )*( mIncludeParser->getProgress() ) / 100 );
    else					return progress;
}

std::size_t cParser::countTokens( std::string const &Stream, std::string Path ) {

    return cParser( Stream, buffer_FILE, Path ).count();
}

std::size_t cParser::count() {

    std::string token;
    size_t count { 0 };
    do {
        token = "";
        token = readToken( false );
        ++count;
    } while( false == token.empty() );

    return count - 1;
}

void cParser::addCommentStyle( std::string const &Commentstart, std::string const &Commentend ) {

    mComments.insert( commentmap::value_type(Commentstart, Commentend) );
}

// returns name of currently open file, or empty string for text type stream
std::string
cParser::Name() const {

    if( mIncludeParser ) { return mIncludeParser->Name(); }
    else                 { return mPath + mFile; }
}

// returns number of currently processed line
std::size_t
cParser::Line() const {

    if( mIncludeParser ) { return mIncludeParser->Line(); }
    else                 { return mLine; }
}

int cParser::LineMain() const {
	return mIncludeParser ? -1 : mLine;
}
