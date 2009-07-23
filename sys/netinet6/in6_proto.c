/*-
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$KAME: in6_proto.c,v 1.91 2001/05/27 13:28:35 itojun Exp $
 */

/*-
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)in_proto.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include "opt_inet.h"
#include "opt_inet6.h"
#include "opt_ipsec.h"
#include "opt_ipstealth.h"
#include "opt_carp.h"
#include "opt_sctp.h"
#include "opt_mpath.h"

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/proc.h>
#include <sys/protosw.h>
#include <sys/jail.h>
#include <sys/kernel.h>
#include <sys/domain.h>
#include <sys/mbuf.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/vimage.h>

#include <net/if.h>
#include <net/radix.h>
#include <net/route.h>
#ifdef RADIX_MPATH
#include <net/radix_mpath.h>
#endif

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip_encap.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet/icmp6.h>

#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet6/tcp6_var.h>
#include <netinet6/raw_ip6.h>
#include <netinet6/udp6_var.h>
#include <netinet6/pim6_var.h>
#include <netinet6/nd6.h>

#ifdef DEV_CARP
#include <netinet/ip_carp.h>
#endif

#ifdef SCTP
#include <netinet/in_pcb.h>
#include <netinet/sctp_pcb.h>
#include <netinet/sctp.h>
#include <netinet/sctp_var.h>
#include <netinet6/sctp6_var.h>
#endif /* SCTP */

#ifdef IPSEC
#include <netipsec/ipsec.h>
#include <netipsec/ipsec6.h>
#endif /* IPSEC */

#include <netinet6/ip6protosw.h>

/*
 * TCP/IP protocol family: IP6, ICMP6, UDP, TCP.
 */

extern	struct domain inet6domain;
static	struct pr_usrreqs nousrreqs;

#define PR_LISTEN	0
#define PR_ABRTACPTDIS	0

