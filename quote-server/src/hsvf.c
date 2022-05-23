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


#include "hsvf.h"
#include "qfh.h"
#include "zmqmsg.h"

float getPx(char *px, int szpx) {
	float fpx = 0;
	char cpx[20] = { 0 };
	int pw, n;
	memcpy(cpx + 1, px, szpx);
	//float sign = 1;
	if (*(px + szpx) >= 'A') {
		cpx[0] = '-';
		pw = *(px + szpx) - 'A';
	} else {
		cpx[0] = '+';
		pw = *(px + szpx) - '0';
	}
	for (n = 0; n < pw; n++) {
		cpx[szpx - n + 1] = cpx[szpx - n];
	}
	if (pw > 1)
		cpx[szpx - n + 1] = '.';
	
	fpx = atof(cpx);
	return fpx;
}

int getSize(char *sz, int len) {
	char clen[18] = { 0 };
	memcpy(clen, sz, len);
	int retval = 0;
	int n = 0;
	if (clen[5] > '9') {
		for (n = 0; n < clen[5] - '9'; n++) {
			clen[6 + n] = '0';
		}
		clen[5] = '0';
	}
	retval = atoi(clen);
	return retval;
}

int getFutExpMonth(char mnth) {
	if (mnth < 'I') {
		return (mnth - 'E');
	} else if (mnth < 'L') {
		return (mnth - 'F');
	} else if (mnth < 'O') {
		return (mnth - 'G');
	} else if (mnth == 'Q') {
		return (8);
	} else if (mnth < 'W') {
		return (mnth - 'L');
	} else if (mnth == 'X') {
		return 11;
	} else if (mnth == 'Z') {
		return 12;
	}
	return 0;
}
void getFutOptSymbol(char *sym, char *root, char * buff, float *strike,
		char * emm, char * eyy, char *edd) {
	//memset(root, 0, QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX);
	snprintf(root, 4, "%s", buff + 13);
	int em = getFutExpMonth(*(buff + 16));
	snprintf(emm, 3, "%02d", em);
	*(eyy + 1) = *(buff + 17);
	*strike = getPx(buff + 19, 7);
	snprintf(edd, 3, "%s", buff + 27);
	snprintf(sym, QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX, "%s-%d-%s-%g-%s", root,
			em, eyy, *strike, *(buff + 18) > 'L' ? "P" : "C");
}
void getOptionSymbol(char *sym, char *root, char * buff, float *strike,
		char * emm, char * eyy, char *edd) {
	int n = 0;
	memcpy(sym, buff + 13, 6);
	memcpy(eyy, buff + 29, 2);
	memcpy(edd, buff + 31, 2);
	while (!isspace(*(sym + n++)) && n < QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX)
		;
	if (n > QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX)
		return;
	n--;
	memcpy(root, sym, n);
	int em = OPT_EXPIRY_MONTH( *(buff+19) );
	*strike = getPx(buff + 21, 7);
	snprintf(emm, 3, "%02d", em);
	snprintf(sym + n, QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX - n, "-%d-%s-%g-%s",
			em, eyy, *strike, *(buff + 19) > 'L' ? "P" : "C");

}

void getFutureSymbol(char *sym, char *root, char * buff, char * emm, char * eyy,
		char *edd) {
	int n = 3;
	memcpy(sym, buff + 13, 3);
	memcpy(root, sym, n);
	*(eyy + 1) = *(buff + 17);
	memcpy(edd, buff + 18, 2);
	int em = getFutExpMonth(*(buff + 16));
	snprintf(emm, 3, "%02d", em);
	snprintf(sym + n, QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX - n, "%c%s-%c",
			*(buff + 16), eyy, 'F');
}

void getStratSymbol(char *sym, char * buff, char * emm, char * eyy, char *edd) {
	int n = 0;
	memcpy(sym, buff + 13, 30);
	while (!isspace(*(sym + n++)) && n < QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX)
		;	
	*emm = '0';
	*(emm + 1) = '1';
	//int em = FUT_EXPIRY_MONTH( *(buff+44) );
	snprintf(sym + n - 1, QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX - n, "-%c", 'F');
}

