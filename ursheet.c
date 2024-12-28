/*
 *	 __         __
 *	/  \.-"""-./  \		UrSheet - main file.
 *	\    -   -    /		abc19
 *	 |   o   o   |		Dec 27 2024
 *	 \  .-'''-.  /
 *	  '-\__Y__/-'
 *	     `---`
 */
#include <err.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <assert.h>

#define	u8		unsigned char
#define	u16		unsigned short
#define	u32		unsigned int
#define	u64		unsigned long

#define	i8		signed char
#define	i16		signed short
#define	i32		signed int
#define	i64		signed long

#define	Bool	unsigned char
#define	True	1
#define False	0

#define	FAMILY_SIZE	64
#define	MAX_COLS	26 + 25 * 26
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
	TokenIsNumber,
};

enum CellKind
{
	CellIsEmpty		= 0,
	CellIsError		= 1,
	CellIsNumber	= TokenIsNumber,
	CellIsText		= TokenIsString,
};

enum CellErrs
{
	ErrCellOverflow		= 0,
	ErrCellUnknown		= 1,
	ErrCellMalformed	= 2
};

struct Cell;

union As
{
	struct { const char *s; size_t len; } txt;
	struct { const struct Cell *to; u16 col, row; } ref;
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
	Bool	solved;
};

struct UrSh
{
	struct	{ u16 rows, cols; } sz;
	struct	Cell *grid;
	size_t	length;
	char	*filename, *src;
};

static void readContents (struct UrSh *const, FILE*);
static void getTableDimensions (const char*, u16*, u16*);

static void lexTable (struct UrSh *const);
static void pushTokenIntoCell (struct Cell *const, enum TokenKind, union As);

static void setCell2Error (struct Cell *const, u8);
static void operateCell (struct Cell *const);

static void getStringAsToken (const char *const, size_t*, union As*);
static void getReferenceAsToken (const char *const, size_t*, union As*);

static long double getNumberAsToken (const char *const, size_t*);

int main (int argc, char **argv)
{
	if (argc != 2)
		errx(EXIT_SUCCESS, "usage: %s <sheet>\nThat simple ;)", *argv);

	struct UrSh us = {0};
	us.filename = argv[1];

	readContents(&us, fopen(us.filename, "r"));
	getTableDimensions(us.src, &us.sz.rows, &us.sz.cols);

	if (us.sz.cols >= MAX_COLS)
		errx(EXIT_FAILURE, "Maximum number of columns reached which is %d\n", MAX_COLS);
	if (us.sz.rows >= MAX_ROWS)
		errx(EXIT_FAILURE, "Maximum number of rows reached which is %d\n", MAX_ROWS);

	us.grid = (struct Cell*) calloc(us.sz.rows * us.sz.cols, sizeof(struct Cell));
	assert(us.grid && "cannot allocate memory");

	lexTable(&us);
	free(us.grid);
	free(us.src);

	return 0;
}

static void readContents (struct UrSh *const us, FILE *file)
{
	if (!file)
		err(EXIT_FAILURE, "fatal: %s generating some issues", us->filename);

	fseek(file, 0, SEEK_END);
	us->length = ftell(file);
	fseek(file, 0, SEEK_SET);

	us->src = (char*) calloc(1 + us->length, 1);
	assert(us->src && "cannot allocate memory");

	const size_t BRead = fread(us->src, 1, us->length, file);

	if (BRead != us->length)
		errx(EXIT_FAILURE, "cannot read whole file (%ld/%ld B)", us->length, BRead);	

	fclose(file);
}

static void getTableDimensions (const char *src, u16 *row, u16 *col)
{
	u16 mcol = 0;

	while (*src) {
		switch (*src++) {
			case '|':
				mcol++;
				break;
			case '\n':
				*row += 1;
				*col = (*col > mcol) ? *col : mcol;
				mcol = 0;
				break;
		}
	}

	*col = (*col > mcol) ? *col : mcol;
}

