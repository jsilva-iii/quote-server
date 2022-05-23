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

package org.bwcc.marketdata.zmq;
import java.math.BigDecimal;
import java.util.Date;




import org.acme.marketdata.zmq.Msgs.Depthquote;
import org.marketcetera.event.AskEvent;
import org.marketcetera.event.BidEvent;
import org.marketcetera.event.QuoteAction;
import org.marketcetera.event.impl.QuoteEventBuilder;
import org.marketcetera.marketdata.DateUtils;
import org.marketcetera.options.ExpirationType;
import org.marketcetera.trade.Equity;
import org.marketcetera.trade.Instrument;
import org.marketcetera.trade.Option;


public class BDMDepthQuotes {
	public final int DEPTH_OF_QUOTES = 5;
	private DepthQuoteElement bids_asks[] = new DepthQuoteElement[DEPTH_OF_QUOTES];
	private QuoteEventBuilder<BidEvent> bbid_fut = QuoteEventBuilder
			.futureBidEvent();// = TradeEventBuilder.futureBidevent();
	private QuoteEventBuilder<AskEvent> bask_fut = QuoteEventBuilder
			.futureAskEvent();
	private QuoteEventBuilder<BidEvent> bbid_opt = QuoteEventBuilder
			.optionBidEvent();// = TradeEventBuilder.futureBidevent();
	private QuoteEventBuilder<AskEvent> bask_opt = QuoteEventBuilder
			.optionAskEvent();
	private Instrument mInstr;
	private boolean mIsFuture;
	private boolean mIsAmerican;
	private Equity underlying = null;
	public enum ChangeType {
		NEW, DEL, UPDT, NOCHG
	};

	public class DepthQuoteElement {
		public int bidsz = 0;
		public double bid = 0.0;
		public int asksz = 0;
		public int askOrds = 0;
		public int bidOrds = 0;
		public double ask = 0.0;
		public ChangeType bidChg = ChangeType.NOCHG;
		public ChangeType askChg = ChangeType.NOCHG;
		public Date asktime = new Date();
		public Date bidtime =new Date();
	}

	public BDMDepthQuotes(org.marketcetera.trade.Instrument instrument,
			boolean isAmerican) {
		mInstr = instrument;
		mIsAmerican = isAmerican;
		if (mInstr instanceof Option) {
			underlying = new Equity(instrument.getSymbol().substring(0,
					instrument.getSymbol().indexOf('-')));
			mIsFuture = false;
		} else
			mIsFuture = true;
		for (int n = 0; n < DEPTH_OF_QUOTES; n++) {
			bids_asks[n] = new DepthQuoteElement();
		}
	}

	public void update(Depthquote dqi) {
		int level = dqi.getLevel();// - 1;
		if (level < 5) {
			double nbid = dqi.getBid();
			double nask = dqi.getAsk();
			int nasksz =  dqi.getAskSize();
			int nbidsz =  dqi.getBidSize();
			int naskords =  dqi.getNaskOrd();
			int nbidords = dqi.getNbidOrd();
			if (nasksz != 0 && bids_asks[level].asksz == 0) { // New
				bids_asks[level].askChg = ChangeType.NEW;
			} else if (nasksz == 0 && bids_asks[level].asksz != 0) { // Del
				bids_asks[level].askChg = ChangeType.DEL;
			} else if (nasksz != bids_asks[level].asksz
					|| (nasksz == bids_asks[level].asksz && nask != bids_asks[level].ask)) {
				bids_asks[level].askChg = ChangeType.UPDT;
			} else {
				bids_asks[level].askChg = ChangeType.NOCHG;
			}

			if (bids_asks[level].askChg != ChangeType.NOCHG) {
				bids_asks[level].ask = nask;
				bids_asks[level].asksz = nasksz;
				bids_asks[level].askOrds = naskords;
				bids_asks[level].asktime.setTime(dqi.getAsktime());
			}

			if (nbidsz != 0 && bids_asks[level].bidsz == 0 ) { // New
				bids_asks[level].bidChg = ChangeType.NEW;
			} else if (nbidsz == 0 && bids_asks[level].bidsz != 0) { // Del
				bids_asks[level].bidChg = ChangeType.DEL;
			} else if (nbidsz != bids_asks[level].bidsz
					|| (nbidsz == bids_asks[level].bidsz && nbid != bids_asks[level].bid)) {
				bids_asks[level].bidChg = ChangeType.UPDT;
			} else {
				bids_asks[level].bidChg = ChangeType.NOCHG;
			}

			if (bids_asks[level].bidChg != ChangeType.NOCHG) {
				bids_asks[level].bid = nbid;
				bids_asks[level].bidsz = nbidsz;
				bids_asks[level].bidOrds = nbidords;
				bids_asks[level].bidtime.setTime(dqi.getBidtime());
			}
		}
	}

