" Vim syntax file
" Language: nemesis-lang
" Maintainer: Emanuel Buttaci
" Latest Revision: 12 December 2020
"
if exists("b:current_syntax")
	finish
endif

syn keyword nemesisKeyword
                        \ app
	                \ as
	                \ behaviour
	                \ break
	                \ continue
	                \ else
	                \ ensure
	                \ extend
	                \ extern
	                \ hide
	                \ if
	                \ in
	                \ invariant
	                \ is
                        \ lib
                        \ mutable
	                \ require
	                \ return
	                \ when
                        \ static
	                \ test
                        \ when

syn keyword nemesisForKeyword skipwhite skipempty
	                \ for

syn keyword nemesisPrimitiveType
			\ u8 u16 u32 u64 u128 usize 
			\ i8 i16 i32 i64 i128 isize 
			\ r16 r32 r64 r128 r256 
			\ f32 f64 f128 
			\ c64 c128 c256
			\ char
			\ chars
			\ string
			\ bool

syn keyword nemesisSuffix
			\ u8 u16 u32 u64 u128
			\ i8 i16 i32 i64 i128
			\ f32 f64 f128
			\ s

syn keyword nemesisVariableDeclarator skipwhite skipempty
			\ val
			\ const

syn keyword nemesisTypeDeclarator skipwhite skipempty nextgroup=nemesisNameDeclarator
			\ concept
			\ union
			\ range
			\ type
			\ behaviour

syn keyword nemesisFunctionDeclarator skipwhite skipempty
			\ function

syn keyword nemesisImportDeclarator skipwhite skipempty
			\ use

syn keyword nemesisBoolLiteral
			\ true
			\ false

syn match nemesisNameDeclarator contained skipwhite skipempty
      \ /\<[A-Za-z_][A-Za-z_0-9]*\>/

syn region nemesisLineComment contains=nemesisTodo
			\ start="//" end="$"

syn region nemesisMultiLineComment contains=nemesisMultiLineComment,nemesisTodo
			\ start="/\*" end="\*/"

syn keyword nemesisTodo MARK TODO FIXME contained

syn region nemesisStringLiteral contains=nemesisInterpolationRegion
			\ start=/"/ skip=/\\\\\|\\"/ end=/"/

syn region nemesisInterpolationRegion contained contains=TOP
			\ matchgroup=nemesisInterpolation start=/{/ end=/}/

syn match nemesisChar
			\ /'\([^'\\]\|\\\)*'/

syn match nemesisDecimal
			\ /[+\-]\?\<\([0-9][0-9_]*\)\([.][0-9_]*\)\?\([eE][+\-]\?[0-9][0-9_]*\)\?\>/

syn match nemesisHex
			\ /[+\-]\?\<0x[0-9A-Fa-f][0-9A-Fa-f_]*\>/

syn match nemesisOct
			\ /[+\-]\?\<0o[0-7][0-7_]*\>/

syn match nemesisBin
			\ /[+\-]\?\<0b[01][01_]*\>/

syn match nemesisOperator	
			\ ')\|(\|!\|=\|-\|+\|%\|\.\|>\|<\|;\|,\|:\|*\|&\|@\|\[\|\]\|{\|}'

hi def link nemesisImportDeclarator Include
hi def link nemesisPrimitiveType Type
hi def link nemesisKeyword Statement
hi def link nemesisForKeyword Statement
hi def link nemesisFunctionDeclarator Define
hi def link nemesisVariableDeclarator Define
hi def link nemesisTypeDeclarator Define
hi def link nemesisBoolLiteral Boolean
hi def link nemesisLineComment Comment
hi def link nemesisMultiLineComment Comment
hi def link nemesisTodo Todo
hi def link nemesisStringLiteral String
hi def link nemesisInterpolation Special
hi def link nemesisDecimal Number
hi def link nemesisHex Number
hi def link nemesisOct Number
hi def link nemesisBin Number
hi def link nemesisOperator Operator
hi def link nemesisChar Character
hi def link nemesisNameDeclarator Identifier

let b:current_syntax = "nemesis"