struct ip6protosw inet6sw[] = {
{
	.pr_type =		0,
	.pr_domain =		&inet6domain,
	.pr_protocol =		IPPROTO_IPV6,
	.pr_init =		ip6_init,
#ifdef VIMAGE
	.pr_destroy =		ip6_destroy,
#endif
	.pr_slowtimo =		frag6_slowtimo,
	.pr_drain =		frag6_drain,
	.pr_usrreqs =		&nousrreqs,
},
{
	.pr_type =		SOCK_DGRAM,
	.pr_domain =		&inet6domain,
	.pr_protocol =		IPPROTO_UDP,
	.pr_flags =		PR_ATOMIC|PR_ADDR,
	.pr_input =		udp6_input,
	.pr_ctlinput =		udp6_ctlinput,
	.pr_ctloutput =		ip6_ctloutput,
	.pr_usrreqs =		&udp6_usrreqs,
},
{
	.pr_type =		SOCK_STREAM,
	.pr_domain =		&inet6domain,
	.pr_protocol =		IPPROTO_TCP,
	.pr_flags =		PR_CONNREQUIRED|PR_WANTRCVD|PR_LISTEN,
	.pr_input =		tcp6_input,
	.pr_ctlinput =		tcp6_ctlinput,
	.pr_ctloutput =		tcp_ctloutput,
#ifndef INET	/* don't call initialization and timeout routines twice */
	.pr_init =		tcp_init,
	.pr_fasttimo =		tcp_fasttimo,
	.pr_slowtimo =		tcp_slowtimo,
#endif
	.pr_drain =		tcp_drain,
	.pr_usrreqs =		&tcp6_usrreqs,
},
#ifdef SCTP
{
	.pr_type =	SOCK_DGRAM,
	.pr_domain =	&inet6domain,
        .pr_protocol =	IPPROTO_SCTP,
        .pr_flags =	PR_WANTRCVD,
        .pr_input =	sctp6_input,
        .pr_ctlinput =  sctp6_ctlinput,
        .pr_ctloutput = sctp_ctloutput,
        .pr_drain =	sctp_drain,
        .pr_usrreqs =	&sctp6_usrreqs
},
{
	.pr_type =	SOCK_SEQPACKET,
	.pr_domain =	&inet6domain,
        .pr_protocol =	IPPROTO_SCTP,
        .pr_flags =	PR_WANTRCVD,
        .pr_input =	sctp6_input,
        .pr_ctlinput =  sctp6_ctlinput,
        .pr_ctloutput = sctp_ctloutput,
        .pr_drain =	sctp_drain,
        .pr_usrreqs =	&sctp6_usrreqs
},

{
	.pr_type =	SOCK_STREAM,
	.pr_domain =	&inet6domain,
        .pr_protocol =	IPPROTO_SCTP,
        .pr_flags =	PR_WANTRCVD,
        .pr_input =	sctp6_input,
        .pr_ctlinput =  sctp6_ctlinput,
        .pr_ctloutput = sctp_ctloutput,
        .pr_drain =	sctp_drain,
        .pr_usrreqs =	&sctp6_usrreqs
},
#endif /* SCTP */
{
	.pr_type =		SOCK_RAW,
	.pr_domain =		&inet6domain,
	.pr_protocol =		IPPROTO_RAW,
	.pr_flags =		PR_ATOMIC|PR_ADDR,
	.pr_input =		rip6_input,
	.pr_output =		rip6_output,
	.pr_ctlinput =		rip6_ctlinput,
	.pr_ctloutput =		rip6_ctloutput,
	.pr_usrreqs =		&rip6_usrreqs
},
{
	.pr_type =		SOCK_RAW,
	.pr_domain =		&inet6domain,
	.pr_protocol =		IPPROTO_ICMPV6,
	.pr_flags =		PR_ATOMIC|PR_ADDR|PR_LASTHDR,
	.pr_input =		icmp6_input,
	.pr_output =		rip6_output,
	.pr_ctlinput =		rip6_ctlinput,
	.pr_ctloutput =		rip6_ctloutput,
	.pr_init =		icmp6_init,
	.pr_fasttimo =		icmp6_fasttimo,
	.pr_slowtimo =		icmp6_slowtimo,
	.pr_usrreqs =		&rip6_usrreqs
},
{
	.pr_type =		SOCK_RAW,
	.pr_domain =		&inet6domain,
	.pr_protocol =		IPPROTO_DSTOPTS,
	.pr_flags =		PR_ATOMIC|PR_ADDR,
	.pr_input =		dest6_input,
	.pr_usrreqs =		&nousrreqs
},
{
	.pr_type =		SOCK_RAW,
	.pr_domain =		&inet6domain,
	.pr_protocol =		IPPROTO_ROUTING,
	.pr_flags =		PR_ATOMIC|PR_ADDR,
	.pr_input =		route6_input,
	.pr_usrreqs =		&nousrreqs
},
{
	.pr_type =		SOCK_RAW,
	.pr_domain =		&inet6domain,
	.pr_protocol =		IPPROTO_FRAGMENT,
	.pr_flags =		PR_ATOMIC|PR_ADDR,
	.pr_input =		frag6_input,
	.pr_usrreqs =		&nousrreqs
},
#ifdef IPSEC
{
	.pr_type =		SOCK_RAW,
	.pr_domain =		&inet6domain,
	.pr_protocol =		IPPROTO_AH,
	.pr_flags =		PR_ATOMIC|PR_ADDR,
	.pr_input =		ipsec6_common_input,
	.pr_usrreqs =		&nousrreqs,
},
{
	.pr_type =		SOCK_RAW,
	.pr_domain =		&inet6domain,
	.pr_protocol =		IPPROTO_ESP,
	.pr_flags =		PR_ATOMIC|PR_ADDR,
        .pr_input =		ipsec6_common_input,
	.pr_ctlinput =		esp6_ctlinput,
	.pr_usrreqs =		&nousrreqs,
},
{
	.pr_type =		SOCK_RAW,
	.pr_domain =		&inet6domain,
	.pr_protocol =		IPPROTO_IPCOMP,
	.pr_flags =		PR_ATOMIC|PR_ADDR,
        .pr_input =		ipsec6_common_input,
	.pr_usrreqs =		&nousrreqs,
},
#endif /* IPSEC */
#ifdef INET
{
	.pr_type =		SOCK_RAW,
	.pr_domain =		&inet6domain,
	.pr_protocol =		IPPROTO_IPV4,
	.pr_flags =		PR_ATOMIC|PR_ADDR|PR_LASTHDR,
	.pr_input =		encap6_input,
	.pr_output =		rip6_output,
	.pr_ctloutput =		rip6_ctloutput,
	.pr_init =		encap_init,
	.pr_usrreqs =		&rip6_usrreqs
},
#endif /* INET */
{
	.pr_type =		SOCK_RAW,
	.pr_domain =		&inet6domain,
	.pr_protocol =		IPPROTO_IPV6,
	.pr_flags =		PR_ATOMIC|PR_ADDR|PR_LASTHDR,
	.pr_input =		encap6_input,
	.pr_output =		rip6_output,
	.pr_ctloutput =		rip6_ctloutput,
	.pr_init =		encap_init,
	.pr_usrreqs =		&rip6_usrreqs
},
{
	.pr_type =		SOCK_RAW,
	.pr_domain =		&inet6domain,
	.pr_protocol =		IPPROTO_PIM,
	.pr_flags =		PR_ATOMIC|PR_ADDR|PR_LASTHDR,
	.pr_input =		encap6_input,
	.pr_output =		rip6_output,
	.pr_ctloutput =		rip6_ctloutput,
	.pr_usrreqs =		&rip6_usrreqs
},
#ifdef DEV_CARP
{
	.pr_type =		SOCK_RAW,
	.pr_domain =		&inet6domain,
	.pr_protocol =		IPPROTO_CARP,
	.pr_flags =		PR_ATOMIC|PR_ADDR,
	.pr_input =		carp6_input,
	.pr_output =		rip6_output,
	.pr_ctloutput =		rip6_ctloutput,
	.pr_usrreqs =		&rip6_usrreqs
},
#endif /* DEV_CARP */
/* raw wildcard */
{
	.pr_type =		SOCK_RAW,
	.pr_domain =		&inet6domain,
	.pr_flags =		PR_ATOMIC|PR_ADDR,
	.pr_input =		rip6_input,
	.pr_output =		rip6_output,
	.pr_ctloutput =		rip6_ctloutput,
	.pr_usrreqs =		&rip6_usrreqs
},
};

