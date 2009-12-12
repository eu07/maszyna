
#include "parser.h"
#include "logs.h"
//#include "Globals.h"


/*
    MaSzyna EU07 locomotive simulator parser
    Copyright (C) 2003  TOLARIS

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


/////////////////////////////////////////////////////////////////////////////////////////////////////
// cParser -- generic class for parsing text data.

// constructors
cParser::cParser( std::string Stream, buffertype Type, std::string Path ) {

	// build comments map
	mComments.insert( commentmap::value_type( "/*", "*/" ) );
	mComments.insert( commentmap::value_type( "//", "\n" ) );
	mComments.insert( commentmap::value_type( "--", "\n" ) );

	// store to calculate sub-sequent includes from relative path
	mPath = Path;

	// reset pointers and attach proper type of buffer
	switch( Type ) {
	case buffer_FILE:	Path.append( Stream ); mStream = new std::ifstream( Path.c_str() ); break;
	case buffer_TEXT:	mStream = new std::istringstream( Stream ); break;
	default:			mStream = NULL; break;
	}
	mIncludeParser = NULL;

	// calculate stream size
	if( mStream ) {
		mSize = mStream->rdbuf()->pubseekoff( 0, std::ios_base::end );
		mStream->rdbuf()->pubseekoff( 0, std::ios_base::beg );
	}
	else
		mSize = 0;
}

// destructor
cParser::~cParser() {

	if( mIncludeParser ) delete mIncludeParser;
	if( mStream ) delete mStream;
	mComments.clear();
}

// methods
bool cParser::getTokens( int Count, bool ToLower, const char* Break )
{
                 if (LoadTraction==true)
                  trtest = "niemaproblema";
                  else
                  trtest = "x";
        int i=0;
	this->str(""); this->clear();
	for(i = 0; i < Count; ++i )
               {
		std::string string = readToken( ToLower, Break, trtest );
		// collect parameters
		if( i == 0 )
			this->str( string );
		else {
			std::string temp = this->str();
			temp.append( "\n" );
			temp.append( string );
			this->str( temp );
		}
	}
	if( i < Count )
		return false;
	else
		return true;
}

std::string cParser::readToken( bool ToLower, const char* Break, std::string trtest ) {

	std::string token = "";
	// see if there's include parsing going on. clean up when it's done.
	if( mIncludeParser ) {
		token = (*mIncludeParser).readToken( ToLower, Break, trtest );
		if( !token.empty() ) {
			// check if the token is a parameter which should be replaced with stored true value
			if( token.find( "(p" ) != std::string::npos ) {
				std::string parameter = token.substr( token.find( "(p" ) + 2, token.find( ")", token.find( "(p" ) ) - (token.find( "(p" ) + 2) );
				token.insert( token.find( "(p" ), parameters.at( atoi(parameter.c_str()) - 1 ) );
				token.erase( token.find( "(p" ), token.find( ")", token.find( "(p" ) ) - token.find( "(p" ) + 1 );
			}
			return token;
		}
		else {
			delete mIncludeParser;
			mIncludeParser = NULL;
			parameters.clear();
		}
	}
	
	// get the token yourself if there's no child to delegate it to.
	char c;
	do {
		while( mStream->peek()!=EOF&& strchr( Break, c = mStream->get() ) == NULL ) {
			if( ToLower )
				c = tolower( c );
			token += c;
			if( trimComments( token ) )			// don't glue together words separated with comment
				break;
		}
	} while( token == "" && mStream->peek()!=EOF);	// double check to deal with trailing spaces

	// launch child parser if include directive found.
	// NOTE: parameter collecting uses default set of token separators.
	if( token.compare( "include" ) == 0 )
                {
 		  std::string includefile = readToken( ToLower );
                  std::string trtest2 = "niemaproblema";
                  if (trtest=="x")
                  trtest2 = includefile;
		 std::string parameter = readToken( ToLower );
		 while( parameter.compare( "end" ) != 0 ) {
		 	parameters.push_back( parameter );
		 	parameter = readToken( ToLower );
		 }
 		 if (trtest2.find("tr/")!=0)
                   mIncludeParser = new cParser( includefile, buffer_FILE, mPath );

		 token = readToken( ToLower, Break, trtest );
	}
	return token;
}

bool cParser::trimComments( std::string& String ) {

	for( commentmap::iterator cmIt = mComments.begin(); cmIt != mComments.end(); ++cmIt ) {
		if( String.find( (*cmIt).first ) != std::string::npos ) {
			readComment( (*cmIt).second );
			String.resize( String.find( (*cmIt).first ) );
			return true;
		}
	}
	return false;
}

std::string cParser::readComment( const std::string Break ) {

	std::string token = "";

	while( mStream->peek()!=EOF) {
		token += mStream->get();
		if( token.find( Break ) != std::string::npos )
			break;
	}

	return token;
}

int cParser::getProgress() const { return mStream->rdbuf()->pubseekoff( 0, std::ios_base::cur ) * 100 / mSize; }
