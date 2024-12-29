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
	u16 spos, qpos, firstOpPos;
	Bool refsUsed;
};

static enum CellErrs pushStack (struct Exprssn*, const long double, const enum TokenKind);
static enum CellErrs pushQueue (struct Exprssn*, const enum TokenKind);

static Bool gottaExchange (enum TokenKind, enum TokenKind);
static enum CellErrs rightParFound (struct Exprssn*);

static enum CellErrs mergeFamily (struct Exprssn*);
static enum CellErrs setUpExprInCell (struct Exprssn*, struct Cell*);

static enum CellErrs doMath (struct Cell*);
static long double dOp (long double, long double, enum TokenKind);

enum CellErrs solverSolve (struct Cell *cc)
{
	struct Exprssn ex = {
		.spos     = 0,
		.qpos     = PARITION,
		.refsUsed = False
	};

	enum CellErrs e = ErrCellNotErr;

	for (u16 k = 1; k < cc->nthT && e == ErrCellNotErr; k++) {
		struct Token t = cc->family[k];

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
				ex.refsUsed = True;
				// TODO
				break;

			case TokenIsNumber:
				e = pushStack(&ex, t.as.num, TokenIsNumber);
				break;

			default:
				e = ErrCellMalformed;
				break;
		}
	}

	if ((e != ErrCellNotErr) || (e = mergeFamily(&ex)) != ErrCellNotErr || (e = setUpExprInCell(&ex, cc)) != ErrCellNotErr)
		return e;

	for (u16 k = 0; k < cc->nthT; k++) {
		struct Token t = cc->family[k];
		if (t.kind == TokenIsNumber)
			printf("%Lf ", t.as.num);
		else
			printf("%c ", t.kind);
	}
	putchar(10);

	return doMath(cc);
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
		return rightParFound(ex);
	
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

static enum CellErrs rightParFound (struct Exprssn *ex)
{
	Bool thereWasPar = False;

	while (ex->qpos > PARITION) {
		const enum TokenKind kind = ex->family[--ex->qpos].kind;
		if (kind == TokenIsLpar) { thereWasPar = True; break; }

		pushStack(ex, 0, ex->family[ex->qpos].kind);
	}

	return thereWasPar ? ErrCellNotErr : ErrCellMalformed;
}

static enum CellErrs mergeFamily (struct Exprssn *ex)
{
	enum CellErrs e = ErrCellNotErr;
	ex->firstOpPos = ex->spos;

	while (ex->qpos > PARITION && e == ErrCellNotErr)
		e = pushStack(ex, 0, ex->family[--ex->qpos].kind);
	return e;
}

static enum CellErrs setUpExprInCell (struct Exprssn *ex, struct Cell *cc)
{
	if (ex->spos == 0)
	{ return ErrCellMalformed; }

	if (ex->refsUsed)
	{ cc->clonable = True; }

	memcpy(cc->family, ex->family, ex->spos * sizeof(*cc->family));

	cc->nthT = ex->spos;
	cc->opPos = ex->firstOpPos;

	const u16 nOpds = ex->firstOpPos, nOpts = ex->spos - ex->firstOpPos;
	return ((nOpds - nOpts) != 1) ?  ErrCellMalformed : ErrCellNotErr;
}

static enum CellErrs doMath (struct Cell *cc)
{
	const long double a = cc->family[0].as.num, b = cc->family[1].as.num;
	cc->as.num = dOp(a, b, cc->family[cc->opPos].kind);

	for (u16 k = 2, j = cc->opPos + 1; k < cc->opPos; k++)
		cc->as.num = dOp(cc->as.num, cc->family[k].as.num, cc->family[j++].kind);

	errx(0, "%Lf\n", cc->as.num);
}

static long double dOp (long double a, long double b, enum TokenKind o)
{
	switch (o) {
		case TokenIsAdd: return a + b;
		case TokenIsSub: return a - b;
		case TokenIsMul: return a * b;
		case TokenIsDiv: return a / b;
	}

	/*  ______________________________________
	*  < The program should never get here... >
	*   --------------------------------------
	*       \
	*        \
	*            oO)-.                       .-(Oo
	*           /__  _\                     /_  __\
	*           \  \(  |     ()~()         |  )/  /
	*            \__|\ |    (-___-)        | /|__/
	*            '  '--'    ==`-'==        '--'  '
	*/
	return 0;
}
