/*
 *	 __         __
 *	/  \.-"""-./  \		UrSheet - includes.
 *	\    -   -    /		abc19
 *	 |   o   o   |		Dec 28 2024
 *	 \  .-'''-.  /
 *	  '-\__Y__/-'
 *	     `---`
 */
#ifndef URSH_INCL_H
#define URSH_INCL_H

#include <err.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <assert.h>

#define	u8			unsigned char
#define	u16			unsigned short
#define	u32			unsigned int
#define	u64			unsigned long

#define	i8			char
#define	i16			short
#define	i32			int
#define	i64			long

#define	Bool		unsigned char
#define	True		1
#define False		0

#define	FAMILY_SIZE	64
#define	MAX_COLS	26 * 26 + 25
#define	MAX_ROWS	256

enum TokenKind
{
	TokenIsString		= '"',
	TokenIsReference	= '@',
	TokenIsDelimiter	= '|',
	TokenIsAdd			= '+',
	TokenIsMul			= '*',
	TokenIsDiv			= '/',
	TokenIsSub			= '-',
	TokenIsExpr			= '=',
	TokenIsClone		= '^',
	TokenIsRpar			= ')',
	TokenIsLpar			= '(',
	TokenIsNumber		= 256,
};

enum CellKind
{
	CellIsEmpty			= 0,
	CellIsError			= 1,
	CellIsNumber		= TokenIsNumber,
	CellIsText			= TokenIsString,
};

enum CellErrs
{
	ErrCellNotErr		= -1,
	ErrCellOverflow		= 0,
	ErrCellUnknown		= 1,
	ErrCellMalformed	= 2,
	ErrCellBounds		= 3,
	ErrCellPremature	= 4,
};

struct Cell;

union As
{
	struct { const char *s; size_t len; } txt;
	struct Cell *ref;
	long double num;
};

struct Token
{
	union	As as;
	enum	TokenKind kind;
};

struct Cell
{
	struct	Token family[FAMILY_SIZE];
	union	As as;
	u16		nthT;
	enum	CellKind kind;
	Bool	clonable;
};

struct UrSh
{
	struct	{ u16 rows, cols; } sz;
	struct	Cell *grid;
	size_t	length;
	char	*filename, *src;
};

#endif
