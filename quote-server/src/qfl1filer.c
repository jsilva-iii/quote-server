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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <syslog.h>
#include <libev/ev.h>
#include <sys/stat.h>
#include <time.h>
#include <assert.h>
#include "qfh.h"
#include "mc_socket_r.h"


void *
qfl1quotefileR(void *thrdarg) {
	stl1fr *farg;
	FILE *fp;
	struct stat lastread, curtime;
	farg = (stl1fr *) thrdarg;
	char rcvBuff[CONFIG_READER_BUF_SZ];
	char transBuff[CONFIG_READER_BUF_SZ];
	//printf("Here... \n");
	if ((fp = fopen(farg->fname, "r")) == NULL ) {
		printf("Unable to open %s - exiting\n", farg->fname);
		exit(EXIT_FAILURE);
	}
	stat(farg->fname, &lastread);
	printf("I opened the FP now going read data \n");
	while (farg->brun) {
		if (lastread.st_mtim.tv_sec != curtime.st_mtim.tv_sec) {
			while (fgets(rcvBuff, CONFIG_READER_BUF_SZ - 1, fp) != NULL ) {
				char zsz[5] = { 0 };
				int sz;
				char *start = strstr(rcvBuff, "]->[");
				if (start != NULL ) {
					sscanf(start + 5, "%4c", zsz);
					sz = atoi(zsz);
					if (sz > 0) {
						memset(transBuff, 0, CONFIG_READER_BUF_SZ);
						strncpy(transBuff, start + 4, sz + 2);
						memccpy(transBuff, start + 4, ']',
								(strlen(start + 4) - 2));
						parseQFL1(transBuff, strlen(transBuff), farg->zmqs_pub,
								farg->inproc_sock);						
					}
					nanosleep(&farg->sleept, NULL );
				}
			}
			if( curtime.st_mtim.tv_sec ==0){
				curtime.st_mtim.tv_sec =lastread.st_mtim.tv_sec;
			}else
			lastread.st_mtim.tv_sec = curtime.st_mtim.tv_sec;
			fseek(fp, 0, SEEK_SET);
		}
		sleep(2);
		stat(farg->fname, &curtime);
	}
	fclose(fp);
	return NULL ;
}

void init_qfl1quotefileR(void * htHandle) {

	pthread_attr_t attr;
	sHTentry * sht = htHandle;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);	
	int retval = pthread_create(&((stl1fr *) sht->value)->pthrd, &attr,
			qfl1quotefileR, sht->value);
	pthread_attr_destroy(&attr);
	assert(!retval);

}

