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

import org.acme.marketdata.zmq.Msgs.MsgTypes;
import org.acme.marketdata.zmq.Msgs.OptStat;
import org.acme.marketdata.zmq.Msgs.OptTrade;
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
import org.marketcetera.options.ExpirationType;
import org.marketcetera.trade.Equity;
import org.marketcetera.trade.Option;
import org.marketcetera.trade.Instrument;
import org.marketcetera.util.log.SLF4JLoggerProxy;

public class OptionMarketEvent {
    private Instrument mInstr;

    private Equity opt_ul;
    private QuoteEventBuilder<BidEvent> bidBuilder = QuoteEventBuilder
            .optionBidEvent();
    private QuoteEventBuilder<AskEvent> askBuilder = QuoteEventBuilder
            .optionAskEvent();
    private TopOfBookEventBuilder tobBuilder = TopOfBookEventBuilder
            .topOfBookEvent();
    private MarketstatEventBuilder mksBuilder = MarketstatEventBuilder
            .optionMarketstat();
    private TradeEventBuilder<TradeEvent> trdbuilder = TradeEventBuilder
            .optionTradeEvent();

    private OptStat.Builder mktBuilder = OptStat.newBuilder();

    private float mOpenPx = Float.NaN;

    private float mHighPx = Float.NaN;

    private float mLowPx = Float.NaN;

    private float mVolume = (float) 0.0;

    private float mPClose = Float.NaN;

    private float mClose = Float.NaN;

    private float mValue = (float) 0.0;

    private String lowTime = "080000";

    private String highTime = "080000";

    private String Exch = "BDM";

    private int contractsz = 0;

    private int mOpenInt = 0;

    private BigDecimal bdcontractsz;

    private boolean misAmer;

    private float mStrike;

    private String mExpiry;

    private boolean mhaveMktStat = false;

    public OptionMarketEvent(Instrument instr, String xchg, int csz,
            boolean isamer) {
        mInstr = instr;
        Exch = xchg;
        contractsz = csz;
        opt_ul = new Equity(instr.getSymbol().substring(0,
                instr.getSymbol().indexOf('-')));
        misAmer = isamer;
        bdcontractsz = new BigDecimal(contractsz);
        mStrike = ((Option) instr).getStrikePrice().floatValue();
        mExpiry = ((Option) instr).getExpiry();

    }

    public synchronized TopOfBookEvent getTopOfBookEvent(float bid, float ask,
            float bidsz, float asksz) {
        Date now = new Date();
        BidEvent be = bidBuilder
                .withInstrument(mInstr)
                .withMultiplier(bdcontractsz)
                .withMessageId(System.nanoTime())
                .withTimestamp(now)
                .withQuoteDate(DateUtils.dateToString(now))
                .withExchange(Exch)
                .withPrice(
                        new BigDecimal(bid).setScale(3,
                                BigDecimal.ROUND_HALF_EVEN))
                .withUnderlyingInstrument(opt_ul)
                .withExpirationType(
                        misAmer ? ExpirationType.AMERICAN
                                : ExpirationType.EUROPEAN)
                .withSize(new BigDecimal(bidsz)).create();
        AskEvent ae = askBuilder
                .withInstrument(mInstr)
                .withMultiplier(bdcontractsz)
                .withMessageId(System.nanoTime())
                .withTimestamp(now)
                .withQuoteDate(DateUtils.dateToString(now))
                .withExchange(Exch)
                .withPrice(
                        new BigDecimal(ask).setScale(3,
                                BigDecimal.ROUND_HALF_EVEN))
                .withUnderlyingInstrument(opt_ul)
                .withExpirationType(
                        misAmer ? ExpirationType.AMERICAN
                                : ExpirationType.EUROPEAN)
                .withSize(new BigDecimal(asksz)).create();
        return tobBuilder.withMessageId(System.nanoTime()).withTimestamp(now)
                .withBid(be).withInstrument(mInstr).withAsk(ae).create();
    }

    public synchronized TradeEvent getTradeEvent(OptTrade trd) {
        float px = trd.getLast();
        float sz = trd.getVolume();
        String trdtime = trd.getTrdTime();
        if (mhaveMktStat) {
            if (Double.isNaN(mLowPx) || px < mLowPx) {
                mLowPx = px;
                lowTime = trdtime;
            }
            if (Double.isNaN(mHighPx) || px > mHighPx) {
                mHighPx = px;
                highTime = trdtime;
            }
            mVolume += sz;
            mValue += px * sz * contractsz;
            if (Float.isNaN(mOpenPx))
                mOpenPx = px;
            if (Float.isNaN(mPClose))
                mPClose = px - trd.getChg();
        }
      
        return trdbuilder
                // .withContractSize(contractsz)
                .withMultiplier(bdcontractsz)
                .withMessageId(System.nanoTime())
                .withTimestamp(new Date())
                .withTradeDate(trdtime)
                .withInstrument(mInstr)
                .withExchange(Exch)
                .withUnderlyingInstrument(opt_ul)
                .withExpirationType(
                        misAmer ? ExpirationType.AMERICAN
                                : ExpirationType.EUROPEAN)
                .withPrice(
                        Float.isNaN(px)?BigDecimal.ZERO:(new BigDecimal(px).setScale(3,
                                BigDecimal.ROUND_HALF_EVEN)))
                .withSize(new BigDecimal(sz)).create();
    }

    public synchronized MarketstatEvent getMktStat(OptStat stat, String today,
            String yest) {
       
        if (stat != null) {
            mOpenPx = stat.getOpen();
            mPClose = stat.getPdclose();
            mClose = stat.getClose();
            mHighPx = stat.getHigh();            
            highTime = stat.getHitime();
            mLowPx = stat.getLow();
            lowTime = stat.getLotime();
            if (lowTime.length()==0)
                lowTime="093000";
            if (highTime.length()==0)
                highTime="093000";
            mVolume = stat.getVolume();
            mValue = stat.getValue();
            mOpenInt = stat.getOi();
            mhaveMktStat = true;
        }
        
        SLF4JLoggerProxy.debug(this,
                "Marketstat Event symbol {} close {} low {} high {} close {}", //$NON-NLS-1$
                mInstr.getSymbol(), mPClose, mLowPx, mHighPx, mPClose);
        return mksBuilder
                .withMultiplier(bdcontractsz)
                // .withMultiplier(new BigDecimal(stat.getMultiplier()))
                .withOpenPrice(
                        Float.isNaN(mOpenPx) ? null : new BigDecimal(mOpenPx)
                                .setScale(3, BigDecimal.ROUND_HALF_EVEN))
                .withHighPrice(
                        Float.isNaN(mHighPx) ? null : new BigDecimal(mHighPx)
                                .setScale(3, BigDecimal.ROUND_HALF_EVEN))
                .withLowPrice(
                        Float.isNaN(mLowPx) ? null : new BigDecimal(mLowPx).setScale(
                                3, BigDecimal.ROUND_HALF_EVEN))
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
                        new BigDecimal(mValue).setScale(2,
                                BigDecimal.ROUND_HALF_EVEN))
                .withCloseDate(today)
                .withPreviousCloseDate(yest)
                .withTradeHighTime(highTime)
                .withUnderlyingInstrument(opt_ul)
                .withMultiplier(bdcontractsz)
                .withExpirationType(
                        misAmer ? ExpirationType.AMERICAN
                                : ExpirationType.EUROPEAN)
                .withTradeLowTime(lowTime).withOpenExchange(Exch)
                .withHighExchange(Exch).withLowExchange(Exch)
                .withCloseExchange(Exch).withInstrument(mInstr).create();
    }   
}
