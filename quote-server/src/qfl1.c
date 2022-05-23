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

#include "qfl1.h"
#include "qfh.h"
void qfSymb(char *dest, char *src) {
	memcpy(dest, src, (QF_SYM_LEN - 1) * sizeof(char));
	int g = 8;
	do {
		g--;
	} while (*(dest + g) == ' ' && g > 0);
	*(dest + g + 1) = 0;
}

int getStkState(char * loc) {
	if (*loc == 'A') {
		if (*(loc + 1) == 'R') {
			return QUOTE_LIB__STOCK_STATUS__AR;
		} else if (*(loc + 1) == 'S') {
			return QUOTE_LIB__STOCK_STATUS__AS;
		} else if (*(loc + 1) == 'G') {
			return QUOTE_LIB__STOCK_STATUS__AG;
		} else if (*(loc + 1) == 'E') {
			return QUOTE_LIB__STOCK_STATUS__AE;
		} else if (*(loc + 1) == 'F') {
			return QUOTE_LIB__STOCK_STATUS__AF;
		} else {
			return QUOTE_LIB__STOCK_STATUS__A;
		}
	} else {
		if (*(loc + 1) == 'R') {
			return QUOTE_LIB__STOCK_STATUS__IR;
		} else if (*(loc + 1) == 'S') {
			return QUOTE_LIB__STOCK_STATUS__IS;
		} else if (*(loc + 1) == 'G') {
			return QUOTE_LIB__STOCK_STATUS__IG;
		} else if (*(loc + 1) == 'E') {
			return QUOTE_LIB__STOCK_STATUS__IE;
		} else if (*(loc + 1) == 'F') {
			return QUOTE_LIB__STOCK_STATUS__IF;
		} else {
			return QUOTE_LIB__STOCK_STATUS__I;
		}
	}
}

