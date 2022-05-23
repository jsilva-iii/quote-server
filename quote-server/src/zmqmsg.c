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

#include <stdio.h>
#include <string.h>
#include "zmqmsg.h"
#include "czmq.h"

kvmsg_t * kvmsg_new() {
	kvmsg_t *self;
	self = (kvmsg_t *) calloc(1, sizeof(kvmsg_t));
#ifdef DEBUG
	assert(self);
#endif
	if (self == NULL){
		printf("kvmsg_new() - failed\n");
		return (kvmsg_t * )NULL;
	}
	self->props_size = sizeof(kvmsg_t);
	self->msg_type = QUOTE_LIB__MSG_TYPES__UNDEFINED;
	return self;
}

kvmsg_t * kvmsg_init_value(kvmsg_t *self) {
#ifdef DEBUG
	assert(self);
#endif
	if (self == NULL){
		printf ("kvmsg_init_value(kvmsg_t *self) - failed\n");
		return NULL;
	}
	if (self) {
		switch (self->msg_type) {
	        case QUOTE_LIB__MSG_TYPES__FOREX_QUOTE:
		case QUOTE_LIB__MSG_TYPES__QUOTE:
			self->value = calloc(1, sizeof(QuoteLib__Quote));
			if (self->value)
				quote_lib__quote__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__TRADE_CXL:
		case QUOTE_LIB__MSG_TYPES__TRADE:
			self->value = calloc(1, sizeof(QuoteLib__Trade));
			if (self->value)
				quote_lib__trade__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__STK_STATUS:
			self->value = calloc(1, sizeof(QuoteLib__StkStatus));
			if (self->value)
				quote_lib__stk_status__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__INDEX:
			self->value = calloc(1, sizeof(QuoteLib__Index));
			if (self->value)
				quote_lib__index__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__EQ_SUMM:
			self->value = calloc(1, sizeof(QuoteLib__EquitySummary));
			if (self->value)
				quote_lib__equity_summary__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__DIVIDEND:
			self->value = calloc(1, sizeof(QuoteLib__Dividend));
			if (self->value)
				quote_lib__dividend__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__HILO52:
			self->value = calloc(1, sizeof(QuoteLib__HiLo52));
			if (self->value)
				quote_lib__hi_lo52__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__EQUITY_VOLUME:
			self->value = calloc(1, sizeof(QuoteLib__EquityVolume));
			if (self->value)
				quote_lib__equity_volume__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__QF_MOC_PMD:
		case QUOTE_LIB__MSG_TYPES__MOC:
			self->value = calloc(1, sizeof(QuoteLib__Moc));
			if (self->value)
				quote_lib__moc__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__INFO_MSG:
			self->value = calloc(1, sizeof(QuoteLib__Info));
			if (self->value)
				quote_lib__info__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__MKT_STAT:
			self->value = calloc(1, sizeof(QuoteLib__MktStat));
			if (self->value)
				quote_lib__mkt_stat__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__OPTION_QUOTE:
			self->value = calloc(1, sizeof(QuoteLib__OptQuote));
			if (self->value)
				quote_lib__opt_quote__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__OPTION_RFQ:
			self->value = calloc(1, sizeof(QuoteLib__OptRFQ));
			if (self->value)
				quote_lib__opt_rfq__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__OPTION_TRADE:
			self->value = calloc(1, sizeof(QuoteLib__OptTrade));
			if (self->value)
				quote_lib__opt_trade__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__OPTION_STAT:
			self->value = calloc(1, sizeof(QuoteLib__OptStat));
			if (self->value)
				quote_lib__opt_stat__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__FUTURE_QUOTE:
			self->value = calloc(1, sizeof(QuoteLib__FutQuote));
			if (self->value)
				quote_lib__fut_quote__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__FUTURE_RFQ:
			self->value = calloc(1, sizeof(QuoteLib__FutRFQ));
			if (self->value)
				quote_lib__fut_rfq__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__FUTURE_TRADE:
			self->value = calloc(1, sizeof(QuoteLib__FutTrade));
			if (self->value)
				quote_lib__fut_trade__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__FUTURE_STAT:
			self->value = calloc(1, sizeof(QuoteLib__FutStat));
			if (self->value)
				quote_lib__fut_stat__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__FUTOPT_BULLETIN:
			self->value = calloc(1, sizeof(QuoteLib__FutOptBulletin));
			if (self->value)
				quote_lib__fut_opt_bulletin__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__FUTURE_DFN:
		case QUOTE_LIB__MSG_TYPES__STRAT_DFN:
		case QUOTE_LIB__MSG_TYPES__FUTOPT_DFN:
		case QUOTE_LIB__MSG_TYPES__OPTION_DFN:
			self->value = calloc(1, sizeof(QuoteLib__InstrDfn));
			if (self->value)
				quote_lib__instr_dfn__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__FUT_DEPTH:
		case QUOTE_LIB__MSG_TYPES__OPT_DEPTH:
			self->value = calloc(1, sizeof(QuoteLib__BDMDepth));
			if (self->value)
				quote_lib__bdmdepth__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__FO_END_OF_TRANS:
			self->value = calloc(1, sizeof(QuoteLib__EndOfSales));
			if (self->value)
				quote_lib__end_of_sales__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__QF_SYM_STATUS:
			self->value = calloc(1, sizeof(QuoteLib__QfSymbStatus));
			if (self->value)
				quote_lib__qf_symb_status__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__QF_TRD_TIER_STATUS:
			self->value = calloc(1, sizeof(QuoteLib__TradingTierStatus));
			if (self->value)
				quote_lib__trading_tier_status__init(self->value);
			break;
		case QUOTE_LIB__MSG_TYPES__QF_MKT_STATE:
			self->value = calloc(1, sizeof(QuoteLib__MarketState));
			if (self->value)
				quote_lib__market_state__init(self->value);
			break;
		}
	}
	return self;
}

