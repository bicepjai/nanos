/*  T R I M -- Trim trailing whitespace from file lines and/or untabify  */

/*
  Author:      F. da Cruz, The Kermit Project, Columbia University
  Email:       fdc@columbia.edu
  Portability: Should be portable to most UNIXes and maybe VMS.
  Written in 1999, updated in 2008 to add lowercasing.

  Please send any changes back to the author.

  Copyright (C) 1999, 2008
  Trustees of Columbia University in the City of New York

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>

#define MAX 32767

char buf[MAX];				/* I/O buffers */
char obuf[MAX];
char tmpname[256];			/* Filename buffers */
char tmp2[256];
FILE *fp, *op, *fopen();		/* Streams */
int n = 0;				/* File counter */
int lower = 0;				/* Convert to lowercase */
int warn = 0;				/* Warn-only flag */
int untab = 0;				/* Untabify flag */
int changed = 0;			/* Flag for file changed */
int stdo = 0;				/* Write to stdout instead of file */
struct stat statbuf;			/* For stat() */

/* NOTE: argument bundling doesn't work - easy to fix - do this one day */

char * hlptxt[] = {
    "Usage: trim [ -w -s -c -t -h ] [ file [ file [ file ... ] ] ]",
    " ",
    "Trims trailing whitespace from lines in given file(s), replacing each",
    "file with a trimmed version and renaming the original to *.untrimmed.",
    "If no files are specified on the command line, operates on stdin and",
    "writes to stdout.",
    " ",
    "  -c = also convert to lowercase",
    "  -t = also untabify (convert tabs to spaces)",
    "  -s = write to stdout instead of replacing source file",
    "  -w = warn only, don't change the files.",
    "  -h = print this message",
    " ",
    "-w lists both lines with trailing whitespace and lines that would be",
    "longer than 80 characters after tab expansion.  Tab settings are assumed",
    "to be every 8 spaces",
    "",
};

usage(x) int x; {			/* Usage function */
    int i;				/* Print usage message and exit */
    for (i = 0; *hlptxt[i]; i++)
      fprintf(stderr,"%s\n",hlptxt[i]);
    exit(x);
}

main(argc,argv) int argc; char *argv[]; { /* Main program */
    int i;				/* Declare local variables */
    int result;

    for (i = 1; i < argc; i++) {	/* For each argument... */
	changed = 0;
	if (n == 0 && *argv[i] == '-') { /* First do any options */
	    switch (*(argv[i]+1)) {
	      case 't':
		untab = 1;
		continue;
	      case 'w':
		warn = 1;
		continue;
	      case 'c':
		lower = 1;
		continue;
	      case 's':
		stdo = 1;
		continue;
	      case 'h':
		usage(0);
	      default:
		usage(1);
	    }
	    continue;
	}
	fprintf(stderr,"%3d. %s...",n,argv[i]);	/* Got a file */
	if (stat(argv[i],&statbuf) < 0) {
	    printf("(skipped - stat error)\n");
	    continue;
	}
	if (!S_ISREG(statbuf.st_mode)) {
	    fprintf(stderr,"(skipped - not regular file)\n");
	    continue;
	}
	fp = fopen(argv[i], "r");	/* Try to open it */
	if (fp == NULL) {		/* Check for errors */
	    perror(argv[i]);
	    continue;
	}
	n++;
	result = process();		/* "Process" the file */
	if (fclose(fp) == EOF) {	/* Close the file */
	    perror(argv[i]);
	    continue;
	}
	if (result < 0) continue;
	if (!warn && changed) {
	    sprintf(tmp2,"%s.untrimmed",argv[i]);
	    if (rename(argv[i],tmp2) < 0) {
		perror("rename");
		exit(1);
	    }
	    if (rename(tmpname,argv[i]) < 0) {
		perror("rename");
		exit(1);
	    } else
	      fprintf(stderr,"(replaced OK)\n");
	} else
	  if (result == 0)
	    fprintf(stderr,"(OK)\n");
    }
    if (n == 0) {			/* No files given - use stdio */
	fp = stdin;
	result = process();
    }
    exit(result == 0 ? 0 : 1);
}

int
process() {				/* Process the file */
    int i, j, k, x, flag, bads = 0;
    char * p;
    long line = 0L;
    if (!warn) {
	sprintf(tmpname,"...tmp%03d",n);
	if (fp == stdin || stdo)
	  op = stdout;
	else
	  op = fopen(tmpname,"w");
	if (op == NULL) { perror("fopen tmp file"); exit(1); }
    }
/*
  Note: this used to be "while (fgets(...)) ..." but, despite what the
  man page says, this terminates early in Solaris 2.5.1 (at least when
  built with Sun CC).  For some reason, changing the loop exit condition
  as you see it below stopped the early termination, even though the
  feof() call does not catch the eof, so it's the test on the fgets()
  return status that breaks the loop -- which is exactly what didn't work
  when it was the while-condition.
*/
    while (1) {
	if (feof(fp))
	  break;
	if (!fgets(buf, MAX, fp))
	  break;
	line++;
	flag = 0;
	x = strlen(buf) - 1;
	while (buf[x] == '\n' || buf[x] == '\r') /* Trim trailing whitespace */
	  buf[x--] = 0;
	while (buf[x] == ' ' || buf[x] == '\t') {
	    changed++;
	    flag = 1;
	    if (warn) break;
	    buf[x--] = 0;
	}
	if (lower) {
	    int i = 0;
	    for (i = 0; i < x; i++) {
		if (isupper(buf[i])) {
		    buf[i] = tolower(buf[i]);
		    changed++;
		}
	    }
	}
	if (warn) {
	    if (flag) {
		bads++;
		if (bads == 1) printf("\n");
		fprintf(stderr,"TRAILING: %4ld. [%s]\n",line,buf);
	    }
	}
	p = buf;
	if (warn || untab) {		/* Expand tabs / check line length */
	    int z;
	    p = obuf;
	    x = strlen(buf);
	    for (i = 0, k = 0; k < x; k++) {
		if (buf[k] != '\t') {
		    if (i >= MAX) {
			printf("Overflow: %d\n", i);
			exit(1);
		    }
		    obuf[i++] = buf[k];
		    continue;
		}
		changed++;
		z = 8 - i%8;
		if (z == 0) z = 8;
		for (j = 0; j < z && i < MAX; j++)
		  obuf[i++] = ' ';
	    }
	    obuf[i] = 0;
	    if (i > 79) {
		bads++;
		if (bads == 1) printf("\n");
		if (warn) {
		    obuf[58] = '.';
		    obuf[59] = '.';
		    obuf[60] = '.';
		    obuf[61] = 0;
		}
		fprintf(stderr,"LONGLINE: %4ld. [%s]\n",line,obuf);
	    }
	}
	if (!warn)
	  fprintf(op,"%s\n",p);
    }
    if (!warn && op != stdout)
      fclose(op);
    return(bads);
}
