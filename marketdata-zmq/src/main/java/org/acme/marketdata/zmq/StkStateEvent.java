//Copyright (C) Apr 2013 Jorge Silva, Gordon Tani
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
import java.text.SimpleDateFormat;
import java.util.Date;

import org.acme.marketdata.zmq.Msgs.StockStatus;
import org.marketcetera.event.EventType;
import org.marketcetera.event.QuoteAction;
import org.marketcetera.event.QuoteEvent;
import org.marketcetera.trade.Equity;
import org.marketcetera.trade.Instrument;


public class StkStateEvent implements QuoteEvent {
	/**
     * 
     */
    private static final long serialVersionUID = -492980682848747036L;
    /**
     * 
     */  
    private String msymbol;
	private Equity mequity;
	private String mStrTimeStamp;
	private long mID;
	private StockStatus mstate;
	private String mopenTime;
	private Object msource;
	private String exch;
	private long mtimestap = System.currentTimeMillis();
	private int mStkGrp;
	//private static final String description = "Equity Moc"; //$NON-NLS-1$
	//private static final long serialVersionUID = 1L;
	public StkStateEvent(String sym, StockStatus state, String topen, int stkGrp) {
		msymbol = sym;		
		mequity = new Equity(sym);		
		mID = System.nanoTime();		
		mStrTimeStamp = new SimpleDateFormat("hhmmss").format( mtimestap);
		mstate = state;
		mopenTime = topen;
		mStkGrp = stkGrp;
		exch = sym.endsWith("-V")?"V":"TO";		
	}

	public String getSymbol() {
		return msymbol;
	}

	@Override
	public String toString() {
		return mequity.toString() + " : State " +  mstate + "expected Open " + mopenTime ;
	}

	@Override
	public String getExchange() {
		return exch;
	}

	@Override
	public BigDecimal getPrice() {
		return null;
	}

	@Override
	public BigDecimal getSize() {
		return null;
	}

	@Override
	public String getExchangeTimestamp() {
		return mStrTimeStamp;
	}

	@Override
	public EventType getEventType() {
		return null;
	}

	@Override
	public void setEventType(EventType inEventType) {
		// TODO Auto-generated method stub
	}

	
	@Override
	public long getMessageId() {
		return mID;
	}

	@Override
	public Date getTimestamp() {
		return new Date(mtimestap);
	}

	@Override
	public Object getSource() {
		return msource;
	}

	@Override
	public void setSource(Object inSource) {
		msource = inSource;
	}

	@Override
	public long getTimeMillis() {
		return mtimestap;
	}

	@Override
	public Instrument getInstrument() {
		return mequity;
	}

	@Override
	public String getInstrumentAsString() {
		return mequity.toString();
	}

	@Override
	public String getQuoteDate() {
		return mStrTimeStamp;
	}

	@Override
	public QuoteAction getAction() {
		// TODO Auto-generated method stub
		return null;
	}
	public String getExpectedOpen() {
	    return mopenTime;
	}
	
	public StockStatus getState() {
	    return mstate;
	}
	
	public int getStkGrp() {
	    return mStkGrp;
	}
}