extern int in6_inithead(void **, int);
#ifdef VIMAGE
extern int in6_detachhead(void **, int);
#endif

struct domain inet6domain = {
	.dom_family =		AF_INET6,
	.dom_name =		"internet6",
	.dom_protosw =		(struct protosw *)inet6sw,
	.dom_protoswNPROTOSW =	(struct protosw *)
				&inet6sw[sizeof(inet6sw)/sizeof(inet6sw[0])],
#ifdef RADIX_MPATH
	.dom_rtattach =		rn6_mpath_inithead,
#else
	.dom_rtattach =		in6_inithead,
#endif
#ifdef VIMAGE
	.dom_rtdetach =		in6_detachhead,
#endif
	.dom_rtoffset =		offsetof(struct sockaddr_in6, sin6_addr) << 3,
	.dom_maxrtkey =		sizeof(struct sockaddr_in6),
	.dom_ifattach =		in6_domifattach,
	.dom_ifdetach =		in6_domifdetach
};

VNET_DOMAIN_SET(inet6);

/*
 * Internet configuration info
 */
VNET_DEFINE(int, ip6_forwarding);
VNET_DEFINE(int, ip6_sendredirects);
VNET_DEFINE(int, ip6_defhlim);
VNET_DEFINE(int, ip6_defmcasthlim);
VNET_DEFINE(int, ip6_accept_rtadv);
VNET_DEFINE(int, ip6_maxfragpackets);
VNET_DEFINE(int, ip6_maxfrags);
VNET_DEFINE(int, ip6_log_interval);
VNET_DEFINE(int, ip6_hdrnestlimit);
VNET_DEFINE(int, ip6_dad_count);
VNET_DEFINE(int, ip6_auto_flowlabel);
VNET_DEFINE(int, ip6_use_deprecated);
VNET_DEFINE(int, ip6_rr_prune);
VNET_DEFINE(int, ip6_mcast_pmtu);
VNET_DEFINE(int, ip6_v6only);
VNET_DEFINE(int, ip6_keepfaith);
VNET_DEFINE(time_t, ip6_log_time);
VNET_DEFINE(int, ip6stealth);
VNET_DEFINE(int, nd6_onlink_ns_rfc4861);

