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


#ifndef HSVF_H_
#define HSVF_H_
#include "uthash.h"
#include "QuoteLib.pb-c.h"

// Messages start with 1 char as Option type, char 2 is the instrument
#define OPT ' '			// Option
#define FUT_OPT 'B' 		// Future Option
#define FUT 'F'				// Future
#define STR 'S'				// Strategy
#define UL	'E'				//
#define GTPSTAT 'R'			// used by group status
#define BDM_DEPTH 5

//Trade Message
#define TRADE_MSG 'C'
//Request for Quote
#define OPT_RFQ 'D'
//Quote Message
#define OPT_QUO 'F'
//Market Depth
#define OPT_DPT 'H'
//Trade CXL
#define OPT_CXL 'I'
//Instrument Key messages
#define OPT_IKM 'J'
//Summary Messages
#define OPT_SUM 'N'
//Beginning of Summary Message
#define BEG_OPT_SUM 'Q'
//Group Status
#define GRP_STAT 'G'
//Bulletins
#define BULL	'L'
//Begining of Fut Sumary
#define BEG_FUT_SUM 'L'
// End of sales
#define EOS 'S'
// End of Trans
#define EOT 'U'
//Heart Beat
#define HB 'V'
//Gap sequence
#define GS 'W'
//End of Transmission
#define EOD 'S'
#define OPT_EXPIRY_MONTH(EM) ( ((EM) >'L' )?(EM-'L'):(EM-'@' ))
//( (EM)<'L')?( (EM)-'I' ):((EM)<'O')?(EM -'L'):((EM)<'W')?(EM-'P'):(((EM) =='X')?11:12)
//#define FUT_EXPIRY_MONTH(EM)  ( (EM) > 'H' )?( ( (EM) < 'L' )?((EM)-'F'):( ((EM) < 'O')?((EM) - 'G'):( ((EM) < 'W' )?((EM) - 'L'):( ((EM) == 'X')?(11):(12) ) )  )  ):( (EM)-'E')
#define TRD_MRKRS(MK)( MK==' ')
typedef struct htssecdef{
	int size;
	int ccy;
	int ulccy;
	char type;
}htssecdef;

typedef struct htSecDef {
	UT_hash_handle hh;
        char key[QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX];
        htssecdef value;
} htSecDef;

// pass b uffer, message length and the destination fd
extern void parseHSVF (char *, int , void *, void *, int *,  const char *yy, htSecDef *, int use_std);

#endif /* HSVF_H_ */