	public synchronized AskEvent getAskEvent(int level) {
		AskEvent askevent = null;
		if (bids_asks[level].askChg != ChangeType.NOCHG) {
			//Date now = new Date();
			if (mIsFuture) {
				askevent = bask_fut
						.withMessageId(level + DEPTH_OF_QUOTES)
						.withTimestamp(bids_asks[level].asktime)
						.withQuoteDate(DateUtils.dateToString(bids_asks[level].asktime))
						.withInstrument(mInstr)
						.withExchange( Integer.toString(bids_asks[level].askOrds))
						.withAction(
								bids_asks[level].askChg == ChangeType.NEW ? QuoteAction.ADD
										: (bids_asks[level].askChg == ChangeType.DEL ? QuoteAction.DELETE
												: QuoteAction.CHANGE))
						.withPrice(
								new BigDecimal(bids_asks[level].ask)
										.setScale(3, BigDecimal.ROUND_HALF_EVEN))
						.withSize(new BigDecimal(bids_asks[level].asksz))
						.create();
			} else {
				askevent = bask_opt
						.withMessageId(level + DEPTH_OF_QUOTES)
						.withTimestamp(bids_asks[level].asktime)
						.withQuoteDate(DateUtils.dateToString(bids_asks[level].asktime))
						.withInstrument(mInstr)
						.withExchange( Integer.toString(bids_asks[level].askOrds))
						.withUnderlyingInstrument(underlying)
						.withAction(
								bids_asks[level].askChg == ChangeType.NEW ? QuoteAction.ADD
										: (bids_asks[level].askChg == ChangeType.DEL ? QuoteAction.DELETE
												: QuoteAction.CHANGE))
						.withExpirationType(
								mIsAmerican ? ExpirationType.AMERICAN
										: ExpirationType.EUROPEAN)
						.withPrice(
								new BigDecimal(bids_asks[level].ask)
										.setScale(
												bids_asks[level].ask < 0.01 ? 3
														: 2,
												BigDecimal.ROUND_HALF_EVEN))
						.withSize(new BigDecimal(bids_asks[level].asksz))
						.create();
			}
		}
		return askevent;
	}

	public synchronized BidEvent getBidEvent(int level) {
		BidEvent bidevent = null;
		if (bids_asks[level].bidChg != ChangeType.NOCHG) {
			//Date now = new Date();
			if (mIsFuture) {
				bidevent = bbid_fut
						.withMessageId(level)
						.withTimestamp(bids_asks[level].bidtime)
						.withQuoteDate(DateUtils.dateToString(bids_asks[level].bidtime))
						.withInstrument(mInstr)
						.withExchange( Integer.toString(bids_asks[level].bidOrds))
						.withAction(
								bids_asks[level].bidChg == ChangeType.NEW ? QuoteAction.ADD
										: (bids_asks[level].bidChg == ChangeType.DEL ? QuoteAction.DELETE
												: QuoteAction.CHANGE))
						.withPrice(
								new BigDecimal(bids_asks[level].bid)
										.setScale(3, BigDecimal.ROUND_HALF_EVEN))
						.withSize(new BigDecimal(bids_asks[level].bidsz))
						.create();
			} else {
				bidevent = bbid_opt
						.withMessageId(level)
						.withTimestamp(bids_asks[level].bidtime)
						.withQuoteDate(DateUtils.dateToString(bids_asks[level].bidtime))
						.withInstrument(mInstr)
						.withExchange( Integer.toString(bids_asks[level].bidOrds))
						.withUnderlyingInstrument(underlying)
						.withAction(
								bids_asks[level].bidChg == ChangeType.NEW ? QuoteAction.ADD
										: (bids_asks[level].bidChg == ChangeType.DEL ? QuoteAction.DELETE
												: QuoteAction.CHANGE))
						.withExpirationType(
								mIsAmerican ? ExpirationType.AMERICAN
										: ExpirationType.EUROPEAN)
						.withPrice(
								new BigDecimal(bids_asks[level].bid)
										.setScale(
												bids_asks[level].bid < 0.01 ? 3
														: 2,
												BigDecimal.ROUND_HALF_EVEN))
						.withSize(new BigDecimal(bids_asks[level].bidsz))
						.create();
			}
		}
		return bidevent;
	}
}