/* icmp6 */
/*
 * BSDI4 defines these variables in in_proto.c...
 * XXX: what if we don't define INET? Should we define pmtu6_expire
 * or so? (jinmei@kame.net 19990310)
 */
VNET_DEFINE(int, pmtu_expire);
VNET_DEFINE(int, pmtu_probe);

/* raw IP6 parameters */
/*
 * Nominal space allocated to a raw ip socket.
 */
VNET_DEFINE(u_long, rip6_sendspace);
VNET_DEFINE(u_long, rip6_recvspace);

/* ICMPV6 parameters */
VNET_DEFINE(int, icmp6_rediraccept);
VNET_DEFINE(int, icmp6_redirtimeout);
VNET_DEFINE(int, icmp6errppslim);
/* control how to respond to NI queries */
VNET_DEFINE(int, icmp6_nodeinfo);

/* UDP on IP6 parameters */
VNET_DEFINE(int, udp6_sendspace);
VNET_DEFINE(int, udp6_recvspace);

/*
 * sysctl related items.
 */
SYSCTL_NODE(_net,	PF_INET6,	inet6,	CTLFLAG_RW,	0,
	"Internet6 Family");

/* net.inet6 */
SYSCTL_NODE(_net_inet6,	IPPROTO_IPV6,	ip6,	CTLFLAG_RW, 0,	"IP6");
SYSCTL_NODE(_net_inet6,	IPPROTO_ICMPV6,	icmp6,	CTLFLAG_RW, 0,	"ICMP6");
SYSCTL_NODE(_net_inet6,	IPPROTO_UDP,	udp6,	CTLFLAG_RW, 0,	"UDP6");
SYSCTL_NODE(_net_inet6,	IPPROTO_TCP,	tcp6,	CTLFLAG_RW, 0,	"TCP6");
#ifdef SCTP
SYSCTL_NODE(_net_inet6,	IPPROTO_SCTP,	sctp6,	CTLFLAG_RW, 0,	"SCTP6");
#endif
#ifdef IPSEC
SYSCTL_NODE(_net_inet6,	IPPROTO_ESP,	ipsec6,	CTLFLAG_RW, 0,	"IPSEC6");
#endif /* IPSEC */

/* net.inet6.ip6 */
static int
sysctl_ip6_temppltime(SYSCTL_HANDLER_ARGS)
{
	int error = 0;
	int old;

	VNET_SYSCTL_ARG(req, arg1);

	error = SYSCTL_OUT(req, arg1, sizeof(int));
	if (error || !req->newptr)
		return (error);
	old = V_ip6_temp_preferred_lifetime;
	error = SYSCTL_IN(req, arg1, sizeof(int));
	if (V_ip6_temp_preferred_lifetime <
	    V_ip6_desync_factor + V_ip6_temp_regen_advance) {
		V_ip6_temp_preferred_lifetime = old;
		return (EINVAL);
	}
	return (error);
}

