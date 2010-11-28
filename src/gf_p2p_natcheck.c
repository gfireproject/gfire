/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2005-2006, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008-2009	Laurent De Marez <laurentdemarez@gmail.com>
 * Copyright (C) 2009-2010  Warren Dumortier <nwarrenfl@gmail.com>
 * Copyright (C) 2009-2010  Oliver Ney <oliver@dryder.de>
 *
 * This file is part of Gfire.
 *
 * Gfire is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gfire.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gf_p2p_natcheck.h"

gfire_p2p_natcheck *gfire_p2p_natcheck_create()
{
	gfire_p2p_natcheck *ret = g_malloc0(sizeof(gfire_p2p_natcheck));
	return ret;
}

void gfire_p2p_natcheck_destroy(gfire_p2p_natcheck *p_nat)
{
	if(!p_nat)
		return;

	if(p_nat->timeout)
		g_source_remove(p_nat->timeout);

	if(p_nat->dnsdata)
		purple_dnsquery_destroy(p_nat->dnsdata);

	if(p_nat->prpl_inpa)
		purple_input_remove(p_nat->prpl_inpa);

	g_free(p_nat);
}

static void gfire_p2p_natcheck_query(gfire_p2p_natcheck *p_nat, int p_server, int p_stage)
{
	static unsigned char packet[18] = { 'S', 'C', '0', '1',
										0x04, 0x5a, 0xcb, 0x00,
										0x01, 0x00, 0x00, 0x00,
										0x00, 0x00, 0x00, 0x00,
										0x00, 0x00 };

	p_stage = GINT32_TO_LE(p_stage);
	memcpy(packet + 12, &p_stage, 4);

	guint16 port = g_htons(purple_network_get_port_from_fd(p_nat->socket));
	memcpy(packet + 16, &port, 2);

	purple_debug_misc("gfire", "P2P: NAT Check: Sending IP request to server %d...\n", p_server + 1);
	sendto(p_nat->socket, packet, 18, 0, (struct sockaddr*)&p_nat->nat_servers[p_server], sizeof(struct sockaddr_in));
}

static gboolean gfire_p2p_natcheck_timeout(gpointer p_data)
{
	gfire_p2p_natcheck *nat = (gfire_p2p_natcheck*)p_data;
	// Check whether the server really timed out...
	if(++nat->retries == 5)
	{
		purple_debug_error("gfire", "P2P: NAT Check: Server %d timed out...check failed!\n", nat->server + 1);
		purple_input_remove(nat->prpl_inpa);
		nat->state = GF_NATCHECK_DONE;
		if(nat->callback) nat->callback(0, 0, 0, nat->callback_data);
		return FALSE;
	}
	// Query the server once again
	gfire_p2p_natcheck_query(nat, nat->server, nat->stage);

	return TRUE;
}

