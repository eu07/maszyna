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
#include "logs.h"

/*
    MaSzyna EU07 locomotive simulator parser
    Copyright (C) 2003  TOLARIS

*/

/////////////////////////////////////////////////////////////////////////////////////////////////////
// cParser -- generic class for parsing text data.

// constructors
cParser::cParser( std::string const &Stream, buffertype const Type, std::string Path, bool const Loadtraction ) :
    mPath(Path),
    LoadTraction( Loadtraction ) {
    // build comments map
    mComments.insert(commentmap::value_type("/*", "*/"));
    mComments.insert(commentmap::value_type("//", "\n"));
    // mComments.insert(commentmap::value_type("--","\n")); //Ra: to chyba nie używane
    // store to calculate sub-sequent includes from relative path
    if( Type == buffertype::buffer_FILE ) {
        mFile = Stream;
    }
    // reset pointers and attach proper type of buffer
    switch (Type) {
        case buffer_FILE: {
            Path.append( Stream );
            mStream = std::make_shared<std::ifstream>( Path );
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
}

// destructor
cParser::~cParser() {

    mComments.clear();
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

template <>
bool
cParser::getToken<bool>( bool const ToLower, const char *Break ) {

    auto const token = getToken<std::string>( true, Break );
    return ( ( token == "true" )
          || ( token == "yes" )
          || ( token == "1" ) );
}

// methods
bool cParser::getTokens(unsigned int Count, bool ToLower, const char *Break)
{
    tokens.clear(); // emulates old parser behaviour. TODO, TBD: allow manual reset?
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
    for (unsigned int i = 0; i < Count; ++i)
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

std::string cParser::readToken(bool ToLower, const char *Break)
{
    std::string token = "";
    size_t pos; // początek podmienianego ciągu
    // see if there's include parsing going on. clean up when it's done.
    if (mIncludeParser)
    {
        token = mIncludeParser->readToken(ToLower, Break);
        if (!token.empty()) {

            while( ( pos = token.find( "(p" ) ) != std::string::npos ) {
                // check if the token is a parameter which should be replaced with stored true value
                auto const parameter { token.substr( pos + 2, token.find( ")", pos ) - ( pos + 2 ) ) }; // numer parametru
                token.erase(pos, token.find(")", pos) - pos + 1); // najpierw usunięcie "(pN)"
                size_t nr = atoi(parameter.c_str()) - 1;
                if (nr < parameters.size())
                {
                    token.insert(pos, parameters.at(nr)); // wklejenie wartości parametru
                    if (ToLower)
                        for (; pos < parameters.at( nr ).size(); ++pos)
                            token[pos] = tolower(token[pos]);
                }
                else
                    token.insert(pos, "none"); // zabezpieczenie przed brakiem parametru
            }
            return token;
        }
        else
        {
            mIncludeParser = NULL;
            parameters.clear();
        }
    }
    // get the token yourself if there's no child to delegate it to.
    char c { 0 };
    do
    {
        while (mStream->peek() != EOF && strchr(Break, c = mStream->get()) == NULL)
        {
            if (ToLower)
                c = tolower(c);
            token += c;
            if (findQuotes(token)) // do glue together words enclosed in quotes
                break;
            if (trimComments(token)) // don't glue together words separated with comment
                break;
        }
        if( c == '\n' ) {
            // update line counter
            ++mLine;
        }
    } while (token == "" && mStream->peek() != EOF); // double check to deal with trailing spaces
    // launch child parser if include directive found.
    // NOTE: parameter collecting uses default set of token separators.
    if (token.compare("include") == 0)
    { // obsługa include
        std::string includefile = readToken(ToLower); // nazwa pliku
        if( ( true == LoadTraction )
         || ( ( includefile.find( "tr/" ) == std::string::npos )
           && ( includefile.find( "tra/" ) == std::string::npos ) ) ) {

            std::string parameter = readToken(false); // w parametrach nie zmniejszamy
            while( (parameter.empty() == false)
				&& (parameter.compare("end") != 0) )
            {
                parameters.emplace_back(parameter);
                parameter = readToken(false);
            }
            // if (trtest2.find("tr/")!=0)
            mIncludeParser = std::make_shared<cParser>(includefile, buffer_FILE, mPath, LoadTraction);
            if (mIncludeParser->mSize <= 0)
                ErrorLog("Bad include: can't open file \"" + includefile + "\"" );
        }
        else {
            while( token.compare( "end" ) != 0 ) {
                token = readToken( true ); // minimize risk of case mismatch on comparison
            }
        }
        token = readToken(ToLower, Break);
    }
    return token;
}

std::string cParser::readQuotes(char const Quote) { // read the stream until specified char or stream end
    std::string token = "";
    char c { 0 };
    while( mStream->peek() != EOF && Quote != (c = mStream->get()) ) { // get all chars until the quote mark
        if( c == '\n' ) {
            // update line counter
            ++mLine;
        }
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
        if( input.find( Endmark ) != std::string::npos ) // szukanie znacznika końca
            break;
        if( input.size() >= endmarksize ) {
            // keep the read text short, to avoid pointless string re-allocations on longer comments
            input = input.substr( 1 );
        }
    }
    return;
}

bool cParser::findQuotes( std::string &String ) {

    if( String.rfind( '\"' ) != std::string::npos ) {

        String.erase( String.rfind( '\"' ), 1 );
        String += readQuotes();
        return true;
    }
    return false;
}

bool cParser::trimComments(std::string &String)
{
    for (commentmap::iterator cmIt = mComments.begin(); cmIt != mComments.end(); ++cmIt)
    {
        if (String.rfind((*cmIt).first) != std::string::npos)
        {
            skipComment((*cmIt).second);
            String.resize(String.rfind((*cmIt).first));
            return true;
        }
    }
    return false;
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
