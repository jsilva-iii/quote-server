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

#ifndef QFH_H_
#define QFH_H_
#include <libev/ev.h>
#include <time.h>
#include "libconfig.h"
#include "uthash.h"
#include "zmqmsg.h"
#include "zmq.h"
#include "hsvf.h"
#include "qfl1filer.h"
//#include "qsnapshot.h"
#include "qfhbuffer.h"

#define CONFIG_READER_NAME_SZ  10
#define CONFIG_READER_IPADDR_SZ 16
#define CONFIG_CONSTR_SZ 40
#define CONFIG_READER_BUF_SZ 1024 //2048
#define CONFIG_CON_STR_LEN 256
#define LINGER 100000
#define CONFIG_MSG_T_SZ	6
#define SZ_MARKERS 6
#define SZ_DATE 8
#define MSG_HWM 5000

#define FEED_TYPE_TL1 0
#define FEED_TYPE_TL2 1
#define FEED_TYPE_QRTM1 2
#define FEED_TYPE_QRTM2 3
#define FEED_TYPE_TL1_F 4
#define FEED_TYPE_TL2_F 5
#define FEED_TYPE_QRTM1_F 6
#define FEED_TYPE_QRTM2_F 7
#define FEED_TYPE_STATS 8
#define FEED_TYPE_HSVF 9
#define FEED_TYPE_SNAPSHOT 10
//#define TL1_INPROC_SOCK "inproc://Tl1Queue"
#define QFL1_INPROC_SOCK "inproc://tsxQueue"
#define HSVF_INPROC_SOCK "inproc://bdmQueue"

#define STAT_TYPE_EQT 0
#define STAT_TYPE_FUT 1
#define STAT_TYPE_OPT 2

// Market Status for Equities
typedef struct smkt_stat {
	int type;
	double bid;
	int bidsz;
	double ask;
	int asksz;
	double last;
	int lastsz;
	double hi;
	double low;
	int volume;
	int trades;
	int eps;
	int epsccy;
	double adivs;
	double value;
	double open;
	double close;
	double pclose;
	double dividend;
	int buyerid;
	int sellerid;
	int moc;
	int state;
	char trdtime[SZ_DATE];
	char lowtime[SZ_DATE];
	char hightime[SZ_DATE];
	char exdate[SZ_DATE];
	char paydate[SZ_DATE];
	char recdate[SZ_DATE];
	char decdate[SZ_DATE];
	int divmkrks;
	char *divstr;
	char cusip[13];
	char name[41];
	int stkgrp;
	int moceligible;
} smkt_eq_stat;

//Motnreal Exchange publishes depth up to BDM_DEPTH
typedef struct sBDMDepth {
	float bid[BDM_DEPTH] ;//= { 0 };
	float ask[BDM_DEPTH];// = { 0 };
	int bidSize[BDM_DEPTH];// = { 0 };
	int askSize[BDM_DEPTH];// = { 0 };
	int nbidOrd[BDM_DEPTH];// = { 0 };
	int naskOrd[BDM_DEPTH];// = { 0 };
	long asktime[BDM_DEPTH];// = { 0 };
	long bidtime[BDM_DEPTH];// = { 0 };
} sBDMDepth;

// Market Status for Options
typedef struct sfuopstat {
	int type;
	float bid;
	int bidsz;
	float ask;
	int asksz;
	float last;
	float chg;
	int lastsz;
	float hi;
	float low;
	int volume;
	int oi;
	int trades;
	float value;
	float open;
	float close;
	float pclose;
	char trdtime[SZ_DATE];
	char lowtime[SZ_DATE];
	char hightime[SZ_DATE];
	char expiry[SZ_DATE + 2];
	float multiplier;
	float strike;
	int isAmer;
	int isHalted;
	sBDMDepth depth;
} smkt_fo_stat;

// Hashtable for qoutes
typedef struct htUTS {
	UT_hash_handle hh;
	char key[QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX];
	void * value;
} htUTS;

//Call back timer 
typedef struct stimercb {
	int feed_t;     // type of thread
	struct tm shutdown;
	struct ev_periodic cbtimer;
} stimercb;

// Quote Stat Structuture
typedef struct squotestat {
	double time; // total time
	double count; // number of messages
} squotestat;

// typedef for buffer/monitor
typedef struct sqfhmembuf {
	int feed_t;     // type of object
	int type;
	char * puburl;
	char * inproc_sock; // control socket
	pthread_t pthrd;
	int brun;
	void * zmq_ctx;
	const char * fpath;
	htUTS *buffer;
	htUTS *depthbuffer;
	squotestat total;
	squotestat quotes;
	squotestat trades;
	squotestat requests;
	int count;
	//FILE * fsymdump;
} sqfhbuffer;

typedef struct socket_listener {
	//char *name;
	int port;
	ev_io io; // io event lister
	int infd; // fd to read from
	void * zmqs_pub; // fd to write to
	void * inproc_sock; // control socket
	char ripAddr[CONFIG_READER_IPADDR_SZ]; /* remote ip Addr to listen to */
	char lipAddr[CONFIG_READER_IPADDR_SZ]; /* listen on this IP address */
	char constr[CONFIG_CONSTR_SZ]; /* connection string */
	int feed_t;
	htSecDef *htSecDef; // hashtable needed for non-standard option types
	int use_stdout;
	char yyyy[5];
} ssocklistener;

// Structure to pass to file reader thread - to simulate the socket reading;
typedef struct stl1fr {
	char * fname;
	ev_io * io;
	struct timespec sleept;
	int brun;
	void * zmqs_pub;
	void * inproc_sock; // control socket
	pthread_t pthrd;
} stl1fr;
typedef struct sHTentry {
	char name[CONFIG_READER_NAME_SZ]; /* key */
	void *value;
	UT_hash_handle hh;
} sHTentry;

#endif /* QFH_H_ */
