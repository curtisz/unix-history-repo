#ifndef lint
static	char *sccsid = "@(#)wwclose.c	3.4 83/08/26";
#endif

#include "ww.h"
#include <signal.h>

wwclose(w)
register struct ww *w;
{
	wwindex[w->ww_index] = 0;
	if (w->ww_state == WWS_HASPROC)
		(void) kill(w->ww_pid, SIGHUP);
	if (w->ww_haspty) {
		(void) close(w->ww_tty);
		(void) close(w->ww_pty);
	}
	wwfree((char **)w->ww_win);
	wwfree((char **)w->ww_cov);
	wwfree((char **)w->ww_buf);
	if (w->ww_fmap != 0)
		wwfree((char **)w->ww_fmap);
	free((char *)w->ww_nvis);
	free((char *)w);
}
