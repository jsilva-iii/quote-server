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
package org.acme.marketdata.zmq;

import java.math.BigDecimal;
import java.util.Date;

import org.acme.marketdata.zmq.Msgs.FutStat;
import org.acme.marketdata.zmq.Msgs.FutTrade;
import org.marketcetera.event.AskEvent;
import org.marketcetera.event.BidEvent;
import org.marketcetera.event.MarketstatEvent;
import org.marketcetera.event.TopOfBookEvent;
import org.marketcetera.event.TradeEvent;
import org.marketcetera.event.impl.MarketstatEventBuilder;
import org.marketcetera.event.impl.QuoteEventBuilder;
import org.marketcetera.event.impl.TopOfBookEventBuilder;
import org.marketcetera.event.impl.TradeEventBuilder;
import org.marketcetera.marketdata.DateUtils;
import org.marketcetera.trade.Instrument;
import org.marketcetera.util.log.SLF4JLoggerProxy;

public class FutureMarketEvent {
    private Instrument mInstr;
    private QuoteEventBuilder<BidEvent> bidBuilder = QuoteEventBuilder
            .futureBidEvent();
    private QuoteEventBuilder<AskEvent> askBuilder = QuoteEventBuilder
            .futureAskEvent();
    private TopOfBookEventBuilder tobBuilder = TopOfBookEventBuilder
            .topOfBookEvent();
    private MarketstatEventBuilder mksBuilder = MarketstatEventBuilder
            .futureMarketstat();
    private TradeEventBuilder<TradeEvent> trdbuilder = TradeEventBuilder
            .futureTradeEvent();

    //private FutStat.Builder mktBuilder = FutStat.newBuilder();

    private float mOpenPx = Float.NaN;

    private float mHighPx = Float.NaN;

    private float mLowPx = Float.NaN;

    private float mVolume = (float) 0.0;

    private float mPClose = Float.NaN;

    private float mClose = Float.NaN;

    private float mValue = (float) 0.0;

    private boolean mhaveMktStat = false;
    // private int mNumTrades = 0;

    private String mlowTime = "080000";

    private String mhighTime = "080000";
    private String Exch;

    private int contractsz;

    public FutureMarketEvent(Instrument instr, String xchg, int csz) {
        mInstr = instr;
        Exch = xchg;
        contractsz = csz;
    }

    public synchronized TopOfBookEvent getTopOfBookEvent(float bid, float ask,
            float bidsz, float asksz) {
        Date now = new Date();
        BidEvent be = bidBuilder
                .withInstrument(mInstr)
                .withContractSize(contractsz)
                .withMessageId(System.nanoTime())
                .withTimestamp(now)
                .withQuoteDate(DateUtils.dateToString(now))
                .withExchange(Exch)
                .withPrice(
                        new BigDecimal(bid).setScale(3,
                                BigDecimal.ROUND_HALF_EVEN))
                .withSize(new BigDecimal(bidsz)).create();
        AskEvent ae = askBuilder
                .withInstrument(mInstr)
                .withContractSize(contractsz)
                .withMessageId(System.nanoTime())
                .withTimestamp(now)
                .withQuoteDate(DateUtils.dateToString(now))
                .withExchange(Exch)
                .withPrice(
                        new BigDecimal(ask).setScale(3,
                                BigDecimal.ROUND_HALF_EVEN))
                .withSize(new BigDecimal(asksz)).create();
        return tobBuilder.withMessageId(System.nanoTime()).withTimestamp(now)
                .withBid(be).withInstrument(mInstr).withAsk(ae).create();
    }

    public synchronized TradeEvent getTradeEvent(FutTrade trd) {
        float px = trd.getLast();
        float sz = trd.getVolume();
        String trdtime = trd.getTrdTime();   
        if (mhaveMktStat) {
            if (Double.isNaN(mLowPx) || px < mLowPx  ) {
                mLowPx = px;
                mlowTime = trdtime;
            }

            if (Double.isNaN(mHighPx)|| px > mHighPx  ) {
                mHighPx = px;
                mhighTime = trdtime;
            }

            if (Float.isNaN(mPClose))
                mPClose = trd.getLast() - trd.getChg();
            if (Float.isNaN(mOpenPx))
                mOpenPx = trd.getLast();

            mVolume += sz;
            mValue += px * sz * contractsz;
        }
        return trdbuilder
                .withContractSize(contractsz)
                .withMessageId(System.nanoTime())
                .withTimestamp(new Date())
                .withTradeDate(trdtime)
                .withInstrument(mInstr)
                .withExchange(Exch)
                .withPrice(
                        new BigDecimal(Float.isNaN(px)?0:px).setScale(3,
                                BigDecimal.ROUND_HALF_EVEN))
                .withSize(new BigDecimal(sz)).create();
    }

    public synchronized MarketstatEvent getMktStat(FutStat stat, String today,
            String yest) {

        if (stat != null) {
            mOpenPx = stat.getOpen();
            mClose = stat.getSettle();
            mPClose = stat.getPsettle();
            mHighPx = stat.getHigh();
            mhighTime = stat.getHitime();
            mLowPx = stat.getLow();
            mlowTime = stat.getLotime();
            mVolume = stat.getVolume();
            mValue = stat.getValue();
            mhaveMktStat = true;
        }
        // mNumTrades = stat.getNumtrades();

        SLF4JLoggerProxy.debug(this,
                "Marketstat Event symbol {} close {} low {} high {} close {}", //$NON-NLS-1$
                mInstr.getSymbol(), mPClose, mLowPx, mHighPx, mPClose);
        return mksBuilder
                .withContractSize(contractsz)
                // .withMultiplier(new BigDecimal(stat.getMultiplier()))
                .withOpenPrice(
                        Float.isNaN(mOpenPx) ?null : new BigDecimal(mOpenPx)
                                .setScale(3, BigDecimal.ROUND_HALF_EVEN))
                .withHighPrice(
                        Float.isNaN(mHighPx) ? BigDecimal.ZERO : new BigDecimal(mHighPx)
                                .setScale(3, BigDecimal.ROUND_HALF_EVEN))
                .withLowPrice(
                        Float.isNaN(mLowPx) ? BigDecimal.ZERO : new BigDecimal(mLowPx)
                                .setScale(3, BigDecimal.ROUND_HALF_EVEN))
                .withClosePrice(
                        Float.isNaN(mClose) ? null : new BigDecimal(mClose)
                                .setScale(3, BigDecimal.ROUND_HALF_EVEN))
                .withPreviousClosePrice(
                        Float.isNaN(mPClose) ? null : new BigDecimal(mPClose)
                                .setScale(3, BigDecimal.ROUND_HALF_EVEN))
                .withVolume(
                        new BigDecimal(mVolume).setScale(0,
                                BigDecimal.ROUND_HALF_EVEN))
                .withValue(
                        Float.isNaN(mValue)?BigDecimal.ZERO:new BigDecimal(mValue).setScale(2,
                                BigDecimal.ROUND_HALF_EVEN))
                .withCloseDate(today).withPreviousCloseDate(yest)
                .withTradeHighTime(mhighTime).withTradeLowTime(mlowTime)
                .withOpenExchange(Exch).withHighExchange(Exch)
                .withLowExchange(Exch).withCloseExchange(Exch)
                .withInstrument(mInstr).create();
    }    
}
