{
  pcRegExp - Perl compatible regular expressions for Virtual Pascal
  (c) 2001 Peter S. Voronov aka Chem O'Dun <petervrn@yahoo.com>

  Based on PCRE library interface unit for Virtual Pascal.
  (c) 2001 Alexander Tokarev <dwalin@dwalin.ru>

  The current PCRE version is: 3.7

  This software may be distributed under the terms of the modified BSD license
  Copyright (c) 2001, Alexander Tokarev
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of the <ORGANIZATION> nor the names of its contributors
      may be used to endorse or promote products derived from this software without
      specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  The PCRE library is written by: Philip Hazel <ph10@cam.ac.uk>
  Copyright (c) 1997-2004 University of Cambridge

  AngelsHolocaust 4-11-04 updated to use version v5.0
  (INFO: this is regex-directed, NFA)
  AH:  9-11-04 - pcre_free: removed var, pcre already gives the ptr, now
			    everything works as it should (no more crashes)
		 -> removed CheckRegExp because pcre handles errors perfectly
      10-11-04 - added pcError (errorhandling), pcInit
      13-11-04 - removed the ErrorPos = 0 check -> always print erroroffset
      17-10-05 - support for \1-\9 backreferences in TpcRegExp.GetReplStr
      17-02-06 - added RunTimeOptions: caller can set options while searching
      19-02-06 - added SearchOfs(): let PCRE use the complete string and offset
		 into the string itself
      20-12-06 - support for version 7.0
      27.08.08 - support for v7.7
}

{$H+} {$DEFINE PCRE_3_7} {$DEFINE PCRE_5_0} {$DEFINE PCRE_7_0} {$DEFINE PCRE_7_7}

Unit pcregexp;

Interface

uses objects;

Type
 PpcRegExp = ^TpcRegExp;
// TpcRegExp = object
 TpcRegExp = object(TObject)
  MatchesCount: integer;
  RegExpC, RegExpExt : Pointer;
  Matches:Pointer;
  RegExp: shortstring;
  SourceLen: integer;
  PartialMatch : boolean;
  Error : boolean;
  ErrorMsg : Pchar;
  ErrorPos : integer;
  RunTimeOptions: Integer; // options which can be set by the caller
  constructor Init(const ARegExp : shortstring; AOptions : integer; ALocale : Pointer);
  function Search(AStr: Pchar; ALen : longint) : boolean; virtual;
  function SearchNext( AStr: Pchar; ALen : longint) : boolean; virtual;
  function SearchOfs ( AStr: Pchar; ALen, AOfs : longint) : boolean; virtual;
  function MatchSub(ANom: integer; var Pos, Len : longint) : boolean; virtual;
  function MatchFull(var Pos, Len : longint) : boolean; virtual;
  function GetSubStr(ANom: integer; AStr: Pchar) : string; virtual;
  function GetFullStr(AStr: Pchar) : string; virtual;
  function GetReplStr(AStr: Pchar; const ARepl: string) : string; virtual;
  function GetPreSubStr(AStr: Pchar) : string; virtual;
  function GetPostSubStr(AStr: Pchar) : string; virtual;
  function ErrorStr : string; virtual;
  destructor Done; virtual;
 end;

 function pcGrepMatch(WildCard, aStr: string; AOptions:integer; ALocale : Pointer): Boolean;
 function pcGrepSub(WildCard, aStr, aRepl: string; AOptions:integer; ALocale : Pointer): string;

 function pcFastGrepMatch(WildCard, aStr: string): Boolean;
 function pcFastGrepSub(WildCard, aStr, aRepl: string): string;

{$IFDEF PCRE_5_0}
 function pcGetVersion : pchar;
{$ENDIF}

 function pcError (var pRegExp : Pointer) : Boolean;
 function pcInit  (const Pattern: Shortstring; CaseSens: Boolean) : Pointer;