//  zhash_free_fn callback helper that does the low level destruction:
void kvmsg_init(kvmsg_t *self, char * key_name, int msg_type) {
	self->msg_type = msg_type;
	kvmsg_init_value(self);
	kvmsg_set_key(self, key_name);
}
void kvmsg_free(void *ptr) {
	if (ptr) {
		kvmsg_t *self = (kvmsg_t *) ptr;
		//  Destroy message frames if any
		int frame_nbr;
		for (frame_nbr = 0; frame_nbr < KVMSG_FRAMES; frame_nbr++)
			if (self->present[frame_nbr])
				zmq_msg_close(&self->frame[frame_nbr]);
		//  Free object itself
		if (self->value) {			
			if (self->msg_type == QUOTE_LIB__MSG_TYPES__FUTOPT_BULLETIN
					&& *((QuoteLib__FutOptBulletin *) self->value)->message
							!= 0) {
				free(((QuoteLib__FutOptBulletin *) self->value)->message);
			} else if (self->msg_type == QUOTE_LIB__MSG_TYPES__FUTURE_DFN
					|| self->msg_type == QUOTE_LIB__MSG_TYPES__STRAT_DFN
					|| self->msg_type == QUOTE_LIB__MSG_TYPES__FUTOPT_DFN
					|| self->msg_type == QUOTE_LIB__MSG_TYPES__OPTION_DFN) {
				if (((QuoteLib__InstrDfn*) self->value)->externid
						&& *((QuoteLib__InstrDfn *) self->value)->externid != 0)
					free(((QuoteLib__InstrDfn *) self->value)->externid);
				if (((QuoteLib__InstrDfn*) self->value)->ulrootsym
						&& *((QuoteLib__InstrDfn *) self->value)->ulrootsym
								!= 0)
					free(((QuoteLib__InstrDfn *) self->value)->ulrootsym);
			} else if (self->msg_type == QUOTE_LIB__MSG_TYPES__OPT_DEPTH
					|| self->msg_type == QUOTE_LIB__MSG_TYPES__FUT_DEPTH) {
				QuoteLib__BDMDepth * bdm = (QuoteLib__BDMDepth *) self->value;
				int i;
				if (bdm->n_quote) {
					for (i = 0; i < bdm->n_quote; i++)
						free(bdm->quote[i]);
					free(bdm->quote);
				}
			}
			free(self->value);
		}
		free(self);
	}
}

