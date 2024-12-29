/*
 *	 __         __
 *	/  \.-"""-./  \		UrSheet - expression solver.
 *	\    -   -    /		abc19
 *	 |   o   o   |		Dec 28 2024
 *	 \  .-'''-.  /
 *	  '-\__Y__/-'
 *	     `---`
 */
#include "solver.h" 

#define	PARITION	FAMILY_SIZE / 2

struct Exprssn
{
	struct Token family[FAMILY_SIZE];
	u16 spos, qpos;
};

static enum CellErrs pushStack (struct Exprssn*, const long double, const enum TokenKind);
static enum CellErrs pushQueue (struct Exprssn*, const enum TokenKind);

static Bool gottaExchange (enum TokenKind, enum TokenKind);
static enum CellErrs mergeFamily (struct Exprssn*);

void solverSolve (struct Cell *cell)
{
	struct Exprssn ex = {
		.spos	= 0,
		.qpos	= PARITION
	};

	enum CellErrs e = ErrCellNotErr;

	for (u16 k = 0; k < cell->nthT && e == ErrCellNotErr; k++) {
		struct Token t = cell->family[k];

		switch (t.kind) {
			case TokenIsLpar:
			case TokenIsRpar:
			case TokenIsAdd :
			case TokenIsSub :
			case TokenIsMul :
			case TokenIsDiv :
				e = pushQueue(&ex, t.kind);
				break;

			case TokenIsReference:
				break;

			case TokenIsNumber:
				e = pushStack(&ex, t.as.num, TokenIsNumber);
				break;

			default:
				e = ErrCellMalformed;
				break;
		}
	}

	if ((e != ErrCellNotErr) || (e = mergeFamily(&ex)) != ErrCellNotErr) {
		errx(0, "%d\n", e);
		return;
	}

	cell->kind = CellIsNumber;

	for (u16 k = 0; k < ex.spos; k++) {
		struct Token t = ex.family[k];
		if (t.kind == TokenIsNumber)
			printf("%Lf ", t.as.num);
		else
			printf("%c ", t.kind);
	}

	puts("");
}

static enum CellErrs pushStack (struct Exprssn *ex, const long double asNum, const enum TokenKind asOp)
{
	if (ex->spos == PARITION) return ErrCellOverflow;
	struct Token *this = &ex->family[ex->spos++];

	this->as.num = asNum;
	this->kind = asOp;

	return ErrCellNotErr;
}

static enum CellErrs pushQueue (struct Exprssn *ex, const enum TokenKind kind)
{
	if (ex->qpos == PARITION || kind == TokenIsLpar) {
		ex->family[ex->qpos++].kind = kind;
		return ErrCellNotErr;
	}

	if (kind == TokenIsRpar)
		return ErrCellNotErr;
	
	enum TokenKind top = ex->family[ex->qpos - 1].kind;
	while (gottaExchange(top, kind) && ex->qpos > PARITION) {
		pushStack(ex, 0, top);
		top = ex->family[--ex->qpos - 1].kind;
	}

	ex->family[ex->qpos++].kind = kind;
	return ErrCellNotErr;
}

static Bool gottaExchange (enum TokenKind top, enum TokenKind curr)
{
	static const u16 samePrec[2] = { '-' * '+', '*' * '/' };
	u16 prec = top * curr;

	if (prec == samePrec[0] || prec == samePrec[1]) return True;
	if ((curr == '-' || curr == '+') && (top == '*' || top == '/')) return True;

	return False;
}

static enum CellErrs mergeFamily (struct Exprssn *ex)
{
	enum CellErrs e = ErrCellNotErr;

	while (ex->qpos >= PARITION && e == ErrCellNotErr)
		e = pushStack(ex, 0, ex->family[--ex->qpos].kind);
	return e;
}

int main ()
{
	struct Token tokens[] = {
		{ .as.num = 1, .kind = TokenIsNumber },
		{ .as.num = 0, .kind = TokenIsDiv },
		{ .as.num = 2, .kind = TokenIsNumber },
		{ .as.num = 0, .kind = TokenIsMul },
		{ .as.num = 3, .kind = TokenIsNumber },
	};

	struct Cell c;
	memcpy(c.family, tokens, sizeof(struct Token) * 5);
	c.nthT = 5;

	solverSolve(&c);

	return 0;
}
