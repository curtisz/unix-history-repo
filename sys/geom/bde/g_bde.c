/*-
 * Copyright (c) 2002 Poul-Henning Kamp
 * Copyright (c) 2002 Networks Associates Technology, Inc.
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by Poul-Henning Kamp
 * and NAI Labs, the Security Research Division of Network Associates, Inc.
 * under DARPA/SPAWAR contract N66001-01-C-8035 ("CBOSS"), as part of the
 * DARPA CHATS research program.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 *
 */

#include <sys/param.h>
#include <sys/stdint.h>
#include <sys/bio.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/malloc.h>
#include <geom/geom.h>
#include <geom/bde/g_bde.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kthread.h>

#define BDE_CLASS_NAME "BDE"

static void
g_bde_start(struct bio *bp)
{
	struct g_geom *gp;
	struct g_consumer *cp;
	struct g_bde_softc *sc;

	gp = bp->bio_to->geom;
	cp = LIST_FIRST(&gp->consumer);
	sc = gp->softc;
	switch (bp->bio_cmd) {
	case BIO_DELETE:
	case BIO_READ:
	case BIO_WRITE:
		g_bde_start1(bp);
		break;
	case BIO_GETATTR:
	case BIO_SETATTR:
		if (g_handleattr_off_t(bp, "GEOM::mediasize", sc->mediasize))
			return;
		if (g_handleattr_int(bp, "GEOM::sectorsize", sc->sectorsize))
			return;
		g_io_deliver(bp, EOPNOTSUPP);
		break;
	default:
		g_io_deliver(bp, EOPNOTSUPP);
		return;
	}
	return;
}

static void
g_bde_orphan(struct g_consumer *cp)
{
	struct g_geom *gp;
	struct g_provider *pp;
	struct g_bde_softc *sc;
	int error;

	g_trace(G_T_TOPOLOGY, "g_bde_orphan(%p/%s)", cp, cp->provider->name);
	g_topology_assert();
	KASSERT(cp->provider->error != 0,
		("g_bde_orphan with error == 0"));

	gp = cp->geom;
	sc = gp->softc;
	gp->flags |= G_GEOM_WITHER;
	error = cp->provider->error;
	LIST_FOREACH(pp, &gp->provider, provider)
		g_orphan_provider(pp, error);
	bzero(sc, sizeof(struct g_bde_softc));	/* destroy evidence */
	return;
}

static int
g_bde_access(struct g_provider *pp, int dr, int dw, int de)
{
	struct g_geom *gp;
	struct g_consumer *cp;

	gp = pp->geom;
	cp = LIST_FIRST(&gp->consumer);
	if (cp->acr == 0 && cp->acw == 0 && cp->ace == 0) {
		de++;
		dr++;
	}
	/* ... and let go of it on last close */
	if ((cp->acr + dr) == 0 && (cp->acw + dw) == 0 && (cp->ace + de) == 1) {
		de--;
		dr--;
	}
	return (g_access_rel(cp, dr, dw, de));
}