void kvmsg_destroy(kvmsg_t **self_p) {
#ifdef DEBUG
	assert(self_p);
#endif
	if (*self_p) {
		kvmsg_free(*self_p);
		*self_p = NULL;
	}
}

static void s_decode_body(kvmsg_t *self) {
	zmq_msg_t *msg = &self->frame[FRAME_BODY];
	self->props_size = 0;
	size_t msgsz = zmq_msg_size(msg);
	byte *data = (byte *) zmq_msg_data(msg);
	switch (self->msg_type) {
	case QUOTE_LIB__MSG_TYPES__FOREX_QUOTE:
	case QUOTE_LIB__MSG_TYPES__QUOTE:
		self->value = quote_lib__quote__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__TRADE_CORR:
	case QUOTE_LIB__MSG_TYPES__TRADE_CXL:
	case QUOTE_LIB__MSG_TYPES__TRADE:
		self->value = quote_lib__trade__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__STK_STATUS:
		self->value = quote_lib__stk_status__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__INDEX:
		self->value = quote_lib__index__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__EQ_SUMM:
		self->value = quote_lib__equity_summary__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__DIVIDEND:
		self->value = quote_lib__dividend__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__HILO52:
		self->value = quote_lib__hi_lo52__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__EQUITY_VOLUME:
		self->value = quote_lib__equity_volume__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__QF_MOC_PMD:
	case QUOTE_LIB__MSG_TYPES__MOC:
		self->value = quote_lib__moc__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__MKT_STAT:
		self->value = quote_lib__mkt_stat__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__OPTION_QUOTE:
		self->value = quote_lib__opt_quote__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__OPTION_TRADE:
		self->value = quote_lib__opt_trade__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__OPTION_RFQ:
		self->value = quote_lib__opt_rfq__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__OPTION_STAT:
		self->value = quote_lib__opt_stat__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__FUTURE_TRADE:
		self->value = quote_lib__fut_trade__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__FUTURE_QUOTE:
		self->value = quote_lib__fut_quote__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__FUTURE_RFQ:
		self->value = quote_lib__fut_rfq__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__FUTURE_STAT:
		self->value = quote_lib__fut_stat__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__FUTOPT_BULLETIN:
		self->value = quote_lib__fut_opt_bulletin__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__FUTURE_DFN:
	case QUOTE_LIB__MSG_TYPES__STRAT_DFN:
	case QUOTE_LIB__MSG_TYPES__FUTOPT_DFN:
	case QUOTE_LIB__MSG_TYPES__OPTION_DFN:
		self->value = quote_lib__instr_dfn__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__FUT_DEPTH:
	case QUOTE_LIB__MSG_TYPES__OPT_DEPTH:
		self->value = quote_lib__bdmdepth__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__FO_END_OF_TRANS:
		self->value = quote_lib__end_of_sales__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__QF_SYM_STATUS:
		self->value = quote_lib__qf_symb_status__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__QF_TRD_TIER_STATUS:
		self->value = quote_lib__trading_tier_status__unpack(NULL, msgsz, data);
		break;
	case QUOTE_LIB__MSG_TYPES__QF_MKT_STATE:
		self->value = quote_lib__market_state__unpack(NULL, msgsz, data);
		break;
	}
}