Const { Options }
 PCRE_CASELESS         = $0001;
 PCRE_MULTILINE        = $0002;
 PCRE_DOTALL           = $0004;
 PCRE_EXTENDED         = $0008;
 PCRE_ANCHORED         = $0010;
 PCRE_DOLLAR_ENDONLY   = $0020;
 PCRE_EXTRA            = $0040;
 PCRE_NOTBOL           = $0080;
 PCRE_NOTEOL           = $0100;
 PCRE_UNGREEDY         = $0200;
 PCRE_NOTEMPTY         = $0400;
{$IFDEF PCRE_5_0}
 PCRE_UTF8             = $0800;
 PCRE_NO_AUTO_CAPTURE  = $1000;
 PCRE_NO_UTF8_CHECK    = $2000;
 PCRE_AUTO_CALLOUT     = $4000;
 PCRE_PARTIAL          = $8000;
{$ENDIF}
{$IFDEF PCRE_7_0}
 PCRE_DFA_SHORTEST     = $00010000;
 PCRE_DFA_RESTART      = $00020000;
 PCRE_FIRSTLINE        = $00040000;
 PCRE_DUPNAMES         = $00080000;
 PCRE_NEWLINE_CR       = $00100000;
 PCRE_NEWLINE_LF       = $00200000;
 PCRE_NEWLINE_CRLF     = $00300000;
 PCRE_NEWLINE_ANY      = $00400000;
 PCRE_NEWLINE_ANYCRLF  = $00500000;

 PCRE_NEWLINE_BITS     = PCRE_NEWLINE_CR or PCRE_NEWLINE_LF or PCRE_NEWLINE_ANY;

{$ENDIF}
{$IFDEF PCRE_7_7}
 PCRE_BSR_ANYCRLF      = $00800000;
 PCRE_BSR_UNICODE      = $01000000;
 PCRE_JAVASCRIPT_COMPAT= $02000000;
{$ENDIF}

 PCRE_COMPILE_ALLOWED_OPTIONS = PCRE_ANCHORED + PCRE_AUTO_CALLOUT + PCRE_CASELESS  +
				PCRE_DOLLAR_ENDONLY + PCRE_DOTALL + PCRE_EXTENDED  +
				PCRE_EXTRA + PCRE_MULTILINE + PCRE_NO_AUTO_CAPTURE +
				PCRE_UNGREEDY + PCRE_UTF8 + PCRE_NO_UTF8_CHECK
				{$IFDEF PCRE_7_0}
				+ PCRE_DUPNAMES + PCRE_FIRSTLINE + PCRE_NEWLINE_BITS
				{$ENDIF}
				{$IFDEF PCRE_7_7}
				+ PCRE_BSR_ANYCRLF + PCRE_BSR_UNICODE + PCRE_JAVASCRIPT_COMPAT
				{$ENDIF}
				;

 PCRE_EXEC_ALLOWED_OPTIONS = PCRE_ANCHORED + PCRE_NOTBOL + PCRE_NOTEOL +
			     PCRE_NOTEMPTY + PCRE_NO_UTF8_CHECK + PCRE_PARTIAL
			     {$IFDEF PCRE_7_0}
			     + PCRE_NEWLINE_BITS
			     {$ENDIF}
			     {$IFDEF PCRE_7_7}
			     + PCRE_BSR_ANYCRLF + PCRE_BSR_UNICODE
			     {$ENDIF}
			     ;

{$IFDEF PCRE_7_0}
 PCRE_DFA_EXEC_ALLOWED_OPTIONS = PCRE_ANCHORED + PCRE_NOTBOL + PCRE_NOTEOL +
				 PCRE_NOTEMPTY + PCRE_NO_UTF8_CHECK + PCRE_PARTIAL +
				 PCRE_DFA_SHORTEST + PCRE_DFA_RESTART +
				 PCRE_NEWLINE_BITS
				 {$IFDEF PCRE_7_7}
				 + PCRE_BSR_ANYCRLF + PCRE_BSR_UNICODE
				 {$ENDIF}
				 ;
{$ENDIF}