static void lexTable (struct UrSh *const us)
{
	union As tokInfo;
	struct Cell *currentCell = &us->grid[0];

	u16 nrow = 0;

	for (size_t k = 0; k < us->length; k++) {
		const char chr = us->src[k];
		Bool isNumber = False;

		switch (chr) {
			case '+':
			case '/':
			case '*':
			case '^':
			case '=':
				pushTokenIntoCell(currentCell, chr, tokInfo);
				continue;

			case '|':
				operateCell(currentCell++);
				continue;

			case  10:
				currentCell = &us->grid[++nrow * us->sz.cols];
				continue;

			case '"':
				getStringAsToken(us->src, &k, &tokInfo);
				pushTokenIntoCell(currentCell, TokenIsString, tokInfo);
				continue;

			case '@':
				getReferenceAsToken(us->src, &k, &tokInfo);
				pushTokenIntoCell(currentCell, TokenIsReference, tokInfo);
				continue;
		}

		if (chr == '-') {
			if ((k + 1) < us->length && isdigit(us->src[k + 1]))
			{ isNumber = True; goto getNumber; }

			else
			pushTokenIntoCell(currentCell, chr, tokInfo);

			continue;
		}

getNumber:
		if (isdigit(chr) || isNumber) {
			tokInfo.num = getNumberAsToken(us->src, &k);
			pushTokenIntoCell(currentCell, TokenIsNumber, tokInfo);
			continue;
		}

		setCell2Error(currentCell, ErrCellUnknown);
	}
}

static void pushTokenIntoCell (struct Cell *const cc, enum TokenKind kind, union As as)
{
	if (cc->nthT == FAMILY_SIZE) {
		setCell2Error(cc, ErrCellOverflow);
		return;
	}
	
	struct Token* this = &cc->family[cc->nthT++];
	this->kind = kind;
	memcpy(&this->as, &as, sizeof(as));
}

static void setCell2Error (struct Cell *const cc, u8 which)
{
	/* Errors
	 * 1. So many tokens for that cell (family so big).
	 * 2. Unknown character found in the cell (dont know how to interpret it).
	 * 3. Valid tokens but meaningless (i.e. + - 3 *)
	 */
	static const char *const errors[] = {
		"!overflow",
		"!unknown",
		"!malformed"
	};

	cc->as.txt.s = errors[which];
	cc->kind = CellIsError;
}

static void operateCell (struct Cell *const cc)
{
	if (cc->nthT == 0) return;

	const enum TokenKind header = cc->family[0].kind;

	switch (cc->family[0].kind) {
		case TokenIsString:
		case TokenIsNumber:
			memcpy(&cc->as, &cc->family[0].as, sizeof(cc->as));
			cc->kind = (enum CellKind) header;
			break;
	}
}

static void getStringAsToken (const char *const src, size_t *k, union As *info)
{
	info->txt.len = 0;
	info->txt.s = src + *k + 1;

	do {
		*k += 1;
		info->txt.len++;
	} while (src[*k] != '"');

	info->txt.len--;
}

static void getReferenceAsToken (const char *const src, size_t *k, union As *info)
{
	u16 col = 0, row = 0;

	*k += 1;
	if (isalpha(src[*k])) col = (u16) (src[*k] - 'a');

	*k += 1;
	if (isalpha(src[*k - 1]) && isalpha(src[*k])) col = ((u16) (src[*k - 1] - 'a' + 1) * 26) + ((u16) (src[*k] - 'a'));

	*k += 1;
	if (!isdigit(src[*k])) goto setPosition;

	/* If the row provied is way too big it will become zero therefore
	 * it will not produce error. */
	char *ends;
	row = (u16) strtold(src + *k, &ends);
	*k += (size_t) (ends - (src + *k)) - 1;

setPosition:
	info->ref.col = col;
	info->ref.row = row;
}

static long double getNumberAsToken (const char *const src, size_t *k)
{
	/* In case the number is too big it will be turned into inf
	 * so no problem either. */
	char *ends;
	long double numero = strtold(src + *k, &ends);

	*k += (size_t) (ends - (src + *k)) - 1;
	return numero;
}



