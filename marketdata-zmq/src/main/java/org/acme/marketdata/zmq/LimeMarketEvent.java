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

import org.acme.marketdata.zmq.Msgs.Index;
import org.acme.marketdata.zmq.Msgs.MktStat;
import org.acme.marketdata.zmq.Msgs.Trade;
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
import org.marketcetera.trade.Equity;
import org.marketcetera.util.log.SLF4JLoggerProxy;

public class LimeMarketEvent {
    private Equity equity;
    private QuoteEventBuilder<BidEvent> bidBuilder = QuoteEventBuilder
            .equityBidEvent();
    private QuoteEventBuilder<AskEvent> askBuilder = QuoteEventBuilder
            .equityAskEvent();
    private TopOfBookEventBuilder tobBuilder = TopOfBookEventBuilder
            .topOfBookEvent();
    private MarketstatEventBuilder mksBuilder = MarketstatEventBuilder
            .equityMarketstat();
    private TradeEventBuilder<TradeEvent> trdbuilder = TradeEventBuilder
            .equityTradeEvent();

    // private MktStat.Builder mktBuilder = MktStat.newBuilder();

    private double mOpenPx = Double.NaN;

    private double mHighPx = Double.NaN;

    private double mLowPx = Double.NaN;

    private double mVolume = 0;

    private double mClose = Double.NaN;

    private double mPClose = Double.NaN;

    private double mValue = 0.0;

    @SuppressWarnings("unused")
    private long mNumTrades = 0;

    private String lowTime = "080000";

    private String highTime = "080000";

    private String Exch;

    private boolean mhaveMktStat = false;

    private int isBoardLot(double px) {
        return Double.isNaN(px) ? 1 : (px > 1.0 ? 100 : px < 0.1 ? 1000 : 500);
    }

    public LimeMarketEvent(String symbol, String xchg) {
        equity = new Equity(symbol);
        Exch = xchg;
    }

    public synchronized TopOfBookEvent getTopOfBookEvent(double bid,
            double ask, int bidsz, int asksz) {
        Date now = new Date();

        BidEvent be = bidBuilder
                .withInstrument(equity)
                .withContractSize(1)
                .withMessageId(System.nanoTime())
                .withTimestamp(now)
                .withQuoteDate(DateUtils.dateToString(now))
                .withExchange(Exch)
                .withPrice(
                        new BigDecimal(bid).setScale(bid < 0.5 ? 4 : 3,
                                BigDecimal.ROUND_HALF_EVEN))
                .withSize(new BigDecimal(bidsz)).create();
        AskEvent ae = askBuilder
                .withInstrument(equity)
                .withContractSize(1)
                .withMessageId(System.nanoTime())
                .withTimestamp(now)
                .withQuoteDate(DateUtils.dateToString(now))
                .withExchange(Exch)
                .withPrice(
                        new BigDecimal(ask).setScale(ask < 0.5 ? 4 : 3,
                                BigDecimal.ROUND_HALF_EVEN))
                .withSize(new BigDecimal(asksz)).create();
        return tobBuilder.withMessageId(System.nanoTime()).withTimestamp(now)
                .withBid(be).withInstrument(equity).withAsk(ae).create();
    }

    public synchronized TopOfBookEvent getIndexQuoteEvent(Index index) {
        Date now = new Date();

        BidEvent be = bidBuilder
                .withInstrument(equity)
                .withContractSize(1)
                .withMessageId(System.nanoTime())
                .withTimestamp(now)
                .withQuoteDate(DateUtils.dateToString(now))
                .withExchange(Exch)
                .withPrice(
                        new BigDecimal(index.getBid()).setScale(4,
                                BigDecimal.ROUND_HALF_EVEN))
                .withSize(new BigDecimal(0)).create();
        AskEvent ae = askBuilder
                .withInstrument(equity)
                .withContractSize(1)
                .withMessageId(System.nanoTime())
                .withTimestamp(now)
                .withQuoteDate(DateUtils.dateToString(now))
                .withExchange(Exch)
                .withPrice(
                        new BigDecimal(index.getAsk()).setScale( 4,
                                BigDecimal.ROUND_HALF_EVEN))
                .withSize(new BigDecimal(0)).create();
        return tobBuilder.withMessageId(System.nanoTime()).withTimestamp(now)
                .withBid(be).withInstrument(equity).withAsk(ae).create();
    }
    public synchronized TradeEvent getIndexTradeEvent(Index index) {

        double px = index.getLast();
        double sz = index.getVolume();
       // String trdtime = index.getTrdTime();

        if (Double.isNaN(mLowPx) || px < mLowPx) {
            mLowPx = px;
           // lowTime = "093000";
        }

        if (Double.isNaN(mHighPx) || px > mHighPx) {
            mHighPx = px;
          //  highTime = trdtime;
        }

        if (Double.isNaN(mOpenPx))
            mOpenPx = px;

        if (Double.isNaN(mClose))
            mClose = px - index.getChange();

      
        mValue = index.getValue();
        mNumTrades += 1;
        double diff = sz - mVolume;
        mVolume += diff;
        return trdbuilder
                .withContractSize(1)
                .withMessageId(System.nanoTime())
                .withTimestamp(new Date())
                .withTradeDate("093000")
                .withInstrument(equity)
                .withExchange(Exch)
                .withPrice(
                        new BigDecimal(px).setScale( 4,
                                BigDecimal.ROUND_HALF_EVEN))
                .withSize(new BigDecimal(diff).setScale( 2,
                        BigDecimal.ROUND_HALF_EVEN)).create();       
    }

