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

#ifndef QFL1_H_
#define QFL1_H_

#define QF_HEADER 23
#define SZ_A_EQTY_TRADE ( 107 - HEADER )
#define SZ_B_EQTY_STATUS ( 141 - HEADER )
#define SZ_C_EQTY_MOC ( 40 - HEADER)
#define SZ_CA_EQTY_MOC_MOVEMENT ( 74 - HEADER )
#define SZ_D_EQTY_STATE ( 98 - HEADER)
#define SZ_E_EQTY_QUOTE ( 86 - HEADER)
#define SZ_G_EQTY_NEG ( 123 - HEADER)
#define SZ_G_EQTY_TRD_CXL ( 102 - HEADER)
#define SZ_S_MKT_STATE ( 45 - HEADER)
#define SZ_T_TRAD_STATUS ( 59 - HEADER)
#define SZ_X_EQTY_TRD_COR ( 116 - HEADER)
#define QF_SZ_HEARTBEAT 209
#define TSX_LOT_SZ(PX) ( ( (PX) >1.00 )?(100):( ( ( PX)<0.10 )?1000:500 ))


#define TRD_PRECISION 0.00001
#define QUO_PRECISION 0.001
#define QF_SYM_LEN 12
//Message types
#define MSG_LOC  19
#define MSG_EXCH_LOC 21
#define MSG_TYPE_A 'A'
#define MSG_TYPE_B 'B'
#define MSG_TYPE_C 'C'
#define MSG_TYPE_CA 'A'
#define MSG_TYPE_D 'D'
#define MSG_TYPE_E 'E'
#define MSG_TYPE_G 'G'
#define MSG_TYPE_H 'H'
#define MSG_TYPE_S 'S'
#define MSG_TYPE_T 'T'
#define MSG_TYPE_X 'X'
// Reference Data
#define MSG_TYPE_M 'M' // SOD/EOD
// Index Data
#define MSG_TYPE_K 'K' // Index Info
#define MSG_TYPE_MD 'D' // Index Info
// pass buffer, message length and the destination fd
extern void parseQFL1 (char *, int , void *, void *);
#endif /* QFL1_H_ */
