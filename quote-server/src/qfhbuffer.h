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

#ifndef QFHBUFFER_H_
#define QFHBUFFER_H_
#include "qfh.h"
#include "uthash.h"

#define FPATH_LEN 255
#define F_TYPES_SOD 0
#define F_DES_SOD "tsx_symbols.csv"

#define F_TYPES_EOD 1
#define F_DES_EOD "tsx_eod.csv"

#define F_TYPES_DIV 2
#define F_DES_DIV "tsx_dividends.csv"

#define F_TYPES_MOC 3
#define F_DES_MOC "tsx_moc.csv"

#define F_TYPES_FUT_EOD 4
#define F_DES_FUT_EOD "bdm_eod.csv"

#define F_TYPES_FUT_STAT 5
#define F_DES_FUT_STAT "bdm_symbols.csv"

#define F_TYPES_NUM (F_TYPES_FUT_STAT+1)


int init_qfbuffer( void * ) ;

#endif /* QFMONITOR_H_ */
