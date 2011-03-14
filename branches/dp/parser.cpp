
#define _USE_OLD_RW_STL

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
cParser::cParser(std::string Stream,buffertype Type,std::string Path,bool tr)
{
 LoadTraction=tr;
 // build comments map
 mComments.insert(commentmap::value_type("/*","*/"));
 mComments.insert(commentmap::value_type("//","\n"));
 mComments.insert(commentmap::value_type("--","\n"));
 // store to calculate sub-sequent includes from relative path
 mPath=Path;
 // reset pointers and attach proper type of buffer
 switch (Type)
 {
  case buffer_FILE:
   Path.append(Stream);
   mStream=new std::ifstream(Path.c_str());
   break;
  case buffer_TEXT:
   mStream=new std::istringstream(Stream);
   break;
  default:
   mStream=NULL;
 }
 mIncludeParser=NULL;
 // calculate stream size
 if (mStream)
 {
  mSize=mStream->rdbuf()->pubseekoff(0,std::ios_base::end);
  mStream->rdbuf()->pubseekoff(0,std::ios_base::beg);
 }
 else
  mSize=0;
}

// destructor
cParser::~cParser()
{
 if (mIncludeParser) delete mIncludeParser;
 if (mStream) delete mStream;
 mComments.clear();
}

// methods
bool cParser::getTokens(int Count,bool ToLower,const char* Break)
{
/*
 if (LoadTraction==true)
  trtest="niemaproblema"; //wczytywa�
 else
  trtest="x"; //nie wczytywa�
*/
 int i;
 this->str("");
 this->clear();
 for (i=0;i<Count;++i)
 {
  std::string string=readToken(ToLower,Break);
  // collect parameters
  if (i==0)
   this->str(string);
  else
  {
   std::string temp=this->str();
   temp.append("\n");
   temp.append(string);
   this->str(temp);
  }
 }
 if (i<Count)
  return false;
 else
  return true;
}

std::string cParser::readToken(bool ToLower,const char* Break)
{
 std::string token="";
 size_t pos; //pocz�tek podmienianego ci�gu
 // see if there's include parsing going on. clean up when it's done.
 if (mIncludeParser)
 {
  token=(*mIncludeParser).readToken(ToLower,Break);
  if (!token.empty())
  {pos=token.find("(p");
   // check if the token is a parameter which should be replaced with stored true value
   if (pos!=std::string::npos) //!=npos to znalezione
   {
    std::string parameter=token.substr(pos+2,token.find(")",pos)-pos+2); //numer parametru
    token.erase(pos,token.find(")",pos)-pos+1); //najpierw usuni�cie "(pN)"
    size_t nr=atoi(parameter.c_str())-1;
    if (nr<parameters.size())
     token.insert(pos,parameters.at(nr)); //wklejenie warto�ci parametru
    else
     token.insert(pos,"none"); //zabezpieczenie przed brakiem parametru
   }
   return token;
  }
  else
  {delete mIncludeParser;
   mIncludeParser=NULL;
   parameters.clear();
  }
 }
 // get the token yourself if there's no child to delegate it to.
 char c;
 do
 {while (mStream->peek()!=EOF&& strchr( Break, c = mStream->get() ) == NULL)
  {
   if (ToLower) c=tolower(c);
   token+=c;
   if (trimComments(token))			// don't glue together words separated with comment
    break;
  }
 } while (token=="" && mStream->peek()!=EOF);	// double check to deal with trailing spaces
 // launch child parser if include directive found.
 // NOTE: parameter collecting uses default set of token separators.
 if (token.compare("include")==0)
 {//obs�uga include
  std::string includefile=readToken(ToLower); //nazwa pliku
  if (LoadTraction?true:((includefile.find("tr/")==std::string::npos)&&(includefile.find("tra/")==std::string::npos)))
  {
   //std::string trtest2="niemaproblema"; //nazwa odporna na znalezienie "tr/"
   //if (trtest=="x") //je�li nie wczytywa� drut�w
   //trtest2=includefile; //kopiowanie �cie�ki do pliku
   std::string parameter=readToken(ToLower);
   while (parameter.compare("end")!=0)
   {
    parameters.push_back(parameter);
    parameter=readToken(ToLower);
   }
   //if (trtest2.find("tr/")!=0)
   mIncludeParser=new cParser(includefile,buffer_FILE,mPath,LoadTraction);
  }
  else
   while (token.compare("end")!=0)
    token=readToken(ToLower);
  token=readToken(ToLower,Break);
 }
 return token;
}

bool cParser::trimComments(std::string &String)
{
 for (commentmap::iterator cmIt=mComments.begin();cmIt!=mComments.end();++cmIt)
 {
  if (String.find((*cmIt).first)!=std::string::npos)
  {
   readComment((*cmIt).second);
   String.resize(String.find((*cmIt).first));
   return true;
  }
 }
 return false;
}

std::string cParser::readComment(const std::string Break)
{//pobieranie znak�w a� do znalezienia znacznika ko�ca
 std::string token="";
 while (mStream->peek()!=EOF)
 {//o ile nie koniec pliku
  token+=mStream->get(); //pobranie znaku
  if (token.find(Break)!=std::string::npos) //szukanie znacznika ko�ca
   break;
 }
 return token;
}

int cParser::getProgress() const
{
 return mStream->rdbuf()->pubseekoff( 0, std::ios_base::cur ) * 100 / mSize;
}

