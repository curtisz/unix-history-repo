/*-
 * Copyright (c) 2000-2013 Mark R V Murray
 * Copyright (c) 2004 Robert N. M. Watson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#if !defined(YARROW_RNG) && !defined(FORTUNA_RNG)
#define YARROW_RNG
#elif defined(YARROW_RNG) && defined(FORTUNA_RNG)
#error "Must define either YARROW_RNG or FORTUNA_RNG"
#endif

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/random.h>
#include <sys/selinfo.h>
#include <sys/sysctl.h>
#include <sys/uio.h>
#include <sys/unistd.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <dev/random/random_adaptors.h>
#include <dev/random/randomdev.h>
#include <dev/random/randomdev_soft.h>
#if defined(YARROW_RNG)
#include <dev/random/yarrow.h>
#endif
#if defined(FORTUNA_RNG)
#include <dev/random/fortuna.h>
#endif

#define RANDOM_FIFO_MAX	256	/* How many events to queue up */

static void random_kthread(void *);
static void random_harvest_internal(u_int64_t, const void *, u_int,
    u_int, u_int, enum esource);
static int randomdev_poll(int event, struct thread *td);
static int randomdev_block(int flag);
static void randomdev_flush_reseed(void);

#if defined(YARROW_RNG)
static struct random_adaptor random_context = {
	.ident = "Software, Yarrow",
	.init = randomdev_init,
	.deinit = randomdev_deinit,
	.block = randomdev_block,
	.read = random_yarrow_read,
	.write = randomdev_write,
	.poll = randomdev_poll,
	.reseed = randomdev_flush_reseed,
	.seeded = 0,
};
#define RANDOM_MODULE_NAME	yarrow
#define RANDOM_CSPRNG_NAME	"yarrow"
#endif

#if defined(FORTUNA_RNG)
static struct random_adaptor random_context = {
	.ident = "Software, Fortuna",
	.init = randomdev_init,
	.deinit = randomdev_deinit,
	.block = randomdev_block,
	.read = random_fortuna_read,
	.write = randomdev_write,
	.poll = randomdev_poll,
	.reseed = randomdev_flush_reseed,
	.seeded = 0,
};
#define RANDOM_MODULE_NAME	fortuna
#define RANDOM_CSPRNG_NAME	"fortuna"

#endif

MALLOC_DEFINE(M_ENTROPY, "entropy", "Entropy harvesting buffers for " RANDOM_CSPRNG_NAME);

/*
 * The harvest mutex protects the consistency of the entropy fifos and
 * empty fifo.
 */
struct mtx	harvest_mtx;

/* Lockable FIFO queue holding entropy buffers */
struct entropyfifo {
	int count;
	STAILQ_HEAD(harvestlist, harvest) head;
};

/* Empty entropy buffers */
static struct entropyfifo emptyfifo;

#define EMPTYBUFFERS	1024

/* Harvested entropy */
static struct entropyfifo harvestfifo[ENTROPYSOURCE];

/* <0 to end the kthread, 0 to let it run, 1 to flush the harvest queues */
static int random_kthread_control = 0;

static struct proc *random_kthread_proc;

/* List for the dynamic sysctls */
static struct sysctl_ctx_list random_clist;

/* ARGSUSED */
static int
random_check_boolean(SYSCTL_HANDLER_ARGS)
{
	if (oidp->oid_arg1 != NULL && *(u_int *)(oidp->oid_arg1) != 0)
		*(u_int *)(oidp->oid_arg1) = 1;
	return sysctl_handle_int(oidp, oidp->oid_arg1, oidp->oid_arg2, req);
}

