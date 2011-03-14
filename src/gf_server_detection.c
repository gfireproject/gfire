/*
 * purple - Xfire Protocol Plugin
 *
 * This file is part of Gfire.
 *
 * See the AUTHORS file distributed with Gfire for a full list of
 * all contributors and this files copyright holders.
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

#include "gf_server_detection.h"

gfire_server_detector *gfire_server_detector_create(GCallback p_server_callback)
{
	if(!p_server_callback)
		return NULL;

	gfire_server_detector *ret = g_malloc0(sizeof(gfire_server_detector));
	if(!ret)
		return NULL;

	ret->mutex = g_mutex_new();
	ret->server_callback = p_server_callback;

	return ret;
}

void gfire_server_detector_free(gfire_server_detector *p_detector)
{
	if(!p_detector)
		return;

	// Stop the detector first
	gfire_server_detector_stop(p_detector);

	g_mutex_free(p_detector->mutex);

	g_free(p_detector);
}

gboolean gfire_server_detector_running(const gfire_server_detector *p_detector)
{
	if(!p_detector)
		return FALSE;

	g_mutex_lock(p_detector->mutex);
	gboolean ret = p_detector->running;
	g_mutex_unlock(p_detector->mutex);
	return ret;
}

gboolean gfire_server_detector_finished(const gfire_server_detector *p_detector)
{
	if(!p_detector)
		return FALSE;

	g_mutex_lock(p_detector->mutex);
	gboolean ret = p_detector->finished;
	g_mutex_unlock(p_detector->mutex);
	return ret;
}

void gfire_server_detection_remove_invalid_servers(gfire_server_detector *p_detection, const GList *p_local_ips)
{
	// Check detected TCP servers
	GList *cur = p_detection->tcp_servers;
	while(cur)
	{
		gboolean deleted = FALSE;
		gfire_server *server = (gfire_server*)cur->data;

		// Check against local IPs
		const GList *lip = p_local_ips;
		while(lip)
		{
			if(server->ip == *((guint32*)lip->data))
			{
				deleted = TRUE;
				GList *to_delete = cur;
				cur = g_list_next(cur);

				g_free(server);
				p_detection->tcp_servers = g_list_delete_link(p_detection->tcp_servers, to_delete);

				break;
			}
			lip = g_list_next(lip);
		}
		if(deleted)
			continue;

		// Check against invalid ports
		GList *port = p_detection->excluded_ports;
		while(port)
		{
			if(server->port == *((guint16*)port->data))
			{
				deleted = TRUE;
				GList *to_delete = cur;
				cur = g_list_next(cur);

				g_free(server);
				p_detection->tcp_servers = g_list_delete_link(p_detection->tcp_servers, to_delete);

				break;
			}
			port = g_list_next(port);
		}
		if(deleted)
			continue;

		cur = g_list_next(cur);
	}

	// Check detected UDP servers
	cur = p_detection->udp_servers;
	while(cur)
	{
		gboolean deleted = FALSE;
		gfire_server *server = (gfire_server*)cur->data;

		// Check against local IPs
		const GList *lip = p_local_ips;
		while(lip)
		{
			if(server->ip == *((guint32*)lip->data))
			{
				deleted = TRUE;
				GList *to_delete = cur;
				cur = g_list_next(cur);

				g_free(server);
				p_detection->udp_servers = g_list_delete_link(p_detection->udp_servers, to_delete);

				break;
			}
			lip = g_list_next(lip);
		}
		if(deleted)
			continue;

		// Check against invalid ports
		GList *port = p_detection->excluded_ports;
		while(port)
		{
			if(server->port == *((guint16*)port->data))
			{
				deleted = TRUE;
				GList *to_delete = cur;
				cur = g_list_next(cur);

				g_free(server);
				p_detection->udp_servers = g_list_delete_link(p_detection->udp_servers, to_delete);

				break;
			}
			port = g_list_next(port);
		}
		if(deleted)
			continue;

		cur = g_list_next(cur);
	}
}

// Sorts from highest to lowest priority
static gint gfire_server_cmp(const gfire_server *p_server1, const gfire_server *p_server2)
{
	if(p_server1->priority < p_server2->priority)
		return 1;
	else if(p_server1->priority == p_server2->priority)
		return 0;
	else
		return -1;
}

const gfire_server *gfire_server_detection_guess_server(gfire_server_detector *p_detection)
{
	// Get best TCP server
	gfire_server *tcp = NULL;
	if(p_detection->tcp_servers)
	{
		p_detection->tcp_servers = g_list_sort(p_detection->tcp_servers, (GCompareFunc)gfire_server_cmp);
		tcp = p_detection->tcp_servers->data;
	}

	// Get best UDP server
	gfire_server *udp = NULL;
	if(p_detection->udp_servers)
	{
		p_detection->udp_servers = g_list_sort(p_detection->udp_servers, (GCompareFunc)gfire_server_cmp);
		udp = p_detection->udp_servers->data;
	}

	if(!tcp)
		return udp;
	else if(!udp)
		return tcp;
	else
		// Return TCP only if it has better priority than UDP (most servers use UDP anyway)
		return (tcp->priority > udp->priority) ? tcp : udp;
}
