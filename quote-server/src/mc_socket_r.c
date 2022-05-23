//Copyright (C) Apr 2013 Jorge Silva, Gordon Tani
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either version 2
//of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA./*
//
// This is part of a Multicast Quote Feed Server for :
// Toronto TL1 and Montreak HSVF depth quotes

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <netinet/ip.h>
#include <syslog.h>
#include <time.h>
#include <errno.h>
#include "mc_socket_r.h"
#include "qfl1.h"
#include "hsvf.h"

// use this to keep the remainder of re-connect message
static char end_buff[CONFIG_READER_BUF_SZ] = { 0 };
static int fut_seq_num = 0;
int doconnect(ssocklistener * sCDetails) {
	struct ip_mreq mcRequest; /* Multicast Address join Structure*/
	struct sockaddr_in svr_addr; /* Multicast Address */
	int sock;
	if (sCDetails->port < (int) MIN_PORT || sCDetails->port > (int) MAX_PORT) {
		syslog(LOG_NOTICE, "Specified a bad Port %d for destination %s\n",
				sCDetails->port, sCDetails->ripAddr);
		return -1;
	}
	// Open Socket
	if (sCDetails->feed_t == FEED_TYPE_QRTM1
			&& (sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		syslog(LOG_NOTICE, "Unable to Open Multicast Socket on %s port %d\n",
				sCDetails->ripAddr, sCDetails->port);
		return -1;
	} else if (sCDetails->feed_t == FEED_TYPE_HSVF
			&& (sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) { //socket(AF_INET, SOCK_STREAM, 0
		syslog(LOG_NOTICE, "Unable to Open Stream Socket on %s port %d\n",
				sCDetails->ripAddr, sCDetails->port);
		return -1;
	}

	sCDetails->infd = sock;
	// Set non blocking
	int nReqFlags = fcntl(sock, F_GETFL, 0);
	if (nReqFlags < 0) {
		syslog(LOG_NOTICE, "Unable to set Socket Options - Exiting\n");
		return -1;
	}

	int reuse = 1;
	if (fcntl(sock, F_SETFL, nReqFlags | O_NONBLOCK) != 0) {
		syslog(LOG_NOTICE, "Unable to set Socket to NO_NONBLOCK - Exiting\n");
		return -1;
	}
	// bind to multicast address
	memset(&svr_addr, 0, sizeof(svr_addr));
	svr_addr.sin_family = AF_INET;
	svr_addr.sin_port = htons(sCDetails->port);

	if (sCDetails->feed_t == FEED_TYPE_QRTM1) {
		svr_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse,
				sizeof(reuse)) < 0) {
			syslog(LOG_NOTICE, "Unable to set SO_REUSEADDR - Exiting\n");
			return -1;
		}
		//sCDetails->lipAddr ? sCDetails->lipAddr : INADDR_ANY );
		if (bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) < 0) {
			syslog(LOG_NOTICE, "Unable to bind to %s on port %d- Exiting\n",
					sCDetails->ripAddr ? sCDetails->ripAddr : INADDR_ANY,
					sCDetails->port);
			return -1;
		}
		// construct IGMP join request
		memset(&mcRequest, 0, sizeof(mcRequest));
		mcRequest.imr_multiaddr.s_addr = inet_addr(sCDetails->ripAddr);
		mcRequest.imr_interface.s_addr = inet_addr(         //htonl(INADDR_ANY);
				sCDetails->lipAddr ? sCDetails->lipAddr : INADDR_ANY);
		// send ADD Membership Message
		if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *) &mcRequest,
				sizeof(mcRequest)) < 0) {
			syslog(LOG_NOTICE, "Unable to join Multicast Group - Exiting\n");
			return -1;
		}
	} else if (sCDetails->feed_t == FEED_TYPE_HSVF) {
		svr_addr.sin_addr.s_addr = inet_addr(sCDetails->ripAddr);
		fd_set fd;
		struct timeval timeout;
		int t_out_s = 5;
		int t_out_us = 500000;
		timeout.tv_sec = t_out_s;
		timeout.tv_usec = t_out_us;

		connect(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr));
		if (errno == EINPROGRESS) {
			FD_ZERO(&fd);
			FD_SET(sock, &fd);
			int rc = select(sock+1,&fd , &fd,(fd_set *) NULL ,(struct timeval *) (&timeout));
			if ( rc == 0 || rc == -1 ) {
				if ( rc ==0){

					printf("Timed out after %.3fs\n", (t_out_s + 500000*0.000001));
				}else
					printf("Unable to connect - Error");
				fflush(stdout);
				return -1;
			} else {
				printf("Connected to %s\n",sCDetails->ripAddr);
					return 0;
			}
		} else {
			printf("Unable to connect - %s\n", strerror(errno));
			fflush(stdout);
			return -1;
		}
	}
	return 0;
}

