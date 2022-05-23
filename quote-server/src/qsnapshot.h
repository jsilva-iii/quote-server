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
// This will take snapshots of current quotes in memory

#ifndef QSNAPSHOT_H_
#define QSNAPSHOT_H_
#include "qfh.h"

// typedef for snapshot
typedef struct sqsnapshot {
	int feed_type;     // type of object
	pthread_t pthrd;
	int brun;
	const sqfhbuffer *buffer;
	config_t cfg_file;
	char configfile[CONFIG_CON_STR_LEN];
	char refstr[CONFIG_CONSTR_SZ];
} sqsnapshot;

int init_qsnapshot(sqsnapshot * config);

#endif /* QFH_H_ */