static void s_encode_body(kvmsg_t *self) {
	zmq_msg_t *msg = &self->frame[FRAME_BODY];
	size_t sz;
	byte *dest;
	if (self->present[FRAME_BODY])
		zmq_msg_close(msg);
	switch (self->msg_type) {
        case QUOTE_LIB__MSG_TYPES__FOREX_QUOTE:
	case QUOTE_LIB__MSG_TYPES__QUOTE:
		sz = quote_lib__quote__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__quote__pack(self->value, dest);
		break;
	case QUOTE_LIB__MSG_TYPES__TRADE_CORR:
	case QUOTE_LIB__MSG_TYPES__TRADE_CXL:
	case QUOTE_LIB__MSG_TYPES__TRADE:
		sz = quote_lib__trade__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);		
		quote_lib__trade__pack(self->value, dest);
		break;

	case QUOTE_LIB__MSG_TYPES__STK_STATUS:
		sz = quote_lib__stk_status__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);		
		quote_lib__stk_status__pack(self->value, dest);
		break;

	case QUOTE_LIB__MSG_TYPES__INDEX:
		sz = quote_lib__index__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__index__pack(self->value, dest);
		break;

	case QUOTE_LIB__MSG_TYPES__EQ_SUMM:
		sz = quote_lib__equity_summary__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__equity_summary__pack(self->value, dest);
		break;

	case QUOTE_LIB__MSG_TYPES__DIVIDEND:
		sz = quote_lib__dividend__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__dividend__pack(self->value, dest);
		break;

	case QUOTE_LIB__MSG_TYPES__HILO52:
		sz = quote_lib__hi_lo52__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__hi_lo52__pack(self->value, dest);
		break;

	case QUOTE_LIB__MSG_TYPES__EQUITY_VOLUME:
		sz = quote_lib__equity_volume__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__equity_volume__pack(self->value, dest);
		break;
	case QUOTE_LIB__MSG_TYPES__QF_MOC_PMD:
	case QUOTE_LIB__MSG_TYPES__MOC:
		sz = quote_lib__moc__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__moc__pack(self->value, dest);
		break;
	case QUOTE_LIB__MSG_TYPES__INFO_MSG:
		sz = quote_lib__info__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__info__pack(self->value, dest);
		break;

	case QUOTE_LIB__MSG_TYPES__MKT_STAT:
		sz = quote_lib__mkt_stat__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__mkt_stat__pack(self->value, dest);
		break;

	case QUOTE_LIB__MSG_TYPES__OPTION_QUOTE:
		sz = quote_lib__opt_quote__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__opt_quote__pack(self->value, dest);
		break;
	case QUOTE_LIB__MSG_TYPES__OPTION_RFQ:
		sz = quote_lib__opt_rfq__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__opt_rfq__pack(self->value, dest);
		break;
	case QUOTE_LIB__MSG_TYPES__OPTION_STAT:
		sz = quote_lib__opt_stat__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__opt_stat__pack(self->value, dest);
		break;
	case QUOTE_LIB__MSG_TYPES__OPTION_TRADE:
		sz = quote_lib__opt_trade__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__opt_trade__pack(self->value, dest);
		break;
	case QUOTE_LIB__MSG_TYPES__FUTURE_QUOTE:
		sz = quote_lib__fut_quote__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__fut_quote__pack(self->value, dest);
		break;
	case QUOTE_LIB__MSG_TYPES__FUTURE_RFQ:
		sz = quote_lib__fut_rfq__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__fut_rfq__pack(self->value, dest);
		break;
	case QUOTE_LIB__MSG_TYPES__FUTURE_STAT:
		sz = quote_lib__fut_stat__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__fut_stat__pack(self->value, dest);
		break;
	case QUOTE_LIB__MSG_TYPES__FUTURE_TRADE:
		sz = quote_lib__fut_trade__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__fut_trade__pack(self->value, dest);
		break;
	case QUOTE_LIB__MSG_TYPES__FUTOPT_BULLETIN:
		sz = quote_lib__fut_opt_bulletin__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__fut_opt_bulletin__pack(self->value, dest);
		break;
	case QUOTE_LIB__MSG_TYPES__FUTURE_DFN:
	case QUOTE_LIB__MSG_TYPES__STRAT_DFN:
	case QUOTE_LIB__MSG_TYPES__FUTOPT_DFN:
	case QUOTE_LIB__MSG_TYPES__OPTION_DFN:
		sz = quote_lib__instr_dfn__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__instr_dfn__pack(self->value, dest);
		break;
	case QUOTE_LIB__MSG_TYPES__OPT_DEPTH:
	case QUOTE_LIB__MSG_TYPES__FUT_DEPTH:
		sz = quote_lib__bdmdepth__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__bdmdepth__pack(self->value, dest);
		break;
	case QUOTE_LIB__MSG_TYPES__FO_END_OF_TRANS:
		sz = quote_lib__end_of_sales__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__end_of_sales__pack(self->value, dest);
		break;
	case QUOTE_LIB__MSG_TYPES__QF_SYM_STATUS:
		sz = quote_lib__qf_symb_status__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__qf_symb_status__pack(self->value, dest);
		break;
	case QUOTE_LIB__MSG_TYPES__QF_TRD_TIER_STATUS:
		sz = quote_lib__trading_tier_status__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__trading_tier_status__pack(self->value, dest);
		break;
	case QUOTE_LIB__MSG_TYPES__QF_MKT_STATE:
		sz = quote_lib__market_state__get_packed_size(self->value);
		zmq_msg_init_size(msg, sz);
		dest = (byte *) zmq_msg_data(msg);
		quote_lib__market_state__pack(self->value, dest);
		break;
	}
	self->present[FRAME_BODY] = 1;
}

