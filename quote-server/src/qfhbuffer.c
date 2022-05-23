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
// Monitor quote feed handler

#include "qfhbuffer.h"
#include <math.h>
#include <time.h>
#include <limits.h>
#include "zmq.h"
#include "qfl1.h"
#define TOTAL 0
#define TRADE 1
#define QOUTE 2
#define REQUESTS 3

void *
qfbuffer(void *thrdarg) {
	sqfhbuffer *pargs;
	// Equity
	QuoteLib__Quote *q;
	QuoteLib__Trade *t;
	QuoteLib__Index *ind;
	QuoteLib__Dividend *div;
	QuoteLib__StkStatus * stkstate;
	QuoteLib__EquitySummary *eqs;
	QuoteLib__QfSymbStatus *symstatus;
	QuoteLib__Moc * moc;
	// Futures & options
	QuoteLib__FutQuote *fquote;
	QuoteLib__FutTrade *ftrade;
	QuoteLib__FutStat *fstat;
	QuoteLib__OptQuote *oquote;
	QuoteLib__OptTrade *otrade;
	QuoteLib__OptStat *ostat;
	QuoteLib__BDMDepth *depth;
	QuoteLib__BDMDepth *dMsg;

	htUTS * obj_lu = NULL;
	htUTS * buff = NULL;
	smkt_eq_stat *eqstat = NULL;
	smkt_fo_stat *fuopstat = NULL;
	struct timespec tm1, tm12, tm2, tm22;
	time_t timer;
	struct tm* tm_info;
	int seq_num = 0;
	int bEqtyHaveOpened = 0;
	int bHaveClosed = 0;
	pargs = (sqfhbuffer *) thrdarg;
	char cmsg[3] = { 0 };
	char reqstr[QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX + 1] = { 0 };
	char symbol[QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX + 1] = { 0 };
	char floc[FPATH_LEN + 1] = { 0 };
	FILE *FPs[F_TYPES_NUM] = { 0 };
	const char * file_names[] = { F_DES_SOD, F_DES_EOD, F_DES_DIV, F_DES_MOC,
	F_DES_FUT_EOD,
	F_DES_FUT_STAT };
	zmq_msg_t req;
	pargs->buffer = buff;
	int isEquity = 1;
	int linger = 0;
	int HWM = MSG_HWM;
	time(&timer);
	tm_info = localtime(&timer);
	char yymmdd[7] = { 0 };
	strftime(yymmdd, SZ_DATE, "%y%m%d", tm_info);
	if (pargs->fpath) {
		int n = 0;
		for (n = 0; n < F_TYPES_NUM; n++) {
			snprintf(floc, FPATH_LEN, "%s/%s_%s", pargs->fpath, yymmdd,
					file_names[n]);
			if (pargs->type == FEED_TYPE_QRTM1
					|| pargs->type == FEED_TYPE_QRTM1_F) {
				if (n == F_TYPES_SOD || n == F_TYPES_DIV || n == F_TYPES_EOD
						|| n == F_TYPES_DIV || n == F_TYPES_MOC)
					FPs[n] = fopen(floc, "w+");
				switch (n) {
				case F_TYPES_SOD:
					fprintf(FPs[n],
							"Symbol,Exch,Cusip,Issuer,Last Sale,MOC,Type,Stock Grp,Lot Sz,Face Value\n");

					break;
				case F_TYPES_EOD:
					fprintf(FPs[n],
							"Symbol,CUSIP,Name,Bid,BidSz,Ask,AskSz,Last,LastSz,Hi,Low,Volume,Trades,Value,Open,Close,PrevClose,Moc,LowTime,HighTime,StkGrp,moceligible,IMO\n");

					break;
				case F_TYPES_DIV:
					fprintf(FPs[n],
							"Symbol,Dividend,ExDate,PayDate,RecordDate,Notes\n");

					break;
				case F_TYPES_MOC:
					fprintf(FPs[n], "Symbol,Imbalance,BidSz,Bid,Ask,Asksz\n");
					break;
				}
			} else if (pargs->type == FEED_TYPE_HSVF) {
				if (n == F_TYPES_FUT_STAT || n == F_TYPES_FUT_EOD)
					FPs[n] = fopen(floc, "w+");
				switch (n) {
				case F_TYPES_FUT_STAT:
					fprintf(FPs[n],
							"Symbol,Exp. Year,Exp. Month,Exp. Day,Strike,Multiplier,Type,Identifier\n");
					break;
				case F_TYPES_FUT_EOD:
					fprintf(FPs[n],
							"Symbol,Expiry,Strike,Multiplier,Type,Bid,BidSz,Ask,AskSz,Last,LastSz,Hi,Low,Volume\n");
					break;
				}
			}
			memset(floc, 0, FPATH_LEN);
		}
	}

	void *rr_socket = zsocket_new(pargs->zmq_ctx, ZMQ_ROUTER);
	zmq_setsockopt(rr_socket, ZMQ_RCVHWM, &HWM, sizeof(int));
	zmq_setsockopt(rr_socket, ZMQ_LINGER, &linger, sizeof(int));
	void *ip_socket = zsocket_new(pargs->zmq_ctx, ZMQ_PAIR);
	if (zsocket_connect(ip_socket, pargs->inproc_sock) == -1) {
		printf("Unable to bind to  %s - exiting, Error %s\n",
				pargs->inproc_sock, strerror(errno));
		pargs->brun = 0;
		exit(EXIT_FAILURE);
	}
	if (zsocket_bind(rr_socket, pargs->puburl) == -1) {
		printf("Unable to bind to  %s - exiting, Error %s\n", pargs->puburl,
				strerror(errno));
		pargs->brun = 0;
		exit(EXIT_FAILURE);
	}
	zmq_setsockopt(ip_socket, ZMQ_SUBSCRIBE, "", 0);
	__sync_fetch_and_add(&pargs->brun, 1);	// =1;
	
//  Initialize poll set
	zmq_pollitem_t items[] = { { rr_socket, 0, ZMQ_POLLIN, 0 }, { ip_socket, 0,
	ZMQ_POLLIN, 0 } };
	kvmsg_t *qmsg;
	kvmsg_t *rmsg;
	int numOpenGrps = 0;
	printf("Staring the Memory Store\n");
	while (pargs->brun == 1) {		
		zmq_poll(items, 2, 1000); // timout in miliseconds
		clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tm2);
		if (items[0].revents & ZMQ_POLLIN) { // request reply
			zmq_msg_init(&req);
			int len = zmq_msg_recv(&req, rr_socket, 0);
			rmsg = kvmsg_new();
			if (len != -1) {
				memcpy(cmsg, zmq_msg_data(&req), 2);
				if (cmsg[0] != 0 && cmsg[1] != 107) {
					memset(symbol, 0, QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX);
					memcpy(symbol, zmq_msg_data(&req) + 3, len - 4);
					int mtype = atoi(cmsg);
					HASH_FIND_STR(pargs->buffer, symbol, obj_lu);
					if (len > 6 && symbol[len - 6] == '-'
							&& (symbol[len - 5] == 'C' || symbol[len - 5] == 'P'
									|| symbol[len - 5] == 'F')) {
						if (obj_lu) {
							fuopstat = (smkt_fo_stat *) obj_lu->value;
						} else {
							fuopstat = NULL;
						}
						isEquity = 0;
					} else {
						if (obj_lu) {
							eqstat = (smkt_eq_stat *) obj_lu->value;
						} else {
							eqstat = NULL;
						}
						isEquity = 1;
					}

					switch (mtype) {
					case QUOTE_LIB__MSG_TYPES__QUOTE:
						if (symbol[0] == 'I' && symbol[1] == '-') {
							kvmsg_init(rmsg, symbol,
									QUOTE_LIB__MSG_TYPES__INDEX);
							QuoteLib__Index * ind =
									(QuoteLib__Index *) rmsg->value;
							ind->bid = eqstat ? eqstat->bid : 0;
							ind->has_bid = 1;
							ind->ask = eqstat ? eqstat->ask : 0;
							ind->has_ask = 1;							
						} else {
							kvmsg_init(rmsg, symbol,
									QUOTE_LIB__MSG_TYPES__QUOTE);
							QuoteLib__Quote * quote =
									(QuoteLib__Quote *) rmsg->value;
							quote->ask = eqstat ? eqstat->ask : 0;
							quote->bid = eqstat ? eqstat->bid : 0;
							quote->asksize = eqstat ? eqstat->asksz : 0;
							quote->bidsize = eqstat ? eqstat->bidsz : 0;
						}
						break;
					case QUOTE_LIB__MSG_TYPES__TRADE:
						if (symbol[0] == 'I' && symbol[1] == '-') {
							kvmsg_init(rmsg, symbol,
									QUOTE_LIB__MSG_TYPES__INDEX);
							QuoteLib__Index * ind =
									(QuoteLib__Index *) rmsg->value;
							ind->has_last = 1;
							ind->last = eqstat ? eqstat->last : 0;
							ind->has_volume = 1;
							ind->volume = eqstat ? eqstat->volume : 0;							
						} else {
							kvmsg_init(rmsg, symbol,
									QUOTE_LIB__MSG_TYPES__TRADE);
							QuoteLib__Trade * trade =
									(QuoteLib__Trade *) rmsg->value;
							trade->last = eqstat ? eqstat->last : NAN;
							trade->volume = eqstat ? eqstat->lastsz : 0;
							memcpy(trade->trdtime,
									eqstat ? eqstat->trdtime : "093000",
									SZ_DATE - 1);
							trade->has_buyerid = 1; //obj ? obj->value.buyerid : 0;
							trade->has_sellerid = 1; // obj ? obj->value.sellerid : 0;
							trade->buyerid = eqstat ? eqstat->buyerid : 0;
							trade->sellerid = eqstat ? eqstat->sellerid : 0;							
						}
						break;
					case QUOTE_LIB__MSG_TYPES__MKT_STAT:
						kvmsg_init(rmsg, symbol,
								QUOTE_LIB__MSG_TYPES__MKT_STAT);
						QuoteLib__MktStat * stat =
								(QuoteLib__MktStat *) rmsg->value;
						stat->open = eqstat ? eqstat->open : NAN;
						stat->close = eqstat ? eqstat->close : NAN;
						stat->pclose = eqstat ? eqstat->pclose : NAN;
						stat->high = eqstat ? eqstat->hi : NAN;
						stat->low = eqstat ? eqstat->low : NAN;
						stat->state =
								eqstat ?
										eqstat->state :
										QUOTE_LIB__STOCK_STATUS__A;
						stat->moc = eqstat ? eqstat->moc : 0;
						stat->stkgrp = eqstat ? eqstat->stkgrp : -1;
						memcpy(stat->hightime,
								eqstat ? eqstat->hightime : "093000",
								SZ_DATE - 1);
						memcpy(stat->lowtime,
								eqstat ? eqstat->lowtime : "093000",
								SZ_DATE - 1);
						stat->volume = eqstat ? eqstat->volume : 0;
						stat->value = eqstat ? eqstat->value : 0;
						stat->stkgrp = eqstat ? eqstat->stkgrp : 0;
						stat->has_stkgrp = 1;
						break;
					case QUOTE_LIB__MSG_TYPES__FUTURE_QUOTE:
						kvmsg_init(rmsg, symbol,
								QUOTE_LIB__MSG_TYPES__FUTURE_QUOTE);
						QuoteLib__FutQuote * fquote =
								(QuoteLib__FutQuote *) rmsg->value;
						fquote->bid = fuopstat ? fuopstat->bid : 0;
						fquote->bidsize = fuopstat ? fuopstat->bidsz : 0;
						fquote->ask = fuopstat ? fuopstat->ask : 0;
						fquote->asksize = fuopstat ? fuopstat->asksz : 0;
						snprintf(fquote->expiry, 7, "%s",
								fuopstat ?
										(fuopstat->expiry[0] == 0 ?
												"200101" : fuopstat->expiry) :
										"200101");
						fquote->multiplier =
								fuopstat ? fuopstat->multiplier : 0;
						if (fuopstat && fuopstat->isHalted)
							fquote->ishalted = fuopstat->isHalted;
						seq_num = fquote->seq_num;
						break;
					case QUOTE_LIB__MSG_TYPES__FUTURE_TRADE:
						kvmsg_init(rmsg, symbol,
								QUOTE_LIB__MSG_TYPES__FUTURE_TRADE);
						ftrade = (QuoteLib__FutTrade *) rmsg->value;
						ftrade->chg = fuopstat ? fuopstat->chg : 0;
						ftrade->last = fuopstat ? fuopstat->last : NAN;
						ftrade->multiplier =
								fuopstat ? fuopstat->multiplier : 0;
						snprintf(ftrade->expiry, 7, "%s",
								fuopstat ?
										(fuopstat->expiry[0] == 0 ?
												"200101" : fuopstat->expiry) :
										"200101");
						seq_num = ftrade->seq_num;

						break;
					case QUOTE_LIB__MSG_TYPES__FUTURE_STAT:
						kvmsg_init(rmsg, symbol,
								QUOTE_LIB__MSG_TYPES__FUTURE_STAT);

						fstat = (QuoteLib__FutStat *) rmsg->value;
						fstat->last = fuopstat ? fuopstat->last : NAN;
						fstat->open = fuopstat ? fuopstat->open : NAN;
						fstat->high = fuopstat ? fuopstat->hi : NAN;
						fstat->low = fuopstat ? fuopstat->low : NAN;
						fstat->open = fuopstat ? fuopstat->open : NAN;
						fstat->settle = fuopstat ? fuopstat->close : NAN;
						fstat->psettle = fuopstat ? fuopstat->pclose : NAN;
						snprintf(fstat->hitime, 7,
								fuopstat ? fuopstat->hightime : "080001");
						snprintf(fstat->lotime, 7,
								fuopstat ? fuopstat->lowtime : "080001");
						snprintf(fstat->expiry, 7, "%s",
								fuopstat ? fuopstat->expiry : "200101");
						fstat->volume = fuopstat ? fuopstat->volume : 0;
						fstat->value = fuopstat ? fuopstat->value : 0;
						fstat->oi = fuopstat ? fuopstat->oi : 0;
						fstat->multiplier = fuopstat ? fuopstat->multiplier : 0;

						//snprintf(fstat->high
						break;
					case QUOTE_LIB__MSG_TYPES__OPTION_QUOTE:
						kvmsg_init(rmsg, symbol,
								QUOTE_LIB__MSG_TYPES__OPTION_QUOTE);
						oquote = (QuoteLib__OptQuote *) rmsg->value;
						oquote->bid = fuopstat ? fuopstat->bid : 0;
						oquote->bidsize = fuopstat ? fuopstat->bidsz : 0;
						oquote->ask = fuopstat ? fuopstat->ask : 0;
						oquote->asksize = fuopstat ? fuopstat->asksz : 0;
						snprintf(oquote->expiry, 9, "%s",
								fuopstat ? fuopstat->expiry : "20010101");
						oquote->multiplier =
								fuopstat ? fuopstat->multiplier : 0;
						oquote->isamer = fuopstat ? fuopstat->isAmer : 1;
						oquote->strike = fuopstat ? fuopstat->strike : 0;
						if (fuopstat && fuopstat->isHalted)
							oquote->ishalted = fuopstat->isHalted;
						break;
					case QUOTE_LIB__MSG_TYPES__OPTION_TRADE:
						kvmsg_init(rmsg, symbol,
								QUOTE_LIB__MSG_TYPES__OPTION_TRADE);
						otrade = (QuoteLib__OptTrade *) rmsg->value;
						otrade->chg = fuopstat ? fuopstat->chg : 0;
						otrade->last = fuopstat ? fuopstat->last : NAN;
						otrade->volume = fuopstat ? fuopstat->lastsz : 0;
						otrade->multiplier =
								fuopstat ? fuopstat->multiplier : 0;
						otrade->oi = fuopstat ? fuopstat->oi : 0;
						snprintf(otrade->trdtime, 7, "%s",
								fuopstat ? fuopstat->trdtime : "080001");
						otrade->strike = fuopstat ? fuopstat->strike : 0;
						otrade->isamer = fuopstat ? fuopstat->isAmer : 1;
						break;
					case QUOTE_LIB__MSG_TYPES__OPTION_STAT:
						kvmsg_init(rmsg, symbol,
								QUOTE_LIB__MSG_TYPES__OPTION_STAT);
						ostat = (QuoteLib__OptStat *) rmsg->value;
						ostat->open = fuopstat ? fuopstat->open : NAN;
						ostat->high = fuopstat ? fuopstat->hi : NAN;
						ostat->low = fuopstat ? fuopstat->low : NAN;
						ostat->open = fuopstat ? fuopstat->open : NAN;
						ostat->close = fuopstat ? fuopstat->close : NAN;
						ostat->pdclose = fuopstat ? fuopstat->pclose : NAN;
						ostat->last = fuopstat ? fuopstat->last : NAN;
						snprintf(ostat->hitime, 7,
								fuopstat ? fuopstat->hightime : "080001");
						snprintf(ostat->lotime, 7,
								fuopstat ? fuopstat->lowtime : "080001");
						snprintf(ostat->expiry, 9, "%s",
								fuopstat ? fuopstat->expiry : "200101");
						ostat->volume = fuopstat ? fuopstat->volume : 0;
						ostat->value = fuopstat ? fuopstat->value : 0;
						ostat->oi = fuopstat ? fuopstat->oi : 0;
						ostat->isamer = fuopstat ? fuopstat->isAmer : 0;
						ostat->strike = fuopstat ? fuopstat->strike : 0;
						break;
					case QUOTE_LIB__MSG_TYPES__OPT_DEPTH:
						kvmsg_init(rmsg, symbol,
								QUOTE_LIB__MSG_TYPES__OPT_DEPTH);
						depth = (QuoteLib__BDMDepth *) rmsg->value;
						memcpy(depth->expiry,
								fuopstat ? fuopstat->expiry : "20010101", 8);
						depth->isamer = fuopstat ? fuopstat->isAmer : 0;
						depth->strike = fuopstat ? fuopstat->strike : 0;
						/* no break */
					case QUOTE_LIB__MSG_TYPES__FUT_DEPTH:
						if (mtype == QUOTE_LIB__MSG_TYPES__FUT_DEPTH) {
							kvmsg_init(rmsg, symbol,
									QUOTE_LIB__MSG_TYPES__FUT_DEPTH);
							depth = (QuoteLib__BDMDepth *) rmsg->value;
							memcpy(depth->expiry,
									fuopstat ? fuopstat->expiry : "200101", 6);
						}
						depth->multiplier = fuopstat ? fuopstat->multiplier : 0;
						if (fuopstat != NULL) {
							depth->n_quote = 5;
							depth->quote = calloc(depth->n_quote,
									sizeof(QuoteLib__Depthquote*));
							int n;
							for (n = 0; n < depth->n_quote; n++) {
								depth->quote[n] = calloc(1,
										sizeof(QuoteLib__Depthquote));
								quote_lib__depthquote__init(depth->quote[n]);
								depth->quote[n]->level = n;
								depth->quote[n]->bid = fuopstat->depth.bid[n];
								depth->quote[n]->ask = fuopstat->depth.ask[n];
								depth->quote[n]->bidsize =
										fuopstat->depth.bidSize[n];
								depth->quote[n]->asksize =
										fuopstat->depth.askSize[n];
								depth->quote[n]->nbidord =
										fuopstat->depth.nbidOrd[n];
								depth->quote[n]->naskord =
										fuopstat->depth.naskOrd[n];
								if (depth->quote[n]->naskord)
									depth->quote[n]->asktime =
											fuopstat->depth.asktime[n];
								if (depth->quote[n]->nbidord)
									depth->quote[n]->bidtime =
											fuopstat->depth.bidtime[n];
							}
						} else {
							depth->n_quote = 0;
						}
						break;
					}
				} else {
					zmq_send(rr_socket, zmq_msg_data(&req), len, ZMQ_SNDMORE);
					zmq_send(rr_socket, "", 1, ZMQ_SNDMORE);
				}
			} else {
				kvmsg_init(rmsg, "Error", QUOTE_LIB__MSG_TYPES__INFO_MSG);
				QuoteLib__Info *er = (QuoteLib__Info *) rmsg->value;
				er->msg = "Error - Received a bad request - Ignoring";
			}
			if (rmsg->value)
				kvmsg_send(rmsg, rr_socket);
			zmq_msg_close(&req);
			kvmsg_destroy(&rmsg);
			clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tm12);
			pargs->requests.time += ((tm12.tv_sec * 1e9 + tm12.tv_nsec)
					- (tm1.tv_sec * 1e9 + tm1.tv_nsec));
			pargs->requests.count++;
		}

		if (items[1].revents & ZMQ_POLLIN) { // update the mem db
			clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tm1);
			zmq_recv(ip_socket, &qmsg, sizeof(kvmsg_t *), 0);

			if (qmsg) {
				int keylen = strlen(qmsg->key);
				if (keylen > 0) {
					if ((qmsg->msg_type == QUOTE_LIB__MSG_TYPES__DIVIDEND)
							|| (qmsg->msg_type == QUOTE_LIB__MSG_TYPES__MOC)) {
						keylen = keylen - 3;
					} else
						keylen = keylen - 4;
#ifdef DEBUG
					printf("Recieved -> %s\n",qmsg->key);
#endif
					memset(reqstr, 0, QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX);
					memcpy(reqstr, qmsg->key + 3, keylen);
					time(&timer);
					tm_info = localtime(&timer);
					HASH_FIND_STR(pargs->buffer, reqstr, obj_lu);

					if (keylen > 2 && reqstr[keylen - 2] == '-'
							&& (reqstr[keylen - 1] == 'C'
									|| reqstr[keylen - 1] == 'P'
									|| reqstr[keylen - 1] == 'F')) {
						isEquity = 0;
						if (obj_lu)
							fuopstat = (smkt_fo_stat *) obj_lu->value;
					} else {
						isEquity = 1;
						if (obj_lu)
							eqstat = (smkt_eq_stat *) obj_lu->value;
					}

					if (!obj_lu
							&& (qmsg->msg_type
									!= QUOTE_LIB__MSG_TYPES__FO_END_OF_TRANS)) { // create new entry
						obj_lu = (htUTS *) calloc(1, sizeof(htUTS));
						if (isEquity == 0) {
							obj_lu->value = (smkt_fo_stat *) calloc(1,
									sizeof(smkt_fo_stat));
							fuopstat = (smkt_fo_stat *) obj_lu->value;
							fuopstat->low = NAN;
							fuopstat->open = NAN;
							fuopstat->last = 0;
							fuopstat->oi = 0;
							fuopstat->hi = NAN;
							fuopstat->close = NAN;
							fuopstat->pclose = NAN;
							fuopstat->multiplier = 0;
							snprintf(fuopstat->expiry, 8, "%s", "20120101");
							;
							strftime(fuopstat->trdtime, SZ_DATE, "%H%M%S",
									tm_info);							
						} else {
							obj_lu->value = (smkt_eq_stat *) calloc(1,
									sizeof(smkt_eq_stat));
							eqstat = obj_lu->value;
							eqstat->open = NAN;
							strftime(eqstat->trdtime, SZ_DATE, "%H%M%S",
									tm_info);
							eqstat->low = NAN;
							eqstat->hi = NAN;
							eqstat->divstr = NULL;
							eqstat->close = NAN;
							eqstat->pclose = NAN;
							eqstat->state = QUOTE_LIB__STOCK_STATUS__A;
							eqstat->moc = 0;
#ifdef DEBUG							
							printf("Adding %s type %d >%s< #%d \n", qmsg->key + 3,
									qmsg->msg_type, reqstr, pargs->count++);
#endif
						}
						snprintf(obj_lu->key,
								QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX, "%s",
								reqstr);
						HASH_ADD_STR(pargs->buffer, key, obj_lu);
					}
				}				
				if (qmsg->msg_type == QUOTE_LIB__MSG_TYPES__FO_END_OF_TRANS) {
					if (FPs[F_TYPES_FUT_EOD] != NULL) {
						htUTS * tmp, *obj;
						HASH_ITER(hh, pargs->buffer, obj, tmp)
						{
							fuopstat = (smkt_fo_stat *) obj->value;
							//"Symbol,Expiry. Strike,Multiplier,Bid,BidSz,Ask,AskSz,Last,LastSz,Hi,Low,Volume,Trades,Open,Close,Open Intrest\n");
							fprintf(FPs[F_TYPES_FUT_EOD],
									"%s,%s,%.f,%.f,%.3f,%d,%.3f,%d,%.3f,%d,%.3f,%.3f,%d,%d,%.3f,%.3f,%d\n",
									obj->key, fuopstat->expiry,
									fuopstat->strike, fuopstat->multiplier,
									fuopstat->bid, fuopstat->bidsz,
									fuopstat->ask, fuopstat->asksz,
									fuopstat->last, fuopstat->lastsz,
									fuopstat->hi, fuopstat->low,
									fuopstat->volume, fuopstat->trades,
									fuopstat->open, fuopstat->close,
									fuopstat->oi);
						}
						fflush(FPs[F_TYPES_FUT_EOD]);
					}
					bHaveClosed = 1;
				} else if (qmsg->msg_type
						== QUOTE_LIB__MSG_TYPES__QF_MKT_STATE) {
					QuoteLib__MarketState *mktstat =
							(QuoteLib__MarketState*) qmsg->value;
					eqstat->state = mktstat->state;
					if (mktstat->state == QUOTE_LIB__MKT_STATE__OPEN) {
						numOpenGrps++;
					} else if (mktstat->state
							== QUOTE_LIB__MKT_STATE__EXT_HRS_CLS) {
						numOpenGrps--;
					}
				} else if (qmsg->msg_type == QUOTE_LIB__MSG_TYPES__UNDEFINED) {
					printf("Uknownwn Message \n");
				} else {

					switch (qmsg->msg_type) {
					case QUOTE_LIB__MSG_TYPES__QUOTE:
						q = (QuoteLib__Quote *) qmsg->value;						
						eqstat->bid = q->bid;
						eqstat->bidsz = q->bidsize;
						eqstat->ask = q->ask;
						eqstat->asksz = q->asksize;
						clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tm12);
						pargs->quotes.time +=
								((tm12.tv_sec * 1e9 + tm12.tv_nsec)
										- (tm1.tv_sec * 1e9 + tm1.tv_nsec));
						pargs->quotes.count += 1;
						break;
					case QUOTE_LIB__MSG_TYPES__STK_STATUS:
						stkstate = (QuoteLib__StkStatus *) qmsg->value;
						eqstat->state = stkstate->state;
						break;
					case QUOTE_LIB__MSG_TYPES__INDEX:
						ind = (QuoteLib__Index *) qmsg->value;
						if (ind->has_bid)
							eqstat->bid = ind->bid;
						if (ind->has_ask)
							eqstat->ask = ind->ask;
						if (ind->has_last)
							eqstat->last = ind->last;
						if (ind->has_open)
							eqstat->open = ind->open;
						if (ind->has_value)
							eqstat->value = ind->value;
						if (ind->has_volume)
							eqstat->volume = ind->volume;
						if (ind->has_low)
							eqstat->low = ind->low;
						if (ind->has_high)
							eqstat->hi = ind->high;
						break;
					case QUOTE_LIB__MSG_TYPES__TRADE:
						t = (QuoteLib__Trade *) qmsg->value;
						
						if (t->iscxl || t->iscor) { 
							eqstat->volume -= t->volume;
							eqstat->value -= t->volume * t->last;
							eqstat->trades--;
						} else {  
							eqstat->trades++;
							eqstat->last = t->last;
							eqstat->lastsz = t->volume;
							eqstat->volume += t->volume;
							eqstat->value += t->volume * t->trd_px;
							memcpy(eqstat->trdtime, t->trdtime, SZ_DATE - 1);
							eqstat->buyerid = t->buyerid;
							eqstat->sellerid = t->sellerid;
							if (t->volume >= TSX_LOT_SZ(
									t->last)) {
								if (isnan(eqstat->low)
										|| eqstat->low > t->last) {
									memcpy(eqstat->lowtime, eqstat->trdtime,
											(SZ_DATE - 1) * sizeof(char));
									eqstat->low = t->last;
								}
								if (isnan(eqstat->hi) || eqstat->hi < t->last) {
									memcpy(eqstat->hightime, eqstat->trdtime,
											(SZ_DATE - 1) * sizeof(char));
									eqstat->hi = t->last;
								}
								bEqtyHaveOpened = 1;
								if (isnan(eqstat->open) && t->isopen)
									eqstat->open = t->last;
							}
						}
						clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tm12);
						pargs->trades.time +=
								((tm12.tv_sec * 1e9 + tm12.tv_nsec)
										- (tm1.tv_sec * 1e9 + tm1.tv_nsec));
						pargs->trades.count += 1;
						break;
					case QUOTE_LIB__MSG_TYPES__DIVIDEND:
						div = (QuoteLib__Dividend *) qmsg->value;
						eqstat->dividend = div->div;
						eqstat->divmkrks = div->markers;
						memcpy(eqstat->exdate, div->exdate, SZ_DATE - 1);
						memcpy(eqstat->recdate, div->recdate, SZ_DATE - 1);
						memcpy(eqstat->paydate, div->paydate, SZ_DATE - 1);
						if ((div->markers & QUOTE_LIB__DIV_CODES__D_TO_EX)
								== 0) {
							time(&timer);
							tm_info = localtime(&timer);
							strftime(eqstat->decdate, SZ_DATE, "%Y%m%d",
									tm_info);
						}
						if (*div->divstr != 0) {
							div->divstr = calloc(11, sizeof(char));
							memcpy(eqstat->divstr, div->divstr, 10);
						} else {
							div->divstr = NULL;
						}
						if (FPs[F_TYPES_DIV] != NULL) {
							//"Symbol,Dividend,ExDate,PayDate,RecordDate,Notes\n");
							int code = (QUOTE_LIB__DIV_CODES__D_TO_EX
									& div->markers);
							int mrkr1 = (QUOTE_LIB__DIV_CODES__MRKR1
									& div->markers)
									>> QUOTE_LIB__DIV_CODES__L_SHIFT;
							int mrkr2 = (QUOTE_LIB__DIV_CODES__MRKR2
									& div->markers)
									>> (2 * QUOTE_LIB__DIV_CODES__L_SHIFT);
							int mrkr3 = (QUOTE_LIB__DIV_CODES__MRKR3
									& div->markers)
									>> (3 * QUOTE_LIB__DIV_CODES__L_SHIFT);
							fprintf(FPs[F_TYPES_DIV],
									"%s,%f,%s,%s,%s,%d,%d,%d,%d\n", reqstr,
									eqstat->dividend, eqstat->exdate,
									eqstat->paydate, eqstat->recdate, code,
									mrkr1, mrkr2, mrkr3);
							fflush(FPs[F_TYPES_DIV]);
						}
						break;
					case QUOTE_LIB__MSG_TYPES__EQ_SUMM:
						eqs = (QuoteLib__EquitySummary *) qmsg->value;
						eqstat->bid = eqs->bid;
						eqstat->bidsz = eqs->bidsize;
						eqstat->ask = eqs->ask;
						eqstat->asksz = eqs->asksize;
						eqstat->last = eqs->last;
						eqstat->value = eqs->value;
						eqstat->trades = eqs->trades;
						eqstat->volume = eqs->volume;
						eqstat->eps = eqs->eps;
						eqstat->adivs = eqs->adivs;
						if (numOpenGrps != 0) {
							eqstat->close = eqs->last;
							eqstat->low = eqs->low;
							eqstat->hi = eqs->high;
							eqstat->open = eqs->open;
						}
						if (bEqtyHaveOpened && FPs[F_TYPES_EOD]) {
							//"Symbol,CUSIP,Name,Bid,BidSz,Ask,AskSz,Last,LastSz,Hi,Low,Volume,Trades,Value,Open,Close,PrevClose,Moc,LowTime,HighTime,StkGrp,moceligible\n"
							fprintf(FPs[F_TYPES_EOD],
									"%s,%s,%s,%.3g,%d,%.3g,%d,%.3g,%d,%.3g,%.3g,%d,%d,%.f,%.3g,%.3g,%.3g,%d,%s,%s,%d,%s,%c\n",
									reqstr, eqstat->cusip, eqstat->name,
									eqstat->bid, eqstat->bidsz, eqstat->ask,
									eqstat->asksz, eqs->last, eqstat->lastsz,
									eqstat->hi, eqstat->low, eqstat->volume,
									eqstat->trades, eqstat->value, eqstat->open,
									eqstat->close, eqstat->pclose, eqstat->moc,
									eqstat->lowtime, eqstat->hightime,
									eqstat->stkgrp,
									((eqstat->moceligible == 1) ? "Y" : "N"),
									eqs->imo);
							fflush(FPs[F_TYPES_EOD]);
						}
						break;
					case QUOTE_LIB__MSG_TYPES__QF_SYM_STATUS:
						symstatus = (QuoteLib__QfSymbStatus *) qmsg->value;
						eqstat->pclose = symstatus->last;
						eqstat->stkgrp = symstatus->stkgrp;
						memcpy(eqstat->name, symstatus->issuer,
								40 * sizeof(char));
						int loc = 40;
						do {
							loc--;
						} while (loc > 0 && *(eqstat->name + loc) == ' ');
						*(eqstat->name + loc + ((loc == 40) ? 0 : 1)) = 0;
						memcpy(eqstat->cusip, symstatus->cusip,
								12 * sizeof(char));
						loc = 12;
						do {
							loc--;
						} while (loc > 0 && *(eqstat->cusip + loc) == ' ');
						*(eqstat->cusip + loc + ((loc == 12) ? 0 : 1)) = 0;
						eqstat->moceligible = symstatus->moc_elig;
						if (FPs[F_TYPES_SOD] != NULL) {
							//"Symbol,Exch,Cusip,Issuer,Last Sale,MOC,Type,Stock Grp,Lot Sz\n"
							fprintf(FPs[F_TYPES_SOD],
									"%s,%s,%s,%s,%.3g,%s,%s,%d,%d,%g\n", reqstr,
									symstatus->exchid, eqstat->cusip,
									eqstat->name, eqstat->pclose,
									((symstatus->moc_elig == 1) ? "Y" : "N"),
									((symstatus->is_stk == 1) ? "Eqty" : "Deb"),
									symstatus->stkgrp, symstatus->lotsz,
									symstatus->db_fv);
							fflush(FPs[F_TYPES_SOD]);
						}
						break;
					case QUOTE_LIB__MSG_TYPES__MOC:
						moc = (QuoteLib__Moc *) qmsg->value;
						eqstat->moc = moc->side;
						if (moc->side != 0 && FPs[F_TYPES_MOC] != NULL) {
							fprintf(FPs[F_TYPES_MOC], "%s,%d,%f,%d,%f,%d\n",
									qmsg->key + 3, eqstat->moc, eqstat->bid,
									eqstat->bidsz, eqstat->ask, eqstat->asksz);
							fflush(FPs[F_TYPES_MOC]);
							//"Symbol,Imbalance,BidSz,Bid,Ask,Asksz\n");
						}
						break;
					case QUOTE_LIB__MSG_TYPES__OPTION_QUOTE:
						oquote = (QuoteLib__OptQuote *) qmsg->value;
						fuopstat->bid = oquote->bid;
						fuopstat->ask = oquote->ask;
						fuopstat->bidsz = oquote->bidsize;
						fuopstat->asksz = oquote->asksize;
						fuopstat->isHalted = oquote->ishalted;
						if (fuopstat->multiplier == 0) {
							fuopstat->multiplier = oquote->multiplier;
							fuopstat->isAmer = oquote->isamer;
							fuopstat->strike = oquote->strike;
							memcpy(fuopstat->expiry, oquote->expiry, 8);
						}
						seq_num = oquote->seq_num;
						clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tm12);
						pargs->quotes.time +=
								((tm12.tv_sec * 1e9 + tm12.tv_nsec)
										- (tm1.tv_sec * 1e9 + tm1.tv_nsec));
						pargs->quotes.count += 1;