static int
sysctl_ip6_tempvltime(SYSCTL_HANDLER_ARGS)
{
	int error = 0;
	int old;

	VNET_SYSCTL_ARG(req, arg1);

	error = SYSCTL_OUT(req, arg1, sizeof(int));
	if (error || !req->newptr)
		return (error);
	old = V_ip6_temp_valid_lifetime;
	error = SYSCTL_IN(req, arg1, sizeof(int));
	if (V_ip6_temp_valid_lifetime < V_ip6_temp_preferred_lifetime) {
		V_ip6_temp_preferred_lifetime = old;
		return (EINVAL);
	}
	return (error);
}

SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_FORWARDING, forwarding, CTLFLAG_RW,
	&VNET_NAME(ip6_forwarding), 0, "");
SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_SENDREDIRECTS, redirect, CTLFLAG_RW,
	&VNET_NAME(ip6_sendredirects), 0, "");
SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_DEFHLIM, hlim, CTLFLAG_RW,
	&VNET_NAME(ip6_defhlim), 0, "");
SYSCTL_VNET_STRUCT(_net_inet6_ip6, IPV6CTL_STATS, stats, CTLFLAG_RD,
	&VNET_NAME(ip6stat), ip6stat, "");
SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_MAXFRAGPACKETS, maxfragpackets,
	CTLFLAG_RW, &VNET_NAME(ip6_maxfragpackets), 0, "");
SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_ACCEPT_RTADV, accept_rtadv,
	CTLFLAG_RW, &VNET_NAME(ip6_accept_rtadv), 0, "");
SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_KEEPFAITH, keepfaith, CTLFLAG_RW,
	&VNET_NAME(ip6_keepfaith), 0, "");
SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_LOG_INTERVAL, log_interval,
	CTLFLAG_RW, &VNET_NAME(ip6_log_interval), 0, "");
SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_HDRNESTLIMIT, hdrnestlimit,
	CTLFLAG_RW, &VNET_NAME(ip6_hdrnestlimit), 0, "");
SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_DAD_COUNT, dad_count, CTLFLAG_RW,
	&VNET_NAME(ip6_dad_count), 0, "");
SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_AUTO_FLOWLABEL, auto_flowlabel,
	CTLFLAG_RW, &VNET_NAME(ip6_auto_flowlabel), 0, "");
SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_DEFMCASTHLIM, defmcasthlim,
	CTLFLAG_RW, &VNET_NAME(ip6_defmcasthlim), 0, "");
SYSCTL_STRING(_net_inet6_ip6, IPV6CTL_KAME_VERSION, kame_version,
	CTLFLAG_RD, __KAME_VERSION, 0, "");
SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_USE_DEPRECATED, use_deprecated,
	CTLFLAG_RW, &VNET_NAME(ip6_use_deprecated), 0, "");
SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_RR_PRUNE, rr_prune, CTLFLAG_RW,
	&VNET_NAME(ip6_rr_prune), 0, "");
SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_USETEMPADDR, use_tempaddr,
	CTLFLAG_RW, &VNET_NAME(ip6_use_tempaddr), 0, "");
SYSCTL_VNET_PROC(_net_inet6_ip6, IPV6CTL_TEMPPLTIME, temppltime,
	CTLTYPE_INT|CTLFLAG_RW, &VNET_NAME(ip6_temp_preferred_lifetime), 0,
   	sysctl_ip6_temppltime, "I", "");
SYSCTL_VNET_PROC(_net_inet6_ip6, IPV6CTL_TEMPVLTIME, tempvltime,
	CTLTYPE_INT|CTLFLAG_RW, &VNET_NAME(ip6_temp_valid_lifetime), 0,
   	sysctl_ip6_tempvltime, "I", "");
SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_V6ONLY, v6only,	CTLFLAG_RW,
	&VNET_NAME(ip6_v6only), 0, "");
SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_AUTO_LINKLOCAL, auto_linklocal,
	CTLFLAG_RW, &VNET_NAME(ip6_auto_linklocal), 0, "");
SYSCTL_VNET_STRUCT(_net_inet6_ip6, IPV6CTL_RIP6STATS, rip6stats, CTLFLAG_RD,
	&VNET_NAME(rip6stat), rip6stat, "");
SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_PREFER_TEMPADDR, prefer_tempaddr,
	CTLFLAG_RW, &VNET_NAME(ip6_prefer_tempaddr), 0, "");
SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_USE_DEFAULTZONE, use_defaultzone,
	CTLFLAG_RW, &VNET_NAME(ip6_use_defzone), 0,"");
SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_MAXFRAGS, maxfrags, CTLFLAG_RW,
	&VNET_NAME(ip6_maxfrags), 0, "");
SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_MCAST_PMTU, mcast_pmtu, CTLFLAG_RW,
	&VNET_NAME(ip6_mcast_pmtu), 0, "");
#ifdef IPSTEALTH
SYSCTL_VNET_INT(_net_inet6_ip6, IPV6CTL_STEALTH, stealth, CTLFLAG_RW,
	&VNET_NAME(ip6stealth), 0, "");
#endif

/* net.inet6.icmp6 */
SYSCTL_VNET_INT(_net_inet6_icmp6, ICMPV6CTL_REDIRACCEPT, rediraccept,
	CTLFLAG_RW, &VNET_NAME(icmp6_rediraccept), 0, "");
SYSCTL_VNET_INT(_net_inet6_icmp6, ICMPV6CTL_REDIRTIMEOUT, redirtimeout,
	CTLFLAG_RW, &VNET_NAME(icmp6_redirtimeout), 0, "");
SYSCTL_VNET_STRUCT(_net_inet6_icmp6, ICMPV6CTL_STATS, stats, CTLFLAG_RD,
	&VNET_NAME(icmp6stat), icmp6stat, "");
SYSCTL_VNET_INT(_net_inet6_icmp6, ICMPV6CTL_ND6_PRUNE, nd6_prune, CTLFLAG_RW,
	&VNET_NAME(nd6_prune), 0, "");
SYSCTL_VNET_INT(_net_inet6_icmp6, ICMPV6CTL_ND6_DELAY, nd6_delay, CTLFLAG_RW,
	&VNET_NAME(nd6_delay), 0, "");
SYSCTL_VNET_INT(_net_inet6_icmp6, ICMPV6CTL_ND6_UMAXTRIES, nd6_umaxtries,
	CTLFLAG_RW, &VNET_NAME(nd6_umaxtries), 0, "");
SYSCTL_VNET_INT(_net_inet6_icmp6, ICMPV6CTL_ND6_MMAXTRIES, nd6_mmaxtries,
	CTLFLAG_RW, &VNET_NAME(nd6_mmaxtries), 0, "");
SYSCTL_VNET_INT(_net_inet6_icmp6, ICMPV6CTL_ND6_USELOOPBACK, nd6_useloopback,
	CTLFLAG_RW, &VNET_NAME(nd6_useloopback), 0, "");
SYSCTL_VNET_INT(_net_inet6_icmp6, ICMPV6CTL_NODEINFO, nodeinfo, CTLFLAG_RW,
	&VNET_NAME(icmp6_nodeinfo), 0, "");
SYSCTL_VNET_INT(_net_inet6_icmp6, ICMPV6CTL_ERRPPSLIMIT, errppslimit,
	CTLFLAG_RW, &VNET_NAME(icmp6errppslim), 0, "");
SYSCTL_VNET_INT(_net_inet6_icmp6, ICMPV6CTL_ND6_MAXNUDHINT, nd6_maxnudhint,
	CTLFLAG_RW, &VNET_NAME(nd6_maxnudhint), 0, "");
SYSCTL_VNET_INT(_net_inet6_icmp6, ICMPV6CTL_ND6_DEBUG, nd6_debug, CTLFLAG_RW,
	&VNET_NAME(nd6_debug), 0, "");

SYSCTL_VNET_INT(_net_inet6_icmp6, ICMPV6CTL_ND6_ONLINKNSRFC4861,
	nd6_onlink_ns_rfc4861, CTLFLAG_RW, &VNET_NAME(nd6_onlink_ns_rfc4861),
	0, "Accept 'on-link' nd6 NS in compliance with RFC 4861.");
