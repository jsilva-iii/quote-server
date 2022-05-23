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

import org.marketcetera.event.AskEvent;
import org.marketcetera.event.BidEvent;
import org.marketcetera.event.TopOfBookEvent;
import org.marketcetera.event.impl.QuoteEventBuilder;
import org.marketcetera.event.impl.TopOfBookEventBuilder;
import org.marketcetera.marketdata.DateUtils;
import org.marketcetera.trade.Currency;

public class ForexMarketEvent {
    private Currency currency;
    private QuoteEventBuilder<BidEvent> bidBuilder = QuoteEventBuilder
            .currencyBidEvent();
    private QuoteEventBuilder<AskEvent> askBuilder = QuoteEventBuilder
            .currencyAskEvent();
    private TopOfBookEventBuilder tobBuilder = TopOfBookEventBuilder
            .topOfBookEvent();

    public ForexMarketEvent(String symbol) {
        currency = new Currency(symbol);
    }

    public synchronized TopOfBookEvent getTopOfBookEvent(double bid,
            double ask, int bidsz, int asksz) {
        Date now = new Date();

        BidEvent be = bidBuilder
                .withInstrument(currency)
                .withContractSize(1)
                .withMessageId(System.nanoTime())
                .withTimestamp(now)
                .withQuoteDate(DateUtils.dateToString(now))
                .withExchange("FX")
                .withPrice(
                        new BigDecimal(bid).setScale(5, BigDecimal.ROUND_HALF_EVEN))
                .withSize(new BigDecimal(bidsz)).create();
        AskEvent ae = askBuilder
                .withInstrument(currency)
                .withContractSize(1)
                .withMessageId(System.nanoTime())
                .withTimestamp(now)
                .withQuoteDate(DateUtils.dateToString(now))
                .withExchange("FX")
                .withPrice(
                        new BigDecimal(ask).setScale(5, BigDecimal.ROUND_HALF_EVEN))
                .withSize(new BigDecimal(asksz)).create();
        return tobBuilder.withMessageId(System.nanoTime()).withTimestamp(now)
                .withBid(be).withInstrument(currency).withAsk(ae).create();
    }
}
