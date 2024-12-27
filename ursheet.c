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
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <assert.h>

// TODO: replace all asserts checkers

struct UrSh
{
	struct	{ unsigned short rows, cols; } sz;
	size_t	length;
	char	*filename, *src;
};

static void readContents (struct UrSh *const, FILE*);
static void getTableDimensions (const char*, unsigned short*, unsigned short*);

int main (int argc, char **argv)
{
	if (argc != 2)
		errx(EXIT_SUCCESS, "usage: %s <sheet>\nThat simple ;)", *argv);

	struct UrSh us = {0};

	us.filename = argv[1];

	readContents(&us, fopen(us.filename, "r"));
	getTableDimensions(us.src, &us.sz.rows, &us.sz.cols);

	printf("%d %d", us.sz.rows, us.sz.cols);

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
	assert(us->src);

	const size_t BRead = fread(us->src, 1, us->length, file);

	if (BRead != us->length)
		errx(EXIT_FAILURE, "cannot read whole file (%ld/%ld B)", us->length, BRead);	

	fclose(file);
}

static void getTableDimensions (const char *src, unsigned short *row, unsigned short *col)
{
	unsigned short mcol = 0;

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
