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
#include "Logs.h"

/*
    MaSzyna EU07 locomotive simulator parser
    Copyright (C) 2003  TOLARIS

*/

/////////////////////////////////////////////////////////////////////////////////////////////////////
// cParser -- generic class for parsing text data.

// constructors
cParser::cParser( std::string const &Stream, buffertype const Type, std::string Path, bool const Loadtraction )
{
    LoadTraction = Loadtraction;
    // build comments map
    mComments.insert(commentmap::value_type("/*", "*/"));
    mComments.insert(commentmap::value_type("//", "\n"));
    // mComments.insert(commentmap::value_type("--","\n")); //Ra: to chyba nie używane
    // store to calculate sub-sequent includes from relative path
    mPath = Path;
    if( Type == buffertype::buffer_FILE ) {
        mFile = Stream;
    }
    // reset pointers and attach proper type of buffer
    switch (Type)
    {
    case buffer_FILE:
        Path.append(Stream);
		std::replace(Path.begin(), Path.end(), '\\', '/');
        mStream = new std::ifstream(Path);
        break;
    case buffer_TEXT:
        mStream = new std::istringstream(Stream);
        break;
    default:
        mStream = NULL;
    }
    mIncludeParser = NULL;
    // calculate stream size
    if (mStream)
    {
        if( true == mStream->fail() ) {
            ErrorLog( "Failed to open file \"" + Path + "\"" );
        }
        else {
            mSize = mStream->rdbuf()->pubseekoff( 0, std::ios_base::end );
            mStream->rdbuf()->pubseekoff( 0, std::ios_base::beg );
        }
    }
    else
        mSize = 0;
}

// destructor
cParser::~cParser()
{
    if (mIncludeParser)
        delete mIncludeParser;
    if (mStream)
        delete mStream;
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
        token = (*mIncludeParser).readToken(ToLower, Break);
        if (!token.empty())
        {
            pos = token.find("(p");
            // check if the token is a parameter which should be replaced with stored true value
            if (pos != std::string::npos) //!=npos to znalezione
            {
                std::string parameter =
                    token.substr(pos + 2, token.find(")", pos) - pos + 2); // numer parametru
                token.erase(pos, token.find(")", pos) - pos + 1); // najpierw usunięcie "(pN)"
                size_t nr = atoi(parameter.c_str()) - 1;
                if (nr < parameters.size())
                {
                    token.insert(pos, parameters.at(nr)); // wklejenie wartości parametru
                    if (ToLower)
                        for (; pos < token.length(); ++pos)
                            token[pos] = tolower(token[pos]);
                }
                else
                    token.insert(pos, "none"); // zabezpieczenie przed brakiem parametru
            }
            return token;
        }
        else
        {
            delete mIncludeParser;
            mIncludeParser = NULL;
            parameters.clear();
        }
    }
    // get the token yourself if there's no child to delegate it to.
    char c;
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
    } while (token == "" && mStream->peek() != EOF); // double check to deal with trailing spaces
    // launch child parser if include directive found.
    // NOTE: parameter collecting uses default set of token separators.
    if (token.compare("include") == 0)
    { // obsługa include
        std::string includefile = readToken(ToLower); // nazwa pliku
        if (LoadTraction ? true : ((includefile.find("tr/") == std::string::npos) &&
                                   (includefile.find("tra/") == std::string::npos)))
        {
            // std::string trtest2="niemaproblema"; //nazwa odporna na znalezienie "tr/"
            // if (trtest=="x") //jeśli nie wczytywać drutów
            // trtest2=includefile; //kopiowanie ścieżki do pliku
            std::string parameter = readToken(false); // w parametrach nie zmniejszamy
            while( (parameter.empty() == false)
				&& (parameter.compare("end") != 0) )
            {
                parameters.push_back(parameter);
                parameter = readToken(ToLower);
            }
            // if (trtest2.find("tr/")!=0)
            mIncludeParser = new cParser(includefile, buffer_FILE, mPath, LoadTraction);
            if (mIncludeParser->mSize <= 0)
                ErrorLog("Missed include: " + includefile);
        }
        else
            while (token.compare("end") != 0)
                token = readToken(ToLower);
        token = readToken(ToLower, Break);
    }
    return token;
}

std::string cParser::readQuotes(char const Quote) { // read the stream until specified char or stream end
    std::string token = "";
    char c;
    while( mStream->peek() != EOF && Quote != (c = mStream->get()) ) { // get all chars until the quote mark
        token += c;
    }
    return token;
}

void cParser::skipComment( std::string const &Endmark ) { // pobieranie znaków aż do znalezienia znacznika końca
    std::string input = "";
    auto const endmarksize = Endmark.size();
    while( mStream->peek() != EOF ) { // o ile nie koniec pliku
        input += mStream->get(); // pobranie znaku
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
    size_t count{ 0 };
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
cParser::Name() {

    if( mIncludeParser ) { return mIncludeParser->Name(); }
    else                 { return mPath + mFile; }
}
