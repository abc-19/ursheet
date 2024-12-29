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
#include <getopt.h>
#include <assert.h>

#include "incl.h"
#include "solver.h"

static void parseArgs (int, char**, char**, u16*);
static void readContents (struct UrSh *const, FILE*);

static void getTableDimensions (const char*, u16*, u16*);
static void lexTable (struct UrSh *const);

static void getStringAsToken (const char *const, size_t*, union As*);
static i16 getReferenceAsToken (const char *const, size_t*, const u16, const u16);

static long double getNumberAsToken (const char *const, size_t*);
static void pushTokenIntoCell (struct Cell *const, enum TokenKind, union As);

static void setCell2Error (struct Cell *const, u8);
static void operateCell (struct Cell *const, const struct Cell *const, const u16);

static void solveCopying (struct Cell*, struct Cell*);
static void adjustColWidth (const struct Cell *const, u16*, const u16);

static void printTable (struct Cell*, const u16*, const u16, const u16, const u16);

int main (int argc, char **argv)
{
	struct UrSh us = {0};
	parseArgs(argc, argv, &us.filename, &us.dp);

	readContents(&us, fopen(us.filename, "r"));
	getTableDimensions(us.src, &us.sz.rows, &us.sz.cols);

	if (us.sz.cols >= MAX_COLS)
		errx(EXIT_FAILURE, "Maximum number of columns reached which is %d\n", MAX_COLS);
	if (us.sz.rows >= MAX_ROWS)
		errx(EXIT_FAILURE, "Maximum number of rows reached which is %d\n", MAX_ROWS);

	us.grid = (struct Cell*) calloc(us.sz.rows * us.sz.cols, sizeof(struct Cell));
	assert(us.grid && "cannot allocate memory");

	us.sz.widths = (u16*) calloc(us.sz.cols, sizeof(u16));
	assert(us.sz.widths && "cannot allocate memory");

	lexTable(&us);
	printTable(&us.grid[0], us.sz.widths, us.sz.rows, us.sz.cols, us.dp);

	free(us.sz.widths);
	free(us.grid);
	free(us.src);
	return 0;
}

static void parseArgs (int argc, char **argv, char **filename, u16 *dp)
{
	*dp = 1;
	i32 op;

	while ((op = getopt(argc, argv, ":s:d:h")) != -1) {
		switch (op) {
			case 's': *filename = optarg; break;
			case 'd': *dp = atoi(optarg); break;
			case ':': errx(EXIT_FAILURE, "'-%c' expects an argument.", optopt);
			case '?': errx(EXIT_FAILURE, "'-%c' is not an option.", optopt);
			case 'h':
				errx(EXIT_SUCCESS, "uage: %s -s <sheet> -d <decimal-presicion>", *argv);
		}
	}
}