void
randomdev_init(void)
{
	int error, i;
	struct harvest *np;
	struct sysctl_oid *random_sys_o, *random_sys_harvest_o;
	enum esource e;

#if defined(YARROW_RNG)
	random_yarrow_init_alg(&random_clist);
#endif
#if defined(FORTUNA_RNG)
	random_fortuna_init_alg(&random_clist);
#endif

	random_sys_o = SYSCTL_ADD_NODE(&random_clist,
	    SYSCTL_STATIC_CHILDREN(_kern_random),
	    OID_AUTO, "sys", CTLFLAG_RW, 0,
	    "Entropy Device Parameters");

	SYSCTL_ADD_PROC(&random_clist,
	    SYSCTL_CHILDREN(random_sys_o),
	    OID_AUTO, "seeded", CTLTYPE_INT | CTLFLAG_RW,
	    &random_context.seeded, 1, random_check_boolean, "I",
	    "Seeded State");

	random_sys_harvest_o = SYSCTL_ADD_NODE(&random_clist,
	    SYSCTL_CHILDREN(random_sys_o),
	    OID_AUTO, "harvest", CTLFLAG_RW, 0,
	    "Entropy Sources");

	SYSCTL_ADD_PROC(&random_clist,
	    SYSCTL_CHILDREN(random_sys_harvest_o),
	    OID_AUTO, "ethernet", CTLTYPE_INT | CTLFLAG_RW,
	    &harvest.ethernet, 1, random_check_boolean, "I",
	    "Harvest NIC entropy");
	SYSCTL_ADD_PROC(&random_clist,
	    SYSCTL_CHILDREN(random_sys_harvest_o),
	    OID_AUTO, "point_to_point", CTLTYPE_INT | CTLFLAG_RW,
	    &harvest.point_to_point, 1, random_check_boolean, "I",
	    "Harvest serial net entropy");
	SYSCTL_ADD_PROC(&random_clist,
	    SYSCTL_CHILDREN(random_sys_harvest_o),
	    OID_AUTO, "interrupt", CTLTYPE_INT | CTLFLAG_RW,
	    &harvest.interrupt, 0, random_check_boolean, "I",
	    "Harvest IRQ entropy");
	SYSCTL_ADD_PROC(&random_clist,
	    SYSCTL_CHILDREN(random_sys_harvest_o),
	    OID_AUTO, "swi", CTLTYPE_INT | CTLFLAG_RW,
	    &harvest.swi, 0, random_check_boolean, "I",
	    "Harvest SWI entropy");

	/* Initialise the harvest fifos */
	STAILQ_INIT(&emptyfifo.head);
	emptyfifo.count = 0;
	for (i = 0; i < EMPTYBUFFERS; i++) {
		np = malloc(sizeof(struct harvest), M_ENTROPY, M_WAITOK);
		STAILQ_INSERT_TAIL(&emptyfifo.head, np, next);
	}
	for (e = RANDOM_START; e < ENTROPYSOURCE; e++) {
		STAILQ_INIT(&harvestfifo[e].head);
		harvestfifo[e].count = 0;
	}

	mtx_init(&harvest_mtx, "entropy harvest mutex", NULL, MTX_SPIN);

	/* Start the hash/reseed thread */
	error = kproc_create(random_kthread, NULL,
	    &random_kthread_proc, RFHIGHPID, 0, RANDOM_CSPRNG_NAME);
	if (error != 0)
		panic("Cannot create entropy maintenance thread.");

	/* Register the randomness harvesting routine */
	randomdev_init_harvester(random_harvest_internal,
	    random_context.read);
}

