/*
 *	 __         __
 *	/  \.-"""-./  \		UrSheet - expression solver [H]
 *	\    -   -    /		abc19
 *	 |   o   o   |		Dec 28 2024
 *	 \  .-'''-.  /
 *	  '-\__Y__/-'
 *	     `---`
 */
#ifndef URSH_SOVER_H
#define URSH_SOVER_H
#include "incl.h"

enum CellErrs solverSolve (struct Cell*);
Bool solverClone (struct Cell*, struct Cell*, const u16);

#endif