static void parse_msg_cb(struct ev_loop *loop, ev_io *w, int revents) {
	ssocklistener * sCDetails = (ssocklistener *) w->data;
	char buff[CONFIG_READER_BUF_SZ + 1];
	int len = 0;
	int beg = 0;
	int loc = 0;
	char *last_delim;
	struct sockaddr_in rcv_addr;
	int sz_rcv_addr = sizeof(rcv_addr);
	memset(buff, 0, sizeof(buff));
	if (sCDetails->feed_t == FEED_TYPE_QRTM1) {
		len = recvfrom(sCDetails->infd, buff, CONFIG_READER_BUF_SZ, 0,
				(struct sockaddr *) &rcv_addr, &sz_rcv_addr);
	} else {
		len = recv(sCDetails->infd, buff, CONFIG_READER_BUF_SZ, 0);
	}

	if (len == -1) {
		syslog(LOG_NOTICE, "Error at recvffrom for %d for destination %s\n",
				sCDetails->port, sCDetails->ripAddr);
	} else if (len == 0) {
		syslog(LOG_NOTICE,
				"Connection Closed for %d for destination %s - error %s\n",
				sCDetails->port, sCDetails->ripAddr, strerror(errno));
		ev_io_stop(loop, &sCDetails->io);
		close(sCDetails->infd);
	} else {
#ifdef DEBUG
		syslog(LOG_DEBUG, "Read: %s:%d ->len[%d]->[%s]\n", sCDetails->ripAddr,
				sCDetails->port, len, buff);
#endif
		// TL1 socket and File Reader
		if ((sCDetails->feed_t == FEED_TYPE_QRTM1)) {
#ifdef DEBUG
			printf("Read: %s:%d ->len[%d]->[%s]\n", sCDetails->ripAddr,
								sCDetails->port, len, buff);
			syslog(LOG_DEBUG, "Parsing QFL1 MSG: len[%d]->[%s]\n", len, buff);
#endif
			if (len != QF_SZ_HEARTBEAT) {
				if (sCDetails->use_stdout == 1) {
					struct timespec _timespec;
					clock_gettime(CLOCK_REALTIME, &_timespec);
					printf("%12d %12d %s\n", _timespec.tv_sec, _timespec.tv_nsec, buff);
				} else
					parseQFL1(buff, len - 2, (void *) sCDetails->zmqs_pub,
							(void *) sCDetails->inproc_sock);
			}
		} else if (sCDetails->feed_t == FEED_TYPE_HSVF) {
#ifdef DEBUG
			printf("Read %d\n", mnum++);
			printf("Read: %s:%d ->len[%d]->[%s]\n", sCDetails->ripAddr,
					sCDetails->port, len, buff);
#endif
			// check to see if we have left over from previoius read
			if (buff[0] != 0x2) {
				beg = 0;
				do {
					beg++;
				} while (loc < CONFIG_READER_BUF_SZ && *(buff + beg) != 0x2);
				int nlen = strlen(end_buff);
				memcpy(end_buff + nlen, buff, beg);
				parseHSVF(end_buff, nlen + beg, (void *) sCDetails->zmqs_pub,
						(void *) sCDetails->inproc_sock, &fut_seq_num, sCDetails->yyyy,
						sCDetails->htSecDef, sCDetails->use_stdout);
				memset(end_buff, 0, (nlen + 1 + beg) * sizeof(char));
			}
			// more to follow
			if (buff[len - 1] != 0x03) {
				loc = CONFIG_READER_BUF_SZ;
				do {
					loc--;
				} while (loc > 0 && *(buff + loc) != 0x2);
				int stub_msg = (len - loc);
				memcpy(end_buff, buff + loc, stub_msg * sizeof(char));
				memset(buff + loc, 0, stub_msg * sizeof(char));
				len = loc;
			}
			parseHSVF((buff + beg), len, (void *) sCDetails->zmqs_pub,
					(void *) sCDetails->inproc_sock, &fut_seq_num,
					sCDetails->yyyy, sCDetails->htSecDef, sCDetails->use_stdout);
			memset(buff, 0, (len + 1 > CONFIG_READER_BUF_SZ) ?
			CONFIG_READER_BUF_SZ :
																(len + 1));
		}
	}
}

int mc_socket_r(ssocklistener * mc_args) {
	if (doconnect(mc_args) == 0) {
		// pass our data to io_ev
		mc_args->io.data = mc_args;
		if ((mc_args->feed_t == FEED_TYPE_HSVF)
				&& (strlen(mc_args->constr) > 0)) {	// log on and request message
			int n = send(mc_args->infd, mc_args->constr,
					strlen(mc_args->constr), 0);
			if (n != strlen(mc_args->constr)) {
				syslog(LOG_NOTICE,
						"Unable to subscribe to Futures & Options - %s \n",
						strerror(errno));
				return -1;
			}
			//mc_args->htSecDef = NULL;
		}
		ev_io_init(&(mc_args->io), parse_msg_cb, mc_args->infd, EV_READ);
		syslog(LOG_DEBUG, "Added Listener for %d for destination %s\n",
				mc_args->port, mc_args->ripAddr);
		return 0;
	} else {
		syslog(LOG_NOTICE,
				"Unable to add  Listener for %d for destination %s\n",
				mc_args->port, mc_args->ripAddr);
		printf("Unable to add  Listener for %d for destination %s\n", mc_args->port, mc_args->ripAddr);
		exit(EXIT_FAILURE);
		//return -1;
	}
	return 0;
}