void
randomdev_deinit(void)
{
	struct harvest *np;
	enum esource e;

	/* Deregister the randomness harvesting routine */
	randomdev_deinit_harvester();

	/*
	 * Command the hash/reseed thread to end and wait for it to finish
	 */
	random_kthread_control = -1;
	tsleep((void *)&random_kthread_control, 0, "term", 0);

	/* Destroy the harvest fifos */
	while (!STAILQ_EMPTY(&emptyfifo.head)) {
		np = STAILQ_FIRST(&emptyfifo.head);
		STAILQ_REMOVE_HEAD(&emptyfifo.head, next);
		free(np, M_ENTROPY);
	}
	for (e = RANDOM_START; e < ENTROPYSOURCE; e++) {
		while (!STAILQ_EMPTY(&harvestfifo[e].head)) {
			np = STAILQ_FIRST(&harvestfifo[e].head);
			STAILQ_REMOVE_HEAD(&harvestfifo[e].head, next);
			free(np, M_ENTROPY);
		}
	}

#if defined(YARROW_RNG)
	random_yarrow_deinit_alg();
#endif
#if defined(FORTUNA_RNG)
	random_fortuna_deinit_alg();
#endif

	mtx_destroy(&harvest_mtx);

	sysctl_ctx_free(&random_clist);
}

/* ARGSUSED */
static void
random_kthread(void *arg __unused)
{
	STAILQ_HEAD(, harvest) local_queue;
	struct harvest *event = NULL;
	int local_count;
	enum esource source;

	STAILQ_INIT(&local_queue);
	local_count = 0;

	/* Process until told to stop */
	mtx_lock_spin(&harvest_mtx);
	for (; random_kthread_control >= 0;) {

		/* Cycle through all the entropy sources */
		for (source = RANDOM_START; source < ENTROPYSOURCE; source++) {
			/*
			 * Drain entropy source records into a thread-local
			 * queue for processing while not holding the mutex.
			 */
			STAILQ_CONCAT(&local_queue, &harvestfifo[source].head);
			local_count += harvestfifo[source].count;
			harvestfifo[source].count = 0;
		}

		/*
		 * Deal with events, if any, dropping the mutex as we process
		 * each event.  Then push the events back into the empty
		 * fifo.
		 */
		if (!STAILQ_EMPTY(&local_queue)) {
			mtx_unlock_spin(&harvest_mtx);
			STAILQ_FOREACH(event, &local_queue, next)
				random_process_event(event);
			mtx_lock_spin(&harvest_mtx);
			STAILQ_CONCAT(&emptyfifo.head, &local_queue);
			emptyfifo.count += local_count;
			local_count = 0;
		}

		KASSERT(local_count == 0, ("random_kthread: local_count %d",
		    local_count));

		/*
		 * If a queue flush was commanded, it has now happened,
		 * and we can mark this by resetting the command.
		 */
		if (random_kthread_control == 1)
			random_kthread_control = 0;

		/* Work done, so don't belabour the issue */
		msleep_spin_sbt(&random_kthread_control, &harvest_mtx,
		    "-", SBT_1S / 10, 0, C_PREL(1));

	}
	mtx_unlock_spin(&harvest_mtx);

	random_set_wakeup_exit(&random_kthread_control);
	/* NOTREACHED */
}

/* Entropy harvesting routine. This is supposed to be fast; do
 * not do anything slow in here!
 */
static void
random_harvest_internal(u_int64_t somecounter, const void *entropy,
    u_int count, u_int bits, u_int frac, enum esource origin)
{
	struct harvest *event;

	KASSERT(origin >= RANDOM_START && origin <= RANDOM_PURE,
	    ("random_harvest_internal: origin %d invalid\n", origin));

	/* Lockless read to avoid lock operations if fifo is full. */
	if (harvestfifo[origin].count >= RANDOM_FIFO_MAX)
		return;

	mtx_lock_spin(&harvest_mtx);

	/*
	 * Don't make the harvest queues too big - help to thwart low-grade
	 * entropy swamping
	 */
	if (harvestfifo[origin].count < RANDOM_FIFO_MAX) {
		event = STAILQ_FIRST(&emptyfifo.head);
		if (event != NULL) {
			count = MIN(count, HARVESTSIZE);
			/* Add the harvested data to the fifo */
			STAILQ_REMOVE_HEAD(&emptyfifo.head, next);
			harvestfifo[origin].count++;
			event->somecounter = somecounter;
			event->size = count;
			event->bits = bits;
			event->frac = frac;
			event->source = origin;

			memcpy(event->entropy, entropy, count);

#if 1
			{
			int i;
			printf("Harvest:%16jX ", event->somecounter);
			for (i = 0; i < event->size; i++)
				printf("%02X", event->entropy[i]);
			for (; i < 16; i++)
				printf("  ");
			printf(" %2d 0x%2X.%03X %02X\n", event->size, event->bits, event->frac, event->source);
			}
#endif

			STAILQ_INSERT_TAIL(&harvestfifo[origin].head,
			    event, next);
		}
	}
	mtx_unlock_spin(&harvest_mtx);
}

