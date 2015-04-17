unit QueryParserComp;

(*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*)

interface

uses
  Classes, Sysutils;

resourcestring
  sTextNotSet = 'You must set the TextToParse property first.';
  sIllegalSpecialChar = 'Illegal special character.';
  sIllegalStringChar = 'Illegal string delimiter.';

type
  TTokenType = (ttString, ttSymbol, ttComment, ttDelimiter, ttSpecialChar,
    ttStatementDelimiter, ttCommentedSymbol, ttCommentDelimiter);
  TSetOfChar = set of Char;
  TComment = array[0..1] of string;
  TCharacterType = (ctSymbol, ctBeginComment, ctEndComment, ctDelimiter,
    ctString, ctSpecialChar);
  TCommentType = set of (cmt1, cmt2, cmt3);

const
  CR = #13;
  LF = #10;
  TAB = #9;
  CRLF = CR + LF;
  Delimiters: TSetOfChar = [' ', ',', ';', CR, LF, TAB];
  Comments: array[0..2] of TComment = (('/*', '*/'), ('#', LF), ('//', LF));

type
  TEndOfStatement = procedure(Sender: TObject; SQLStatement: String) of object;

  TQueryParserComp = class(TComponent)
  private
    FStream: TStringStream;
    FEOF: Boolean;
    FToken: String;
    FTokenType: TTokenType;
    FComment: Boolean;
    FString: Boolean;
    FWasString: Boolean;
    FCommentType: TCommentType;
    FStringDelimiters: String;
    FLastStringDelimiterFound: String;
    FSymbolsCount: Integer;
    FSpecialCharacters: String;
    FRemoveStrDelimiter: Boolean;
    FStringToParse: String;
    FGoPosition: Integer;
    FOnStatementDelimiter: TEndOfStatement;
    FCountFromStatement: Boolean;
    FStatementDelimiters: TStringList;
    FStringDelimiter: Char;
    FGenerateOnStmtDelimiter: Boolean;
    procedure Init;
    procedure SetStringToParse(AStringToParse: String);
    function StatementDelimiter: Boolean;
    function CheckForBeginComment: Boolean;
    function CheckForEndComment(Character: Char): Boolean;
    function CharacterType(Character: Char): TCharacterType;
    function CheckCharcterType(Character: Char): Boolean;
    function StringDelimiter(Character: Char): Boolean;
    function SpecialCharacter(Character: Char): Boolean;
    procedure RemoveStringDelimiter(var Source: String);
    procedure SetDelimiterType(Source: String);
    procedure SetToken;
    procedure SetSD(ASD: TStringList);
    procedure SetSpecialCharacters(ASpecialCharacters: String);
    procedure SetStringDelimiters(AStringDelimiters: String);
  protected
    procedure DoStatementDelimiter; dynamic;
  public
    constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
    procedure LoadStringToParse(FileName: string);
    procedure First;
    procedure FirstToken;
    procedure NextToken;
    function GetNextSymbol : string;
    property EndOfFile: Boolean read FEOF;
    property Comment: Boolean read FComment;
    property Token: String read FToken;
    property TokenType: TTokenType read FTokenType;
    property CurrentStringDelimiter: Char read FStringDelimiter;
    property SymbolsCount: Integer read FSymbolsCount default 0;
    property StringStream: TStringStream read FStream;
  published
    property IsEOFStmtDelimiter: Boolean read FGenerateOnStmtDelimiter
      write FGenerateOnStmtDelimiter;
    property StringDelimiters: String read FStringDelimiters
      write SetStringDelimiters;
    property SpecialCharacters: String read FSpecialCharacters
      write SetSpecialCharacters;
    property RemoveStrDelimiter: Boolean read FRemoveStrDelimiter
      write FRemoveStrDelimiter;
    property CountFromStatement: Boolean read FCountFromStatement
      write FCountFromStatement;
    property TextToParse: String read FStringToParse
      write SetStringToParse;
    property StatementDelimiters: TStringList read FStatementDelimiters write SetSD;
    property OnStatementDelimiter: TEndOfStatement read FOnStatementDelimiter
      write FOnStatementDelimiter;
  end;

procedure Register;

implementation

{ TSQLParser }

procedure Register;
begin
  RegisterComponents('Samples', [TQueryParserComp]);
end;

constructor TQueryParserComp.Create(AOwner: TComponent);
begin
  inherited;
  FStatementDelimiters := TStringList.Create;
  FStatementDelimiters.Add('GO');
  FStatementDelimiters.Add(';');
  FCountFromStatement := True;
end;

procedure TQueryParserComp.LoadStringToParse(FileName: string);
var
  fs:TFileStream;
  size:integer;