kvmsg_t * kvmsg_recv(void *socket) {
#ifdef DEBUG
	assert(socket);
#endif
	if (socket == NULL)
		return NULL ;
	kvmsg_t *self = kvmsg_new();
	//  Read all frames off the wire, reject if bogus
	int frame_nbr;
	for (frame_nbr = 0; frame_nbr < KVMSG_FRAMES; frame_nbr++) {
		if (self->present[frame_nbr])
			zmq_msg_close(&self->frame[frame_nbr]);
		zmq_msg_init(&self->frame[frame_nbr]);
		self->present[frame_nbr] = 1;
		if (zmq_msg_recv(&self->frame[frame_nbr], socket, 0) == -1) {
			kvmsg_destroy(&self);
			break;
		}
		//  Verify multipart framing
		int rcvmore = (frame_nbr < KVMSG_FRAMES - 1) ? 1 : 0;
		if (zsockopt_rcvmore(socket) != rcvmore) {
			kvmsg_destroy(&self);
			break;
		}
	}
	if (self->present[FRAME_KEY]) {
#ifdef DEBUG
		printf("Key : >%s<>%s<", self->key, self->frame[FRAME_KEY]);
#endif
		if (!*self->key) {
			size_t size = zmq_msg_size(&self->frame[FRAME_KEY]);
			if (size > QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX) {
				size = QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX;
			}
			self->msg_type = atoi(
					strchr((char *) &self->frame[FRAME_KEY],
							QUOTE_LIB__CONSTANTS__KEY_TYPE_SEPARATOR) + 1);
			memcpy(self->key, zmq_msg_data(&self->frame[FRAME_KEY]), size);
			self->key[size] = 0;
		}
	}
	if (self)
		s_decode_body(self);
	return self;
}

// use this for inproc
void kvmsg_dispatch(kvmsg_t *self, void *publicskt, void *inprocskt) {
	kvmsg_send(self, publicskt);
	if (inprocskt != NULL) {
		zmq_send(inprocskt, &self, sizeof(kvmsg_t *), 0);
	}
}

void kvmsg_send(kvmsg_t *self, void *socket) {
#ifdef DEBUG
	assert(self);
	assert(socket);
#endif
	if ( socket == NULL || self == NULL)
		return;
	s_encode_body(self);
	//  The rest of the method is unchanged from kvsimple
	//  .skip
	int frame_nbr;
	for (frame_nbr = 0; frame_nbr < KVMSG_FRAMES; frame_nbr++) {
		
			zmq_msg_send(&self->frame[frame_nbr], socket,
								(frame_nbr < KVMSG_FRAMES - 1) ? ZMQ_SNDMORE : 0);		
	}
}

// rcv from buffer to key
char * kvmsg_key(kvmsg_t *self) {
#ifdef DEBUG
	assert(self);
#endif
	if ( self == NULL){
		printf("*self is NULL, :599\n");
		return NULL;
	}
	char tchar[3] = { 0 };
	if (self->present[FRAME_KEY]) {
		if (!*self->key) {
			size_t size = zmq_msg_size(&self->frame[FRAME_KEY]);
			if (size > QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX) {
				size = QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX;
			}
			memcpy(tchar, zmq_msg_data(&self->frame[FRAME_KEY]), 2);
			self->msg_type = atoi(tchar);
			memcpy(self->key, zmq_msg_data(&self->frame[FRAME_KEY]), size);
			self->key[size] = 0;
		}
		return self->key;
	} else
		return NULL;
}