void
randomdev_write(void *buf, int count)
{
	int i;
	u_int chunk;

	/*
	 * Break the input up into HARVESTSIZE chunks. The writer has too
	 * much control here, so "estimate" the entropy as zero.
	 */
	for (i = 0; i < count; i += HARVESTSIZE) {
		chunk = HARVESTSIZE;
		if (i + chunk >= count)
			chunk = (u_int)(count - i);
		random_harvest_internal(get_cyclecount(), (char *)buf + i,
		    chunk, 0, 0, RANDOM_WRITE);
	}
}

void
randomdev_unblock(void)
{
	if (!random_context.seeded) {
		random_context.seeded = 1;
		selwakeuppri(&random_context.rsel, PUSER);
		wakeup(&random_context);
	}
	(void)atomic_cmpset_int(&arc4rand_iniseed_state, ARC4_ENTR_NONE,
	    ARC4_ENTR_HAVE);
}

static int
randomdev_poll(int events, struct thread *td)
{
	int revents = 0;
	mtx_lock(&random_reseed_mtx);

	if (random_context.seeded)
		revents = events & (POLLIN | POLLRDNORM);
	else
		selrecord(td, &random_context.rsel);

	mtx_unlock(&random_reseed_mtx);
	return revents;
}

static int
randomdev_block(int flag)
{
	int error = 0;

	mtx_lock(&random_reseed_mtx);

	/* Blocking logic */
	while (!random_context.seeded && !error) {
		if (flag & O_NONBLOCK)
			error = EWOULDBLOCK;
		else {
			printf("Entropy device is blocking.\n");
			error = msleep(&random_context,
			    &random_reseed_mtx,
			    PUSER | PCATCH, "block", 0);
		}
	}
	mtx_unlock(&random_reseed_mtx);

	return error;
}

/* Helper routine to perform explicit reseeds */
static void
randomdev_flush_reseed(void)
{
	/* Command a entropy queue flush and wait for it to finish */
	random_kthread_control = 1;
	while (random_kthread_control)
		pause("-", hz / 10);

#if defined(YARROW_RNG)
	random_yarrow_reseed();
#endif
#if defined(FORTUNA_RNG)
	random_fortuna_reseed();
#endif
}

static int
randomdev_modevent(module_t mod, int type, void *unused)
{

	switch (type) {
	case MOD_LOAD:
		random_adaptor_register(RANDOM_CSPRNG_NAME, &random_context);
		/*
		 * For statically built kernels that contain both device
		 * random and options PADLOCK_RNG/RDRAND_RNG/etc..,
		 * this event handler will do nothing, since the random
		 * driver-specific handlers are loaded after these HW
		 * consumers, and hence hasn't yet registered for this event.
		 *
		 * In case where both the random driver and RNG's are built
		 * as seperate modules, random.ko is loaded prior to *_rng.ko's
		 * (by dependency). This event handler is there to delay
		 * creation of /dev/{u,}random and attachment of this *_rng.ko.
		 */
		EVENTHANDLER_INVOKE(random_adaptor_attach, &random_context);
		return (0);
	}

	return (EINVAL);
}

RANDOM_ADAPTOR_MODULE(RANDOM_MODULE_NAME, randomdev_modevent, 1);