begin

  fs:= TFileStream.Create(FileName, fmOpenRead);

  size:= fs.Size;

  if Assigned(FStream) then
    FStream.Free;

  FStream := TStringStream.Create('');
  FStream.CopyFrom(fs,size);
  fs.Free;
  Init;
  First;
end;

procedure TQueryParserComp.FirstToken;
begin
  Init;
  NextToken;
end;

function TQueryParserComp.CheckForBeginComment: Boolean;
var
  Buffer: String;
begin
  Result := False;
  if not FEOF and not FString then
  begin
    Buffer := FStream.ReadString(1);
    if cmt1 in FCommentType then
      if Buffer[1] = '*' then
      begin
        FCommentType := [cmt1];
        Result := True;
      end;
    if cmt2 in FCommentType then
      if Buffer[1] = '/' then
      begin
        FCommentType := [cmt2];
        Result := True;
      end;
    if cmt3 in FCommentType then
      if Buffer[1] = '-' then
      begin
        FCommentType := [cmt3];
        Result := True;
      end;
    FStream.Seek(-1, soFromCurrent);
  end;
end;

procedure TQueryParserComp.SetDelimiterType(Source: String);
begin
  FToken := Source[1];

  if ((cmt2 in FCommentType) or (cmt3 in FCommentType)) and FComment
    and ((Source[1] = CR) or (Source[1] = LF)) then
  begin
    FComment := False;
    FTokenType := ttCommentDelimiter;
  end
  else
    FTokenType := ttDelimiter;
end;

procedure TQueryParserComp.NextToken;
var
  Buffer: String;
  ETextNotSet: Exception;
begin
  if FEOF then
    Exit;

  if not Assigned(FStream) then
  begin
    ETextNotSet := Exception.Create(sTextNotSet);
    raise ETextNotSet;
  end;

  if not FString then
    FStringDelimiter := ' ';

  FToken := '';
  if not FEOF then
  begin
    Buffer := FStream.ReadString(1);
    if Length(Buffer) > 0 then
    begin
      if (Buffer[1] in Delimiters) and not (FString or FComment)then
        SetDelimiterType(Buffer)
      else
      begin
        if FStream.Position > 0 then
          FStream.Seek(-1, soFromCurrent);
        SetToken;
      end;
    end
    else
      FEOF := True;
  end;

  case FTokenType of
    ttSymbol: Inc(FSymbolsCount);
    ttString:
      begin
        FStringDelimiter := FToken[1];
        if FRemoveStrDelimiter then
          RemoveStringDelimiter(FToken);
      end;
  end;

  if StatementDelimiter then
    FTokenType := ttStatementDelimiter;

  if FEOF then
  begin
    FLastStringDelimiterFound := '';
    FWasString := False;
    FString := False;

    if FGenerateOnStmtDelimiter then
      DoStatementDelimiter;
  end;
end;

function TQueryParserComp.GetNextSymbol : string;
begin
    GetNextSymbol:= '';
    while ( not EndOfFile) do
    begin
        NextToken;
        if (TokenType=ttSymbol) then
        begin
            GetNextSymbol:= Token;
            break;
        end
    end

end;

function TQueryParserComp.StatementDelimiter: Boolean;
var
  i: Integer;
begin
  Result := False;
  if not FString then
    for i := 0 to FStatementDelimiters.Count - 1 do
    begin
      Result := (UpperCase(FToken) = UpperCase(FStatementDelimiters.Strings[i]));
      if Result then
      begin
        if FCountFromStatement then
          FSymbolsCount := 0;
        DoStatementDelimiter;
        Break;
      end;
    end;
end;

function TQueryParserComp.CharacterType(Character: Char): TCharacterType;
begin
  Result := ctSymbol;
  case Character of
    '/':
    begin
      if not FComment then
      begin
        FCommentType := [cmt1, cmt2];
        if CheckForBeginComment then
          Result := ctBeginComment;
      end;
    end;
    '-':
    begin
      if not FComment then
      begin
        FCommentType := [cmt3];
        if CheckForBeginComment then
          Result := ctBeginComment;
      end;
    end;
    '*':
    begin
      if CheckForEndComment(Character) then
        Result := ctEndComment;
    end;
    CR, LF, ' ', ',', TAB:
    begin
      if CheckForEndComment(Character) then
        Result := ctEndComment
      else
        Result := ctDelimiter;

      if FString and ((Character = CR) or (Character = LF)) then
      begin
        FString := False;
        FWasString := False;
        Result := ctDelimiter;
      end;
    end;
  end;

  if not FString then
    if SpecialCharacter(Character) then
    begin
      if not FComment then
        Result := ctSpecialChar;
    end;

  if not FComment then
    if StringDelimiter(Character) then
    begin
      Result := ctSymbol;
      if FString then
      begin
        FLastStringDelimiterFound := '';
        FWasString := True;
        FString := False;
      end
      else
      begin
        FWasString := False;
        FString := True;
      end;
    end;
