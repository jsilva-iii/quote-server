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

import java.util.Random;
import java.util.concurrent.BlockingQueue;

import org.acme.marketdata.zmq.Msgs.MsgTypes;
import org.zeromq.ZFrame;
import org.zeromq.ZMsg;

import com.google.protobuf.UninitializedMessageException;

public class QuotePrimer implements Runnable {
    public BlockingQueue<ZMsg> mInqeue;
    String msymbol;
    int nMsg;
    int mtype;

    public QuotePrimer(final BlockingQueue<ZMsg> inqeue, final String symbol,
            int nMesages, int type) {
        mInqeue = inqeue;
        msymbol = symbol;
        nMsg = nMesages;
        mtype = type;
    }

    @Override
    public void run() {        
        int n = 0;
        for (n = 0; n < nMsg; n++) {
            switch (mtype) {
            case MsgTypes.QUOTE_VALUE:
                mInqeue.add(newQuote(n));
                break;
            case MsgTypes.TRADE_VALUE:
                mInqeue.add(newTrade(n));
                break;
            }
        }
    }

    public ZMsg newQuote(int n) {
        Random rn = new Random();
        ZFrame hdr = new ZFrame(String.format("%02d", MsgTypes.QUOTE_VALUE)
                + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE + msymbol
                + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
        org.acme.marketdata.zmq.Msgs.Quote quote = Msgs.Quote.newBuilder()
                .setMsgType(MsgTypes.QUOTE)
                .setAsk(rn.nextInt(510) / 100).setAskSize(n)
                .setBid(rn.nextInt(500) / 100).setBidSize(n).build();
        ZFrame body = new ZFrame(quote.toByteArray());
        ZMsg msg = new ZMsg();
        msg.add(hdr);
        msg.add(body);
        return msg;
    }

    public ZMsg newTrade(int n) {
        Random rn = new Random();
        org.acme.marketdata.zmq.Msgs.Trade quote = null;
        ZFrame hdr = new ZFrame(String.format("%02d", MsgTypes.TRADE_VALUE)
                + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE + msymbol
                + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
        try {
        quote = Msgs.Trade.newBuilder()
                .setMsgType(MsgTypes.TRADE)
                .setTrdTime("080000")                
                .setLast(rn.nextInt(510) / 100).setVolume(n)
                .build();
        }catch( UninitializedMessageException ie) {
            ie.printStackTrace();    
        }
        byte [] ba = quote.toByteArray();
        ZFrame body = new ZFrame(ba);
        ZMsg msg = new ZMsg();
        msg.add(hdr);
        msg.add(body);
        return msg;
    }
}
