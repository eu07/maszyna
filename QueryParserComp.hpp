// Borland C++ Builder
// Copyright (c) 1995, 1999 by Borland International
// All rights reserved

// (DO NOT EDIT: machine generated header) 'QueryParserComp.pas' rev: 5.00

#ifndef QueryParserCompHPP
#define QueryParserCompHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Queryparsercomp
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TTokenType { ttString, ttSymbol, ttComment, ttDelimiter, ttSpecialChar, ttStatementDelimiter, ttCommentedSymbol, 
	ttCommentDelimiter };
#pragma option pop

typedef Set<char, 0, 255>  TSetOfChar;

typedef AnsiString TComment[2];

#pragma option push -b-
enum TCharacterType { ctSymbol, ctBeginComment, ctEndComment, ctDelimiter, ctString, ctSpecialChar }
	;
#pragma option pop

#pragma option push -b-
enum QueryParserComp__1 { cmt1, cmt2, cmt3 };
#pragma option pop

typedef Set<QueryParserComp__1, cmt1, cmt3>  TCommentType;

typedef AnsiString QueryParserComp__2[3][2];

typedef void __fastcall (__closure *TEndOfStatement)(System::TObject* Sender, AnsiString SQLStatement
	);

class DELPHICLASS TQueryParserComp;
class PASCALIMPLEMENTATION TQueryParserComp : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	Classes::TStringStream* FStream;
	bool FEOF;
	AnsiString FToken;
	TTokenType FTokenType;
	bool FComment;
	bool FString;
	bool FWasString;
	TCommentType FCommentType;
	AnsiString FStringDelimiters;
	AnsiString FLastStringDelimiterFound;
	int FSymbolsCount;
	AnsiString FSpecialCharacters;
	bool FRemoveStrDelimiter;
	AnsiString FStringToParse;
	int FGoPosition;
	TEndOfStatement FOnStatementDelimiter;
	bool FCountFromStatement;
	Classes::TStringList* FStatementDelimiters;
	char FStringDelimiter;
	bool FGenerateOnStmtDelimiter;
	void __fastcall Init(void);
	void __fastcall SetStringToParse(AnsiString AStringToParse);
	bool __fastcall StatementDelimiter(void);
	bool __fastcall CheckForBeginComment(void);
	bool __fastcall CheckForEndComment(char Character);
	TCharacterType __fastcall CharacterType(char Character);
	bool __fastcall CheckCharcterType(char Character);
	bool __fastcall StringDelimiter(char Character);
	bool __fastcall SpecialCharacter(char Character);
	void __fastcall RemoveStringDelimiter(AnsiString &Source);
	void __fastcall SetDelimiterType(AnsiString Source);
	void __fastcall SetToken(void);
	void __fastcall SetSD(Classes::TStringList* ASD);
	void __fastcall SetSpecialCharacters(AnsiString ASpecialCharacters);
	void __fastcall SetStringDelimiters(AnsiString AStringDelimiters);
	
protected:
	DYNAMIC void __fastcall DoStatementDelimiter(void);
	
public:
	__fastcall virtual TQueryParserComp(Classes::TComponent* AOwner);
	__fastcall virtual ~TQueryParserComp(void);
	void __fastcall LoadStringToParse(AnsiString FileName);
	void __fastcall First(void);
	void __fastcall FirstToken(void);
	void __fastcall NextToken(void);
	AnsiString __fastcall GetNextSymbol();
	__property bool EndOfFile = {read=FEOF, nodefault};
	__property bool Comment = {read=FComment, nodefault};
	__property AnsiString Token = {read=FToken};
	__property TTokenType TokenType = {read=FTokenType, nodefault};
	__property char CurrentStringDelimiter = {read=FStringDelimiter, nodefault};
	__property int SymbolsCount = {read=FSymbolsCount, default=0};
	__property Classes::TStringStream* StringStream = {read=FStream};
	
__published:
	__property bool IsEOFStmtDelimiter = {read=FGenerateOnStmtDelimiter, write=FGenerateOnStmtDelimiter
		, nodefault};
	__property AnsiString StringDelimiters = {read=FStringDelimiters, write=SetStringDelimiters};
	__property AnsiString SpecialCharacters = {read=FSpecialCharacters, write=SetSpecialCharacters};
	__property bool RemoveStrDelimiter = {read=FRemoveStrDelimiter, write=FRemoveStrDelimiter, nodefault
		};
	__property bool CountFromStatement = {read=FCountFromStatement, write=FCountFromStatement, nodefault
		};
	__property AnsiString TextToParse = {read=FStringToParse, write=SetStringToParse};
	__property Classes::TStringList* StatementDelimiters = {read=FStatementDelimiters, write=SetSD};
	__property TEndOfStatement OnStatementDelimiter = {read=FOnStatementDelimiter, write=FOnStatementDelimiter
		};
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE System::ResourceString _sTextNotSet;
#define Queryparsercomp_sTextNotSet System::LoadResourceString(&Queryparsercomp::_sTextNotSet)
extern PACKAGE System::ResourceString _sIllegalSpecialChar;
#define Queryparsercomp_sIllegalSpecialChar System::LoadResourceString(&Queryparsercomp::_sIllegalSpecialChar)
	
extern PACKAGE System::ResourceString _sIllegalStringChar;
#define Queryparsercomp_sIllegalStringChar System::LoadResourceString(&Queryparsercomp::_sIllegalStringChar)
	
static const char CR = '\xd';
static const char LF = '\xa';
static const char TAB = '\x9';
#define CRLF "\r\n"
extern PACKAGE TSetOfChar Delimiters;
extern PACKAGE AnsiString Comments[3][2];
extern PACKAGE void __fastcall Register(void);

}	/* namespace Queryparsercomp */
#if !defined(NO_IMPLICIT_NAMESPACE_USE)
using namespace Queryparsercomp;
#endif
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// QueryParserComp