end;

function TQueryParserComp.CheckForEndComment(Character: Char): Boolean;
var
  Buffer: String;
begin
  Result := False;

  if not FComment or FString then
    Exit;

  if not FEOF then
  begin
    if cmt1 in FCommentType then
    begin
      Buffer := FStream.ReadString(1);
      if Buffer[1] = '/' then
        Result := True;

      FStream.Seek(-1, soFromCurrent);
    end;
    if (cmt2 in FCommentType) or (cmt3 in FCommentType) then
    begin
      if (Character = CR) or (Character = LF) then
        Result := True;
    end;
  end;
end;

function TQueryParserComp.CheckCharcterType(Character: Char): Boolean;
var
  Buffer: String;
begin
  Result := False;
  case CharacterType(Character) of
    ctBeginComment:
    begin
      if not FComment then
      begin
        if FToken <> '' then
        begin
          FStream.Seek(-1, soFromCurrent);
          FTokenType := ttSymbol;
        end
        else
        begin
          FComment := True;
          FToken := FToken + Character;
          Buffer := FStream.ReadString(1);
          FToken := FToken + Buffer;
          FTokenType := ttComment;
        end;
        Result := True;
      end;
    end;
    ctEndComment:
    begin
      if FComment then
      begin
        Result := True;
        if FToken <> '' then
        begin
          FStream.Seek(-1, soFromCurrent);
          FTokenType := ttCommentedSymbol;
        end
        else
        begin
          FComment := False;
          FToken := FToken + Character;
          if FCommentType = [cmt1] then
          begin
            Buffer := FStream.ReadString(1);
            FToken := FToken + Buffer;
          end;
          FTokenType := ttComment;
        end;
      end;
    end;
    ctSymbol:
    begin
      FToken := FToken + Character;
      if FComment then
        FTokenType := ttCommentedSymbol
      else
      begin
        if FString or FWasString then
        begin
          if FWasString then
          begin
            FTokenType := ttString;
            FWasString := False;


            Result := True;
          end
          else
            FTokenType := ttString;
        end
        else
          FTokenType := ttSymbol;
      end;
    end;
    ctSpecialChar:
    begin
      if FToken <> '' then
      begin
        FStream.Seek(-1, soFromCurrent);
        FTokenType := ttSymbol;
      end
      else
      begin
        FToken := FToken + Character;
        FTokenType := ttSpecialChar;
      end;
      Result := True;
    end;
    ctDelimiter:
    begin
      FTokenType := ttDelimiter;
      Result := True;
    end;
  end
end;

procedure TQueryParserComp.RemoveStringDelimiter(var Source: String);
var
  EndOfString: Integer;
  i: Integer;
begin
  for i := 1 to Length(FStringDelimiters) do
  begin
    EndOfString := 1;
    while not (EndOfString = 0) do
    begin
      EndOfString := Pos(FStringDelimiters[i], Source);
      if EndOfString <> 0 then
      begin
        FLastStringDelimiterFound := Copy(FStringDelimiters, i, 1);
        Delete(Source, EndOfString, 1);
      end;
    end;
  end;
end;

function TQueryParserComp.StringDelimiter(Character: Char): Boolean;
var
  i: Integer;
  Buffer: String;
begin
  Result := False;

  for i := 1 to Length(FStringDelimiters) do
    if (Character = FStringDelimiters[i]) then
    begin
      if (FLastStringDelimiterFound = '') then
        FLastStringDelimiterFound := FStringDelimiters[i];

      if (FLastStringDelimiterFound = FStringDelimiters[i]) then
      begin
        if not FEOF then
          Buffer := FStream.ReadString(1);

        if Length(Buffer) > 0 then
        begin
          if Buffer[1] <> Character then
          begin
            FStream.Seek(-1, soFromCurrent);
            Result := True;
          end
          else
            FToken := FToken + Buffer;
        end
        else
          Result := True;
      end;
    end;
end;

function TQueryParserComp.SpecialCharacter(Character: Char): Boolean;
var
  i: Integer;
begin
  Result := False;
  for i := 1 to Length(FSpecialCharacters) do
    if Character = FSpecialCharacters[i] then
      Result := True;
end;

procedure TQueryParserComp.SetToken;
var
  EndToken: Boolean;
  Buffer: String;