#ifdef DEBUG
						if (!bhave_summary && pargs->fsymdump != NULL) {
							fprintf(pargs->fsymdump, "</table>\n");
							fclose(pargs->fsymdump);
							pargs->fsymdump = NULL;
							bhave_summary = 1;
						}
#endif
						break;
					case QUOTE_LIB__MSG_TYPES__FUTURE_QUOTE:
						fquote = (QuoteLib__FutQuote *) qmsg->value;
						fuopstat->bid = fquote->bid;
						fuopstat->ask = fquote->ask;
						fuopstat->bidsz = fquote->bidsize;
						fuopstat->asksz = fquote->asksize;
						fuopstat->isHalted = fquote->ishalted;
						if (fuopstat->multiplier == 0) {
							fuopstat->multiplier = fquote->multiplier;
							memcpy(fuopstat->expiry, fquote->expiry, 6);
						}						
						seq_num = fquote->seq_num;
						clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tm12);
						pargs->quotes.time +=
								((tm12.tv_sec * 1e9 + tm12.tv_nsec)
										- (tm1.tv_sec * 1e9 + tm1.tv_nsec));
						pargs->quotes.count += 1;
						break;
					case QUOTE_LIB__MSG_TYPES__OPTION_TRADE:
						otrade = (QuoteLib__OptTrade *) qmsg->value;						
						if (otrade->last)
							fuopstat->last = otrade->last;
						if (otrade->volume)
							fuopstat->lastsz = otrade->volume;
						if (isnan(
								fuopstat->low)
								|| fuopstat->low > otrade->last) {
							snprintf(fuopstat->lowtime, 7, otrade->trdtime);
							fuopstat->low = otrade->last;
						}

						if (isnan(fuopstat->hi)
								|| otrade->last > fuopstat->hi) {
							snprintf(fuopstat->hightime, 7, otrade->trdtime);
							fuopstat->hi = otrade->last;
						}
						fuopstat->trades++;
						if (isnan(fuopstat->open)) { // open trade
							fuopstat->open = otrade->last;
							fuopstat->pclose = otrade->last - otrade->chg;
							if (fuopstat->multiplier == 0) {
								memcpy(fuopstat->expiry, otrade->expiry, 8);
								fuopstat->strike = otrade->strike;
								fuopstat->isAmer = otrade->isamer;
								fuopstat->multiplier = otrade->multiplier;
							}
							bEqtyHaveOpened = 1;
						}

						snprintf(fuopstat->trdtime, 7, "%s", otrade->trdtime);
						fuopstat->volume += otrade->volume;
						fuopstat->value += otrade->multiplier * otrade->last
								* otrade->volume;
						seq_num = otrade->seq_num;
						clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tm12);
						pargs->trades.time +=
								((tm12.tv_sec * 1e9 + tm12.tv_nsec)
										- (tm1.tv_sec * 1e9 + tm1.tv_nsec));
						pargs->trades.count += 1;						
						break;
					case QUOTE_LIB__MSG_TYPES__FUTURE_TRADE:
						
						ftrade = (QuoteLib__FutTrade *) qmsg->value;
						if (ftrade->last)
							fuopstat->last = ftrade->last;
						if (ftrade->volume)
							fuopstat->lastsz = ftrade->volume;						
						if (isnan(
								fuopstat->hi) || fuopstat->hi < ftrade->last) {
							snprintf(fuopstat->hightime, 7, ftrade->trdtime);
							fuopstat->hi = ftrade->last;
						}
						if (isnan(fuopstat->low)
								|| ftrade->last < fuopstat->low) {
							snprintf(fuopstat->lowtime, 7, ftrade->trdtime);
							fuopstat->low = ftrade->last;
						}

						if (isnan(fuopstat->open)) { // open trade
							fuopstat->open = ftrade->last;
							fuopstat->pclose = ftrade->last - ftrade->chg;
							if (fuopstat->multiplier == 0) {
								memcpy(fuopstat->expiry, ftrade->expiry, 6);
								fuopstat->multiplier = ftrade->multiplier;
							}
						}
						memcpy(fuopstat->trdtime, ftrade->trdtime, 6);
						fuopstat->volume += ftrade->volume;
						fuopstat->value += ftrade->multiplier * ftrade->last
								* ftrade->volume;
						fuopstat->trades++;
						seq_num = ftrade->seq_num;
						clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tm12);
						pargs->trades.time +=
								((tm12.tv_sec * 1e9 + tm12.tv_nsec)
										- (tm1.tv_sec * 1e9 + tm1.tv_nsec));
						pargs->trades.count += 1;
						break;					
					case QUOTE_LIB__MSG_TYPES__OPTION_STAT:
						ostat = (QuoteLib__OptStat *) qmsg->value;
						seq_num = ostat->seq_num;
						break;
					case QUOTE_LIB__MSG_TYPES__FUTURE_STAT:
						fstat = (QuoteLib__FutStat *) qmsg->value;
						seq_num = fstat->seq_num;
						break;
					case QUOTE_LIB__MSG_TYPES__OPTION_DFN:
					case QUOTE_LIB__MSG_TYPES__FUTURE_DFN:
					case QUOTE_LIB__MSG_TYPES__FUTOPT_DFN:
					case QUOTE_LIB__MSG_TYPES__STRAT_DFN:
						if (FPs[F_TYPES_FUT_STAT] != NULL) {
							QuoteLib__InstrDfn *idef =
									(QuoteLib__InstrDfn *) qmsg->value;
							fprintf(FPs[F_TYPES_FUT_STAT],
									"%s,%d,%d,%d,%g,%d,%s,%s\n", reqstr,
									idef->has_exyr ? idef->exyr : 0,
									idef->has_exmnth ? idef->exmnth : 0,
									idef->has_expday ? idef->expday : 0,
									idef->has_strike ? idef->strike : 0,
									idef->consize, idef->type, idef->externid);
							fflush(FPs[F_TYPES_FUT_STAT]);
						}
						break;
					case QUOTE_LIB__MSG_TYPES__OPT_DEPTH:
					case QUOTE_LIB__MSG_TYPES__FUT_DEPTH:
						dMsg = (QuoteLib__BDMDepth *) qmsg->value;
						if (fuopstat->multiplier == 0) {
							if (dMsg->msgtype
									== QUOTE_LIB__MSG_TYPES__OPT_DEPTH) {
								memcpy(fuopstat->expiry, dMsg->expiry, 8);
								fuopstat->isAmer = dMsg->isamer;
								fuopstat->strike = dMsg->strike;
							} else
								memcpy(fuopstat->expiry, dMsg->expiry, 6);
							fuopstat->multiplier = dMsg->multiplier;
						}
						int n;
						for (n = 0; n < dMsg->n_quote; n++) {
							int level = dMsg->quote[n]->level;
							fuopstat->depth.ask[level] = dMsg->quote[n]->ask;
							fuopstat->depth.bid[level] = dMsg->quote[n]->bid;
							fuopstat->depth.askSize[level] =
									dMsg->quote[n]->asksize;
							fuopstat->depth.bidSize[level] =
									dMsg->quote[n]->bidsize;
							fuopstat->depth.naskOrd[level] =
									dMsg->quote[n]->naskord;
							fuopstat->depth.nbidOrd[level] =
									dMsg->quote[n]->nbidord;
							if (fuopstat->depth.nbidOrd[level])
								fuopstat->depth.bidtime[level] =
										dMsg->quote[n]->bidtime;
							if (fuopstat->depth.naskOrd[level])
								fuopstat->depth.asktime[level] =
										dMsg->quote[n]->asktime;
						}
						break;
					}
				}
			}
			kvmsg_destroy(&qmsg);
			clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tm22);
			pargs->total.time += ((tm22.tv_sec * 1e9 + tm22.tv_nsec)
					- (tm2.tv_sec * 1e9 + tm2.tv_nsec));
			pargs->total.count += 1;

		}
	}
	int n;
	for (n = 0; n < F_TYPES_NUM; n++) {
		if (FPs[n] != NULL) {
			fclose(FPs[n]);
			FPs[n] = NULL;
		}
	}
	printf("Exiting\n");
	fflush( stdout);
}


int init_qfbuffer(void * htHandle) {
	pthread_attr_t attr;
	sqfhbuffer * ptrstruct = (sqfhbuffer *) htHandle;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	int retval = pthread_create(&ptrstruct->pthrd, &attr, (void *) qfbuffer,
			(void *) ptrstruct);
	pthread_attr_destroy(&attr);
	assert(!retval);
	return retval;
}
