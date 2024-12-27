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
#include <stdio.h>
#include <getopt.h>
#include <assert.h>

#define	u8	unsigned char
#define	u16	unsigned short
#define	u32	unsigned int
#define	u64	unsigned long

#define	i8	signed char
#define	i16	signed short
#define	i32	signed int
#define	i64	signed long

#define	FAMILY_SIZE	64

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
	CellIsNumber	= 2,
	CellIsText		= 3
};

enum CellErrs
{
	ErrCellOverflow	= 0,
};

struct Cell;

union As
{
	const long double	num;
	const char			*txt;
	const struct Cell	*ref;
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
static void pushTokenIntoCell (struct Cell *const, enum TokenKind);

static void setCell2Error (struct Cell *const, u8);

int main (int argc, char **argv)
{
	if (argc != 2)
		errx(EXIT_SUCCESS, "usage: %s <sheet>\nThat simple ;)", *argv);

	struct UrSh us = {0};
	us.filename = argv[1];

	readContents(&us, fopen(us.filename, "r"));
	getTableDimensions(us.src, &us.sz.rows, &us.sz.cols);

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
	struct Cell *currentCell = &us->grid[0];

	for (size_t k = 0; k < us->length; k++) {
		const char chr = us->src[k];

		switch (chr) {
			case '+':
			case '/':
			case '*':
			case '^':
			case '=': pushTokenIntoCell(currentCell, chr); break;

			case 10 : break;
			case '|': break;
			case '"': break;
			case '@': break;
		}
	}

	printf("%s\n", currentCell->as.txt);
}

static void pushTokenIntoCell (struct Cell *const cc, enum TokenKind kind)
{
	if (cc->nthT == 0) {
		setCell2Error(cc, ErrCellOverflow);
		return;
	}
	
	struct Token* this = &cc->family[cc->nthT++];
	this->kind = kind;
}

static void setCell2Error (struct Cell *const cc, u8 which)
{
	static const char *const errors[] = {
		"!overflow"
	};

	cc->as.txt = errors[which];
	cc->kind = CellIsError;
}