void kvmsg_set_key(kvmsg_t *self, char *key) {
#ifdef DEBUG
	assert(self);
	assert(self->msg_type);
#endif
	if ( self == NULL )
		return ;
	zmq_msg_t *msg = &self->frame[FRAME_KEY];
	if (self->present[FRAME_KEY])
		zmq_msg_close(msg);
	if (self->msg_type == QUOTE_LIB__MSG_TYPES__MOC
			|| self->msg_type == QUOTE_LIB__MSG_TYPES__DIVIDEND
			|| self->msg_type == QUOTE_LIB__MSG_TYPES__QF_MOC_PMD) {
		snprintf(self->key, QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX, "%02d%c%s",
				self->msg_type, QUOTE_LIB__CONSTANTS__KEY_TYPE_SEPARATOR, key);
	} else {								// broadcast only to subscribers
		snprintf(self->key, QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX, "%02d%c%s%c",
				self->msg_type, QUOTE_LIB__CONSTANTS__KEY_TYPE_SEPARATOR, key,
				QUOTE_LIB__CONSTANTS__KEY_TYPE_SEPARATOR);
	}
	zmq_msg_init_size(msg, strlen(self->key));
	memcpy(zmq_msg_data(msg), self->key, strlen(self->key));
	self->present[FRAME_KEY] = 1;
}

void kvmsg_dump(kvmsg_t *self) {
	assert(self);
	printf("Message Type : %d for %s\n", self->msg_type, kvmsg_key(self));	
	switch (self->msg_type) {
	case QUOTE_LIB__MSG_TYPES__QUOTE: {
		QuoteLib__Quote *quote = (QuoteLib__Quote *) self->value;
#ifdef DEBUG
			printf("Quote : %8.f@%6.3f : %6.3f@%-8.f %s\n", quote->bidsize,
					quote->bid, quote->ask, quote->asksize,
				quote->ishalted ? "Halted" : "");
#endif
		break;
	}
	case QUOTE_LIB__MSG_TYPES__TRADE: {
		QuoteLib__Trade *trade = (QuoteLib__Trade *) self->value;
		printf("Trade:  %d@%6.3f : Buyer %d Seller %d\n", trade->volume,
				trade->last, trade->has_buyerid ? trade->buyerid : 0,
				trade->has_sellerid ? trade->sellerid : 0);
		break;
	}
	case QUOTE_LIB__MSG_TYPES__STK_STATUS: {
		QuoteLib__StkStatus *stkstate = (QuoteLib__StkStatus *) self->value;
	}

	case QUOTE_LIB__MSG_TYPES__MOC: {
		QuoteLib__Moc *moc = (QuoteLib__Moc *) self->value;
		if (moc->has_side)
			printf("Moc : %s %s %d", self->key, moc->side > 0 ? "Buy" : "Sell",
					abs(moc->side));
		if (moc->has_ccp)
			printf("Moc : CCP %s %6.3f", self->key, moc->ccp);
		if (moc->has_vwap)
			printf("Moc: VWAP %s %6.3f", self->key, moc->vwap);
		break;
	}
	case QUOTE_LIB__MSG_TYPES__INDEX: {
		QuoteLib__Index *index = (QuoteLib__Index *) self->value;
#ifdef DEBUG
		printf("Index : %s %s : %14.2f%14.2f%14.2f%14.2f%14.2f%14d%14d\n",
				self->key, index->trdtime, index->last, index->change,
				index->open, index->high, index->low, index->volume,
				index->value);
#endif
		break;
	}
	case QUOTE_LIB__MSG_TYPES__EQ_SUMM:

		break;
	case QUOTE_LIB__MSG_TYPES__DIVIDEND:

		break;
	case QUOTE_LIB__MSG_TYPES__HILO52:

		break;
	case QUOTE_LIB__MSG_TYPES__EQUITY_VOLUME:

		break;
	}

}

void kvmsg_zero(kvmsg_t *self) {
	assert(self);
}