static void gfire_p2p_natcheck_udpread(gpointer p_data, gint p_fd, PurpleInputCondition p_condition)
{
	gfire_p2p_natcheck *nat = (gfire_p2p_natcheck*)p_data;
	if(!nat)
		return;

	static unsigned char buffer[10];

	struct sockaddr_in addr;
	guint addr_len = sizeof(addr);
	int len = recvfrom(p_fd, buffer, 10, 0, (struct sockaddr*)&addr, &addr_len);

	if(len != 10)
	{
		purple_debug_error("gfire", "P2P: NAT Check: Received too less data\n");
		return;
	}

	int server;
	for(server = 0; server < 3; server++)
		if(addr.sin_addr.s_addr == nat->nat_servers[server].sin_addr.s_addr)
			break;

	if(server == 3)
	{
		guint32 ip = addr.sin_addr.s_addr;
		purple_debug_error("gfire", "P2P: NAT Check: Received response from unknown server (%u.%u.%u.%u:%u)\n",
						   (ip & 0xff000000) >> 24,
						   (ip & 0xff0000) >> 16,
						   (ip & 0xff00) >> 8,
						   ip & 0xff,
						   addr.sin_port);
		return;
	}

	if(memcmp(buffer, "CK01", 4) != 0)
	{
		purple_debug_error("gfire", "P2P: NAT Check: Received bad magic\n");
		return;
	}

	if(g_ntohs(addr.sin_port) != XFIRE_NAT_PORT)
		nat->multiple_ports = TRUE;

	guint32 ip = *(guint32*)(buffer + 4);
	guint16 port = *(guint16*)(buffer + 8);

	if(nat->ips[server] != 0 && (nat->ips[server] != ip || nat->ports[server] != port))
		purple_debug_warning("gfire", "P2P: NAT Check: Server %d reports my IP:Port INCONSISTENTLY as %u.%u.%u.%u:%u\n",
						  server + 1,
						  (ip & 0xff000000) >> 24,
						  (ip & 0xff0000) >> 16,
						  (ip & 0xff00) >> 8,
						  ip & 0xff,
						  port);
	else
		purple_debug_misc("gfire", "P2P: NAT Check: Server %d reports my IP:Port as %u.%u.%u.%u:%u\n",
						  server + 1,
						  (ip & 0xff000000) >> 24,
						  (ip & 0xff0000) >> 16,
						  (ip & 0xff00) >> 8,
						  ip & 0xff,
						  port);

	nat->ips[server] = ip;
	nat->ports[server] = port;

	// Some packet we do not need? (delay etc.)
	if(server != nat->server)
		return;

	if(server != 2)
	{
		nat->server++;
		if((nat->server %= 2) == 0)
		{
			if(++nat->stage == 4)
			{
				// Checks are done, now print some info
				if(nat->ips[0] == nat->ips[1] && nat->ports[0] == nat->ports[1])
					purple_debug_info("gfire", "P2P: NAT Check: UDP consistent translation:\tYES (GOOD for P2P)\n");
				else
					purple_debug_warning("gfire", "P2P: NAT Check: UDP consistent translation:\tNO (BAD for P2P)\n");

				if(nat->ips[2] == 0)
					purple_debug_info("gfire", "P2P: NAT Check: UDP unsolicited messages filtered:\tYES (GOOD for security)\n");
				else
					purple_debug_warning("gfire", "P2P: NAT Check: UDP unsolicited messages filtered:\tNO (BAD for security)\n");


				// Examine the data to build the Xfire NAT Type
				if(nat->ips[0] == nat->ips[1] && nat->ports[0] == nat->ports[1])
				{
					nat->type = 4;
					if(nat->multiple_ports || nat->ips[2] == nat->ips[0] || nat->ips[2] == nat->ips[1])
						nat->type = 1;
				}
				else if(nat->ips[0] == nat->ips[1])
					nat->type = 2;
				else
					nat->type = 0;

				static const char *typeNames[] = {
					"NAT Error",
					"Full Cone or Restricted Cone NAT",
					"Symmetric NAT",
					"Symmetric NAT",
					"Port-Restricted Cone NAT"
				};

				purple_debug_info("gfire", "P2P: NAT Check: check finished; Your Xfire NAT Type is %d - %s\n", nat->type,
								  typeNames[nat->type]);

				g_source_remove(nat->timeout);
				nat->timeout = 0;
				nat->state = GF_NATCHECK_DONE;
				if(nat->callback) nat->callback(nat->type, nat->ips[0], nat->ports[0], nat->callback_data);
				return;
			}
		}

		// Query the next server
		nat->retries = 0;
		g_source_remove(nat->timeout);
		nat->timeout = g_timeout_add_seconds(2, gfire_p2p_natcheck_timeout, nat);
		gfire_p2p_natcheck_query(nat, nat->server, nat->stage);
	}
}

static void gfire_p2p_natcheck_start_query(gfire_p2p_natcheck *p_nat)
{
	p_nat->state = GF_NATCHECK_RUNNING;

	p_nat->server = 0;
	p_nat->stage = 1;
	p_nat->prpl_inpa = purple_input_add(p_nat->socket, PURPLE_INPUT_READ, gfire_p2p_natcheck_udpread, p_nat);

	purple_debug_misc("gfire", "P2P: NAT Check: Starting IP requests...\n");
	p_nat->timeout = g_timeout_add_seconds(2, gfire_p2p_natcheck_timeout, p_nat);
	gfire_p2p_natcheck_query(p_nat, p_nat->server, p_nat->stage);
}