{ Exec-time and get/set-time error codes }
 PCRE_ERROR_NOMATCH        =  -1;
 PCRE_ERROR_NULL	   =  -2;
 PCRE_ERROR_BADOPTION      =  -3;
 PCRE_ERROR_BADMAGIC       =  -4;
 PCRE_ERROR_UNKNOWN_MODE   =  -5;
 PCRE_ERROR_NOMEMORY       =  -6;
 PCRE_ERROR_NOSUBSTRING    =  -7;
{$IFDEF PCRE_5_0}
 PCRE_ERROR_MATCHLIMIT     =  -8;
 PCRE_ERROR_CALLOUT        =  -9;  { Never used by PCRE itself }
 PCRE_ERROR_BADUTF8        = -10;
 PCRE_ERROR_BADUTF8_OFFSET = -11;
 PCRE_ERROR_PARTIAL        = -12;
 PCRE_ERROR_BADPARTIAL     = -13;
 PCRE_ERROR_INTERNAL       = -14;
 PCRE_ERROR_BADCOUNT       = -15;
{$ENDIF}
{$IFDEF PCRE_7_0}
 PCRE_ERROR_DFA_UITEM      = -16;
 PCRE_ERROR_DFA_UCOND      = -17;
 PCRE_ERROR_DFA_UMLIMIT    = -18;
 PCRE_ERROR_DFA_WSSIZE     = -19;
 PCRE_ERROR_DFA_RECURSE    = -20;
 PCRE_ERROR_RECURSIONLIMIT = -21;
 PCRE_ERROR_NULLWSLIMIT    = -22;
 PCRE_ERROR_BADNEWLINE     = -23;
{$ENDIF}

{ Request types for pcre_fullinfo() }

 PCRE_INFO_OPTIONS         =  0;
 PCRE_INFO_SIZE 	   =  1;
 PCRE_INFO_CAPTURECOUNT    =  2;
 PCRE_INFO_BACKREFMAX      =  3;
 PCRE_INFO_FIRSTBYTE       =  4;
 PCRE_INFO_FIRSTCHAR       =  4; { For backwards compatibility }
 PCRE_INFO_FIRSTTABLE      =  5;
{$IFDEF PCRE_5_0}
 PCRE_INFO_LASTLITERAL     =  6;
 PCRE_INFO_NAMEENTRYSIZE   =  7;
 PCRE_INFO_NAMECOUNT       =  8;
 PCRE_INFO_NAMETABLE       =  9;
 PCRE_INFO_STUDYSIZE       = 10;
 PCRE_INFO_DEFAULT_TABLES  = 11;
{$ENDIF PCRE_5_0}
{$IFDEF PCRE_7_7}
 PCRE_INFO_OKPARTIAL       = 12;
 PCRE_INFO_JCHANGED        = 13;
 PCRE_INFO_HASCRORLF       = 14;
{$ENDIF}

{ Request types for pcre_config() }
{$IFDEF PCRE_5_0}
 PCRE_CONFIG_UTF8       	    = 0;
 PCRE_CONFIG_NEWLINE    	    = 1;
 PCRE_CONFIG_LINK_SIZE  	    = 2;
 PCRE_CONFIG_POSIX_MALLOC_THRESHOLD = 3;
 PCRE_CONFIG_MATCH_LIMIT	    = 4;
 PCRE_CONFIG_STACKRECURSE           = 5;
 PCRE_CONFIG_UNICODE_PROPERTIES     = 6;
{$ENDIF PCRE_5_0}
{$IFDEF PCRE_7_0}
 PCRE_CONFIG_MATCH_LIMIT_RECURSION  = 7;
{$ENDIF}
{$IFDEF PCRE_7_7}
 PCRE_CONFIG_BSR		    = 8;
{$ENDIF}

{ Bit flags for the pcre_extra structure }
{$IFDEF PCRE_5_0}
 PCRE_EXTRA_STUDY_DATA  	  = $0001;
 PCRE_EXTRA_MATCH_LIMIT 	  = $0002;
 PCRE_EXTRA_CALLOUT_DATA	  = $0004;
 PCRE_EXTRA_TABLES      	  = $0008;
{$ENDIF PCRE_5_0}
{$IFDEF PCRE_7_0}
 PCRE_EXTRA_MATCH_LIMIT_RECURSION = $0010;
{$ENDIF}

Const
// DefaultOptions : integer = 0;
 DefaultLocaleTable : pointer = nil;

{$IFDEF PCRE_5_0}
{ The structure for passing additional data to pcre_exec(). This is defined in
such as way as to be extensible. Always add new fields at the end, in order to
remain compatible. }

