/*-
 * Copyright (c) 2002 JF Hay.  All rights reserved.
 * Copyright (c) 2001 M. Warner Losh.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
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
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/module.h>
#include <sys/tty.h>
#include <machine/bus.h>
#include <sys/timepps.h>

#include <dev/puc/puc_bus.h>

#include <dev/sio/siovar.h>
#include <dev/sio/sioreg.h>

static	int	sio_puc_attach(device_t dev);
static	int	sio_puc_probe(device_t dev);

static device_method_t sio_puc_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,		sio_puc_probe),
	DEVMETHOD(device_attach,	sio_puc_attach),
	DEVMETHOD(device_detach,	siodetach),

	{ 0, 0 }
};

static driver_t sio_puc_driver = {
	sio_driver_name,
	sio_puc_methods,
	0,
};

/*
 * Don't cut and paste this to other drivers.  It is a horrible kludge
 * which will fail to work and also be unnecessary in future versions.
 */
static void
sio_puc_kludge_unit(device_t dev)
{
	devclass_t	dc;
	int		err;
	int		start;
	int		unit;

	unit = 0;
	start = 0;
	while (resource_int_value("sio", unit, "port", &start) == 0 && 
	    start > 0)
		unit++;
	if (device_get_unit(dev) < unit) {
		dc = device_get_devclass(dev);
		while (devclass_get_device(dc, unit))
			unit++;
		device_printf(dev, "moving to sio%d\n", unit);
		err = device_set_unit(dev, unit);	/* EVIL DO NOT COPY */
		if (err)
			device_printf(dev, "error moving device %d\n", err);
	}
}

static int
sio_puc_attach(device_t dev)
{
	uintptr_t rclk;

	if (BUS_READ_IVAR(device_get_parent(dev), dev, PUC_IVAR_CLOCK,
	    &rclk) != 0)
		rclk = DEFAULT_RCLK;
	sio_puc_kludge_unit(dev);
	return (sioattach(dev, 0, rclk));
}

static int
sio_puc_probe(device_t dev)
{
	device_t parent;
	uintptr_t rclk, type;
	int error;

	parent = device_get_parent(dev);

	if (BUS_READ_IVAR(parent, dev, PUC_IVAR_TYPE, &type))
		return (ENXIO);
	if (type != PUC_TYPE_SERIAL)
		return (ENXIO);

	if (BUS_READ_IVAR(parent, dev, PUC_IVAR_CLOCK, &rclk))
		rclk = DEFAULT_RCLK;
#ifdef PC98
	SET_FLAG(dev, SET_IFTYPE(COM_IF_NS16550));
#endif
	error = sioprobe(dev, 0, rclk, 1);
	return ((error > 0) ? error : BUS_PROBE_LOW_PRIORITY);
}

DRIVER_MODULE(sio, puc, sio_puc_driver, sio_devclass, 0, 0);
