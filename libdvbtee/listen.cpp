/*****************************************************************************
 * Copyright (C) 2011-2013 Michael Krufky
 *
 * Author: Michael Krufky <mkrufky@linuxtv.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "listen.h"
#include "log.h"
#define CLASS_MODULE "listen"

#define dprintf(fmt, arg...) __dprintf(DBG_SERVE, fmt, ##arg)


socket_listen::socket_listen()
  : f_kill_thread(false)
  , sock_fd(-1)
  , port(0)
{
	dprintf("()");
}

socket_listen::~socket_listen()
{
	dprintf("()");

	close_socket();
}

socket_listen::socket_listen(const socket_listen&)
{
	dprintf("(copy)");
	f_kill_thread = false;
	sock_fd = -1;
	port = 0;
}

socket_listen& socket_listen::operator= (const socket_listen& cSource)
{
	dprintf("(operator=)");

	if (this == &cSource)
		return *this;

	f_kill_thread = false;
	sock_fd = -1;
	port = 0;

	return *this;
}

void socket_listen::close_socket()
{
	dprintf("()");

	if (sock_fd >= 0) {
		close(sock_fd);
		sock_fd = -1;
	}
	port = 0;
}

void socket_listen::stop()
{
	dprintf("()");

	stop_without_wait();

	while (-1 != sock_fd) {
		usleep(20*1000);
	}
	return;
}

int socket_listen::start(uint16_t port_requested)
{
	struct sockaddr_in tcp_sock;

	dprintf("()");

	memset(&tcp_sock, 0, sizeof(tcp_sock));

	f_kill_thread = false;

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		perror("open socket failed");
		return sock_fd;
	}

	int reuse = 1;
	if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		perror("setting reuse failed");
		return -1;
	}

	tcp_sock.sin_family = AF_INET;
	tcp_sock.sin_port = htons(port_requested);
	tcp_sock.sin_addr.s_addr = INADDR_ANY;

	if (bind(sock_fd, (struct sockaddr*)&tcp_sock, sizeof(tcp_sock)) < 0) {
		perror("bind to local interface failed");
		return -1;
	}
	port = port_requested;

	int fl = fcntl(sock_fd, F_GETFL, 0);
	if (fcntl(sock_fd, F_SETFL, fl | O_NONBLOCK) < 0) {
		perror("set non-blocking failed");
		return -1;
	}
#define MAX_SOCKETS 4
	listen(sock_fd, MAX_SOCKETS);

	int ret = pthread_create(&h_thread, NULL, listen_thread, this);

	if (0 != ret)
		perror("pthread_create() failed");

	return ret;
}

void* socket_listen::listen_thread(void *p_this)
{
	return static_cast<socket_listen*>(p_this)->listen_thread();
}

void* socket_listen::listen_thread()
{
	struct sockaddr_in tcpsa;
	socklen_t salen = sizeof(tcpsa);

	dprintf("(%d)", sock_fd);

	while (!f_kill_thread) {
		int accepted_sock_fd = accept(sock_fd, (struct sockaddr*)&tcpsa, &salen);
		if (accepted_sock_fd != -1) {
			if (accept_socket_cb)
				accept_socket_cb(accept_socket_data, accepted_sock_fd);
			else
				dprintf("(accept_socket callback not defined!)");
		}
		usleep(20*1000);
	}

	close_socket();
	pthread_exit(NULL);
}