type ppcre_extra = ^tpcre_extra;
     tpcre_extra = record
       flags : longint; 	       { Bits for which fields are set }
       study_data : pointer;           { Opaque data from pcre_study() }
       match_limit : longint;          { Maximum number of calls to match() }
       callout_data : pointer;         { Data passed back in callouts }
       tables : pointer;	       { Pointer to character tables }
       match_limit_recursion: longint; { Max recursive calls to match() }
     end;

type ppcre_callout_block = ^pcre_callout_block;
     pcre_callout_block = record
       version,
  (* ------------------------ Version 0 ------------------------------- *)
       callout_number : integer;
       offset_vector : pointer;
       subject : pchar;
       subject_length, start_match, current_position, capture_top,
       capture_last : integer;
       callout_data : pointer;
  (* ------------------- Added for Version 1 -------------------------- *)
       pattern_position, next_item_length : integer;
     end;
{$ENDIF PCRE_5_0}

{$OrgName+}
{$IFDEF VIRTUALPASCAL} {&Cdecl+} {$ENDIF VIRTUALPASCAL}

 { local replacement of external pcre memory management functions }
 function pcre_malloc( size : integer ) : pointer;
 procedure pcre_free( {var} p : pointer );
{$IFDEF PCRE_5_0}
 const pcre_stack_malloc: function ( size : integer ): pointer = pcre_malloc;
       pcre_stack_free: procedure ( {var} p : pointer ) = pcre_free;
 function pcre_callout(var p : ppcre_callout_block) : integer;
{$ENDIF PCRE_5_0}
{$IFDEF VIRTUALPASCAL} {&Cdecl-} {$ENDIF VIRTUALPASCAL}

Implementation

Uses strings, collect, messages, dnapp, commands, advance0, stringsx
    {$IFDEF VIRTUALPASCAL} ,vpsyslow {$ENDIF VIRTUALPASCAL};

Const
 MAGIC_NUMBER = $50435245; { 'PCRE' }
 MAX_MATCHES = 90; { changed in 3.5 version; should be divisible by 3, was 64}

Type
 PMatchArray = ^TMatchArray;
 TMatchArray = array[0..( MAX_MATCHES * 3 )] of integer;

 PRegExpCollection = ^TRegExpCollection;
 TRegExpCollection =  object(TSortedCollection)
   MaxRegExp : integer;
   SearchRegExp : shortstring;
   CompareModeInsert : boolean;
   constructor Init(AMaxRegExp:integer);
   procedure FreeItem(P: Pointer); virtual;
   function  Compare(P1, P2: Pointer): Integer; virtual;
   function  Find(ARegExp:shortstring;var P: PpcRegExp):boolean; virtual;
   function CheckNew(ARegExp:shortstring):PpcRegExp;virtual;
 end;

Var
 PRegExpCache : PRegExpCollection;


