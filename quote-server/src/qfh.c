
//Copyright (C) Apr 2012 Jorge Silva, Gordon Tani
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
// Config File for Quote Feed Handler 
/*
 ============================================================================
 Name        : qfh.c
 Description : TL1 quote server with simple in memory store for past ticks etc..
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <syslog.h>
#include <libev/ev.h>
#include <time.h>
#include <assert.h>
#include "qfh.h"
#include "mc_socket_r.h"
#include "qfl1filer.h"

#include "qsnapshot.h"

// Hash structure

sHTentry *ht_threads = NULL;
zctx_t *ctx = NULL; //zctx_new();
config_t cfg;
char *cfgname = NULL;
int bCfgDel = 0; // bool set to dealloc if I alloced..
struct ev_loop *loop = NULL;
char qfl1_puburl[CONFIG_CON_STR_LEN] = { 0 };
char qfl1_ctrlurl[CONFIG_CON_STR_LEN] = { 0 };
char hsvf_puburl[CONFIG_CON_STR_LEN] = { 0 };
char hsvf_ctrlurl[CONFIG_CON_STR_LEN] = { 0 };
void *qf11_pub_port;
void *hsvf_pub_port;
void *qfl1_inp_queue;
void *hsvf_inp_queue;

ev_signal signal_watcher; // ctrl c
//Add a worker to the hash
void add_sktl(sHTentry *s) {
	syslog(LOG_DEBUG, "Allocated memory for %s \n", s->name);
	HASH_ADD_STR(ht_threads, name, s);
	syslog(LOG_DEBUG, "Added %s to pool\n", s->name);
}
// remove a worker from the hash
void del_sktl(sHTentry *s_wth) {
	HASH_DEL(ht_threads, s_wth);
	free(s_wth);
	syslog(LOG_DEBUG, "Freed memory for %s \n", s_wth->name);
}

//Describe usage
void usage(void) {
	printf("Usage:\n");
	printf("\t-f <name> Config file name\n");
	printf("\t-d <n> Debug and log level\n");
	printf("\t-s <n> Nanoseconds interval when using an input file\n");
	printf(
			"\t-i Feed file for:\n\tQF1 : Read: XXX.XXX.XXX.XXX:XXXX ->len[50]->[^B0048000001929TL100E T XEI  00200020010020002005 ^C]\n");
	printf("\t-p xxxxx Port to publish on (p > 1000)\n");
	printf("\t-o Dump raw message to StdOut\n");
	exit(EXIT_SUCCESS);
}

static void shutdown_callbacks(void) {
	if (loop)
		ev_break(loop, EVBREAK_ALL);
	struct sHTentry *s, *tmp;
	char cfeed_num[3] = { 0 };
	int feed_t;
	HASH_ITER(hh, ht_threads, s, tmp)
	{
		memset(cfeed_num, 3, sizeof(char));
		snprintf(cfeed_num, 3, "%s", s->name);
		feed_t = atoi(cfeed_num);
		if (feed_t == FEED_TYPE_STATS) {
			sqfhbuffer * sqf = (sqfhbuffer *) s->value;
			__sync_fetch_and_sub(&sqf->brun, 1);
		} else if (feed_t == FEED_TYPE_QRTM1_F) {
			stl1fr * fr = (stl1fr *) s->value;
			__sync_fetch_and_sub(&fr->brun, 1);
		}
	}
}

static void shutdown_cb(struct ev_loop *loop, struct ev_io *w, int revents) {
	stimercb * stmcb = (stimercb *) w->data;
	time_t now;
	double seconds;
	time(&now);
	seconds = difftime(now, mktime(&stmcb->shutdown));
	if (seconds > 0) {
		ev_periodic_stop(loop, w);
		shutdown_callbacks();
		// (EV_A_ w);
	}
}

static void sigint_cb(struct ev_loop *loop, struct ev_signal *w, int revents) {
	ev_io_stop(EV_A_ w);
	//printf("Recvd Break - Exiting..\n");
	//ev_io_stop(ev_loop, w);
	shutdown_callbacks();
	//sleep(5);
	//__sync_fetch_and_sub( &sqf->brun,1);
}

static void program_exits(void) {
	char cfeed_num[3] = { 0 };
	int feed_t;
	struct sHTentry *s, *tmp;
	config_destroy(&cfg);
	if (bCfgDel)
		free(cfgname);
	HASH_ITER(hh, ht_threads, s, tmp)
	{
		memset(cfeed_num, 3, sizeof(char));
		snprintf(cfeed_num, 3, "%s", s->name);
		feed_t = atoi(cfeed_num);
		if (feed_t == FEED_TYPE_QRTM1_F) {
			stl1fr *stf = (stl1fr *) s->value;
		} else if (feed_t == FEED_TYPE_QRTM1) {
			ssocklistener *sml = (ssocklistener*) s->value;
			ev_io_stop(loop, &sml->io); // stop listeners
			if (!sml->infd) // close the socket
				close(sml->infd);
		} else if (feed_t == FEED_TYPE_STATS) {
			sqfhbuffer * sqf = (sqfhbuffer *) s->value;
			__sync_fetch_and_sub(&sqf->brun, 1);
			if (pthread_cancel(sqf->pthrd))
				printf("Can't cxl...\n%s\n", strerror(errno));
			void * rc;
			printf("Issuing - join\n");
			pthread_join(sqf->pthrd, &rc);
			if (rc == PTHREAD_CANCELED) {
				printf("Successfully cxld memory buffer for %s \n",
						sqf->inproc_sock);
			} else {
				printf("Second attempt\n");
				while (rc != 0) {
					pthread_join(sqf->pthrd, &rc);
					sleep(1);
				}
			}
			htUTS * tmp, *obj;
			HASH_ITER(hh, sqf->buffer, obj, tmp)
			{
				HASH_DEL(sqf->buffer, obj);
				if (sqf->buffer)
					free(sqf->buffer->value);
				free(obj);
				obj = NULL;
				sqf->count--;
			}
			printf(
					"Processed for %s\nMessages: %.f@%-9.3f Trades: %.f@%-9.3f Quotes: %.f@%-9.3f Requests %.f@%-9.3f #%d\n",
					sqf->puburl, sqf->total.count,
					(sqf->total.count > 0 ?
							(sqf->total.time / sqf->total.count) : 1) * 0.001,
					sqf->trades.count,
					(sqf->trades.count > 0 ?
							(sqf->trades.time / sqf->trades.count) : 1) * 0.001,
					sqf->quotes.count,
					(sqf->quotes.count > 0 ?
							(sqf->quotes.time / sqf->quotes.count) : 1) * 0.001,
					sqf->requests.count,
					(sqf->requests.count > 0 ?
							(sqf->requests.time / sqf->requests.count) : 1)
							* 0.001, sqf->count);
		} else if (feed_t == FEED_TYPE_HSVF) {
			ssocklistener *sml = (ssocklistener*) s->value;
			ev_io_stop(loop, &sml->io); // stop listeners
			if (!sml->infd) // close the socket
				close(sml->infd);
			htUTS * tmp, *obj;
			HASH_ITER(hh, sml->htSecDef, obj, tmp)
			{
				HASH_DEL(sml->htSecDef, obj);
				free(obj);
				obj = NULL;
			}
		}
		if (s->value) {
			free(s->value);
		}
		HASH_DEL(ht_threads, s);
		if (s) {
			free(s);
			s = NULL;
		}
	}
	ev_signal_stop(loop, &signal_watcher);
	ev_default_destroy();
	if (loop) {
		free(loop);
	}
	zsocket_destroy(ctx, qf11_pub_port);
	zsocket_destroy(ctx, hsvf_pub_port);
	zsocket_destroy(ctx, qfl1_inp_queue);
	zsocket_destroy(ctx, hsvf_inp_queue);
	zctx_destroy(&ctx);

	// remove any socket I may have created
	if (strstr(qfl1_puburl, "ipc://")) {
		unlink(strstr(qfl1_puburl, "ipc://"));
	}
	if (strstr(qfl1_ctrlurl, "ipc://")) {
		unlink(strstr(qfl1_ctrlurl, "ipc://"));
	}
	if (strstr(hsvf_puburl, "ipc://")) {
		unlink(strstr(hsvf_puburl, "ipc://"));
	}
	if (strstr(hsvf_ctrlurl, "ipc://")) {
		unlink(strstr(hsvf_ctrlurl, "ipc://"));
	}
	exit(0);
	//printf(" - Cleaning up \n");
}

void init_mem_store(int ftype, config_t *cfg, void ** p_port, void ** ip_skt,
		char * req_url, char * ipc_url, char * pub_url, int use_std) {
	char * feed_s, *localif_s, *snapshot_s;
	config_setting_t *settings;
	sHTentry * ptrCB;
	ssocklistener *smcl;
	sqfhbuffer *fhmonitor;
	time_t rawtime;
	struct tm *info;
	int sock;
	time(&rawtime);
	info = localtime(&rawtime);
	struct htSecDef *htsd = NULL;
	if (ftype == FEED_TYPE_HSVF) {
		feed_s = "hsvf.feeds";
		localif_s = "hsvf.localif";
	} else if (ftype == FEED_TYPE_QRTM1) {
		feed_s = "qfl1.feeds";
		localif_s = "qfl1.localif";

	}
	if (use_std != 1) {
		*ip_skt = zsocket_new(ctx, ZMQ_PAIR);
		if (zsocket_bind(*ip_skt, ipc_url) == -1) {
			printf("Unable to bind to  %s - exiting, Error %s\n", ipc_url,
					strerror(errno));
			exit(EXIT_FAILURE);
		}

		*p_port = zsocket_new(ctx, ZMQ_PUB);
		if (zsocket_bind(*p_port, pub_url) == -1) {
			printf("Unable to bind to  %s - exiting, Error %s\n", pub_url,
					strerror(errno));
			exit(EXIT_FAILURE);
		}

		printf("Will publish on:\n\t%s\n\t%s\n", (char *) pub_url, ipc_url);
		if (ftype == FEED_TYPE_QRTM1_F)
			return;
		if ((ptrCB = calloc(1, sizeof(sHTentry))) != NULL) {
			if ((fhmonitor = calloc(1, sizeof(sqfhbuffer))) != NULL) {
				fhmonitor->puburl = req_url;
				fhmonitor->zmq_ctx = ctx;
				fhmonitor->type = ftype;
				fhmonitor->feed_t = FEED_TYPE_STATS;
				fhmonitor->inproc_sock = ipc_url;
				ptrCB->value = fhmonitor;
				const char * fname;
				if (cfg != NULL
						&& config_lookup_string(cfg, "output_dir", &fname)) {
					DIR * dir = opendir(fname);
					if (dir) {
						fhmonitor->fpath = fname;
					} else {
						printf("Unable to write any data to %s", fname);
						fhmonitor->fpath = NULL;
					}
				}
				snprintf(ptrCB->name, CONFIG_READER_NAME_SZ, "%d%s",
				FEED_TYPE_STATS,
						(ftype == FEED_TYPE_QRTM1 || ftype == FEED_TYPE_QRTM1_F) ?
								"QF1MEMBUFFER" : "HSVMEMBUFFER");
				add_sktl(ptrCB);
				if (ftype == FEED_TYPE_QRTM1 || ftype == FEED_TYPE_QRTM1_F) {
					if (init_qfbuffer(fhmonitor)) {
						printf(
								"Unable to start the %s memory Store - Exiting\n",
								"QFL1MEMBUFFER");
						exit(EXIT_FAILURE);
					}
				} else if (ftype == FEED_TYPE_HSVF) {
					if (init_qfbuffer(fhmonitor)) {
						printf(
								"Unable to start the %s memory Store - Exiting\n",
								"HSVMEMBUFFER");
						exit(EXIT_FAILURE);
					}
				}
			}
		}

	} else {
		*p_port = NULL;
		*ip_skt = NULL;
	}

	settings = cfg != NULL ? config_lookup(cfg, feed_s) : NULL;
	if (settings != NULL) {
		int count = config_setting_length(settings);
		int i;
		const char * lipAddr = NULL;

		// Check if we have an interface to listen on
		config_lookup_string(cfg, localif_s, &lipAddr);
		for (i = 0; i < count; ++i) {
			const char *feedn, *ipaddr, *constr;
			int port;
			config_setting_t *feed = config_setting_get_elem(settings, i);
			if (!(config_setting_lookup_string(feed, "feed", &feedn)
					&& config_setting_lookup_string(feed, "ipaddress", &ipaddr)
					&& config_setting_lookup_int(feed, "port", &port)))
				continue;
			// create the connection details
			if ((ptrCB = calloc(1, sizeof(sHTentry))) != NULL)
				if ((smcl = calloc(1, sizeof(ssocklistener))) != NULL) {
					snprintf(ptrCB->name, CONFIG_READER_NAME_SZ, "%d%s", ftype,
							feedn);
					ptrCB->value = smcl;
					strncpy(smcl->ripAddr, ipaddr, CONFIG_READER_IPADDR_SZ);
					if (lipAddr != NULL)
						strncpy(smcl->lipAddr, lipAddr,
						CONFIG_READER_IPADDR_SZ);
					smcl->port = port;
					smcl->feed_t = ftype;
					smcl->zmqs_pub = *p_port;
					smcl->inproc_sock = *ip_skt;
					smcl->use_stdout = use_std;
					strftime(smcl->yyyy, 5, "%y%m", info);
					if (ftype == FEED_TYPE_HSVF
							&& config_lookup_string(cfg, "hsvf.constr",
									&constr)) {
						snprintf(smcl->constr, (CONFIG_CONSTR_SZ - 2), "%c%s%c",
								2, constr, 3);
						smcl->htSecDef = htsd;
						htSecDef *obj = (htSecDef *) calloc(1,
								sizeof(htSecDef));
						if (obj) {
							sprintf(obj->key, "%s", "junk");
							obj->value.size = -1;
							HASH_ADD_STR(smcl->htSecDef, key, obj);
						}
					}
					//open socket and setup
					if (mc_socket_r(smcl) == NULL) {
						add_sktl(ptrCB);
						ev_io_start(loop, &smcl->io); // start listening..
						printf("%-30s  %-30s %6d\n", ptrCB->name, smcl->ripAddr,
								smcl->port);
					}
				}
		}
	}

}

int main(int argc, char **argv) {

	loop = ev_loop_new(EVBACKEND_POLL | EVFLAG_SIGNALFD);
	if (!loop) {
		printf("Unable to start Event loop - exiting\n");
		exit(EXIT_FAILURE);
	}

	ctx = zctx_new();
	stimercb timercb;
	int port = -1;		//Default to this port
	int c;
	char *fname = NULL;
	int use_stdout = -1;
	int LogMask = 0;
	int major, minor, patch;
	sHTentry * ptrCB;
	long sleep_t = 10000L;
	char cport[20] = { 0 };
	atexit(program_exits);
	while ((c = getopt(argc, argv, "d:f:i:p:vs:o")) != -1)
		switch (c) {
		case 'f':
			cfgname = optarg;
			break;
		case 'd':
			LogMask = atoi(optarg);
			switch (LogMask) {
			case 1:
				setlogmask(LOG_UPTO(LOG_NOTICE));
				break;
			case 2:
				setlogmask(LOG_UPTO(LOG_DEBUG));
				break;
			}
			break;
		case 'i':
			fname = optarg;
			break;
		case 'o':
			use_stdout = 1;
			break;
		case 's': {
			sleep_t = atol(optarg);
			if (sleep_t < 0)
				sleep_t = 0;
			break;
		}
		case 'v':
			zmq_version(&major, &minor, &patch);
			printf("Current 0MQ version is %d.%d.%d\n", major, minor, patch);
			exit(EXIT_SUCCESS);
			break;
		case 'p':
			port = atoi(optarg);
			if ((port - 4) < 1000) {
				printf("Port is out of range - -p > 1000, unable to start\n");
				return (EXIT_FAILURE);
			}
			snprintf(cport, 20, ":%d", port);
			break;
		case '?':
			if (isprint(optopt))
				usage();
			else
				fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
			return 1;
		default:
			abort();
		}

	config_init(&cfg);

	if (!cfgname) {
		c = strlen(argv[0]) + 5;
		if ((cfgname = (char *) malloc(c * sizeof(char))))
			sprintf(cfgname, "%s.cfg", argv[0]);
		bCfgDel = 1;
	}
	bCfgDel = 0;

	/* Read the file. If there is an error, report it and exit. */
	if (!config_read_file(&cfg, cfgname)) {
		if (config_error_file(&cfg) == NULL) {
			syslog(LOG_NOTICE, "Unable to Open %s\n", cfgname);
			fprintf(stderr, "Unable to Open %s\n", cfgname);
		} else {
			syslog(LOG_NOTICE, "%s:%d - %s\n", config_error_file(&cfg),
			config_error_line(&cfg), config_error_text(&cfg));
			fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
			config_error_line(&cfg), config_error_text(&cfg));
		}
		exit(EXIT_FAILURE);
	}

	const char * stop_time;
	if (config_lookup_string(&cfg, "shutdown", &stop_time) != CONFIG_FALSE) {
		int hh, mm, ss;
		sscanf(stop_time, "%d:%d:%d", &hh, &mm, &ss);
		time_t now;
		time(&now);
		timercb.shutdown = *localtime(&now);
		timercb.shutdown.tm_hour = hh;
		timercb.shutdown.tm_min = mm;
		timercb.shutdown.tm_sec = ss;
		timercb.cbtimer.data = &timercb;
		ev_periodic_init(&timercb.cbtimer, shutdown_cb, 0, 30, 0);
		ev_periodic_start(loop, &timercb.cbtimer);
	}

	if (config_error_file(&cfg) == NULL && port < 0) {
		const char *qfl1_purl, *curl, *hsvf_purl;
		if (config_lookup_string(&cfg, "qfl1.publish",
				&qfl1_purl) == CONFIG_TRUE)
			snprintf(qfl1_puburl, CONFIG_CON_STR_LEN, "%s", qfl1_purl);

		if (config_lookup_string(&cfg, "qfl1.control", &curl) == CONFIG_TRUE)
			snprintf(qfl1_ctrlurl, CONFIG_CON_STR_LEN, "%s", curl);

		if (config_lookup_string(&cfg, "hsvf.publish",
				&hsvf_purl) == CONFIG_TRUE)
			snprintf(hsvf_puburl, CONFIG_CON_STR_LEN, "%s", hsvf_purl);

		if (config_lookup_string(&cfg, "hsvf.control", &curl) == CONFIG_TRUE)
			snprintf(hsvf_ctrlurl, CONFIG_CON_STR_LEN, "%s", curl);
	} else {
		printf("Overriding config port - command line port specified %d", port);
		snprintf(qfl1_puburl, CONFIG_CON_STR_LEN, "tcp://*:%d", port);
		snprintf(qfl1_ctrlurl, CONFIG_CON_STR_LEN, "tcp://*:%d", port + 1);
		snprintf(hsvf_puburl, CONFIG_CON_STR_LEN, "tcp://*:%d", port + 2);
		snprintf(hsvf_ctrlurl, CONFIG_CON_STR_LEN, "tcp://*:%d", port + 3);
	}

	ev_signal_init(&signal_watcher, sigint_cb, SIGINT);
	ev_signal_start(loop, &signal_watcher);

	if (fname != NULL) {
		init_mem_store(FEED_TYPE_QRTM1_F, &cfg, &qf11_pub_port, &qfl1_inp_queue,
				qfl1_ctrlurl, QFL1_INPROC_SOCK, qfl1_puburl, use_stdout);
		if ((ptrCB = calloc(1, sizeof(sHTentry))) != NULL) {
			stl1fr *frpars;
			if ((frpars = calloc(1, sizeof(stl1fr))) != NULL) {				
				frpars->sleept.tv_nsec = sleep_t; // sleep 100 milliseconds;
				frpars->sleept.tv_sec = 0;
				frpars->zmqs_pub = qf11_pub_port;
				frpars->inproc_sock = qfl1_inp_queue;
				frpars->fname = fname;
				frpars->brun = 1;
				snprintf(ptrCB->name, CONFIG_READER_NAME_SZ, "%d%s",
				FEED_TYPE_QRTM1_F, "FILE");
				ptrCB->value = frpars;
				add_sktl(ptrCB);
				init_qfl1quotefileR(ptrCB);
			}
			// wait for this to finish
			ev_loop(loop, 0);
			pthread_join(frpars->pthrd, NULL);
		}

	} else {
		if (LogMask)
			openlog("qfh", LOG_PID | LOG_CONS | LOG_PERROR, LOG_DAEMON);
		if (strlen(qfl1_ctrlurl) > 0 && strlen(qfl1_puburl) > 0)
			init_mem_store(FEED_TYPE_QRTM1, &cfg, &qf11_pub_port,
					&qfl1_inp_queue, qfl1_ctrlurl, QFL1_INPROC_SOCK,
					qfl1_puburl, use_stdout);
		if (strlen(hsvf_ctrlurl) > 0 && strlen(hsvf_puburl) > 0)
			init_mem_store(FEED_TYPE_HSVF, &cfg, &hsvf_pub_port,
					&hsvf_inp_queue, hsvf_ctrlurl, HSVF_INPROC_SOCK,
					hsvf_puburl, use_stdout);
		ev_loop(loop, 0);
	}
	return EXIT_SUCCESS;
}
