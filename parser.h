
#if !defined(rainKERNELTEXTPARSER_H_INCLUDED)
#define rainKERNELTEXTPARSER_H_INCLUDED

#pragma warning ( disable : 4786 )    // 'containers too long for debug' warning

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <fstream>
#include <ctype.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////
// cParser -- generic class for parsing text data, either from file or provided string

class cParser : public std::stringstream
{
public:
 //parameters:
 enum buffertype {buffer_FILE,buffer_TEXT};
 //constructors:
 cParser(std::string Stream,buffertype Type=buffer_TEXT,std::string Path="",bool tr=true);
 //destructor:
 virtual ~cParser();
 //methods:
 template <typename OutputT>
  inline void getToken(OutputT& output) {getTokens(); *this >> output;};
 inline void ignoreToken() {readToken();};
 inline void ignoreTokens(int count) {for (int i=0;i<count;i++) readToken();};
 inline bool expectToken(std::string value) {return readToken()==value;};
 bool eof() {return mStream->eof();};
 bool ok() {return !mStream->fail();};
 bool getTokens(int Count=1,bool ToLower=true,const char* Break="\n\t ;");
 int getProgress() const;	// percentage of file processed.
 //load traction?
 bool LoadTraction;
protected:
 // methods:
 std::string readToken(bool ToLower=true,const char* Break="\n\t ;");
 std::string readComment(const std::string Break="\n\t ;");
 std::string trtest;
 bool trimComments(std::string& String);
 // members:
 std::istream *mStream;	 // relevant kind of buffer is attached on creation.
 std::string mPath;	 // path to open stream, for relative path lookups.
 int mSize;		 // size of open stream, for progress report.
 typedef std::map<std::string,std::string> commentmap;
 commentmap mComments;
 cParser *mIncludeParser; // child class to handle include directives.
 std::vector<std::string> parameters;	// parameter list for included file.
};

#endif // ..!defined(rainKERNELTEXTPARSER_H_INCLUDED)