void parseQFL1(char * buff, int mtype, void * destfd, void * inproc) {
	// Create a message
	kvmsg_t *qmsg = kvmsg_new();
	char key_name[QF_SYM_LEN] = { 0 };
	char t_char;
	switch (*(buff + MSG_LOC)) {

	case MSG_TYPE_A: 	//Equity Trade
		qfSymb(key_name, buff + QF_HEADER);
		kvmsg_init(qmsg, key_name, QUOTE_LIB__MSG_TYPES__TRADE);
		QuoteLib__Trade *trade = (QuoteLib__Trade *) qmsg->value;
		trade->isopen = *(buff + 86) == 'Y' ? 1 : 0;		// is opening trade
		if (trade->isopen)
			trade->has_isopen = 1;
		trade->isbypass = *(buff + 85) == 'Y' ? 1 : 0;			// bypass type
		if (trade->isbypass)
			trade->has_isbypass = 1;
		if (*(buff + 83) != ' ') {
			trade->has_xtype = 1;
			if (*(buff + 84) == 'I') {
				trade->xtype = QUOTE_LIB__CROSS_TYPES__INT;
			} else if (*(buff + 84) == 'B') {
				trade->xtype = QUOTE_LIB__CROSS_TYPES__BASI;
			} else if (*(buff + 84) == 'C') {
				trade->xtype = QUOTE_LIB__CROSS_TYPES__CONT;
			} else if (*(buff + 84) == 'S') {
				trade->xtype = QUOTE_LIB__CROSS_TYPES__SPEC;
			} else if (*(buff + 84) == 'V') {
				trade->xtype = QUOTE_LIB__CROSS_TYPES__VWAP;
			}
		}

		if (*(buff + 87) != ' ') {
			trade->has_sterms = 1;
			if (*(buff + 87) == 'C') {
				trade->xtype = QUOTE_LIB__SETTLEMENT_TYPES__CA;
			} else if (*(buff + 87) == 'N') {
				trade->xtype = QUOTE_LIB__SETTLEMENT_TYPES__NN;
			} else if (*(buff + 84) == 'M') {
				trade->xtype = QUOTE_LIB__SETTLEMENT_TYPES__MS;
			} else if (*(buff + 84) == 'T') {
				trade->xtype = QUOTE_LIB__SETTLEMENT_TYPES__CT;
			} else if (*(buff + 84) == 'D') {
				trade->xtype = QUOTE_LIB__SETTLEMENT_TYPES__DT;
			}
		}
		memcpy(trade->trdtime, buff + 57, 6 * sizeof(char));


		*(buff + 74) = 0;
		trade->last = atoi(buff + 63) * TRD_PRECISION;
		//seller id
		*(buff + 74) = t_char;
		t_char = *(buff + 63);
		*(buff + 57) = 0;
		trade->sellerid = atoi(buff + 54);
		trade->has_sellerid = 1;
		*(buff + 54) = 0;
		trade->buyerid = atoi(buff + 51);
		trade->has_buyerid = 1;
		*(buff + 51) = 0;
		trade->trd_px = atoi(buff + 40) * TRD_PRECISION;
		trade->has_trd_px = 1;
		*(buff + 40) = 0;
		trade->volume = atoi(buff + 31);
		break;
	case MSG_TYPE_B:	// Symbol Status		
		qfSymb(key_name, buff + QF_HEADER);
		kvmsg_init(qmsg, key_name, QUOTE_LIB__MSG_TYPES__QF_SYM_STATUS);
		QuoteLib__QfSymbStatus *symstatus =
				(QuoteLib__QfSymbStatus *) qmsg->value;
		memcpy(symstatus->exchid, (buff + 51), 3 * sizeof(char));
		memcpy(symstatus->cusip, (buff + 54), 12 * sizeof(char));
		symstatus->state = getStkState(buff + 140);
		memcpy(symstatus->issuer, (buff + 98), 40 * sizeof(char));
		symstatus->is_stk = (*(buff + 97) == 'E') ? 1 : 0;
		symstatus->has_is_stk = 1;
		symstatus->moc_elig = (*(buff + 96) == 'Y') ? 1 : 0;
		symstatus->has_moc_elig = 1;
		snprintf(symstatus->ccy, 4, "%s",
				(*(buff + 75) == 'C' ? "CAD" : "USD"));

		*(buff + 140) = 0;
		symstatus->stkgrp = atoi(buff + 138);
		symstatus->has_stkgrp = 1;

		*(buff + 96) = 0;
		symstatus->last = atoi(buff + 85) * TRD_PRECISION;
		symstatus->has_last = 1;

		if (!symstatus->is_stk) {
			*(buff + 85) = 0;
			symstatus->db_fv = atoi(buff + 76);
			if (symstatus->db_fv) {
				symstatus->db_fv = symstatus->db_fv * TRD_PRECISION;
				symstatus->has_db_fv = 1;
			}
		}

		*(buff + 75) = 0;
		symstatus->lotsz = atoi(buff + 66);
		symstatus->has_lotsz = 1;

		break;
	case MSG_TYPE_C:	// MOC or MOC A

		qfSymb(key_name, buff + QF_HEADER);
		if (*(buff + MSG_LOC + 1) == 'A') {
			kvmsg_init(qmsg, key_name, QUOTE_LIB__MSG_TYPES__QF_MOC_PMD);
			QuoteLib__Moc *moc = (QuoteLib__Moc *) qmsg->value;
			moc->vwap = atoi(buff + 64);
			moc->has_vwap = 1;
			moc->has_vwap_pow = 1;
			*(buff + 64) = 0;
			moc->ccp = atoi(buff + 53);
			moc->has_ccp = 1;
			moc->has_ccp_pow = 1;
		} else {
			int size = atoi(buff + 32) * (*(buff + 31) == 'B' ? 1 : -1);
			if (size != 0) {
				kvmsg_init(qmsg, key_name, QUOTE_LIB__MSG_TYPES__MOC);
				QuoteLib__Moc *moc = (QuoteLib__Moc *) qmsg->value;
				moc->side = size;
				moc->has_side = 1;
			}
		}
		break;
	case MSG_TYPE_D:	// Stock State		
		qfSymb(key_name, buff + QF_HEADER);
		kvmsg_init(qmsg, key_name, QUOTE_LIB__MSG_TYPES__STK_STATUS);
		QuoteLib__StkStatus *stkstate = (QuoteLib__StkStatus *) qmsg->value;
		memcpy(stkstate->timehalted, buff + 39, 12 * sizeof(char));
		stkstate->state = getStkState(buff + 91);
		if (stkstate->state != QUOTE_LIB__STOCK_STATUS__A) {
			if (*(buff + 93) != '0' && *(buff + 94) != '0')
				memcpy(stkstate->expectedopen, buff + 93, 6 * sizeof(char));
			if (*(buff + 51) != ' ' && *(buff + 52) != ' '
					&& *(buff + 53) != ' ') {
				memcpy(stkstate->reason, buff + 51, 40 * sizeof(char));
				if (*(stkstate->reason) != ' ') {
					int loc = 40;
					do {
						loc--;
					} while (*(stkstate->reason + loc) == ' ' && loc > 0);
					*(stkstate->reason + loc + 1) = 0;
				} else
					*(stkstate->reason) = 0;
			}
		}
		break;
	case MSG_TYPE_E:
		// Equity Quote		
		qfSymb(key_name, buff + QF_HEADER);
		kvmsg_init(qmsg, key_name, QUOTE_LIB__MSG_TYPES__QUOTE);
		QuoteLib__Quote *quote = (QuoteLib__Quote *) qmsg->value;
		memcpy(quote->ex_time, buff + 75, 12 * sizeof(char));

		*(buff + 67) = 0;
		quote->asksize = atoi(buff + 58);

		*(buff + 58) = 0;
		quote->ask = atoi(buff + 49) * QUO_PRECISION;

		*(buff + 49) = 0;
		quote->bidsize = atoi(buff + 40);

		*(buff + 40) = 0;
		quote->bid = atoi(buff + 31) * QUO_PRECISION;

		break;
	case MSG_TYPE_G:
		// General
		snprintf(key_name, QF_SYM_LEN, "%s", "Bulletin");
		kvmsg_init(qmsg, key_name, QUOTE_LIB__MSG_TYPES__INFO_MSG);
		QuoteLib__Info *info = (QuoteLib__Info *) qmsg->value;
		memcpy(info->ex_time, buff + 31, 12 * sizeof(char));
		memcpy(info->msg, buff + 44, 80 * sizeof(char));
		int loc = 81;
		do {
			loc--;
		} while (*(info->msg + loc) == ' ' && loc > 0);
		*(info->msg + loc + 1) = 0;
		break;
	case MSG_TYPE_H:
		// Equity trade CXL
		qfSymb(key_name, buff + QF_HEADER);		
		kvmsg_init(qmsg, key_name, QUOTE_LIB__MSG_TYPES__TRADE_CXL);
		trade = (QuoteLib__Trade *) qmsg->value;
		memcpy(trade->trdtime, buff + 57, 6 * sizeof(char));
		*(buff + 51) = 0;
		trade->last = atoi(buff + 40) * TRD_PRECISION;
		*(buff + 40) = 0;
		trade->volume = atoi(buff + 40);
		trade->iscxl = 1;
		trade->has_iscxl = 1;

		*(buff + 83) = 0;
		trade->last = atoi(buff + 72);

		*(buff + 57) = 0;
		trade->sellerid = atoi(buff + 54);
		trade->has_sellerid = 1;

		*(buff + 54) = 0;
		trade->buyerid = atoi(buff + 51);
		trade->has_buyerid = 1;

		*(buff + 51) = 0;
		trade->trd_px = atoi(buff + 40);
		trade->has_trd_px = 1;

		trade->volume = atoi(buff + 31);

		break;
	case MSG_TYPE_S:
		// Market State
		snprintf(key_name, 13, "_MKT_STAT=%s", buff + 43);
		kvmsg_init(qmsg, key_name, QUOTE_LIB__MSG_TYPES__QF_MKT_STATE);
		QuoteLib__MarketState *state = (QuoteLib__MarketState *) qmsg->value;
		memcpy(state->ex_time, buff + 23, 12 * sizeof(char));
		switch (*(buff + 45)) {
		case 'P':
			state->state = QUOTE_LIB__MKT_STATE__PRE_OPEN;
			break;
		case 'O':
			state->state = QUOTE_LIB__MKT_STATE__OPENING;
			break;
		case 'S':
			state->state = QUOTE_LIB__MKT_STATE__PRE_OPEN;
			break;
		case 'C':
			state->state = QUOTE_LIB__MKT_STATE__CLOSED;
			break;
		case 'R':
			state->state = QUOTE_LIB__MKT_STATE__EXT_HRS_OPE;
			break;
		case 'F':
			state->state = QUOTE_LIB__MKT_STATE__EXT_HRS_CLS;
			break;
		case 'N':
			state->state = QUOTE_LIB__MKT_STATE__EXT_HRS_CXL;
			break;
		case 'M':
			state->state = QUOTE_LIB__MKT_STATE__MOC_IMB;
			break;
		case 'A':
			state->state = QUOTE_LIB__MKT_STATE__CCP_DET;
			break;
		case 'E':
			state->state = QUOTE_LIB__MKT_STATE__PME_EXT;
			break;
		case 'L':
			state->state = QUOTE_LIB__MKT_STATE__CLOSING;
			break;
		}
		*(buff + 45) = 0;
		state->stkgrp = atoi(buff + 43);
		break;
	case MSG_TYPE_T:
		// Trading Tier Status
		snprintf(key_name, QF_SYM_LEN, "%s", "TTState");
		kvmsg_init(qmsg, key_name, QUOTE_LIB__MSG_TYPES__QF_TRD_TIER_STATUS);
		QuoteLib__TradingTierStatus *tt_state =
				(QuoteLib__TradingTierStatus *) qmsg->value;
		memcpy(tt_state->ex_time, buff + 34, 12 * sizeof(char));
		memcpy(tt_state->tierid, buff + 54, 6 * sizeof(char));
		memcpy(tt_state->exch_id, buff + QF_HEADER, 3 * sizeof(char));
		*(buff + 34) = 0;
		tt_state->num_stk_grp = atoi(buff + 31);
		*(buff + 31) = 0;
		tt_state->num_symbols = atoi(buff + 26);
		break;
	case MSG_TYPE_M: // M type messages
		if (*(buff + MSG_LOC + 1) == ' ') { // M Message SOD/EOD
			qfSymb(key_name, buff + QF_HEADER);
			kvmsg_init(qmsg, key_name, QUOTE_LIB__MSG_TYPES__EQ_SUMM);
			QuoteLib__EquitySummary *summ =
					(QuoteLib__EquitySummary *) qmsg->value;
			summ->imo = *(buff + 171);
			summ->dccy = (*(buff + 170) == ' ') ? 0 : (*(buff + 170) - '@');
			*(buff + 170) = 0;
			summ->adivs = atoi(buff + 163) * 0.0001;
			*(buff + 162) = 0;
			summ->eps = atoi(buff + 153) * 0.0001;
			*(buff + 153) = 0;
			summ->trades = atoi(buff + 148);
			*(buff + 148) = 0;
			summ->value = atoi(buff + 137);
			*(buff + 133) = 0; // less markers
			summ->low = atoi(buff + 122) * TRD_PRECISION;
			*(buff + 122) = 0;
			summ->high = atoi(buff + 111) * TRD_PRECISION;
			*(buff + 111) = 0;
			summ->open = atoi(buff + 100) * TRD_PRECISION;
			*(buff + 100) = 0;
			summ->chgfromclose = atoi(buff + 89) * TRD_PRECISION
					* ((*(buff + 88) == '+') ? 1.0 : -1);
			*(buff + 88) = 0;
			summ->volume = atoi(buff + 79);
			*(buff + 78) = 0;
			summ->last = atoi(buff + 67) * TRD_PRECISION;
			*(buff + 67) = 0;
			summ->asksize = atoi(buff + 58);
			*(buff + 58) = 0;
			summ->ask = atoi(buff + 49) * 0.001;
			*(buff + 49) = 0;
			summ->bidsize = atoi(buff + 40);
			*(buff + 40) = 0;
			summ->bid = atoi(buff + 31) * 0.001;
		} else if (*(buff + MSG_LOC + 1) == 'D') { //MD >0065000000001TRD00MDT MAR     130916|130830|130828|0|0000233340|16|  |  <
			qfSymb(key_name, buff + QF_HEADER);
			kvmsg_init(qmsg, key_name, QUOTE_LIB__MSG_TYPES__DIVIDEND);
			QuoteLib__Dividend *div = (QuoteLib__Dividend *) qmsg->value;
			int imk1, imk2, imk3, dtmkr;
			memcpy(div->paydate, buff + QF_HEADER + 8, 6 * sizeof(char));
			memcpy(div->recdate, buff + QF_HEADER + 8 + 6, 6 * sizeof(char));
			memcpy(div->exdate, buff + QF_HEADER + 8 + 12, 6 * sizeof(char));
			imk3 = atoi(buff + 64);
			*(buff + 64) = 0;
			imk2 = atoi(buff + 62);
			*(buff + 62) = 0;
			imk1 = atoi(buff + 60);
			*(buff + 60) = 0;
			div->div = atoi(buff + 50) * 0.0000001;
			div->markers |= *(buff + 49) - '0';
			div->markers |= (imk1 << QUOTE_LIB__DIV_CODES__L_SHIFT)
					& QUOTE_LIB__DIV_CODES__MRKR1;
			div->markers |= (imk2 << (2 * QUOTE_LIB__DIV_CODES__L_SHIFT))
					& QUOTE_LIB__DIV_CODES__MRKR2;
			div->markers |= (imk3 << (3 * QUOTE_LIB__DIV_CODES__L_SHIFT))
					& QUOTE_LIB__DIV_CODES__MRKR3;
		}
		break;
	case MSG_TYPE_K:
		memset(key_name, 0, QF_SYM_LEN);
		key_name[0] = 'I';
		key_name[1] = '-';
		memcpy(key_name + 2,
				buff + QF_HEADER +  4 ,
				4 * sizeof(char));
		kvmsg_init(qmsg, key_name, QUOTE_LIB__MSG_TYPES__INDEX);
		QuoteLib__Index *index = (QuoteLib__Index *) qmsg->value;
		if (*(buff + MSG_LOC + 1) == ' ') { //\0020141000051740TX100K T 1312TX6000074261+0010200036265548", '0' <repeats 15 times>, "74159000742480007424810000000007442900074048+", '0' <repeats 11 times>, " 00742490074274\003
			index->ask = atoi(buff + 135) * 0.01;
			index->has_ask = 1;
			*(buff + 135) = 0;
			index->bid = atoi(buff + 128) * 0.01;
			index->has_bid = 1;
			index->isclose = (*(buff + 127) == 'C')?1:0;
			index->has_isclose =  index->isclose ;
			*(buff + 127) = 0;
			index->yield = atoi(buff + 122) * 0.01;
			index->has_yield =1;
			*(buff + 122) = 0;
			index->pe = atoi(buff+115)*0.01;
			index->has_pe =1;
			*(buff + 115) = 0;
			index->low = atoi(buff + 107) * 0.01;
			index->has_low = 1;
			*(buff + 107) = 0;
			index->high = atoi(buff + 99) * 0.01;
			index->has_high = 1;
			*(buff + 84)=0;
			index->settle = atoi ( buff +76)*0.01;
			index->has_settle =1;
			*(buff + 76) = 0;
			index->open = atoi(buff + 68) * 0.01;
			index->has_open = 1;
			*(buff + 68) = 0;
			index->value = atoi(buff + 56) * 0.01;
			index->has_value = 1;
			*(buff + 56) = 0;
			index->volume = atoi(buff + 45) * 0.01;
			index->has_volume = 1;
			*(buff + 45) = 0;
			index->change = atoi(buff + 39) * 0.01;
			index->has_change = 1;
			*(buff + 39) = 0;
			index->last = atoi(buff + 31) * 0.01;
			index->has_last = 1;
		}// else { // interim level  //0059000058080TX100KST 124400TXBA0223168+0199102231400223193
		//	index->isinterim = 1;
		//	index->ask = atoi(buff + 53) * 0.01;
		//	index->has_ask = 1;
		//	*(buff + 53) = 0;
		//	index->bid = atoi(buff + 46) * 0.01;
		//	index->has_bid = 1;
		//	*(buff + 46) = 0;
		//	index->change = atoi(buff + 40) * 0.01;
		//	index->has_change = 1;
		//	*(buff + 40) = 0;
		//	index->last = atoi(buff + 33) * 0.01;
		//	index->has_last = 1;
		//}
		break;
	}
	if (qmsg != NULL && qmsg->value != NULL) {
		kvmsg_dispatch(qmsg, destfd, inproc);
		//kvmsg_send(qmsg, destfd);
	} else {
		kvmsg_destroy(&qmsg);
	}
}
