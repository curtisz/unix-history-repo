#ifndef lint
static char sccsid[] = "@(#)uusnap.c	5.3 (Berkeley) %G%";
#endif

/*
 *	Uusnap - displays a snapshot of the uucp system.
 *					RJKing WECo-MG6565 May 83
 */

#include "uucp.h"
#include <sys/types.h>
#include <sys/stat.h>
#ifdef	NDIR
#include "ndir.h"
#else
#include <sys/dir.h>
#endif

#ifndef	SYSBUF
char SYSBUF[BUFSIZ];
#endif

#define	NSYSTEM	100				/* max # of systems queued */

#define	CMDSLEN	5				/* Length of trailer */
#define	DATALEN	5				/* Length of trailer */
#define	XEQTLEN	5				/* Length of trailer */
#define	NUMCTRS	3				/* # file types to count */
#define	CMDTYPE	0				/* Index into scnt.cntr */
#define	DATTYPE	1				/* Index into scnt.cntr */
#define	XEQTYPE	2				/* Index into scnt.cntr */

extern	char *index(), *rindex(), *strcpy(), *strncpy();;
extern	long atol();
extern	time_t	time();

struct	scnt {					/* System count structure */
		char	name[16];		/* Name of system */
		short	cntr[NUMCTRS];		/* Count */
		char	stst[32];		/* STST Message */
		time_t	locked;			/* If LCK..sys present */
		int	st_type;		/* STST Type */
		int	st_count;		/* STST Count */
		time_t	st_lastime;		/* STST Last time tried */
		time_t	st_retry;		/* STST Secs to retry */
	     };

int	sndx;					/* Number of systems */
struct	scnt	sys[NSYSTEM];			/* Systems queued */
int xqtisrunning = 0;

main()
{	register int i, j, nlen = 0;
	time_t	curtime, t;

	setbuf(stdout, SYSBUF);
	scandir(CMDSDIR, "C.", CMDSLEN, NULL, CMDTYPE);
	scandir(DATADIR, "D.", DATALEN, NULL, DATTYPE);
	scandir(XEQTDIR, "X.", XEQTLEN, 'X', XEQTYPE);
	getstst(SPOOL);
	time(&curtime);
	for(i=0; i<sndx; ++i)
		if((j = strlen(sys[i].name)) > nlen)
			nlen = j;
	for(i=0; i<sndx; ++i) {
		t = (sys[i].st_lastime +sys[i].st_retry) - curtime;

		/* decide if STST text is worth printing */
		if (-t < ONEDAY*2 && sys[i].st_type == SS_WRONGTIME) {
			sys[i].stst[0] = '\0';
			if (sys[i].cntr[0]+sys[i].cntr[1]+sys[i].cntr[2] == 0)
				continue;	/* ignore entire line */
		}

		printf("%-*.*s ", nlen, nlen, sys[i].name);
		if(sys[i].cntr[CMDTYPE])
			printf("%3.d Cmd%s ", sys[i].cntr[CMDTYPE],
				sys[i].cntr[CMDTYPE]>1?"s":" ");
		else
			printf("   ---   ");
		if(sys[i].cntr[DATTYPE])
			printf("%3.d Data ", sys[i].cntr[DATTYPE]);
		else
			printf("   ---   ");
		if(sys[i].cntr[XEQTYPE])
			printf("%3.d Xqt%s ", sys[i].cntr[XEQTYPE],
				sys[i].cntr[XEQTYPE]>1?"s":" ");
		else
			printf("   ---   ");
		if(*sys[i].stst == NULL || sys[i].locked > sys[i].st_lastime) {
			if(sys[i].locked)
				printf("LOCKED\n");
			else
				printf("\n");
			continue;
		}
		printf("%s  ", sys[i].stst);
		/* decide if STST info is worth pursuing */
		if (-t < ONEDAY*2 && (sys[i].st_count == 0
		  || sys[i].st_type == SS_WRONGTIME
		  || (sys[i].st_type == SS_INPROGRESS && sys[i].locked))) {
			printf("\n");
			continue;
		}
		t = (sys[i].st_lastime +sys[i].st_retry) - curtime;
		if (-t < ONEDAY*2 && sys[i].st_type != SS_FAIL)
			t = 0;

		if (sys[i].st_count > MAXRECALLS)
			printf("at MAX RECALLS");
		else if (-t >= ONEDAY*2)
			printf("%ld days ago", (long)-t/ONEDAY);
		else if (t <= 0)
			printf("Retry time reached");
		else if (t < 60)
			printf("Retry time %ld sec%s", (long)(t%60),
					(t%60)!=1? "s": "");
		else
			printf("Retry time %ld min%s", (long)(t/60),
				(t/60)!=1? "s": "");
		if(sys[i].st_count > 1)
			printf(" Count: %d\n", sys[i].st_count);
		else
			printf("\n");
	}
	if (xqtisrunning)
		printf("\nUuxqt is running\n");
	exit(0);
}