begin
  EndToken := False;
  while not (EndToken or FEOF) do
  begin
    FEOF := FStream.Position >= FStream.Size-1;
    Buffer := FStream.ReadString(1);
    
    if not (Buffer[1] in Delimiters) then
      EndToken := CheckCharcterType(Buffer[1])
    else
    begin
      if not (FString or FComment) then
      begin
        EndToken := True;
        FStream.Seek(-1, soFromCurrent);
      end
      else
      begin
        if FString and ((Buffer[1] = CR) or (Buffer[1] = LF)) then
        begin
          FString := False;
          FWasString := False;
          EndToken := True;
        end
        else
          if FComment and ((cmt2 in FCommentType) or
             (cmt3 in FCommentType)) and ((Buffer[1] = CR) or
             (Buffer[1] = LF)) then
          begin
            EndToken := True;
            FStream.Seek(-1, soFromCurrent);
            FComment := False;
            FCommentType := [];
          end
          else
            FToken := FToken + Buffer;
      end;
    end;
  end; //while
end;

destructor TQueryParserComp.Destroy;
begin
  if Assigned(FStream) then
    FStream.Free;

  inherited Destroy;
end;

procedure TQueryParserComp.DoStatementDelimiter;
var
  Buffer: String;
  CurrentPosition: Integer;
begin
  if Assigned(FOnStatementDelimiter) then
  begin
    CurrentPosition := FStream.Position;
    FStream.Seek(FGoPosition, soFromBeginning);
    Buffer := FStream.ReadString(CurrentPosition-FGoPosition-Length(FToken));
    FStream.Seek(Length(FToken), soFromCurrent);
    if FStream.Position >= FStream.Size then
      FEOF := True;

    FGoPosition := FStream.Position;

    if FCountFromStatement then
      FSymbolsCount := 0;

    FOnStatementDelimiter(Self, Trim(Buffer));
  end;
end;

procedure TQueryParserComp.SetStringToParse(AStringToParse: String);
begin

  if AStringToParse = '' then
    Exit;

  if Assigned(FStream) then
    FStream.Free;

  TrimRight(AStringToParse);
  FStream := TStringStream.Create(AStringToParse);
  FStringToParse := AStringToParse;

  Init;
end;

procedure TQueryParserComp.SetSD(ASD: TStringList);
begin
  FStatementDelimiters.Assign(ASD);
end;

procedure TQueryParserComp.First;
begin
  Init;
end;

procedure TQueryParserComp.Init;
var
  ETextNotSet: Exception;
begin
  if not Assigned(FStream) then
  begin
    ETextNotSet := Exception.Create(sTextNotSet);
    raise ETextNotSet;
  end;

  FStream.Seek(0, soFromBeginning);
  FToken := '';
  FTokenType := ttString;
  FComment := False;
  FCommentType := [];
  FEOF := False;
  FSymbolsCount := 0;
  FGoPosition := 0;
  FLastStringDelimiterFound := '';
  FWasString := False;
  FString := False;
end;

procedure TQueryParserComp.SetSpecialCharacters(ASpecialCharacters: String);
var
  i: Integer;
  k: Integer;
  IllegalSpecialChar: Exception;
begin
  for i := 1 to Length(ASpecialCharacters) do
  begin
    for k := 0 to FStatementDelimiters.Count - 1 do
    begin
      if (Pos(ASpecialCharacters[i], FStatementDelimiters.Strings[k]) <> 0) then
      begin
        IllegalSpecialChar := Exception.Create(sIllegalSpecialChar);
        raise IllegalSpecialChar;
      end;
    end;

    if (ASpecialCharacters[i] in Delimiters) or
      (Pos(ASpecialCharacters[i], FStringDelimiters) <> 0) then
    begin
      IllegalSpecialChar := Exception.Create(sIllegalSpecialChar);
      raise IllegalSpecialChar;
    end;
  end;

  FSpecialCharacters := ASpecialCharacters;
end;

procedure TQueryParserComp.SetStringDelimiters(AStringDelimiters: String);
var
  i: Integer;
  k: Integer;
  IllegalStringChar: Exception;
begin
  for i := 1 to Length(AStringDelimiters) do
  begin
    for k := 0 to FStatementDelimiters.Count - 1 do
    begin
      if (Pos(AStringDelimiters[i], FStatementDelimiters.Strings[k]) <> 0) then
      begin
        IllegalStringChar := Exception.Create(sIllegalStringChar);
        raise IllegalStringChar;
      end;
    end;

    if (AStringDelimiters[i] in Delimiters) or
      (Pos(AStringDelimiters[i], FSpecialCharacters) <> 0) then
    begin
      IllegalStringChar := Exception.Create(sIllegalStringChar);
      raise IllegalStringChar;
    end;
  end;

  FStringDelimiters := AStringDelimiters;
end;

end.