void parseDepth(char * buff, QuoteLib__Depthquote * od) {
	char tbuff[6] = { 0 };
	struct timeval tv;
	od->bid = getPx(buff + 1, 6);
	memcpy(tbuff, buff + 8, 5);
	od->bidsize = atof(tbuff);
	memset(tbuff, 0, 6);
	memcpy(tbuff, buff + 13, 2);
	od->nbidord = atof(tbuff);
	od->ask = getPx(buff + 15, 6);
	memset(tbuff, 0, 6);
	memcpy(tbuff, buff + 22, 5);
	od->asksize = atof(tbuff);
	memset(tbuff, 0, 6);
	memcpy(tbuff, buff + 27, 2);
	od->naskord = atof(tbuff);
	od->level = *(buff) - '1';
	if (od->naskord) {
		gettimeofday(&tv, NULL);
		od->asktime = tv.tv_sec * 1000 + tv.tv_usec * 0.001;
	}
	if (od->nbidord) {
		gettimeofday(&tv, NULL);
		od->bidtime = tv.tv_sec * 1000 + tv.tv_usec * 0.001;
	}
}
void parseSTRDepth(char * buff, QuoteLib__Depthquote * od) {
	char tbuff[6] = { 0 };
	struct timeval tv;
	od->level = *(buff) - '1';
	if (od->level < 0)
		od->level = 0;
	od->bid = getPx(buff + 2, 6) * ((*(buff + 1) == '-') ? -1 : 1);
	memcpy(tbuff, buff + 9, 5);
	od->bidsize = atof(tbuff);
	memset(tbuff, 0, 6);
	memcpy(tbuff, buff + 14, 2);
	od->nbidord = atof(tbuff);
	od->ask = getPx(buff + 17, 6) * ((*(buff + 16) == '-') ? -1 : 1);
	memset(tbuff, 0, 6);
	memcpy(tbuff, buff + 24, 5);
	od->asksize = atof(tbuff);
	memset(tbuff, 0, 6);
	memcpy(tbuff, buff + 29, 2);
	od->naskord = atof(tbuff);
	if (od->naskord) {
		gettimeofday(&tv, NULL);
		od->asktime = tv.tv_sec * 1000 + tv.tv_usec * 0.001;
	}
	if (od->nbidord) {
		gettimeofday(&tv, NULL);
		od->bidtime = tv.tv_sec * 1000 + tv.tv_usec * 0.001;
	}
}
void parseHSVFmsg(char * buff, int blen, void * destfd, void * inproc,
		 const char * yy, htSecDef *htCSz) {
	//kvmsg_t *qmsg = kvmsg_new();
	char symbol[QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX + 1] = { 0 };
	char root[QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX + 1] = { 0 };
	//char dbug_buffer[1024] = { 0 };
	QuoteLib__BDMDepth * odpth = NULL;
	char csize[9] = { 0 };
	char seq_msg[10] = { 0 };
	char emm[3] = { 0 };
	char edd[3] = { 0 };
	char eyy[3] = { 0 };
	int seq_num = 0;
	int offset = 0;
	htSecDef *obj = NULL;
	QuoteLib__InstrDfn * idefn;
	QuoteLib__FutQuote * fquote;
	QuoteLib__OptQuote * oquote;
	float strike;
	int csz = 0;
	char req_size[10] = { 0 };
	memcpy(eyy,yy,sizeof(char));
	//int n = 0;
	if (!(*buff == '\002' && *(buff + blen) == '\003'))
		return;
#ifdef DEBUG
	memcpy(seq_msg, buff + 1, 9);
	seq_num = atoi(seq_msg);
#endif
	//printf("%d - create\n", seq_num);
	kvmsg_t *qmsg = kvmsg_new();
	kvmsg_t *qmsg_l1 = NULL;
	//memset(symbol,0,)
	//printf("New \n");
	//memcpy(dbug_buffer, buff, blen * sizeof(char));
	//printf("Will Parse: len[%d]->[%s]\n", blen, dbug_buffer);
	switch (*(buff + 10)) {
	case EOT:
		snprintf(symbol, 6, "EOD");
		kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__FO_END_OF_TRANS);
		QuoteLib__EndOfSales * eos = (QuoteLib__EndOfSales *) qmsg->value;
		snprintf(eos->time, 6, "%s", buff + 11);
		break;
	case OPT_DPT:
		if (*(buff + 11) == OPT || *(buff + 11) == FUT_OPT) {
			if (*(buff + 11) == OPT) {
				getOptionSymbol(symbol, root, buff, &strike, emm, eyy, edd);
				offset = 34;
			} else {
				getFutOptSymbol(symbol, root, buff, &strike, emm, eyy, edd);
				offset = 28;
			}
			kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__OPT_DEPTH);
			odpth = (QuoteLib__BDMDepth *) qmsg->value;

			snprintf(odpth->expiry, 9, "20%2s%2s%2s", eyy, emm, edd);

			odpth->strike = strike;
			odpth->has_strike = 1;
			HASH_FIND_STR(htCSz, root, obj);
			if (obj) {		// create new entry
				odpth->isamer = obj->value.type == 'A' ? 1 : 0;
			} else
				odpth->isamer = 1;
			odpth->has_isamer = 1;
			odpth->has_multiplier = 1;
			odpth->multiplier = 100;

		} else if (*(buff + 11) == FUT || *(buff + 11) == STR) {
			if (*(buff + 11) == FUT) {
				getFutureSymbol(symbol, root, buff, emm, eyy, edd);
				offset = 19;
			} else {
				getStratSymbol(symbol, buff, emm, eyy, edd);
				offset = 44;
			}
			kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__FUT_DEPTH);
			odpth = (QuoteLib__BDMDepth *) qmsg->value;
			if (*(buff + 11) == FUT)
				snprintf(odpth->expiry, 7, "20%s%2s", eyy, emm);

			HASH_FIND_STR(htCSz, root, obj);
			if (obj) {		// create new entry
				odpth->multiplier =
						obj->value.size == 0 ? 200 : obj->value.size;
			} else {
				odpth->multiplier = (*(buff + 11) == STR) ? 1 : 200;
			}
		}

		odpth->n_quote = *(buff + offset) - '0';
		int n = 0;
		offset++;
		odpth->quote = calloc(odpth->n_quote, sizeof(QuoteLib__Depthquote*));
		for (n = 0; n < odpth->n_quote; n++) {
			odpth->quote[n] = calloc(1, sizeof(QuoteLib__Depthquote));
			quote_lib__depthquote__init(odpth->quote[n]);
			if (*(buff + 11) == STR) {
				parseSTRDepth(buff + offset + n * 31, odpth->quote[n]);
			} else {
				parseDepth(buff + offset + n * 29, odpth->quote[n]);
			}
			if (odpth->quote[n]->level == 0) { // send the Top of book
				qmsg_l1 = kvmsg_new();
				if (*(buff + 11) == FUT || *(buff + 11) == STR) {
					kvmsg_init(qmsg_l1, symbol,
							QUOTE_LIB__MSG_TYPES__FUTURE_QUOTE);
					fquote = (QuoteLib__FutQuote *) qmsg_l1->value;
					memcpy(fquote->expiry, odpth->expiry, 6);
					fquote->multiplier = odpth->multiplier;
					fquote->bid = odpth->quote[n]->bid;
					fquote->ask = odpth->quote[n]->ask;
					fquote->bidsize = odpth->quote[n]->bidsize;
					fquote->asksize = odpth->quote[n]->asksize;
				} else if (*(buff + 11) == OPT || *(buff + 11) == FUT_OPT) {
					kvmsg_init(qmsg_l1, symbol,
							QUOTE_LIB__MSG_TYPES__OPTION_QUOTE);
					oquote = (QuoteLib__OptQuote *) qmsg_l1->value;
					memcpy(oquote->expiry, odpth->expiry, 8);
					oquote->strike = odpth->strike;
					oquote->isamer = odpth->isamer;
					oquote->multiplier = odpth->multiplier;
					oquote->bid = odpth->quote[n]->bid;
					oquote->ask = odpth->quote[n]->ask;
					oquote->asksize = odpth->quote[n]->asksize;
					oquote->bidsize = odpth->quote[n]->bidsize;
				}
				if (qmsg_l1->value) {
					kvmsg_dispatch(qmsg_l1, destfd, inproc);

				} else {
					kvmsg_destroy(&qmsg_l1);
				}
			}
		}
		break;
	case TRADE_MSG:
		switch (*(buff + 11)) {
		case BULL:

			break;
		case OPT:
			getOptionSymbol(symbol, root, buff, &strike, emm, eyy, edd);
			kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__OPTION_TRADE);
			QuoteLib__OptTrade * qtrd = (QuoteLib__OptTrade *) qmsg->value;
			qtrd->volume = getSize(buff + 33, 8);
			qtrd->last = getPx(buff + 41, 6);
			qtrd->chg = getPx(buff + 49, 6) * (*(buff + 48) == '+' ? 1 : -1);
			memcpy(qtrd->trdtime, buff + 62, 6);

			qtrd->oi = getSize(buff + 68, 7);
			qtrd->has_seq_num = 1;
			qtrd->seq_num = seq_num;
			HASH_FIND_STR(htCSz, root, obj);
			if (obj) {		// create new entry
				qtrd->isamer = obj->value.type == 'A' ? 1 : 0;
			}
			qtrd->trdmrkr = *(buff + 75);
			//qtrd->
			snprintf(qtrd->expiry, 9, "20%s%s%s", eyy, emm, edd);
			qtrd->strike = strike;		//getPx(buff + 41, 6);
			//printf("Trade:%s->%d@%g (%g:%d)\n", symbol, qtrd->volume,
			//		qtrd->last, qtrd->chg, qtrd->oi);
			break;
		case FUT_OPT:
			getFutOptSymbol(symbol, root, buff, &strike, emm, eyy, edd);
			kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__OPTION_TRADE);
			qtrd = (QuoteLib__OptTrade *) qmsg->value;
			//HASH_FIND_STR(htCSz, root, obj);
			qtrd->volume = getSize(buff + 27, 8);
			qtrd->last = getPx(buff + 35, 6);
			qtrd->chg = getPx(buff + 44, 6) * (*(buff + 43) == '+' ? 1 : -1);
			memcpy(qtrd->trdtime, buff + 62, 6);
			qtrd->oi = getSize(buff + 57, 7);
			qtrd->has_seq_num = 1;
			qtrd->seq_num = seq_num;
			HASH_FIND_STR(htCSz, root, obj);
			if (obj) {		// create new entry
				qtrd->isamer = obj->value.type == 'A' ? 1 : 0;
			}
			qtrd->trdmrkr = *(buff + 75);
			//qtrd->
			snprintf(qtrd->expiry, 9, "20%s%s%s", eyy, emm, edd);
			break;
		case FUT:
			getFutureSymbol(symbol, root, buff, emm, eyy, edd);
			kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__FUTURE_TRADE);
			QuoteLib__FutTrade * ftrd = (QuoteLib__FutTrade *) qmsg->value;
			ftrd->volume = getSize(buff + 18, 8);
			ftrd->last = getPx(buff + 26, 6);
			ftrd->chg = getPx(buff + 34, 6) * (*(buff + 33) == '+' ? 1 : -1);
			ftrd->has_seq_num = 1;
			ftrd->seq_num = seq_num;
			snprintf(ftrd->expiry, 7, "20%2s%2s", eyy, emm);
			if (*(ftrd->expiry + 7) == '0')
				*(ftrd->expiry + 6) = 0;
			memcpy(ftrd->trdtime, buff + 47, 6);
			ftrd->trdmrkr = *(buff + 53);
			//printf("Trade:%s %d@%g:%g\n", symbol, ftrd->volume, ftrd->last,
			//		ftrd->chg);
			break;
		case STR:
			//eyy[0] = *yy;
			getStratSymbol(symbol, buff, emm, eyy, edd);
			kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__FUTURE_TRADE);
			ftrd = (QuoteLib__FutTrade *) qmsg->value;
			ftrd->volume = getSize(buff + 43, 8);
			ftrd->last = getPx(buff + 52, 6);
			ftrd->chg = getPx(buff + 60, 6) * (*(buff + 59) == '+' ? 1 : -1);
			snprintf(ftrd->expiry, 7, "20%s%02d", yy, 1);// = getPx(buff + 60, 6) * (*(buff + 59) == '+' ? 1 : -1);
			memcpy(ftrd->trdtime, buff + 73, 6);
			ftrd->has_seq_num = 1;
			ftrd->seq_num = seq_num;
			break;
		}
		break;
	case OPT_QUO: // quotes
		switch (*(buff + 11)) {
		case OPT:
			getOptionSymbol(symbol, root, buff, &strike, emm, eyy, edd);
			kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__OPTION_QUOTE);
			QuoteLib__OptQuote * quote = (QuoteLib__OptQuote *) qmsg->value;
			HASH_FIND_STR(htCSz, root, obj);
			if (obj) {		// create new entry
				quote->multiplier =
						obj->value.size == 0 ? 100 : obj->value.size;
				quote->isamer = obj->value.type == 'A' ? 1 : 0;
			} else {
				quote->multiplier = 100;
			}
			quote->bid = getPx(buff + 33, 6);
			quote->bidsize = getSize(buff + 40, 5);
			quote->ask = getPx(buff + 45, 6);
			quote->asksize = getSize(buff + 52, 5);
			quote->has_seq_num = 1;
			quote->seq_num = seq_num;
			snprintf(quote->expiry, 9, "20%s%s%s", eyy, emm, edd);
			//printf("Quote:%s %d@%g-%g@%d\n", symbol, quote->bidsize, quote->bid,
			//		quote->ask, quote->asksize);
			//kvmsg_init(qmsg, key_name, MSG_TYPES__INDEX);
			break;
		case FUT_OPT:
			getFutOptSymbol(symbol, root, buff, &strike, emm, eyy, edd);
			kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__OPTION_QUOTE);
			QuoteLib__OptQuote *oquote = (QuoteLib__OptQuote *) qmsg->value;
			HASH_FIND_STR(htCSz, root, obj);
			if (obj) {		// create new entry
				oquote->multiplier =
						obj->value.size == 0 ? 100 : obj->value.size;
				oquote->isamer = obj->value.type == 'A' ? 1 : 0;
			} else {
				oquote->multiplier = 100;
			}
			oquote->bid = getPx(buff + 27, 6);
			oquote->bidsize = getSize(buff + 34, 5);
			oquote->ask = getPx(buff + 39, 6);
			oquote->asksize = getSize(buff + 46, 5);
			oquote->has_seq_num = 1;
			oquote->seq_num = seq_num;
			break;
		case FUT:
			getFutureSymbol(symbol, root, buff, emm, eyy, edd);
			kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__FUTURE_QUOTE);
			QuoteLib__FutQuote * fquote = (QuoteLib__FutQuote *) qmsg->value;
			fquote->bid = getPx(buff + 18, 6);
			fquote->bidsize = getSize(buff + 25, 5);
			fquote->ask = getPx(buff + 30, 6);
			fquote->asksize = getSize(buff + 37, 5);
			fquote->has_seq_num = 1;
			fquote->seq_num = seq_num;
			snprintf(fquote->expiry, 9, "20%s%s%s", eyy, emm, edd);
			if (*(fquote->expiry + 7) == '0')
				*(fquote->expiry + 6) = 0;
			HASH_FIND_STR(htCSz, root, obj);
			if (obj) {		// create new entry
				fquote->multiplier =
						obj->value.size == 0 ? 200 : obj->value.size;
				//fquote->isamer = obj->value.type == 'A' ? 1 : 0;
			} else {
				quote->multiplier = 200;
			}
			break;
		case STR:
			//eyy[0] = *yy;
			getStratSymbol(symbol, buff, emm, eyy, edd);
			kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__FUTURE_QUOTE);
			fquote = (QuoteLib__FutQuote *) qmsg->value;
			fquote->bid = getPx(buff + 44, 6)
					* (*(buff + 43) == '+' ? 1.0 : -1.0);
			fquote->bidsize = getSize(buff + 51, 5);
			fquote->ask = getPx(buff + 57, 6)
					* (*(buff + 56) == '+' ? 1.0 : -1.0);
			fquote->asksize = getSize(buff + 64, 5);
			fquote->has_seq_num = 1;
			fquote->ishalted =
					(*(buff + 69) == 'T' || *(buff + 69) == ' ') ? 0 : 1;
			fquote->seq_num = seq_num;
			snprintf(fquote->expiry, 7, "20%s", yy);
			//if (*(fquote->expiry + 7) == '0')
			//	*(fquote->expiry + 6) = 0;
			HASH_FIND_STR(htCSz, root, obj);
			if (obj) {		// create new entry
				fquote->multiplier =
						obj->value.size == 0 ? 200 : obj->value.size;
				//fquote->isamer = obj->value.type == 'A' ? 1 : 0;
			} else {
				fquote->multiplier = 1;
			}
			break;
		}
		break;
	case OPT_IKM:
		// mainly to check for non-standard contract sizes
		switch (*(buff + 11)) {
		case OPT:
			memcpy(csize, buff + 120, 8);
			csz = atoi(csize);
			memset(root, 0, QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX);
			getOptionSymbol(symbol, root, buff, &strike, emm, eyy, edd);
			kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__OPTION_DFN);
			idefn = (QuoteLib__InstrDfn *) qmsg->value;
			if (csz != 100 || *(buff + 69) != 'A') {
				HASH_FIND_STR(htCSz, root, obj);
				if (!obj) {		// create new entry
					obj = (htSecDef *) calloc(1, sizeof(htSecDef));
					if (obj) {
						strcpy(obj->key, root);	// MSG_TYPES__KVMSG_KEY_MAX);
						obj->value.size = csz;
						obj->value.type = *(buff + 69);
						HASH_ADD_STR(htCSz, key, obj);
						//printf("Added %s->%d %c\n", obj->key, obj->value.size,
						//		obj->value.type);
					}
				}
			}
			idefn->consize = csz;
			//char type;
			//sscanf("%s-%d-%d-%f-%c",root,&idefn->exmnth,&idefn->exyr,&idefn->strike,&type);
			idefn->exmnth = atoi(emm);
			idefn->exyr = atoi(eyy) + 2000;
			idefn->strike = strike;
			idefn->has_exmnth = 1;
			idefn->has_strike = 1;
			idefn->has_exyr = 1;
			idefn->expday = atoi(edd);
			idefn->has_expday = 1;
			idefn->has_isamer = 1;
			idefn->isamer = *(buff + 69) != 'A' ? 0 : 1;
			snprintf(idefn->strikeccy, 4, "%s", buff + 33);
			idefn->maxordersz = getSize(buff + 36, 6);
			idefn->minordersz = getSize(buff + 42, 6);
			idefn->maxpxthresh = getPx(buff + 48, 6);
			idefn->minpxthresh = getPx(buff + 55, 6);
			idefn->tickinc = getPx(buff + 62, 6);
			snprintf(idefn->type, 3, "%s", buff + 70);
			snprintf(idefn->group, 3, "%s", buff + 72);
			snprintf(idefn->instr, 5, "%s", buff + 74);
			idefn->externid = calloc(31, sizeof(char));
			snprintf(idefn->externid, 31, "%s", buff + 78);
			idefn->tick = getPx(buff + 128, 6);
			//snprintf(idefn->ulccy)
			//printf("%c%c:%s %d\n", OPT_IKM, OPT, symbol, csz);
			break;
		case FUT:
			memcpy(csize, buff + 91, 8);
			csz = atoi(csize);
			///eyy[0] = *yy;
			//memset(root, 0, QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX);
			getFutureSymbol(symbol, root, buff, emm, eyy, edd);
			kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__OPTION_DFN);
			idefn = (QuoteLib__InstrDfn *) qmsg->value;
			HASH_FIND_STR(htCSz, root, obj);
			if (!obj) {		// create new entry
				obj = (htSecDef *) calloc(1, sizeof(htSecDef));
				if (obj) {
					strcpy(obj->key, root);		// MSG_TYPES__KVMSG_KEY_MAX);
					obj->value.size = csz;
					HASH_ADD_STR(htCSz, key, obj);
					//printf("Added %s->%d %c\n", obj->key, obj->value.size,
					//		obj->value.type);
				}
			}
			idefn->exmnth = atoi(emm);
			idefn->exyr = atoi(eyy) + 2000;
			idefn->strike = 0;
			idefn->has_exmnth = 1;
			idefn->has_strike = 1;
			idefn->has_exyr = 1;
			idefn->consize = csz;
			idefn->expday = atoi(edd);
			idefn->has_expday = 1;
			//idefn->isamer = *(buff + 69) != 'A' ? 0 : 1;
			snprintf(idefn->strikeccy, 4, "%s", buff + 106);
			idefn->maxordersz = getSize(buff + 20, 6);
			idefn->minordersz = getSize(buff + 26, 6);
			idefn->maxpxthresh = getPx(buff + 32, 6);
			idefn->minpxthresh = getPx(buff + 39, 6);
			snprintf(idefn->type, 3, "%s", buff + 53);
			snprintf(idefn->group, 3, "%s", buff + 55);
			snprintf(idefn->instr, 5, "%s", buff + 57);
			idefn->externid = calloc(31, sizeof(char));
			snprintf(idefn->externid, 31, "%s", buff + 61);
			idefn->tick = getPx(buff + 99, 6);
			idefn->tickinc = getPx(buff + 46, 6);
			//printf("%c%c:%s -> %d\n", OPT_IKM, OPT, symbol, csz);
			break;
		case FUT_OPT:
			memcpy(csize, buff + 103, 8);
			csz = atoi(csize);
			//eyy[0] = *yy;
			getFutOptSymbol(symbol, root, buff, &strike, emm, eyy, edd);
			kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__OPTION_DFN);
			idefn = (QuoteLib__InstrDfn *) qmsg->value;
			HASH_FIND_STR(htCSz, root, obj);
			if (!obj) {		// create new entry
				obj = (htSecDef *) calloc(1, sizeof(htSecDef));
				if (obj) {
					strcpy(obj->key, root);	// MSG_TYPES__KVMSG_KEY_MAX);
					obj->value.size = csz;
					obj->value.type = *(buff + 69);
					HASH_ADD_STR(htCSz, key, obj);
				}
			}
			idefn->consize = csz;
			//char type;
			//sscanf("%s-%d-%d-%f-%c",root,&idefn->exmnth,&idefn->exyr,&idefn->strike,&type);
			idefn->exmnth = atoi(emm);
			idefn->exyr = atoi(eyy) + 2000;
			idefn->strike = strike;
			idefn->has_strike = 1;
			idefn->has_exyr = 1;
			idefn->expday = atoi(edd);
			idefn->has_expday = 1;
			idefn->has_isamer = 1;
			//idefn->isamer = *(buff + 69) != 'A' ? 0 : 1;
			snprintf(idefn->strikeccy, 4, "%s", buff + 29);
			idefn->maxordersz = getSize(buff + 32, 6);
			idefn->minordersz = getSize(buff + 38, 6);
			idefn->maxpxthresh = getPx(buff + 44, 6);
			idefn->minpxthresh = getPx(buff + 51, 6);
			idefn->tickinc = getPx(buff + 58, 6);
			snprintf(idefn->type, 3, "%s", buff + 67);
			snprintf(idefn->group, 3, "%s", buff + 69);
			snprintf(idefn->instr, 5, "%s", buff + 71);
			idefn->externid = calloc(31, sizeof(char));
			snprintf(idefn->externid, 31, "%s", buff + 73);
			idefn->tick = getPx(buff + 111, 6);
			//}
			//printf("%c%c:%s %d\n", OPT_IKM, OPT, symbol, csz);
			break;
		case STR:
			//eyy[0] = *yy;
			getStratSymbol(symbol, buff, emm, eyy, edd);
			kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__OPTION_DFN);
			idefn = (QuoteLib__InstrDfn *) qmsg->value;
			snprintf(eyy + 1, 2, "%s", buff + 43);
			idefn->exmnth = getFutExpMonth(buff + 44);
			snprintf(edd, 3, "%s", buff + 45);
			idefn->exyr = atoi(eyy) + 2000;
			idefn->consize = 1;
			idefn->strike = 0;
			idefn->has_strike = 1;
			idefn->has_exmnth = 1;
			idefn->has_exyr = 1;
			idefn->expday = atoi(edd);
			idefn->has_expday = 1;
			//idefn->isamer = *(buff + 69) != 'A' ? 0 : 1;
			//snprintf(idefn->strikeccy, 4, "%s", buff + 106);
			idefn->maxordersz = getSize(buff + 47, 6);
			idefn->minordersz = getSize(buff + 53, 6);
			idefn->maxpxthresh = getPx(buff + 59, 6);
			idefn->minpxthresh = getPx(buff + 66, 6);
			idefn->tickinc = getPx(buff + 73, 6);
			snprintf(idefn->type, 3, "%s", buff + 80);
			snprintf(idefn->group, 3, "%s", buff + 82);
			snprintf(idefn->instr, 5, "%s", buff + 84);
			idefn->externid = calloc(31, sizeof(char));
			snprintf(idefn->externid, 31, "%s", buff + 88);
			//idefn->tick = getPx(buff + 99, 6);
			//printf("%c%c:%s\n", OPT_IKM, STR, symbol);
			break;
		}
		break;
	case OPT_SUM:  // Summary Messages
		switch (*(buff + 11)) {
		case OPT:
			//memset(root, 0, QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX);
			getOptionSymbol(symbol, root, buff, &strike, emm, eyy, edd);
			kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__OPTION_STAT);
			QuoteLib__OptStat * ostat = (QuoteLib__OptStat *) qmsg->value;
			snprintf(ostat->expiry, 9, "20%s%s%s", eyy, emm, edd);
			ostat->strike = strike;
			ostat->bid = getPx(buff + 33, 6);
			ostat->bidsize = getSize(buff + 40, 5);
			ostat->ask = getPx(buff + 45, 6);
			ostat->asksize = getSize(buff + 52, 5);
			ostat->last = getPx(buff + 57, 6);
			ostat->oi = getSize(buff + 64, 7);
			ostat->volume = getSize(buff + 72, 8);
			ostat->chg = getPx(buff + 81, 6) * (*(buff + 80) == '+' ? 1 : -1);
			ostat->open = getPx(buff + 87, 6);
			ostat->high = getPx(buff + 93, 6);
			ostat->low = getPx(buff + 99, 6);
			ostat->has_seq_num = 1;
			ostat->seq_num = seq_num;
			break;
		case FUT:
			//000201989NFQBAXH409875030359309876030030709876030987603098760309875030000000+00002030000544909874030115668
			getFutureSymbol(symbol, root, buff, emm, eyy, edd);
			kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__FUTURE_STAT);
			QuoteLib__FutStat * fstat = (QuoteLib__FutStat *) qmsg->value;
			snprintf(fstat->expiry, 7, "20%2s%2s", eyy, emm);
			fstat->bid = getPx(buff + 18, 6);
			fstat->bidsize = getSize(buff + 25, 5);
			fstat->ask = getPx(buff + 30, 6);
			fstat->asksize = getSize(buff + 37, 5);
			fstat->last = getPx(buff + 42, 6);
			fstat->open = getPx(buff + 49, 6);
			fstat->high = getPx(buff + 56, 6);
			fstat->low = getPx(buff + 63, 6);
			fstat->settle = getPx(buff + 70, 6);
			fstat->chg = getPx(buff + 77, 6) * ((*(buff + 84) == '-') ? -1 : 1);
			fstat->volume = getSize(buff + 85, 8);
			fstat->psettle = getPx(buff + 93, 6);
			fstat->oi = getSize(buff + 100, 7);
			fstat->has_seq_num = 1;
			fstat->seq_num = seq_num;
			break;
		case STR:

			break;
		}
		break;
	case OPT_RFQ:
		switch (*(buff + 11)) {
		case OPT:
			getOptionSymbol(symbol, root, buff, &strike, emm, eyy, edd);
			kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__OPTION_RFQ);
			QuoteLib__OptRFQ * orfq = (QuoteLib__OptRFQ *) qmsg->value;
			orfq->strike = strike;
			snprintf(orfq->expiry, 9, "20%s%s%s", eyy, emm, edd);
			HASH_FIND_STR(htCSz, root, obj);
			if (obj) {		// create new entry
				orfq->multiplier = obj->value.size == 0 ? 100 : obj->value.size;
				orfq->isamer = obj->value.type == 'A' ? 1 : 0;
			} else {
				orfq->multiplier = 100;
				orfq->isamer = 1;
			}

			snprintf(req_size, 9, "%s", buff + 32);
			orfq->size = atoi(req_size);
			break;
		case FUT_OPT:
			getFutOptSymbol(symbol, root, buff, &strike, emm, eyy, edd);
			kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__OPTION_RFQ);
			orfq = (QuoteLib__OptRFQ *) qmsg->value;
			orfq->strike = strike;
			snprintf(orfq->expiry, 9, "20%s%s%s", eyy, emm, edd);
			HASH_FIND_STR(htCSz, root, obj);
			if (obj) {		// create new entry
				orfq->multiplier = obj->value.size == 0 ? 100 : obj->value.size;
				orfq->isamer = obj->value.type == 'A' ? 1 : 0;
			} else {
				orfq->multiplier = 100;
				orfq->isamer = 1;
			}
			//char req_size[9] = { 0 };
			memcpy(req_size, buff + 26, 9);
			orfq->size = atoi(req_size);
			break;
		case FUT:
			getFutureSymbol(symbol, root, buff, emm, eyy, edd);
			kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__FUTURE_RFQ);
			QuoteLib__FutRFQ * frfq = (QuoteLib__FutRFQ *) qmsg->value;
			//frfq->strike = strike;
			snprintf(frfq->expiry, 8, "20%s%s", eyy, emm);
			HASH_FIND_STR(htCSz, root, obj);
			if (obj) {		// create new entry
				frfq->multiplier = obj->value.size == 0 ? 100 : obj->value.size;

			} else {
				frfq->multiplier = 100;
			}
			//har req_size[9] = { 0 };
			snprintf(req_size, 9, "%s", buff + 18);
			frfq->size = atoi(req_size);
			break;
		case STR:
			getFutureSymbol(symbol, root, buff, emm, eyy, edd);
			kvmsg_init(qmsg, symbol, QUOTE_LIB__MSG_TYPES__FUTURE_RFQ);
			frfq = (QuoteLib__FutRFQ *) qmsg->value;
			//frfq->strike = strike;
			//char req_size[9] = { 0 };
			memcpy(req_size, buff + 43, 8);
			frfq->size = atoi(req_size);
			break;
		}
		break;
	}
	if (qmsg->value) {
		//printf("Sending \n");
		kvmsg_dispatch(qmsg, destfd, inproc);
		//kvmsg_send(qmsg, destfd);
	} else {
		kvmsg_destroy(&qmsg);
	}
}

void parseHSVF(char * buff, int blen, void * destfd, void * inproc,
		int * seq_num, const char *yy, htSecDef *htCSz, int use_std) {
	int beg = 0;
	int tlen = 0;
	do {
		char*loc = strchrnul(buff + beg, 0x3);
		tlen = loc - (buff + beg);
		if (use_std == 1) {
			char tbuff[CONFIG_READER_BUF_SZ + 1] = { 0 };
			if (*(buff + beg) != 0x3) {
				memcpy(tbuff, buff + beg, tlen + 1);
				printf("%s\n", tbuff);
			}
		} else {
			parseHSVFmsg(buff + beg, tlen, destfd, inproc, yy, htCSz);
		}
		beg += (tlen + 1);
	} while (beg <= blen && *(buff + beg) != 0);
}