scandir(dnam, prfx, flen, fchr, type)
char *dnam, *prfx, fchr;
{
	register int i, plen;
	char	fnam[MAXNAMLEN+1];
	register struct direct *dentp;
	register DIR *dirp;

	plen = strlen(prfx);
	if(chdir(dnam) < 0) {
		perror(dnam);
		exit(1);
	}
	if ((dirp = opendir(".")) == NULL) {
		perror(dnam);
		exit(1);
	}
	while((dentp = readdir(dirp)) != NULL) {
		if(*dentp->d_name == '.')
			continue;
		if(strncmp(dentp->d_name, prfx, plen) != SAME) {
			fprintf(stderr, "strange file (%s) in %s\n",
				dentp->d_name, dnam);
			continue;
		}
		strcpy(fnam, &dentp->d_name[plen]);
		i = strlen(fnam);
		if(flen > 0)
			fnam[i -flen] = NULL;
		else
			for(; i>0; --i) {
				if(fnam[i] == fchr) {
					fnam[i] = NULL;
					break;
				}
			}
		for(i=0; i<sndx; ++i) {
			if(strcmp(fnam, sys[i].name) == SAME) {
				++sys[i].cntr[type];
				break;
			}
		}
		if(i == sndx) {
			strcpy(sys[i].name, fnam);
			++sys[i].cntr[type];
			++sndx;
		}
	}
	closedir(dirp);
}

getstst(sdir)
char *sdir;
{
	register int i, csys;
	register char *tp;
	char	fnam[MAXNAMLEN+1], buff[128];
	register struct	direct *dentp;
	register DIR *dirp;
	register FILE *st;
	struct stat stbuf;

	if (chdir(sdir) < 0) {
		perror(sdir);
		exit(1);
	}
	if ((dirp = opendir(".")) == NULL) {
		perror(sdir);
		exit(1);
	}
	while ((dentp = readdir(dirp)) != NULL) {
		if (strcmp(dentp->d_name, X_LOCK) == SAME) {
			xqtisrunning++;
			continue;
		}
		if(strncmp(dentp->d_name, "LCK..", 5) == SAME) {
			if(strncmp(&dentp->d_name[5], "tty", 3) == SAME ||
			   strncmp(&dentp->d_name[5], "cul", 3) == SAME)
				continue;
			strcpy(fnam, dentp->d_name);
			for(csys=0; csys<sndx; ++csys) {
				if(strcmp(&fnam[5], sys[csys].name) == SAME)
					break;
			}
			if(csys == sndx) {
				strcpy(sys[csys].name, &fnam[5]);
				++sndx;
			}
			if (stat(fnam, &stbuf) < 0)
				sys[csys].locked = 1;
			else
				sys[csys].locked = stbuf.st_mtime;
			continue;
		}
	}
	closedir(dirp);
	if (chdir("STST") < 0) {
		perror("STST");
		exit(1);
	}
	if ((dirp = opendir(".")) == NULL) {
		perror("STST");
		exit(1);
	}
	while ((dentp = readdir(dirp)) != NULL) {
		if(*dentp->d_name == '.')
			continue;
		strcpy(fnam, dentp->d_name);
		for(csys=0; csys<sndx; ++csys) {
			if(strcmp(fnam, sys[csys].name) == SAME)
				break;
		}
		if(csys == sndx) {
			strcpy(sys[csys].name, fnam);
			++sndx;
		}
		if((st = fopen(fnam, "r")) == NULL) {
			strncpy(sys[csys].stst, "", sizeof(sys[csys].stst));
			continue;
		}
		strncpy(buff, "", sizeof(buff));
		fgets(buff, sizeof(buff), st);
		fclose(st);
		if(tp = rindex(buff, ' '))
			*tp = NULL;		/* drop system name */
		else
			continue;
		for(i=0, tp=buff;  i<4;  ++i, ++tp)
			if((tp = index(tp, ' ')) == NULL)
				break;
		if(i != 4)
			continue;
		strncpy(sys[csys].stst, tp, sizeof(sys[csys].stst));
		tp = buff;
		sys[csys].st_type = atoi(tp);
		tp = index(tp+1, ' ');
		sys[csys].st_count = atoi(tp+1);
		tp = index(tp+1, ' ');
		sys[csys].st_lastime = (time_t)atol(tp+1);
		tp = index(tp+1, ' ');
		sys[csys].st_retry = (time_t)atol(tp+1);
	}
}
/*
 * Return the ptr in sp at which the character c appears;
 * NULL if not found
 */

char *
index(sp, c)
register char *sp, c;
{
	do {
		if (*sp == c)
			return sp;
	} while (*sp++);
	return NULL;
}

/*
 * Return the ptr in sp at which the character c last
 * appears; NULL if not found
*/

char *
rindex(sp, c)
register char *sp, c;
{
	register char *r;

	r = NULL;
	do {
		if (*sp == c)
			r = sp;
	} while (*sp++);
	return r;
}