    public synchronized TradeEvent getTradeEvent(Trade trd) {
        double px = trd.getLast();
        double trd_px = trd.getTrdPx();
        long sz = trd.getVolume();
        String trdtime = trd.getTrdTime();
        if (trd.hasIscxl() && trd.getIscxl()) {
            mVolume -= sz;
            mValue -= px * sz;
            mNumTrades -= 1;
            return null;
        } else {
            if (mhaveMktStat) {
                boolean isBlot = isBoardLot(px) <= sz;
                if (isBlot) {
                    if (Double.isNaN(mLowPx) || px < mLowPx) {
                        mLowPx = px;
                        lowTime = trdtime;
                    }
                    if (Double.isNaN(mHighPx) || px > mHighPx) {
                        mHighPx = px;
                        highTime = trdtime;
                    }
                }
                if (Double.isNaN(mOpenPx)  && trd.getIsopen())
                    mOpenPx = px;

                mVolume += sz;
                mValue += trd_px * sz;
                mNumTrades += sz != 0 ? 1 : 0;
            }
            return trdbuilder
                    .withContractSize(1)
                    .withMessageId(System.nanoTime())
                    .withTimestamp(new Date())
                    .withTradeDate(trdtime)
                    .withInstrument(equity)
                    .withExchange(Exch)
                    .withPrice(
                            new BigDecimal(Double.isNaN(px) ? 0 : px).setScale(
                                    px < 0.5 ? 4 : 3,
                                    BigDecimal.ROUND_HALF_EVEN))
                    .withSize(new BigDecimal(sz)).create();
        }
    }

    /*
     * public synchronized MktStat getStat() { float close = (float) ((mClose ==
     * Double.MAX_VALUE) ? Double.NaN : mClose); return
     * mktBuilder.setClose(close).setHigh((float) mHighPx)
     * .setHightime(highTime).setLow((float) mLowPx)
     * .setLowtime(lowTime).setPclose((float) close).setClose(close)
     * .setVolume(mVolume).setValue((float) mValue) .setNumtrades((int)
     * mNumTrades).setMsgType(MsgTypes.MKT_STAT) .setOpen((float)
     * mOpenPx).build(); }
     */

    public synchronized boolean isInitMktStat() {
        return mhaveMktStat;
    }

    public synchronized MarketstatEvent getMktStat(MktStat stat, String today,
            String yest) {

        SLF4JLoggerProxy.debug(this,
                "Marketstat Event symbol {} close {} low {} high {} close {}", //$NON-NLS-1$ 
                equity.getSymbol(), mClose, mLowPx, mHighPx, mClose);

        if (stat != null) { // have received an update
            mOpenPx = stat.getOpen();
            mPClose = stat.getPclose();
            mLowPx = stat.getLow();
            mHighPx = stat.getHigh();
            mVolume = stat.getVolume();
            mValue = stat.getValue();
            highTime = stat.getHightime();
            lowTime = stat.getLowtime();
            mhaveMktStat = true;
        }

        return mksBuilder
                .withContractSize(1)
                .withOpenPrice(
                        Double.isNaN(mOpenPx) ? null : new BigDecimal(mOpenPx)
                                .setScale(mOpenPx < 0.5 ? 3 : 2,
                                        BigDecimal.ROUND_HALF_EVEN))
                .withHighPrice(
                        new BigDecimal(Double.isNaN(mHighPx) ? 0.0 : mHighPx)
                                .setScale(mHighPx < 0.5 ? 3 : 2,
                                        BigDecimal.ROUND_HALF_EVEN))
                .withLowPrice(
                        new BigDecimal(Double.isNaN(mLowPx) ? 0 : mLowPx)
                                .setScale(mLowPx < 0.5 ? 3 : 2,
                                        BigDecimal.ROUND_HALF_EVEN))
                .withClosePrice(
                        Double.isNaN(mClose) ? null : new BigDecimal(mClose)
                                .setScale(mClose < 0.5 ? 3 : 2,
                                        BigDecimal.ROUND_HALF_EVEN))
                .withPreviousClosePrice(
                        Double.isNaN(mPClose) ? null : new BigDecimal(mPClose)
                                .setScale(mPClose < 0.5 ? 3 : 2,
                                        BigDecimal.ROUND_HALF_EVEN))
                .withVolume(
                        new BigDecimal(mVolume).setScale(0,
                                BigDecimal.ROUND_HALF_EVEN))
                .withValue(
                        new BigDecimal(mValue).setScale(3,
                                BigDecimal.ROUND_HALF_EVEN))
                .withCloseDate(today).withPreviousCloseDate(yest)
                .withTradeHighTime(highTime).withTradeLowTime(lowTime)
                .withOpenExchange(Exch).withHighExchange(Exch)
                .withLowExchange(Exch).withCloseExchange(Exch)
                .withInstrument(equity).create();

        // return null;
    }
}