static int
g_bde_create(struct g_createargs *ga)
{
	struct g_geom *gp;
	struct g_consumer *cp;
	struct g_provider *pp;
	struct g_bde_key *kp;
	int error;
	u_int sectorsize;
	off_t mediasize;
	struct g_bde_softc *sc;

	g_trace(G_T_TOPOLOGY, "g_bde_create(%d)", ga->flag);
	g_topology_assert();
	gp = NULL;
	if (ga->flag == 1) {
		/*
		 * Orderly dettachment.
		 */
		if (ga->geom != NULL) {
			gp = ga->geom;
		} else if (ga->provider != NULL) {
			if (ga->provider->geom->class == ga->class) {
				gp = ga->provider->geom;
			} else {
				LIST_FOREACH(cp, &ga->provider->consumers,
				    consumers) {
					if (cp->geom->class == ga->class) {
						gp = cp->geom;
						break;
					}
				}
			}
			if (gp == NULL)
				return (EINVAL);
		} else {
			return (EINVAL);
		}
		KASSERT(gp != NULL, ("NULL geom"));
		pp = LIST_FIRST(&gp->provider);
		KASSERT(pp != NULL, ("NULL provider"));
		if (pp->acr > 0 || pp->acw > 0 || pp->ace > 0)
			return (EBUSY);
		g_orphan_provider(pp, ENXIO);
		sc = gp->softc;
		cp = LIST_FIRST(&gp->consumer);
		KASSERT(cp != NULL, ("NULL consumer"));
		sc->dead = 1;
		wakeup(sc);
		error = g_access_rel(cp, -1, -1, -1);
		KASSERT(error == 0, ("error on close"));
		g_detach(cp);
		g_destroy_consumer(cp);
		g_topology_unlock();
		while (sc->dead != 2 && !LIST_EMPTY(&pp->consumers))
			tsleep(sc, PRIBIO, "g_bdedie", hz);
		g_topology_lock();
		g_destroy_provider(pp);
		mtx_destroy(&sc->worklist_mutex);
		bzero(&sc->key, sizeof sc->key);
		g_free(sc);
		g_destroy_geom(gp);
		return (0);
	}

	if (ga->flag != 0)
		return (EOPNOTSUPP);

	if (ga->provider == NULL)
		return (EINVAL);
	/*
	 * Attach
	 */
	gp = g_new_geomf(ga->class, "%s.bde", ga->provider->name);
	gp->start = g_bde_start;
	gp->orphan = g_bde_orphan;
	gp->access = g_bde_access;
	gp->spoiled = g_std_spoiled;
	cp = g_new_consumer(gp);
	g_attach(cp, ga->provider);
	error = g_access_rel(cp, 1, 1, 1);
	if (error) {
		g_detach(cp);
		g_destroy_consumer(cp);
		g_destroy_geom(gp);
		return (error);
	}
	g_topology_unlock();
	while (1) {
		error = g_getattr("GEOM::sectorsize", cp, &sectorsize);
		if (error)
			break;
		error = g_getattr("GEOM::mediasize", cp, &mediasize);
		if (error)
			break;
		sc = g_malloc(sizeof(struct g_bde_softc), M_WAITOK | M_ZERO);
		gp->softc = sc;
		sc->geom = gp;
		sc->consumer = cp;

		error = g_bde_decrypt_lock(sc, ga->ptr,
		    (u_char *)ga->ptr + 256, mediasize, sectorsize, NULL);
		bzero(sc->arc4_sbox, sizeof sc->arc4_sbox);
		if (error)
			break;
		kp = &sc->key;

		/* Initialize helper-fields */
		kp->keys_per_sector = kp->sectorsize / G_BDE_SKEYLEN;
		kp->zone_cont = kp->keys_per_sector * kp->sectorsize;
		kp->zone_width = kp->zone_cont + kp->sectorsize;
		kp->media_width = kp->sectorN - kp->sector0 -
		    G_BDE_MAXKEYS * kp->sectorsize;

		/* Our external parameters */
		sc->zone_cont = kp->zone_cont;
		sc->mediasize = g_bde_max_sector(kp);
		sc->sectorsize = kp->sectorsize;

		TAILQ_INIT(&sc->freelist);
		TAILQ_INIT(&sc->worklist);
		mtx_init(&sc->worklist_mutex, "g_bde_worklist", NULL, MTX_DEF);
		mtx_lock(&Giant);
		/* XXX: error check */
		kthread_create(g_bde_worker, gp, &sc->thread, 0, 0,
			"g_bde %s", gp->name);
		mtx_unlock(&Giant);
		g_topology_lock();
		pp = g_new_providerf(gp, gp->name);
		pp->mediasize = sc->mediasize;
		g_error_provider(pp, 0);
		g_topology_unlock();
		break;
	}
	g_topology_lock();
	if (error == 0) {
		ga->geom = gp;
		return (0);
	} else {
		g_access_rel(cp, -1, -1, -1);
	}
	g_detach(cp);
	g_destroy_consumer(cp);
	if (gp->softc != NULL)
		g_free(gp->softc);
	g_destroy_geom(gp);
	return (error);
}

static struct g_class g_bde_class	= {
	BDE_CLASS_NAME,
	NULL,
	g_bde_create,
	G_CLASS_INITIALIZER
};

DECLARE_GEOM_CLASS(g_bde_class, g_bde);
