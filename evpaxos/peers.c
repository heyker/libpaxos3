/*
	Copyright (C) 2013 University of Lugano

	This file is part of LibPaxos.

	LibPaxos is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Libpaxos is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with LibPaxos.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "peers.h"
#include "xdr.h"
#include "message.h"
#include "peers.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

struct peer
{
	int id;
	struct bufferevent* bev;
	struct event* reconnect_ev;
	struct sockaddr_in addr;
	peer_cb callback;
	void* arg;
};

struct peers
{
	int count;
	struct peer** peers;
	struct event_base* base;
	struct evpaxos_config* config;
};

static struct timeval reconnect_timeout = {2,0};
static struct peer* make_peer(struct event_base* base, int id,
	struct sockaddr_in* addr, peer_cb cb, void* arg);
static void free_peer(struct peer* p);
static void connect_peer(struct peer* p);
static void peers_connect(struct peers* p, int id, 
	struct sockaddr_in* addr, peer_cb cb, void* arg);

struct peers*
peers_new(struct event_base* base, struct evpaxos_config* conf)
{
	struct peers* p = malloc(sizeof(struct peers));
	p->count = 0;
	p->peers = NULL;
	p->base = base;
	p->config = conf;
	return p;
}

void
peers_free(struct peers* p)
{
	int i;
	for (i = 0; i < p->count; i++)
		free_peer(p->peers[i]);		
	if (p->count > 0)
		free(p->peers);
	free(p);
}

static void
peers_connect(struct peers* p, int id, struct sockaddr_in* addr,
	peer_cb cb, void* arg)
{
	p->peers = realloc(p->peers, sizeof(struct peer*) * (p->count+1));
	p->peers[p->count] = make_peer(p->base, id, addr, cb, arg);
	p->count++;
}

void
peers_connect_to_acceptors(struct peers* p, peer_cb cb, void* arg)
{
	int i;
	for (i = 0; i < evpaxos_acceptor_count(p->config); i++) {
		struct sockaddr_in addr = evpaxos_acceptor_address(p->config, i);
		peers_connect(p, i, &addr, cb, arg);
	}
}

int
peers_count(struct peers* p)
{
	return p->count;
}

struct bufferevent*
peers_get_buffer(struct peers* p, int i)
{
	return p->peers[i]->bev;
}

static void
on_read(struct bufferevent* bev, void* arg)
{
	paxos_message msg;
	struct peer* p = arg;
	struct evbuffer* in = bufferevent_get_input(bev);
	while (recv_paxos_message(in, &msg)) {
		p->callback(&msg, p->id, p->arg);
		xdr_free((xdrproc_t)xdr_paxos_message, &msg);
	}
}

static void
on_socket_event(struct bufferevent* bev, short ev, void *arg)
{
	struct peer* p = (struct peer*)arg;
	
	if (ev & BEV_EVENT_CONNECTED) {
		paxos_log_info("Connected to %s:%d",
			inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port));
	} else if (ev & BEV_EVENT_ERROR || ev & BEV_EVENT_EOF) {
		struct event_base* base;
		int err = EVUTIL_SOCKET_ERROR();
		paxos_log_error("%s (%s:%d)", evutil_socket_error_to_string(err),
			inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port));
		base = bufferevent_get_base(p->bev);
		bufferevent_free(p->bev);
		p->bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
		bufferevent_setcb(p->bev, on_read, NULL, on_socket_event, p);
		event_add(p->reconnect_ev, &reconnect_timeout);
	} else {
		paxos_log_error("Event %d not handled", ev);
	}
}

static void
on_connection_timeout(int fd, short ev, void* arg)
{
	connect_peer((struct peer*)arg);
}

static void
connect_peer(struct peer* p)
{
	bufferevent_enable(p->bev, EV_READ|EV_WRITE);
	bufferevent_socket_connect(p->bev, 
		(struct sockaddr*)&p->addr, sizeof(p->addr));
	paxos_log_info("Connect to %s:%d", 
		inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port));
}

static struct peer*
make_peer(struct event_base* base, int id, 
	struct sockaddr_in* addr,  peer_cb cb, void* arg)
{
	struct peer* p = malloc(sizeof(struct peer));
	p->id = id;
	p->addr = *addr;
	p->reconnect_ev = evtimer_new(base, on_connection_timeout, p);
	p->bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(p->bev, on_read, NULL, on_socket_event, p);
	p->callback = cb;
	p->arg = arg;
	connect_peer(p);	
	return p;
}

static void
free_peer(struct peer* p)
{
	bufferevent_free(p->bev);
	event_free(p->reconnect_ev);
	free(p);
}