static void gfire_p2p_natcheck_dnsquery(GSList *p_hosts, gpointer p_data, const char *p_error)
{
	gfire_p2p_natcheck *nat = (gfire_p2p_natcheck*)p_data;
	if(!nat)
		return;

	if(!p_hosts)
	{
		purple_debug_error("gfire", "P2P: NAT Check: Hostname lookup failed for \"%s\"\n", purple_dnsquery_get_host(nat->dnsdata));
		nat->state = GF_NATCHECK_DONE;
		nat->type = 0;
		if(nat->callback) nat->callback(0, 0, 0, nat->callback_data);
		return;
	}

	int server = -1;
	switch(nat->state)
	{
	case GF_NATCHECK_DNS1:
		server = 0; break;
	case GF_NATCHECK_DNS2:
		server = 1; break;
	case GF_NATCHECK_DNS3:
		server = 2; break;
	default:
		break;
	}

	GSList *hosts = p_hosts;
	while(hosts)
	{
		size_t len = GPOINTER_TO_INT(hosts->data);
		hosts = g_slist_next(hosts);

		struct sockaddr *addr = (struct sockaddr*)hosts->data;
		if(addr->sa_family == AF_INET)
		{
			memset(&nat->nat_servers[server], 0, sizeof(struct sockaddr_in));
			memcpy(&nat->nat_servers[server], addr, len);
		}
		g_free(addr);

		hosts = g_slist_next(hosts);
	}
	g_slist_free(p_hosts);

	purple_debug_misc("gfire", "P2P: NAT Check: Server %d: %u.%u.%u.%u\n", server + 1,
					  (g_ntohl(nat->nat_servers[server].sin_addr.s_addr) & 0xff000000) >> 24,
					  (g_ntohl(nat->nat_servers[server].sin_addr.s_addr) & 0xff0000) >> 16,
					  (g_ntohl(nat->nat_servers[server].sin_addr.s_addr) & 0xff00) >> 8,
					  g_ntohl(nat->nat_servers[server].sin_addr.s_addr) & 0xff);

	nat->dnsdata = NULL;

	switch(nat->state)
	{
	case GF_NATCHECK_DNS1:
		nat->state = GF_NATCHECK_DNS2;
		nat->dnsdata = purple_dnsquery_a(XFIRE_NAT_SERVER2, XFIRE_NAT_PORT, gfire_p2p_natcheck_dnsquery, nat);
		break;
	case GF_NATCHECK_DNS2:
		nat->state = GF_NATCHECK_DNS3;
		nat->dnsdata = purple_dnsquery_a(XFIRE_NAT_SERVER3, XFIRE_NAT_PORT, gfire_p2p_natcheck_dnsquery, nat);
		break;
	case GF_NATCHECK_DNS3:
		gfire_p2p_natcheck_start_query(nat);
		break;
	default:
		break;
	}
}

gboolean gfire_p2p_natcheck_start(gfire_p2p_natcheck *p_nat, int p_socket, gfire_p2p_natcheck_callback p_callback, gpointer p_data)
{
	if(!p_nat || p_socket < 0 || (p_nat->state != GF_NATCHECK_NOSTATE && p_nat->state != GF_NATCHECK_DONE))
		return FALSE;

	// Callback
	p_nat->callback = p_callback;
	p_nat->callback_data = p_data;

	// Socket to use
	p_nat->socket = p_socket;

	// Restore default state
	p_nat->retries = 0;
	p_nat->type = 0;
	p_nat->multiple_ports = FALSE;
	memset(p_nat->ips, 0, 3 * sizeof(guint32));
	memset(p_nat->ports, 0, 3 * sizeof(guint16));

	// Start by resolving the DNS names of Xfires NAT test servers
	purple_debug_info("gfire", "P2P: NAT Check: Starting NAT type check...\n");
	purple_debug_misc("gfire", "P2P: NAT Check: Resolving Xfire's NAT Check Servers hostnames...\n");

	p_nat->state = GF_NATCHECK_DNS1;
	p_nat->dnsdata = purple_dnsquery_a(XFIRE_NAT_SERVER1, XFIRE_NAT_PORT, gfire_p2p_natcheck_dnsquery, p_nat);

	return TRUE;
}

gboolean gfire_p2p_natcheck_isdone(const gfire_p2p_natcheck *p_nat)
{
	return (p_nat->state == GF_NATCHECK_DONE);
}