static void readContents (struct UrSh *const us, FILE *file)
{
	if (!file)
		err(EXIT_FAILURE, "fatal: %s is generating some issues", us->filename);

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
	const struct Cell *firstRow = &us->grid[us->sz.cols - 1];

	union As tokInfo;
	struct Cell *currentCell = &us->grid[0];

	u16 nrow = 0, ncol = 0;

	for (size_t k = 0; k < us->length; k++) {
		const char chr = us->src[k];
		Bool isNumber = False;

		switch (chr) {
			case ' ': case '\t': continue;

			case '+':
			case '/':
			case '*':
			case '^':
			case '(':
			case ')':
			case '=': {
				pushTokenIntoCell(currentCell, (enum TokenKind) chr, tokInfo);
				continue;
			}

			case '|': {
				operateCell(currentCell, firstRow, us->sz.cols);
				adjustColWidth(currentCell++, &us->sz.widths[ncol++], us->dp);
				continue;
			}

			case  10: {
				currentCell = &us->grid[++nrow * us->sz.cols];
				ncol = 0;
				continue;
			}

			case '"': {
				getStringAsToken(us->src, &k, &tokInfo);
				pushTokenIntoCell(currentCell, TokenIsString, tokInfo);
				continue;
			}

			case '@': {
				i16 at = getReferenceAsToken(us->src, &k, us->sz.rows, us->sz.cols);

				if (at == -1)
				{ setCell2Error(currentCell, ErrCellBounds); }

				else {
					tokInfo.ref = &us->grid[at];
					pushTokenIntoCell(currentCell, TokenIsReference, tokInfo);
				}

				continue;
			}
		}

		if (chr == '-') {
			if ((k + 1) < us->length && isdigit(us->src[k + 1]))
			{ isNumber = True; goto getNumber; }

			else
			{ pushTokenIntoCell(currentCell, chr, tokInfo); }

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

static i16 getReferenceAsToken (const char *const src, size_t *k, const u16 rows, const u16 cols)
{
	u16 col = 0, row = 0;

	*k += 1;
	if (isalpha(src[*k])) col = (u16) (tolower(src[*k]) - 'a');

	*k += 1;
	if (isalpha(src[*k - 1]) && isalpha(src[*k])) {
		col = ((u16) (tolower(src[*k - 1]) - 'a' + 1) * 26) + ((u16) (tolower(src[*k]) - 'a'));
		*k += 1;
	}

	if (!isdigit(src[*k])) goto setPosition;

	/* If the row provied is way too big it will become zero therefore
	 * it will not produce error. */
	char *ends;
	row = (u16) strtold(src + *k, &ends);
	*k += (size_t) (ends - (src + *k)) - 1;

setPosition:
	if (col >= cols || row >= rows)
	{ return -1; }

	return row * cols + col;
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
	 * 4. Reference outta bounds.
	 * 5. Trying to use a reference beyond the current one.
	 */
	static const char *const errors[] = {
		"!overflow",
		"!unknown",
		"!malformed",
		"!bounds",
		"!premature"
	};

	cc->as.txt.s = errors[which];
	cc->kind = CellIsError;
}

static void operateCell (struct Cell *const cc, const struct Cell *fRow, const u16 nCols)
{
	if (cc->nthT == 0 || cc->kind == CellIsError) return;

	const enum TokenKind header = cc->family[0].kind;

	switch (cc->family[0].kind) {
		case TokenIsString:
		case TokenIsNumber:
			memcpy(&cc->as, &cc->family[0].as, sizeof(cc->as));
			cc->kind = (enum CellKind) header;
			break;

		case TokenIsReference:
			solveCopying(cc, cc->family[0].as.ref);
			break;

		case TokenIsExpr:
			if (solverSolve(cc) != ErrCellNotErr)
			{ setCell2Error(cc, ErrCellMalformed); }
			break;

		case TokenIsClone:
			if (cc <= fRow)
			{ setCell2Error(cc, ErrCellPremature); }

			else if (!solverClone(cc, (struct Cell*) (cc - nCols), nCols))
			{ setCell2Error(cc, ErrCellMalformed); }
			break;

		default:
			setCell2Error(cc, ErrCellMalformed);
			break;
	}
}

static void solveCopying (struct Cell *cell, struct Cell *ref)
{
	if (ref >= cell)
	{ setCell2Error(cell, ErrCellPremature); return; }

	memcpy(cell, ref, sizeof(*cell));
}

static void adjustColWidth (const struct Cell *const justMade, u16 *currWidth, const u16 dp)
{
	u16 thisWidth = 0;

	switch (justMade->kind) {
		case CellIsError:
		{ thisWidth = (u16) strlen(justMade->as.txt.s); break; }

		case CellIsText:
		{ thisWidth = justMade->as.txt.len; break; }

		case CellIsNumber: {
			thisWidth += dp + 1;
			i64 asInt = (i64) justMade->as.num;
			while (asInt) { asInt /= 10; thisWidth++; }
			break;
		}
	}

	*currWidth = (*currWidth > thisWidth) ? *currWidth : thisWidth;
}

static void printTable (struct Cell *cell, const u16 *wds, const u16 rows, const u16 cols, const u16 dp)
{
	putchar(10);
	for (u16 row = 0; row < rows; row++) {
		for (u16 col = 0; col < cols; col++, cell++) {
			const u16 maxW = wds[col];

			switch (cell->kind) {
				case CellIsEmpty:	printf(" "); break;
				case CellIsNumber:	printf(" %-*.*Lf ", maxW, dp, cell->as.num); break;
				case CellIsError:	printf(" %-*s ", maxW, cell->as.txt.s); break;
				case CellIsText:	printf(" %-*.*s ", maxW, (int) cell->as.txt.len, cell->as.txt.s); break;
			}
		}
		putchar(10);
	}
	putchar(10);
}