{$IFDEF VIRTUALPASCAL} {&Cdecl+} {$ENDIF VIRTUALPASCAL}

 { imported original pcre functions }

 function pcre_compile( const pattern : PChar; options : integer;
			var errorptr : PChar; var erroroffset : integer;
			const tables : PChar ) : pointer {pcre}; external;
{$IFDEF PCRE_7_0}
 function pcre_compile2( const pattern : PChar; options : integer;
			 var errorcodeptr : Integer;
			 var errorptr : PChar; var erroroffset : integer;
			 const tables : PChar ) : pointer {pcre}; external;
{$ENDIF}
{$IFDEF PCRE_5_0}
 function pcre_config( what : integer; where : pointer) : integer; external;
 function pcre_copy_named_substring( const code : pointer {pcre};
				     const subject : pchar;
				     var ovector : integer;
				     stringcount : integer;
				     const stringname : pchar;
				     var buffer : pchar;
				     size : integer) : integer; external;
 function pcre_copy_substring( const subject : pchar; var ovector : integer;
			       stringcount, stringnumber : integer;
			       var buffer : pchar; size : integer )
			       : integer; external;
 function pcre_exec( const argument_re : pointer {pcre};
		     const extra_data : pointer {pcre_extra};
{$ELSE}
 function pcre_exec( const external_re : pointer;
		     const external_extra : pointer;
{$ENDIF}
		     const subject : PChar;
		     length, start_offset, options : integer;
		     offsets : pointer;
		     offsetcount : integer ) : integer; external;
{$IFDEF PCRE_7_0}
 function pcre_dfa_exec( const argument_re : pointer {pcre};
			 const extra_data : pointer {pcre_extra};
			 const subject : pchar;
			 length, start_offset, options : integer;
			 offsets : pointer;
			 offsetcount : integer;
			 workspace : pointer;
			 wscount : integer ) : integer; external;
{$ENDIF}
{$IFDEF PCRE_5_0}
 procedure pcre_free_substring( const p : pchar ); external;
 procedure pcre_free_substring_list( var p : pchar ); external;
 function pcre_fullinfo( const argument_re : pointer {pcre};
			 const extra_data : pointer {pcre_extra};
			 what : integer;
			 where : pointer ) : integer; external;
 function pcre_get_named_substring( const code : pointer {pcre};
				    const subject : pchar;
				    var ovector : integer;
				    stringcount : integer;
				    const stringname : pchar;
				    var stringptr : pchar ) : integer; external;
 function pcre_get_stringnumber( const code : pointer {pcre};
				 const stringname : pchar ) : integer; external;
 function pcre_get_stringtable_entries( const code : pointer {pcre};
					const stringname : pchar;
					var firstptr,
					    lastptr : pchar ) : integer; external;
 function pcre_get_substring( const subject : pchar; var ovector : integer;
			      stringcount, stringnumber : integer;
			      var stringptr : pchar ) : integer; external;
 function pcre_get_substring_list( const subject : pchar; var ovector : integer;
				   stringcount : integer;
				   listptr : pointer {const char ***listptr}) : integer; external;
 function pcre_info( const argument_re : pointer {pcre};
		     var optptr : integer;
		     var first_byte : integer ) : integer; external;
 function pcre_maketables : pchar; external;
{$ENDIF}
{$IFDEF PCRE_7_0}
 function pcre_refcount( const argument_re : pointer {pcre};
			 adjust : integer ) : pchar; external;
{$ENDIF}
 function pcre_study( const external_re : pointer {pcre};
		      options : integer;
		      var errorptr : PChar ) : pointer {pcre_extra}; external;
{$IFDEF PCRE_5_0}
 function pcre_version : pchar; external;
{$ENDIF}

 function pcre_malloc( size : integer ) : pointer;
 begin
  GetMem( result, size );
 end;

 procedure pcre_free( {var} p : pointer );
 begin
  if (p <> nil) then
    FreeMem( p, 0 );
  {@p := nil;}
 end;

{$IFDEF PCRE_5_0}
(* Called from PCRE as a result of the (?C) item. We print out where we are in
the match. Yield zero unless more callouts than the fail count, or the callout
data is not zero. *)

 function pcre_callout;
 begin
 end;
{$ENDIF}

{$IFDEF VIRTUALPASCAL} {&Cdecl-} {$ENDIF VIRTUALPASCAL}

// Always include the newest version of the library
{$IFDEF PCRE_7_7}
  {$L pcre77.lib}
{$ELSE}
  {$IFDEF PCRE_7_0}
    {$L pcre70.lib}
  {$ELSE}
    {$IFDEF PCRE_5_0}
      {$L pcre50.lib}
    {$ELSE}
      {$IFDEF PCRE_3_7}
	{$L pcre37.lib}
      {$ENDIF PCRE_3_7}
    {$ENDIF PCRE_5_0}
  {$ENDIF PCRE_7_0}
{$ENDIF PCRE_7_7}

{TpcRegExp}

 constructor TpcRegExp.Init(const ARegExp:shortstring; AOptions:integer; ALocale : Pointer);
 var
  pRegExp : PChar;
 begin
  RegExp:=ARegExp;
  RegExpC:=nil;
  RegExpExt:=nil;
  Matches:=nil;
  MatchesCount:=0;
  Error:=true;
  ErrorMsg:=nil;
  ErrorPos:=0;
  RunTimeOptions := 0;
  if length(RegExp) < 255 then
   begin
    RegExp[length(RegExp)+1]:=#0;
    pRegExp:=@RegExp[1];
   end
  else
   begin
    GetMem(pRegExp,length(RegExp)+1);
    pRegExp:=strpcopy(pRegExp,RegExp);
   end;
  RegExpC := pcre_compile( pRegExp,
			   AOptions and PCRE_COMPILE_ALLOWED_OPTIONS,
			   ErrorMsg, ErrorPos, ALocale);
  if length(RegExp) = 255 then
   StrDispose(pRegExp);
  if RegExpC = nil then
   exit;
  ErrorMsg:=nil;
  RegExpExt := pcre_study( RegExpC, 0, ErrorMsg );
  if (RegExpExt = nil) and (ErrorMsg <> nil) then
   begin
    pcre_free(RegExpC);
    exit;
   end;
  GetMem(Matches,SizeOf(TMatchArray));
  Error:=false;
 end;

 destructor TpcRegExp.Done;
 begin
  if RegExpC <> nil then
    pcre_free(RegExpC);
  if RegExpExt <> nil then
    pcre_free(RegExpExt);
  if Matches <> nil then
    FreeMem(Matches,SizeOf(TMatchArray));
 end;

 function TpcRegExp.SearchNext( AStr: Pchar; ALen : longint ) : boolean;
 var Options: Integer;
 begin // must handle PCRE_ERROR_PARTIAL here
  Options := (RunTimeOptions or startup.MiscMultiData.cfgRegEx.DefaultOptions) and
	     PCRE_EXEC_ALLOWED_OPTIONS;
  if MatchesCount > 0 then
    MatchesCount:=pcre_exec( RegExpC, RegExpExt, AStr, ALen, PMatchArray(Matches)^[1],
			     Options, Matches, MAX_MATCHES ) else
    MatchesCount:=pcre_exec( RegExpC, RegExpExt, AStr, ALen, 0,
			     Options, Matches, MAX_MATCHES );
{  if MatchesCount = 0 then
    MatchesCount := MatchesCount div 3;}
  PartialMatch := MatchesCount = PCRE_ERROR_PARTIAL;
  SearchNext := MatchesCount > 0;
 end;

 function TpcRegExp.Search( AStr: Pchar; ALen : longint):boolean;
 begin
  MatchesCount:=0;
  Search:=SearchNext(AStr,ALen);
  SourceLen:=ALen;
 end;

 function TpcRegExp.SearchOfs( AStr: Pchar; ALen, AOfs: longint ) : boolean;
 var Options: Integer;
 begin
  MatchesCount:=0;
  Options := (RunTimeOptions or startup.MiscMultiData.cfgRegEx.DefaultOptions) and
	     PCRE_EXEC_ALLOWED_OPTIONS;
  MatchesCount:=pcre_exec( RegExpC, RegExpExt, AStr, ALen, AOfs,
			   Options, Matches, MAX_MATCHES );
  PartialMatch := MatchesCount = PCRE_ERROR_PARTIAL;
  SearchOfs := MatchesCount > 0;
  SourceLen := ALen-AOfs;
 end;

 function TpcRegExp.MatchSub(ANom:integer; var Pos,Len:longint):boolean;
 begin
  if (MatchesCount > 0) and (ANom <= (MatchesCount-1)) then
   begin
    ANom:=ANom*2;
    Pos:=PMatchArray(Matches)^[ANom];
    Len:=PMatchArray(Matches)^[ANom+1]-Pos;
    MatchSub:=true;
   end
  else
   MatchSub:=false;
 end;

 function TpcRegExp.MatchFull(var Pos,Len:longint):boolean;
 begin
  MatchFull:=MatchSub(0,Pos,Len);
 end;

 function TpcRegExp.GetSubStr(ANom: integer; AStr: Pchar):string;
 var
  s: ansistring;
  pos,len: longint;
 begin
  s:='';
  if MatchSub(ANom, pos, len) then
   begin
    setlength(s, len);
    Move(AStr[pos], s[1], len);
   end;
  GetSubStr:=s;
 end;

 function TpcRegExp.GetPreSubStr(AStr: Pchar):string;
 var
  s: ansistring;
  l: longint;
 begin
  s:='';
  if (MatchesCount > 0) then
   begin
    l:=PMatchArray(Matches)^[0]-1;
    if l > 0 then
     begin
      setlength(s,l);
      Move(AStr[1],s[1],l);
     end;
   end;
  GetPreSubStr:=s;
 end;

 function TpcRegExp.GetPostSubStr(AStr: Pchar):string;
 var
  s: ansistring;
  l: longint;
  ANom: integer;
 begin
  s:='';
  if (MatchesCount > 0) then
   begin
    ANom:=(MatchesCount-1){*2} shl 1;
    l:=SourceLen-PMatchArray(Matches)^[ANom+1]+1;
    if l > 0 then
     begin
      setlength(s,l);
      Move(AStr[PMatchArray(Matches)^[ANom+1]],s[1],l);
     end;
   end;
  GetPostSubStr:=s;
 end;


 function TpcRegExp.GetFullStr(AStr: Pchar):string;
 var
  s: ansistring;
  l: longint;
 begin
  GetFullStr:=GetSubStr(0,AStr);
 end;

 function TpcRegExp.GetReplStr(AStr: Pchar; const ARepl: string):string;
 var
  s: ansistring;
  l,i,lasti: longint;
 begin
  l:=length(ARepl);
  i:=1;
  lasti:=1;
  s:='';
  while i <= l do
   begin
    case ARepl[i] of
     '\' :
      begin
       if i < l then
	begin
	 s:=s+copy(ARepl,lasti,i-lasti){+ARepl[i+1]};
	 {AH 17-10-05 support for POSIX \1-\9 backreferences}
	 case ARepl[i+1] of
	  '0' : s:=s+GetFullStr(AStr);
	  '1'..'9' : s:=s+GetSubStr(ord(ARepl[i+1])-ord('0'),AStr);
	  else s:=s+ARepl[i+1]; // copy the escaped character
	 end;
	end;
       inc(i);
       lasti:=i+1;
      end;
     '$' :
      begin
       if i < l then
	begin
	 s:=s+copy(ARepl,lasti,i-lasti);
	 case ARepl[i+1] of
	  '&' : s:=s+GetFullStr(AStr);
	  '1'..'9' : s:=s+GetSubStr(ord(ARepl[i+1])-ord('0'),AStr);
	  '`' : s:=s+GetPreSubStr(AStr);
	  #39 : s:=s+GetPostSubStr(AStr);
	 end;
	end;
       inc(i);
       lasti:=i+1;
      end;
    end;
    inc(i);
   end;
  if lasti <= {AH 25-10-2004 added =, else l==1 won't work} l then
    s:=s+copy(ARepl,lasti,l-lasti+1);
  GetReplStr:=s;
 end;

 function TpcRegExp.ErrorStr:string;
  begin
   ErrorStr:=StrPas(ErrorMsg);
  end;

{TRegExpCollection}

constructor TRegExpCollection.Init(AMaxRegExp: integer);
begin
 Inherited Init(1,1);
 MaxRegExp:=AMaxRegExp;
 CompareModeInsert:=true;
end;

procedure TRegExpCollection.FreeItem(P: Pointer);
begin
 if P <> nil then
  begin
   Dispose(PpcRegExp(P),Done);
  end;
end;

function  TRegExpCollection.Compare(P1, P2: Pointer): Integer;
//var
// l,l1,l2,i : byte;
//// wPos: pchar;
begin
 if CompareModeInsert then
  begin
//   l1:=length(PpcRegExp(P1)^.RegExp);
//   l2:=length(PpcRegExp(P2)^.RegExp);
//   if l1 > l2 then l:=l2 else
//      	     l:=l1;
//   for i:=1 to l do
//     if PpcRegExp(P1).RegExp[i] <> PpcRegExp(P2).RegExp[i] then break;
//   if i <=l then
//     Compare:=ord(PpcRegExp(P1).RegExp[i])-ord(PpcRegExp(P2).RegExp[i]) else
//     Compare:=l1-l2;
    Compare := stringsx.PasStrCmp(PpcRegExp(P1).RegExp, PpcRegExp(P2).RegExp, False);
  end
 else
  begin
//   l1:=length(PpcRegExp(P1)^.RegExp);
//   l2:=length(SearchRegExp);
//   if l1 > l2 then l:=l2 else
//      	     l:=l1;
//   for i:=1 to l do
//     if PpcRegExp(P1).RegExp[i] <> SearchRegExp[i] then
//     begin
//       Compare:=ord(PpcRegExp(P1).RegExp[i])-ord(SearchRegExp[i]);
//       break;
//     end;
//   if i > l then Compare:=l1-l2;
    Compare := stringsx.PasStrCmp(PpcRegExp(P1).RegExp, SearchRegExp, False);
  end;
end;

function  TRegExpCollection.Find(ARegExp:shortstring;var P: PpcRegExp):boolean;
var I : integer;
begin
 CompareModeInsert:=false;
 SearchRegExp:=ARegExp;
 if Search(nil,I) then
  begin
   P:=PpcRegExp(At(I));
   Find:=true;
  end
 else
  begin
   P:=nil;
   Find:=false;
  end;
 CompareModeInsert:=true;
end;

function TRegExpCollection.CheckNew(ARegExp:shortstring):PpcRegExp;
var
 P : PpcRegExp;
begin
 if not Find(ARegExp,P) then
  begin
   if Count = MaxRegExp then
    AtFree(0);
   P:=New(ppcRegExp,Init(ARegExp,PCRE_CASELESS,nil));
   Insert(P);
  end;
 CheckNew:=P;
end;

function pcGrepMatch(WildCard, aStr: string; AOptions:integer; ALocale : Pointer): Boolean;
var
 PpcRE:PpcRegExp;
begin
 PpcRE:=New(ppcRegExp,Init(WildCard,AOptions,Alocale));
 pcGrepMatch:=PpcRE^.Search(pchar(AStr),Length(AStr));
 Dispose(PpcRE,Done);
end;

function pcGrepSub(WildCard, aStr, aRepl: string; AOptions:integer; ALocale : Pointer): string;
var
 PpcRE:PpcRegExp;
begin
 PpcRE:=New(ppcRegExp,Init(WildCard,AOptions,Alocale));
 if PpcRE^.Search(pchar(AStr),Length(AStr)) then
  pcGrepSub:=PpcRE^.GetReplStr(pchar(AStr),ARepl)
 else
  pcGrepSub:='';
 Dispose(PpcRE,Done);
end;

function pcFastGrepMatch(WildCard, aStr: string): Boolean;
var
 PpcRE:PpcRegExp;
begin
 PpcRE:=PRegExpCache^.CheckNew(WildCard);
 pcFastGrepMatch:=PpcRE^.Search(pchar(AStr),Length(AStr));
end;

function pcFastGrepSub(WildCard, aStr, aRepl: string): string;
var
 PpcRE:PpcRegExp;
begin
 PpcRE:=PRegExpCache^.CheckNew(WildCard);
 if PpcRE^.Search(pchar(AStr),Length(AStr)) then
  pcFastGrepSub:=PpcRE^.GetReplStr(pchar(AStr),ARepl)
 else
  pcFastGrepSub:='';
end;

{$IFDEF PCRE_5_0}
function pcGetVersion : pchar; assembler; {$FRAME-}{$USES none}
asm
  call pcre_version
end;
{$ENDIF PCRE_5_0}

function pcError;
var P: ppcRegExp absolute pRegExp;
begin
  Result := (P = nil) or P^.Error;
  If Result and (P <> nil) then
  begin
{     if P^.ErrorPos = 0 then
      MessageBox(GetString(erRegExpCompile)+'"'+P^.ErrorStr+'"', nil,mfConfirmation+mfOkButton)
    else}
      MessageBox(GetString(erRegExpCompile)+'"'+P^.ErrorStr+'"'+GetString(erRegExpCompPos),
		 @P^.ErrorPos,mfConfirmation+mfOkButton);
    Dispose(P, Done);
    P:=nil;
  end;
end;

function pcInit;
var Options : Integer;
begin
  If CaseSens then Options := 0 else Options := PCRE_CASELESS;
  Result := New( PpcRegExp, Init( Pattern,
				  {DefaultOptions}
				  startup.MiscMultiData.cfgRegEx.DefaultOptions or Options,
				  DefaultLocaleTable) );
end;

Initialization
 PRegExpCache:=New(PRegExpCollection,Init(64));
Finalization
 Dispose(PRegExpCache,Done);
End.
