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

//import static org.acme.marketdata.zmq.Messages.UNSUPPORTED_OPTION_SPECIFICATION;
import static org.marketcetera.marketdata.AssetClass.EQUITY;
import static org.marketcetera.marketdata.AssetClass.FUTURE;
import static org.marketcetera.marketdata.AssetClass.OPTION;
import static org.marketcetera.marketdata.AssetClass.CURRENCY;
import static org.marketcetera.marketdata.Capability.*;

import java.math.BigDecimal;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.text.DecimalFormat;
import java.text.SimpleDateFormat;
import java.util.*;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import org.marketcetera.core.NoMoreIDsException;
import org.marketcetera.core.notifications.INotification;
import org.marketcetera.core.notifications.Notification;
import org.marketcetera.core.notifications.NotificationManager;
import org.marketcetera.event.AskEvent;
import org.marketcetera.event.BidEvent;
import org.marketcetera.event.DividendEvent;
import org.marketcetera.event.DividendFrequency;
import org.marketcetera.event.DividendStatus;
import org.marketcetera.event.DividendType;
import org.marketcetera.event.MarketstatEvent;
import org.marketcetera.event.TopOfBookEvent;
import org.marketcetera.event.TradeEvent;
import org.marketcetera.event.Event;
import org.marketcetera.event.impl.DividendEventBuilder;
import org.marketcetera.marketdata.*;
import org.marketcetera.trade.Equity;
import org.marketcetera.trade.Future;
import org.marketcetera.trade.Option;
import org.marketcetera.trade.OptionType;
import org.marketcetera.trade.Instrument;
import org.marketcetera.util.log.SLF4JLoggerProxy;
import org.marketcetera.util.misc.ClassVersion;
import org.acme.marketdata.zmq.Msgs;
import org.acme.marketdata.zmq.Msgs.BDMDepth;
import org.acme.marketdata.zmq.Msgs.Depthquote;
import org.acme.marketdata.zmq.Msgs.DivCodes;
import org.acme.marketdata.zmq.Msgs.Dividend;
import org.acme.marketdata.zmq.Msgs.FutQuote;
import org.acme.marketdata.zmq.Msgs.FutRFQ;
import org.acme.marketdata.zmq.Msgs.FutStat;
import org.acme.marketdata.zmq.Msgs.FutTrade;
import org.acme.marketdata.zmq.Msgs.Index;
import org.acme.marketdata.zmq.Msgs.MarketState;
import org.acme.marketdata.zmq.Msgs.MktStat;
import org.acme.marketdata.zmq.Msgs.Moc;
import org.acme.marketdata.zmq.Msgs.MsgTypes;
import org.acme.marketdata.zmq.Msgs.OptQuote;
import org.acme.marketdata.zmq.Msgs.OptRFQ;
import org.acme.marketdata.zmq.Msgs.OptStat;
import org.acme.marketdata.zmq.Msgs.OptTrade;
import org.acme.marketdata.zmq.Msgs.Quote;
import org.acme.marketdata.zmq.Msgs.StkStatus;
import org.acme.marketdata.zmq.Msgs.StockStatus;
import org.acme.marketdata.zmq.Msgs.Trade;

import com.google.protobuf.InvalidProtocolBufferException;

import org.zeromq.ZMQ;
import org.zeromq.ZMsg;

//import quickfix.field.MsgType;
 
/* $License$ */

/**
 * Sample implementation of {@link MarketDataFeed}.
 * 
 * <p>
 * This implementation generates random market data for each symbol for which a
 * market data request is received. Data is returned from the feed via
 * {@link Event} objects.
 * 
 * @author <a href="mailto:colin@marketcetera.com">Colin DuPlantis</a>
 * @version $Id: ZMQFeed.java 16364 2012-11-08 06:13:58Z sameer.patil $
 * @since 0.5.0
 */
@ClassVersion("$Id: ZMQFeed.java 16364 2012-11-08 06:13:58Z sameer.patil $")
public class ZMQFeed
        extends
        AbstractMarketDataFeed<ZMQFeedToken, ZMQFeedCredentials, ZMQFeedMessageTranslator, ZMQFeedEventTranslator, MarketDataRequest, ZMQFeed> {
    /**
     * Returns an instance of <code>ZMQFeed</code>.
     * 
     * @param inProviderName
     *            a <code>String</code> value
     * @return a <code>ZMQFeed</code> value
     * @throws NoMoreIDsException
     *             if a unique identifier could not be generated to be assigned
     */
    public synchronized static ZMQFeed getInstance(String inProviderName)
            throws NoMoreIDsException {
        if (sInstance != null) {
            return sInstance;
        }
        sInstance = new ZMQFeed(inProviderName);
        return sInstance;
    }

    /**
     * Create a new ZMQFeed instance.
     * 
     * @param inProviderName
     *            a <code>String</code> value
     * @throws NoMoreIDsException
     *             if a unique identifier could not be generated to be assigned
     */
    private ZMQFeed(String inProviderName) throws NoMoreIDsException {
        super(FeedType.LIVE, inProviderName);
        setLoggedIn(false);
    }

    private void startEquities() {
        Vector<Integer> eqty_subs = new Vector<Integer>();
        eqty_subs.add(new Integer(MsgTypes.MOC_VALUE));
        eqty_subs.add(new Integer(MsgTypes.DIVIDEND_VALUE));
        eqty_subs.add(new Integer(MsgTypes.INFO_MSG_VALUE));
        // eqty_subs.add(new Integer(MsgTypes.HALT_VALUE));
        eqty_thread = new zmqPoller(zmqcontext, EQTY_IPC_SOCK,
                credentials.getEqtySubURL(), eqty_subs, isRunning,
                isconEqtyInProc, eqty_sub_queue); //$NON-NLS-1$
        SLF4JLoggerProxy.info(this, "ZMQFeed - Equities - starting up ");
        // Start up the main processing eqty_thread
        eqty_thread.start();
        int count = 0;
        try {
            while (!isconEqtyInProc.get() || count < 100) {
                Thread.sleep(20);
                SLF4JLoggerProxy.warn(this,
                        "Waiting for {} socket to initialise", EQTY_IPC_SOCK);
                count++;
            }
        } catch (InterruptedException ie) {
        }
        // The 0mq -. Marketcetera
        eqtyhandler = new ZMQtoMkcetHandler(eqty_sub_queue, EQTY_IPC_SOCK, ZMQ_MKCET_HANDLER_TYPE.EQUITY);// new
        // Thread(new
        eqtyhandler.start();
        // Data Requestor
        eqty_quoterequester = new CurrentTickRequestor(zmqcontext,
                EQTY_IPC_SOCK, credentials.getEqtyReqURL(), eqty_req_queue,
                eqty_sub_queue, isconEqtyInProc, false);
        eqty_quoterequester.start();
    }

    private void startLime() {
        Vector<Integer> lime_subs = new Vector<Integer>();
        lime_subs.add(new Integer(MsgTypes.MOC_VALUE));
        lime_subs.add(new Integer(MsgTypes.DIVIDEND_VALUE));
        lime_subs.add(new Integer(MsgTypes.INFO_MSG_VALUE));
        // lime_subs.add(new Integer(MsgTypes.HALT_VALUE));
        lime_thread = new zmqPoller(zmqcontext, LIME_IPC_SOCK,
                credentials.getLimeSubURL(), lime_subs, isRunning,
                isconLimeInProc, lime_sub_queue); //$NON-NLS-1$
        SLF4JLoggerProxy.info(this, "ZMQFeed - Equities - starting up ");
        // Start up the main processing lime_thread
        lime_thread.start();
        int count = 0;
        try {
            while (!isconLimeInProc.get() || count < 100) {
                Thread.sleep(20);
                SLF4JLoggerProxy.warn(this,
                        "Waiting for {} socket to initialise", LIME_IPC_SOCK);
                count++;
            }
        } catch (InterruptedException ie) {
        }
        // The 0mq -. Marketcetera
        limehandler = new ZMQtoMkcetHandler(lime_sub_queue, LIME_IPC_SOCK, ZMQ_MKCET_HANDLER_TYPE.LIME);// new
        // Thread(new
        limehandler.start();
        // Data Requestor
        lime_quoterequester = new CurrentTickRequestor(zmqcontext,
                LIME_IPC_SOCK, credentials.getLimeReqURL(), lime_req_queue,
                lime_sub_queue, isconLimeInProc, true);
        lime_quoterequester.start();
    }

    private void startFuturesOptions() {
        Vector<Integer> fuop_subs = new Vector<Integer>();
        fuop_subs.add(new Integer(MsgTypes.OPTION_RFQ_VALUE));
        fuop_subs.add(new Integer(MsgTypes.FUTURE_RFQ_VALUE));
        fuop_thread = new zmqPoller(zmqcontext, FUOP_IPC_SOCK,
                credentials.getFuOpSubURL(), fuop_subs, isRunning,
                isconFuOpInProc, fuop_sub_queue); //$NON-NLS-1$
        SLF4JLoggerProxy.info(this, "ZMQFeed - Futures - starting up ");
        // Start up the main processing eqty_thread
        fuop_thread.start();
        int count = 0;
        try {
            while (!isconFuOpInProc.get() || count < 100) {
                Thread.sleep(20);
                SLF4JLoggerProxy.warn(this,
                        "Waiting for {} socket to initialise", FUOP_IPC_SOCK);
                count++;
            }
        } catch (InterruptedException ie) {
        }
        // The 0mq -. Marketcetera
        fuophandler = new ZMQtoMkcetHandler(fuop_sub_queue, FUOP_IPC_SOCK, ZMQ_MKCET_HANDLER_TYPE.FUT_OPS);// new
        // Thread(new
        fuophandler.start();
        // Data Requestor
        fuop_quoterequester = new CurrentTickRequestor(zmqcontext,
                FUOP_IPC_SOCK, credentials.getFuOpReqURL(), fuop_req_queue,
                fuop_sub_queue, isconEqtyInProc, false);
        fuop_quoterequester.start();
    }

    private void startForex() {
        Vector<Integer> forex_subs = new Vector<Integer>();
        forex_thread = new zmqPoller(zmqcontext, FOREX_IPC_SOCK,
                credentials.getForexSubURL(), forex_subs, isRunning,
                isconForexInProc, forex_sub_queue); //$NON-NLS-1$
        SLF4JLoggerProxy.info(this, "ZMQFeed - FOREX - starting up ");
        // Start up the main processing eqty_thread
        forex_thread.start();
        int count = 0;
        try {
            while (!isconForexInProc.get() || count < 100) {
                Thread.sleep(20);
                SLF4JLoggerProxy.warn(this,
                        "Waiting for {} socket to initialise", FOREX_IPC_SOCK);
                count++;
            }
        } catch (InterruptedException ie) {
        }
        // The 0mq -. Marketcetera
        forexhandler = new ZMQtoMkcetHandler(forex_sub_queue, FOREX_IPC_SOCK, ZMQ_MKCET_HANDLER_TYPE.FOREX);// new
        // Thread(new
        forexhandler.start();
        // Data Requestor
        forex_quoterequester = new CurrentTickRequestor(zmqcontext,
                FOREX_IPC_SOCK, credentials.getForexReqURL(), forex_req_queue,
                forex_sub_queue, isconForexInProc, false);
        forex_quoterequester.start();
    }

    /*
     * (non-Javadoc)
     * 
     * @see org.marketcetera.marketdata.AbstractMarketDataFeed#start()
     */
    // @Override
    private synchronized void startZMQ() {
        if (isRunning.get()) {
            SLF4JLoggerProxy.info(ZMQFeed.class, "ZMQFeed - Already Running ");
            return;
        }

        isRunning.set(true);
        startEquities();
        startFuturesOptions();
        startForex();
        startLime();

    }

    /**
     * /* Called by Polling Thread to open control socket once the listening
     * socket is open
     */

    private void stopEquities() {
        // stop the converter 0mq-> marketcetera worker pool
        try {
            if (eqtyhandler != null) {
                eqty_sub_queue.put(new ZMsg());
                eqtyhandler.join(2000);
            }
        } catch (InterruptedException e) {
            SLF4JLoggerProxy.warn(this,
                    "Processing Thread - Can't shut down eqty_thread"); //$NON-NLS-1$
        }
        // Stop the Subscriber/requestor
        try {
            if (eqty_quoterequester != null) {
                eqty_quoterequester.join(2000);
            }
        } catch (InterruptedException e) {
            SLF4JLoggerProxy.warn(this,
                    "Quote subscriber Thread - Can't shut down eqty_thread"); //$NON-NLS-1$
        }

        // shutdown the main eqty_thread
        try {
            if (eqty_thread != null) {
                eqty_thread.join(2000);
            }
        } catch (InterruptedException e) {
            SLF4JLoggerProxy.warn(this,
                    "ZMQFeed - stop - Can't shut down eqty_thread"); //$NON-NLS-1$
        }
        eqty_req_queue.clear();
        eqty_sub_queue.clear();
        Equity_Quote_Builders.clear();
        try {
            while (((CurrentTickRequestor) eqty_quoterequester).isRunning()
                    || ((ZMQtoMkcetHandler) eqtyhandler).isRunning()) {
                Thread.sleep(20);
                SLF4JLoggerProxy.debug(this,
                        "stopZMQ waiting on requestor {} converter {}",
                        ((CurrentTickRequestor) eqty_quoterequester)
                                .isRunning(), ((ZMQtoMkcetHandler) eqtyhandler)
                                .isRunning());
            }
        } catch (InterruptedException ie) {
            SLF4JLoggerProxy.warn(this, "stopZMQ got interrupteed");
        }
        eqty_thread = null;
        eqty_quoterequester = null;
        eqtyhandler = null;
    }

    private void stopFuturesOptions() {
        // stop the converter 0mq-> marketcetera worker pool
        try {
            if (fuophandler != null) {
                fuop_sub_queue.put(new ZMsg());
                fuophandler.join(2000);
            }
        } catch (InterruptedException e) {
            SLF4JLoggerProxy.warn(this,
                    "Processing Thread - Can't shut down fuop_thread"); //$NON-NLS-1$
        }
        // Stop the Subscriber/requestor
        try {
            if (fuop_quoterequester != null) {
                fuop_quoterequester.join(2000);
            }
        } catch (InterruptedException e) {
            SLF4JLoggerProxy.warn(this,
                    "Quote subscriber Thread - Can't shut down fuop_thread"); //$NON-NLS-1$
        }

        // shutdown the main eqty_thread
        try {
            if (fuop_thread != null) {
                fuop_thread.join(2000);
            }
        } catch (InterruptedException e) {
            SLF4JLoggerProxy.warn(this,
                    "ZMQFeed - stop - Can't shut down fuop_thread"); //$NON-NLS-1$
        }

        fuop_req_queue.clear();
        fuop_sub_queue.clear();
        try {
            while (((CurrentTickRequestor) fuop_quoterequester).isRunning()
                    || ((ZMQtoMkcetHandler) fuophandler).isRunning()) {
                Thread.sleep(20);
                SLF4JLoggerProxy.debug(this,
                        "stopZMQ waiting on requestor {} converter {}",
                        ((CurrentTickRequestor) fuop_quoterequester)
                                .isRunning(), ((ZMQtoMkcetHandler) fuophandler)
                                .isRunning());
            }
        } catch (InterruptedException ie) {
            SLF4JLoggerProxy.warn(this, "stopZMQ got interrupteed");
        }
        Depth_Quote_Builders.clear();
        Future_Quote_Builders.clear();
        Option_Quote_Builders.clear();
        fuop_thread = null;
        fuop_quoterequester = null;
        fuophandler = null;
    }

    private void stopForex() {
        // stop the converter 0mq-> marketcetera worker pool
        try {
            if (forexhandler != null) {
                forex_sub_queue.put(new ZMsg());
                forexhandler.join(2000);
            }
        } catch (InterruptedException e) {
            SLF4JLoggerProxy.warn(this,
                    "Processing Thread - Can't shut down forex_thread"); //$NON-NLS-1$
        }
        // Stop the Subscriber/requestor
        try {
            if (forex_quoterequester != null) {
                forex_quoterequester.join(2000);
            }
        } catch (InterruptedException e) {
            SLF4JLoggerProxy.warn(this,
                    "Quote subscriber Thread - Can't shut down forex_thread"); //$NON-NLS-1$
        }

        // shutdown the main forex_thread
        try {
            if (forex_thread != null) {
                forex_thread.join(2000);
            }
        } catch (InterruptedException e) {
            SLF4JLoggerProxy.warn(this,
                    "ZMQFeed - stop - Can't shut down forex_thread"); //$NON-NLS-1$
        }
        forex_req_queue.clear();
        forex_sub_queue.clear();
        Forex_Quote_Builders.clear();
        try {
            while (((CurrentTickRequestor) forex_quoterequester).isRunning()
                    || ((ZMQtoMkcetHandler) forexhandler).isRunning()) {
                Thread.sleep(20);
                SLF4JLoggerProxy.debug(this,
                        "stopZMQ waiting on requestor {} converter {}",
                        ((CurrentTickRequestor) forex_quoterequester)
                                .isRunning(), ((ZMQtoMkcetHandler) forexhandler)
                                .isRunning());
            }
        } catch (InterruptedException ie) {
            SLF4JLoggerProxy.warn(this, "stopForex() got interrupteed");
        }
        forex_thread = null;
        forex_quoterequester = null;
        forexhandler = null;
    }

    private void stopLime() {
        // stop the converter 0mq-> marketcetera worker pool
        try {
            if (limehandler != null) {
                lime_sub_queue.put(new ZMsg());
                limehandler.join(2000);
            }
        } catch (InterruptedException e) {
            SLF4JLoggerProxy.warn(this,
                    "Processing Thread - Can't shut down lime_thread"); //$NON-NLS-1$
        }
        // Stop the Subscriber/requestor
        try {
            if (lime_quoterequester != null) {
                lime_quoterequester.join(2000);
            }
        } catch (InterruptedException e) {
            SLF4JLoggerProxy.warn(this,
                    "Quote subscriber Thread - Can't shut down lime_thread"); //$NON-NLS-1$
        }

        // shutdown the main eqty_thread
        try {
            if (lime_thread != null) {
                lime_thread.join(2000);
            }
        } catch (InterruptedException e) {
            SLF4JLoggerProxy.warn(this,
                    "ZMQFeed - stop - Can't shut down lime_thread"); //$NON-NLS-1$
        }
        lime_req_queue.clear();
        lime_sub_queue.clear();
        Lime_Quote_Builders.clear();
        try {
            while (((CurrentTickRequestor) lime_quoterequester).isRunning()
                    || ((ZMQtoMkcetHandler) limehandler).isRunning()) {
                Thread.sleep(20);
                SLF4JLoggerProxy.debug(this,
                        "stopZMQ waiting on requestor {} converter {}",
                        ((CurrentTickRequestor) lime_quoterequester)
                                .isRunning(), ((ZMQtoMkcetHandler) limehandler)
                                .isRunning());
            }
        } catch (InterruptedException ie) {
            SLF4JLoggerProxy.warn(this, "stopZMQ got interrupteed");
        }
        lime_thread = null;
        lime_quoterequester = null;
        limehandler = null;
    }
    
    private void stopZMQ() {
        SLF4JLoggerProxy
                .info(this, "ZMQFeed - stopZMQ - starting to shut down");
        // synchronized (this) {
        bShuttingDown.set(true);
        // tell other threads we are shutting down
        isRunning.set(false);
        stopEquities();
        stopFuturesOptions();
        stopForex();
        stopLime();
        handlebysubrequest.clear();
        requestbyhandle.clear();
        bShuttingDown.set(false);
        // }
    }

    /*
     * (non-Javadoc)
     * 
     * @see org.marketcetera.marketdata.MarketDataFeed#getCapabilities()
     */
    @Override
    public Set<Capability> getCapabilities() {
        return ZMQ_CAPABILITIES;
    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * org.marketcetera.marketdata.MarketDataFeed#getSupportedAssetClasses()
     */
    @Override
    public Set<AssetClass> getSupportedAssetClasses() {
        return assetClasses;
    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * org.marketcetera.marketdata.AbstractMarketDataFeed#doCancel(java.lang
     * .String)
     */
    @Override
    protected void doCancel(String inHandle) {
        SLF4JLoggerProxy.debug(this, "ZMQFeed got cxl string for {}", inHandle);
        String req = requestbyhandle.get(inHandle);
        if (req != null) {
            byte[] rebb = req.getBytes();
            rebb[2] = '-';
            String rm_key = req.substring(3, req.length() - 1);
            if (rm_key.endsWith("-F") || rm_key.endsWith("-C") || rm_key.endsWith("-P") ) {

                try {
                    fuop_req_queue.put(new String(rebb));
                } catch (InterruptedException ie) {
                    SLF4JLoggerProxy.warn(this, "doCancel got Interrupted");
                }

                Depth_Quote_Builders.remove(rm_key);
                Future_Quote_Builders.remove(rm_key);
                Option_Quote_Builders.remove(rm_key);
            } if ( req.contains("/") ) {
                try {
                    forex_req_queue.put(new String(rebb));
                } catch (InterruptedException ie) {
                    SLF4JLoggerProxy.warn(this, "doCancel got Interrupted");
                }

                Forex_Quote_Builders.remove(rm_key);
            } else {
                try {
                    String cxlstr = new String(rebb);
                    eqty_req_queue.put(cxlstr);
                    if (rebb[0] == '0' && rebb[1] == '3') {
                        String cxl = String.format("%02d-%s",
                                MsgTypes.STK_STATUS_VALUE, cxlstr.substring(3));
                        eqty_req_queue.put(cxl);
                    }

                } catch (InterruptedException ie) {
                    SLF4JLoggerProxy.warn(this, "doCancel got Interrupted");
                }
                Equity_Quote_Builders.remove(rm_key);
            }
            SLF4JLoggerProxy.debug(this,
                    "ZMQFeed removing for {} - bytes {}", inHandle, rebb); //$NON-NLS-1$){
            handlebysubrequest.get(req).remove(inHandle);
            requestbyhandle.remove(inHandle);
            if (handlebysubrequest.get(req).isEmpty()) {
                handlebysubrequest.remove(req);
            }

        }
    }

    /*
     * (non-Javadoc)
     * 
     * @see org.marketcetera.marketdata.AbstractMarketDataFeed#
     * doLevelOneMarketDataRequest(java.lang.Object)
     */
    @SuppressWarnings("incomplete-switch")
    @Override
    protected final List<String> doMarketDataRequest(MarketDataRequest inData)
            throws FeedException {
        Set<String> symbols = inData.getSymbols();
        List<String> handleList = new ArrayList<String>();
        if (symbols.isEmpty()) {
            symbols = inData.getUnderlyingSymbols();
        }
        String subrequest = null;
        byte[] send_buf = null;
        String req = null;
        String newid = null;
        String statusreq = null;
        boolean isEquity = true;
        boolean isFuture = false;
        boolean isForex = false;
        boolean isLime = false;
        
        // check that we actually have something
        if (!symbols.isEmpty()) {
            for (String symbol : symbols) {
                if (symbol.toLowerCase().matches("blank"))
                    continue;
                AssetClass assetClass = inData.getAssetClass();
                if (assetClass == AssetClass.EQUITY) {
                    SLF4JLoggerProxy.debug(ZMQFeed.class,
                            "Got Equity request for   {} ", symbol); //$NON-NLS-1$
                    if (symbol.indexOf(":") >= 0 && !(symbol.endsWith("-US") || symbol.contains("-US:")) ) {
                        String[] parts = symbol.split(":");
                        if (parts.length > 1) {
                            symbol = parts[0];
                        }
                    }
                } else if (assetClass == AssetClass.FUTURE) {
                    // transform Marketcetera to ours
                    if (!symbol.endsWith("-F")) {
                        int symlen = symbol.length();
                        // inData.getProvider()
                        String yy = symbol.substring(symlen - 4, symlen - 2);
                        String mm = symbol.substring(symlen - 2, symlen);
                        symbol = symbol.substring(0, symlen - 7) 
                                + (( symlen > 10)?"":( FUTURE_EXPIRY.get(mm) + yy + "-F"));
                    }
                    SLF4JLoggerProxy.debug(ZMQFeed.class,
                            "Got Future request for   {} ", symbol); //$NON-NLS-1$
                } else if (assetClass == AssetClass.OPTION) {
                    // symbol = inData.getUnderlyingSymbols();
                    // int symlen = symbol.length();

                }
                // if (inData.getAssetClass() == AssetClass.EQUITY) { // start
                for (Content content : inData.getContent()) {
                    boolean isMktStat = symbol.startsWith(MKT_STATE_);
                    SLF4JLoggerProxy.debug(ZMQFeed.class,
                            "Got request for   {} - bytes {}", symbol, content); //$NON-NLS-1$
                    if (symbol.endsWith("-P") || symbol.endsWith("-C")) {
                        isEquity = false;
                        isFuture = false;
                        isLime = false;
                    } else if (symbol.endsWith("-F")) {
                        isEquity = false;
                        isFuture = true;
                        isLime = false;
                    } else if ( CURRENCY.equals(assetClass) ) {
                	isEquity = false;
                	isFuture = false;
                	isForex = true;
                	isLime = false;
                    } else if (isMktStat
                            && content != org.marketcetera.marketdata.Content.LATEST_TICK) {
                        return null;
                    } else if ( symbol.endsWith("-US") || symbol.contains("-US:") ) {
                	isEquity = false;
                	isFuture = false;
                	isForex = false;
                	isLime = true;
                    }

                    switch (content) {
                    case OPEN_BOOK:
                	if ( isForex ) {
                	    return handleList;
                	} else if (!isEquity && !isLime) {
                            subrequest = new String(
                                    String.format("%02d",
                                            isFuture ? MsgTypes.FUT_DEPTH_VALUE
                                                    : MsgTypes.OPT_DEPTH_VALUE)
                                            + "+"
                                            + symbol
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
                            req = new String(
                                    String.format("%02d",
                                            isFuture ? MsgTypes.FUT_DEPTH_VALUE
                                                    : MsgTypes.OPT_DEPTH_VALUE)
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE
                                            + symbol
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
                            newid = String.valueOf(counter.incrementAndGet());
                        } else {
                            return handleList;
                        }
                        break;
                    case LATEST_TICK:
                	if (isEquity || isForex || isLime) {                           
                            subrequest = new String(
                                    String.format("%02d+",
                                            isMktStat ? MsgTypes.QF_MKT_STATE_VALUE
                                                    : MsgTypes.TRADE_VALUE)
                                            // + "+"
                                            + symbol
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
                            req = new String(
                                    String.format(
                                            "%02d",
                                            isMktStat ? MsgTypes.QF_MKT_STATE_VALUE:(symbol.startsWith("I-") ? MsgTypes.INDEX_VALUE
                                                    : MsgTypes.TRADE_VALUE))
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE
                                            + symbol
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
                            if (!isMktStat)
                            statusreq = new String(
                                    String.format("%02d+",
                                            MsgTypes.STK_STATUS_VALUE)
                                            + symbol
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
                        } else {
                            subrequest = new String(
                                    String.format(
                                            "%02d+",
                                            isFuture ? MsgTypes.FUTURE_TRADE_VALUE
                                                    : MsgTypes.OPTION_TRADE_VALUE)
                                            + symbol
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
                            req = new String(
                                    String.format(
                                            "%02d",
                                            isFuture ? MsgTypes.FUTURE_TRADE_VALUE
                                                    : MsgTypes.OPTION_TRADE_VALUE)
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE
                                            + symbol
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
                        }
                        newid = String.valueOf(counter.incrementAndGet());
                        break;
                    case TOP_OF_BOOK:
                        if (isEquity || isLime) {
                            subrequest = new String(
                                    String.format("%02d+", MsgTypes.QUOTE_VALUE)
                                            // + "+"
                                            + symbol
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
                            req = new String(
                                    String.format(
                                            "%02d",
                                            symbol.startsWith("I-") ? MsgTypes.INDEX_VALUE
                                                    : MsgTypes.QUOTE_VALUE)
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE
                                            + symbol
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
                        } else if ( isForex ) {
                            subrequest = String.format("%02d+%s%c", 
                                                       MsgTypes.FOREX_QUOTE_VALUE,
                                                       symbol,
                                                       Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
                            req = String.format("%02d%c%s%c",
                                                MsgTypes.FOREX_QUOTE_VALUE,
                                                Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE,
                                                symbol,
                                                Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
                        } else {
                            subrequest = new String(
                                    String.format(
                                            "%02d",
                                            isFuture ? MsgTypes.FUTURE_QUOTE_VALUE
                                                    : MsgTypes.OPTION_QUOTE_VALUE)
                                            + "+"
                                            + symbol
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
                            req = new String(
                                    String.format(
                                            "%02d",
                                            isFuture ? MsgTypes.FUTURE_QUOTE_VALUE
                                                    : MsgTypes.OPTION_QUOTE_VALUE)
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE
                                            + symbol
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
                        }
                        newid = String.valueOf(counter.incrementAndGet());
                        break;
                    case MARKET_STAT:
                	if ( isForex ) {
                	    return handleList;
                	} else if (isEquity || isLime) {
                            subrequest = new String(
                                    String.format("%02d+",
                                            MsgTypes.MKT_STAT_VALUE)
                                            // + "+"
                                            + symbol
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
                            req = new String(
                                    String.format("%02d",
                                            MsgTypes.MKT_STAT_VALUE)
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE
                                            + symbol
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
                            newid = String.valueOf(counter.incrementAndGet());
                        } else {
                            subrequest = new String(
                                    String.format(
                                            "%02d+",
                                            isFuture ? MsgTypes.FUTURE_STAT_VALUE
                                                    : MsgTypes.OPTION_STAT_VALUE)
                                            // + "+"
                                            + symbol
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
                            req = new String(
                                    String.format(
                                            "%02d",
                                            isFuture ? MsgTypes.FUTURE_STAT_VALUE
                                                    : MsgTypes.OPTION_STAT_VALUE)
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE
                                            + symbol
                                            + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
                            newid = String.valueOf(counter.incrementAndGet());
                        }
                        break;
                    case DIVIDEND:
                	if ( isForex ) {
                	    return handleList;
                	}
                	subrequest = new String(
                                String.format("%02d+", MsgTypes.DIVIDEND_VALUE)
                                        // + "+"
                                        + symbol
                                        + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
                        req = new String(
                                String.format("%02d", MsgTypes.DIVIDEND_VALUE)
                                        + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE
                                        + symbol
                                        + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE);
                        newid = String.valueOf(counter.incrementAndGet());
                        break;
                    }
                    if (subrequest != null) {
                        SLF4JLoggerProxy
                                .debug(ZMQFeed.class,
                                        "Got request for   {} - bytes {} - {}", symbol, content, newid); //$NON-NLS-1$
                        if (handlebysubrequest.containsKey(req)) {
                            handlebysubrequest.get(req).add(newid);
                        } else {
                            // List <String> tsrt = new List<String>(newid);
                            handlebysubrequest.put(
                                    req,
                                    new CopyOnWriteArrayList<String>(Arrays
                                            .asList(newid)));
                            // handlebysubrequest.put(req, Collections
                            // .synchronizedList(new ArrayList(Arrays
                            // .asList(newid))));

                        }
                        requestbyhandle.put(newid, req);
                        handleList.add(newid);

                        try {
                            if (isEquity) {
                                eqty_req_queue.put(subrequest);
                                if (statusreq != null)
                                    eqty_req_queue.put(statusreq);
                            } else if ( isLime ) {
                        	lime_req_queue.put(subrequest);
                                if (statusreq != null)
                                    lime_req_queue.put(statusreq);
                            } else if ( isForex ) {
                        	forex_req_queue.put(subrequest);
                        	if ( statusreq != null )
                        	    forex_req_queue.put(statusreq);
                            } else {
                                fuop_req_queue.put(subrequest);
                            }
                        } catch (InterruptedException ie) {
                            SLF4JLoggerProxy
                                    .warn(ZMQFeed.class,
                                            "Interrupted Exception ZMQFeed subscription string for {} - bytes {}", newid, send_buf); //$NON-NLS-1$
                        }
                        SLF4JLoggerProxy.debug(this, "Added  {} - bytes {}",
                                newid, req);
                    }
                }
                // } else if (inData.getAssetClass() == AssetClass.OPTION){

                // || inData.getAssetClass() == AssetClass.FUTURE) {

                // }
            }
        }
        return handleList;
        // return Arrays.asList(newnewid );
    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * org.marketcetera.marketdata.AbstractMarketDataFeed#doLogin(org.marketcetera
     * .marketdata.AbstractMarketDataFeedCredentials)
     */
    @Override
    protected final boolean doLogin(ZMQFeedCredentials inCredentials) {
        //SLF4JLoggerProxy.debug(ZMQFeed.class, "ZMQFeed doing Log on..."); //$NON-NLS-1$
        credentials = inCredentials;
        if (zmqcontext == null) {
            // setFeedStatus(FeedStatus.OFFLINE);
            setLoggedIn(false);
            return false;
        } else {
            today = new Date();
            mycal.setTime(today);
            mycal.add(Calendar.DAY_OF_MONTH,
                    (mycal.get(Calendar.DAY_OF_WEEK) == 1) ? -3 : -1);
            TODAY = dft.format(today);
            YESTERDAY = dft.format(mycal.getTime());
            startZMQ();
            try {
                while (!((CurrentTickRequestor) eqty_quoterequester)
                        .isRunning()
                        || !((ZMQtoMkcetHandler) eqtyhandler).isRunning()) {
                    Thread.sleep(20);
                    SLF4JLoggerProxy
                            .debug(this,
                                    "Waiting for start up of - eqty_quoterequester {} - eqtyhandler {} ",
                                    !((CurrentTickRequestor) eqty_quoterequester)
                                            .isRunning(),
                                    !((ZMQtoMkcetHandler) eqtyhandler)
                                            .isRunning());
                }
            } catch (InterruptedException id) {
                SLF4JLoggerProxy.debug(this, "doLoing Got Interrupted...");
            }
            setLoggedIn(true);
            // setFeedStatus(FeedStatus.AVAILABLE);
            return true;
        }
    }

    /*
     * (non-Javadoc)
     * 
     * @see org.marketcetera.marketdata.AbstractMarketDataFeed#doLogout()
     */
    @Override
    protected void doLogout() {
        // SLF4JLoggerProxy.info(ZMQFeed.class,
        // "ZMQFeed - doLogout - Calling stop");
        // if (isRunning.get()) {
        if (isRunning.get()) {
            stopZMQ();
        }
        setLoggedIn(false);
    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * org.marketcetera.marketdata.AbstractMarketDataFeed#generateToken(quickfix
     * .Message)
     */
    @Override
    protected final ZMQFeedToken generateToken(
            MarketDataFeedTokenSpec inTokenSpec) throws FeedException {
        return ZMQFeedToken.getToken(inTokenSpec, this);
    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * org.marketcetera.marketdata.AbstractMarketDataFeed#getEventTranslator()
     */
    @Override
    protected final ZMQFeedEventTranslator getEventTranslator() {
        return ZMQFeedEventTranslator.getInstance();
    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * org.marketcetera.marketdata.AbstractMarketDataFeed#getMessageTranslator()
     */
    @Override
    protected final ZMQFeedMessageTranslator getMessageTranslator() {
        return ZMQFeedMessageTranslator.getInstance();
    }

    /*
     * (non-Javadoc)
     * 
     * @see org.marketcetera.marketdata.AbstractMarketDataFeed#isLoggedIn(org.
     * marketcetera.marketdata.AbstractMarketDataFeedCredentials)
     */
    @Override
    protected final boolean isLoggedIn() {
        return mLoggedIn;
    }

    /**
     * Sets the loggedIn value.
     * 
     * @param inLoggedIn
     *            Logged-in status of the feed
     */
    private void setLoggedIn(boolean inLoggedIn) {
        mLoggedIn = inLoggedIn;
    }

    /**
     * ZMQ needed parts ZMQ Context
     */
    final ZMQ.Context zmqcontext = ZMQ.context(1);
    /**
     * ZMQ Feed Subscriber
     */
    // private ZMQ.Socket tsx_zmqsubscriber;

    /**
     * ZMQ Feed Subscriber
     */
    // private ZMQ.Socket bdm_zmqsubscriber;

    /**
     * ZMQ Historic Quote Requester
     */
    // private ZMQ.Socket tsx_zmreqsock;

    /**
     * ZMQ Historic Quote Requester
     */
    // private ZMQ.Socket bdm_zmreqsock;
    /**
     * ZMQ IPC Publisher
     */
    // private static ZMQ.Socket ipcxmtsock;
    /**
     * ZMQ_CAPABILITIES for ZMQFeed - note that these are not dynamic as Bogus
     * requires no provisioning
     */
    public static final Set<Capability> ZMQ_CAPABILITIES = Collections
            .unmodifiableSet(EnumSet.of(TOP_OF_BOOK, LATEST_TICK, MARKET_STAT,
                    DIVIDEND, OPEN_BOOK));
    public static final Content[] ZMQ_CONTENT = { Content.TOP_OF_BOOK,
            Content.LATEST_TICK, Content.MARKET_STAT, Content.DIVIDEND,
            Content.OPEN_BOOK };
    private static String[] divstr = { "",
            "Cash equivalent of a stock dividend", "Option traded stock",
            "Increase in rate", "Decrease in rate", "Stock dividend",
            "First dividend since listing on TSX",
            "First dividend since incorporation or issuance",
            "Following stock split", "Extra dividend", "US funds",
            "Estimated dividend", "Funds other than US",
            "Partial arrears payment", "Tax deferred",
            "First dividend since re-org of shares",
            "Rights or warrants also trading ex-div",
            "Or stock in lieu of cash", "Dividend payments resumed",
            "Dividend omitted", "Dividend deferred", "Arrears paid in full",
            "Dividend rescinded", "Formula" };

    /**
     * supported asset classes
     */
    private static final Set<AssetClass> assetClasses = EnumSet.of(EQUITY,
            FUTURE, OPTION, CURRENCY);
    /**
     * indicates if the feed has been logged in to
     */
    private boolean mLoggedIn;
    /**
     * exchanges that make up the group for which the bogus feed can report data
     */
    /**
     * indicates if the client is running
     */
    private final AtomicBoolean isRunning = new AtomicBoolean(false);
    /**
     * To signal that the inproc Socket is alive and ok
     */
    private final AtomicBoolean isconEqtyInProc = new AtomicBoolean(false);

    private final AtomicBoolean isconFuOpInProc = new AtomicBoolean(false);

    private final AtomicBoolean isconForexInProc = new AtomicBoolean(false);

    private final AtomicBoolean isconLimeInProc = new AtomicBoolean(false);

    private final Map<String, String> FUTURE_EXPIRY = new HashMap<String, String>() {
        /**
         * 
         */
        private static final long serialVersionUID = 2651080676635483375L;

        {
            put("01", "F");
            put("02", "G");
            put("03", "H");
            put("04", "J");
            put("05", "K");
            put("06", "M");
            put("07", "N");
            put("08", "Q");
            put("09", "U");
            put("10", "V");
            put("11", "X");
            put("12", "Z");
        }
    };

    /**
     * To signal that the inproc Socket is alive and ok
     */
    private final AtomicBoolean bShuttingDown = new AtomicBoolean(false);
    /**
     * eqty_thread used for recieving handlebysubrequest
     */
    private volatile zmqPoller eqty_thread;
    private volatile zmqPoller fuop_thread;
    private volatile zmqPoller forex_thread;
    private volatile zmqPoller lime_thread;
    /**
     * static instance of <code>ZMQFeed</code>
     */
    private volatile ZMQFeedCredentials credentials;

    // private volatile Quote quote = Quote.getDefaultInstance();
    /*
     * ZMQ context ...
     */

    /**
     * all handlebysubrequest by their ID (represented as string)
     */
    private static final ConcurrentHashMap<String, CopyOnWriteArrayList<String>> handlebysubrequest = new ConcurrentHashMap<String, CopyOnWriteArrayList<String>>();

    private static final ConcurrentHashMap<String, String> requestbyhandle = new ConcurrentHashMap<String, String>();

    private static final ConcurrentHashMap<String, BDMDepthQuotes> Depth_Quote_Builders = new ConcurrentHashMap<String, BDMDepthQuotes>();

    private static final ConcurrentHashMap<String, EquityMarketEvent> Equity_Quote_Builders = new ConcurrentHashMap<String, EquityMarketEvent>();

    private static final ConcurrentHashMap<String, FutureMarketEvent> Future_Quote_Builders = new ConcurrentHashMap<String, FutureMarketEvent>();

    private static final ConcurrentHashMap<String, OptionMarketEvent> Option_Quote_Builders = new ConcurrentHashMap<String, OptionMarketEvent>();

    private static final ConcurrentHashMap<String, ForexMarketEvent> Forex_Quote_Builders = new ConcurrentHashMap<String, ForexMarketEvent>();

    private static final ConcurrentHashMap<String, LimeMarketEvent> Lime_Quote_Builders = new ConcurrentHashMap<String, LimeMarketEvent>();
    /*
 * 
 */

    private static ZMQFeed sInstance;
    private static SimpleDateFormat dft = new SimpleDateFormat("yyMMdd");
    private static Date today = new Date();
    private static String TODAY = dft.format(today);
    private static Calendar mycal = Calendar.getInstance(); // .add(Calendar.DATE,
                                                            // -1);
    private static String YESTERDAY = "";// = dft.format(mycal.getTime());
    private Thread eqtyhandler;
    private Thread fuophandler;
    private Thread forexhandler;
    private ZMQtoMkcetHandler limehandler;
    private int reqTimeout = 2;
    private Thread eqty_quoterequester;
    private Thread fuop_quoterequester;
    private Thread forex_quoterequester;
    private Thread lime_quoterequester;
    private final static String EQTY_IPC_SOCK = new String("inproc://EQTY_wrkr");
    private final static String FUOP_IPC_SOCK = new String("inproc://FUOP_wrkr");
    private final static String FOREX_IPC_SOCK = new String("inproc://FOREX_wrkr");
    private final static String LIME_IPC_SOCK = new String("inproc://LIME_wrkr");
    public final static String MKT_STATE_ = new String("_MKT_STAT=");

    /*
     * counter used for unique ids
     */
    private static final AtomicLong counter = new AtomicLong(0);
    private static final AtomicLong msgCount = new AtomicLong(0);
    private static final AtomicLong TrdmsgCount = new AtomicLong(0);
    private static final AtomicLong TrdmsgTot = new AtomicLong(0);
    private static final AtomicLong QtmsgCount = new AtomicLong(0);
    private static final AtomicLong QtmsgTot = new AtomicLong(0);

    /*
     * Queue to dump object to refresh marketcetera
     */
    private static final BlockingQueue<ZMsg> eqty_sub_queue = new LinkedBlockingQueue<ZMsg>();
    private static final BlockingQueue<ZMsg> fuop_sub_queue = new LinkedBlockingQueue<ZMsg>();
    private static final BlockingQueue<ZMsg> forex_sub_queue = new LinkedBlockingQueue<ZMsg>();
    private static final BlockingQueue<ZMsg> lime_sub_queue = new LinkedBlockingQueue<ZMsg>();

    private static final BlockingQueue<String> eqty_req_queue = new LinkedBlockingQueue<String>();
    private static final BlockingQueue<String> fuop_req_queue = new LinkedBlockingQueue<String>();
    private static final BlockingQueue<String> forex_req_queue = new LinkedBlockingQueue<String>();
    private static final BlockingQueue<String> lime_req_queue = new LinkedBlockingQueue<String>();

    /**
     * Corresponds to a single market data request submitted to {@link ZMQFeed}.
     * 
     * @author <a href="mailto:colin@marketcetera.com">Colin DuPlantis</a>
     * @version $Id: ZMQFeed.java 16364 2012-11-08 06:13:58Z sameer.patil $
     * @since 1.5.0
     */

    private class zmqPoller extends Thread {
        private ZMQ.Socket rcvsocket;
        private ZMQ.Socket ipcsocket;
        private ZMQ.Poller sockets;
        private AtomicBoolean isrunning;
        private AtomicBoolean isrunning_ipc;
        private byte header[];
        private BlockingQueue<ZMsg> queue;
        @SuppressWarnings("unused")
        private boolean bisInit = false;

        public zmqPoller(ZMQ.Context inzmqcontext, String conStrIPC,
                String conStrTCP, Vector<Integer> subequests,
                AtomicBoolean inisrunning, AtomicBoolean inbipc,
                BlockingQueue<ZMsg> qu) {
            super("zmqPoller-" + conStrTCP);
            queue = qu;
            isrunning_ipc = inbipc;
            rcvsocket = inzmqcontext.socket(ZMQ.SUB);
            rcvsocket.connect(conStrTCP);
            ipcsocket = inzmqcontext.socket(ZMQ.PAIR);
            ipcsocket.setLinger(0);
            ipcsocket.bind(conStrIPC);
            sockets = inzmqcontext.poller(2);
            sockets.register(rcvsocket, ZMQ.Poller.POLLIN);
            // use this to recieve messages from other quote requester
            sockets.register(ipcsocket, ZMQ.Poller.POLLIN);
            // setup default subscriptions
            // @SuppressWarnings("unchecked")
            Iterator<Integer> itr = (Iterator<Integer>) subequests.iterator();
            while (itr.hasNext()) {
                byte msg[] = new String(String.format("%02d", itr.next()
                        .intValue())
                        + (char) Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE)
                        .getBytes();
                rcvsocket.subscribe(msg);
                //System.err.println("Subscribe (i): " + new String(msg));
            }
            isrunning = inisrunning;
            isrunning_ipc.set(true);
            SLF4JLoggerProxy.info(this, "ZMQFeed Connected to {}", conStrTCP); //$NON-NLS-1$
            bisInit = true;
        }

        @Override
        public void run() {
            while (isrunning.get()) {
                sockets.poll(2L);// 250L);//2000L);
                if (sockets.pollin(0)) {
                    ZMsg msg = ZMsg.recvMsg(rcvsocket);
                    if (msg.size() == 2) {
                        // SLF4JLoggerProxy.debug(ZMQFeed.class,
                        // "ZMQ Poll eqty_thread - Got Message and added to Q {} ",
                        // msg.toString());
                        try {
                            queue.put(msg);
                            msgCount.incrementAndGet();
                            // msg.destroy();
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }
                }

                if (sockets.pollin(1)) {
                    header = ipcsocket.recv(0);
                    // String req = new String(header);
                    // int delim = header.length - 2;
                    if (header[2] == '+') {
                        header[2] = Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE;
                        rcvsocket.subscribe(header);
//                         SLF4JLoggerProxy.debug(ZMQFeed.class,
//                                                "ZMQ Poll eqty_thread - got subscription request - bytes {} - check {} ",
//                                                header, header);
                         //System.err.println("Subscribe (r): " + new String(header));
                    } else {
                        header[2] = Msgs.CONSTANTS.KEY_TYPE_SEPARATOR_VALUE;
                        rcvsocket.unsubscribe(header);
                        // SLF4JLoggerProxy
                        // .debug(ZMQFeed.class,
                        // "ZMQ Poll eqty_thread - got unsubscribe request {} - bytes {} - check {} ",
                        // req, req.getBytes(), header);
                    }
                }
            }
            sockets.unregister(rcvsocket);
            sockets.unregister(ipcsocket);
            // // Now close us
            rcvsocket.close();
            ipcsocket.close();
            isconEqtyInProc.set(false);
            // SLF4JLoggerProxy.info(this,
            // "ZMQ Poll eqty_thread - Successfully Shut Down feed");
            // /*
            // * try { Thread.sleep(2000); } catch (InterruptedException e) {
            // * e.printStackTrace(); }
            // */
            // // setFeedStatus(FeedStatus.OFFLINE);
        }

        // public boolean isRunning() {
        // return bisInit;
        // }
    }

    // private class EqtyQuoteUpdateWorker extends Module implements
    // Runnable,DataEmitter {
    private class EqtyQuoteUpdateWorker implements Runnable {
        private ZMsg message;

        public EqtyQuoteUpdateWorker(ZMsg msg) {
            // super(ZMQFeedModuleFactory.PROVIDER_URN, true);
            message = msg;
        }

        @Override
        public void run() {
            // SLF4JLoggerProxy
            // .info(this, "Message Processor eqty_thread - Message {}",
            // message.toString());
            if (message.contentSize() > 0) {
                byte header[] = message.getFirst().getData();
                try {
                    // message.pop();
                    header = message.getFirst().getData();
                    while ( header.length ==1 || ((header.length >1)&& header[2] == -117)) {
                        message.pop();
                        header = message.getFirst().getData();
                    }
                    int lenStr = header.length;
                    String symbol;

                    int mtype = MsgTypes.UNDEFINED_VALUE;
                    try {
                        mtype = Integer.parseInt(new String(Arrays.copyOfRange(
                                header, 0, 2)));
                    } catch (NumberFormatException nfe) {
                        nfe.printStackTrace();
                    }

                    if (mtype == MsgTypes.MOC_VALUE
                    // || mtype == MsgTypes.HALT_VALUE
                            || mtype == MsgTypes.DIVIDEND_VALUE) {
                        symbol = new String(Arrays.copyOfRange(header, 3,
                                lenStr));
                    } else {
                        symbol = new String(Arrays.copyOfRange(header, 3,
                                lenStr - 1));
                    }
                    if (mtype == MsgTypes.QUOTE_VALUE) {
                        QtmsgCount.incrementAndGet();
                        long tnow = System.nanoTime();
                        Quote quote = Quote.PARSER.parseFrom(message.getLast()
                                .getData());
                        doupdateQuote(quote, symbol, header);
                        QtmsgTot.addAndGet(System.nanoTime() - tnow);
                    } else if (mtype == MsgTypes.TRADE_VALUE) {
                        TrdmsgCount.incrementAndGet();
                        long tnow = System.nanoTime();
                        Trade trade = Trade.PARSER.parseFrom(message.getLast()
                                .getData());
                        doupdateTrade(trade, symbol, header);
                        TrdmsgTot.addAndGet(System.nanoTime() - tnow);
                    } else if (mtype == MsgTypes.INDEX_VALUE) {
                        Index index = Index.PARSER.parseFrom(message.getLast()
                                .getData());
                        doupdateIndex(index, symbol, header); // message.getLast().getData());
                    } else if (mtype == MsgTypes.MOC_VALUE) {
                        Moc moc_msg = Moc.PARSER.parseFrom(message.getLast()
                                .getData());
                        doupdateMoc(moc_msg, symbol, header);
                    } else if (mtype == MsgTypes.STK_STATUS_VALUE) {
                        StkStatus state = StkStatus.PARSER.parseFrom(message
                                .getLast().getData());
                        doupdateStkStatus(state, symbol, header, -1);
                    } else if (mtype == MsgTypes.MKT_STAT_VALUE) {
                        MktStat stat = MktStat.PARSER.parseFrom(message
                                .getLast().getData());
                        doupdateStats(stat, symbol, header);
                    } else if (mtype == MsgTypes.DIVIDEND_VALUE) {
                        Dividend div = Dividend.PARSER.parseFrom(message
                                .getLast().getData());
                        doupdateDividend(div, symbol, header);
                    } else if (mtype == MsgTypes.QF_MKT_STATE_VALUE) {
                        MarketState mste = MarketState.PARSER.parseFrom(message
                                .getLast().getData());
                        doupdateMktStatus(mste, symbol, header);
                    }

                    SLF4JLoggerProxy
                            .debug(ZMQFeed.class, "Recieved {}", symbol);
                } catch (InvalidProtocolBufferException e) {
                    SLF4JLoggerProxy
                            .warn(ZMQFeed.class,
                                    "Encountered an Protocol buffer exception {} - {} ",
                                    e, message.getLast());
                }
            }
            message.destroy();
        }

        private void doupdateDividend(Dividend div, String symbol, byte[] header) {
            int mkrs = div.getMarkers();
            int d2x = mkrs & DivCodes.D_TO_EX_VALUE;
            int mk1 = (mkrs & DivCodes.MRKR1_VALUE) >> DivCodes.L_SHIFT_VALUE;
            mk1 = mk1 > divstr.length ? divstr.length : mk1;
            int mk2 = (mkrs & DivCodes.MRKR2_VALUE) >> 2 * DivCodes.L_SHIFT_VALUE;
            mk2 = mk2 > divstr.length ? divstr.length : mk2;
            int mk3 = (mkrs & DivCodes.MRKR3_VALUE) >> 3 * DivCodes.L_SHIFT_VALUE;
            mk3 = mk3 > divstr.length ? divstr.length : mk3;
            SLF4JLoggerProxy.info(
                    org.marketcetera.core.Messages.USER_MSG_CATEGORY,
                    "{}{} dividend {} ex: {} pay: {} record: {} {}",
                    (d2x == 0 ? "New - " : ""),
                    symbol,
                    div.getDivstr() == "" ? new DecimalFormat("0.0000")
                            .format(div.getDiv()) : div.getDivstr(),
                    "20" + div.getExdate(), "20" + div.getPaydate(),
                    "20" + div.getRecdate(), divstr[DivCodes.valueOf(mk1)
                            .getNumber()]
                            + " "
                            + divstr[DivCodes.valueOf(mk2).getNumber()]
                            + " "
                            + divstr[DivCodes.valueOf(mk3).getNumber()]);
            Equity equity = new Equity(symbol);
            // String exchange = "TO";
            DividendEvent dividend = DividendEventBuilder.dividend()
                    .withEquity(equity).withCurrency("CAD")
                    .withAmount(new BigDecimal(div.getDiv()))
                    .withDeclareDate(div.getDecdate())
                    .withExecutionDate(div.getExdate())
                    .withType(DividendType.CURRENT)
                    .withFrequency(DividendFrequency.ANNUALLY)
                    .withStatus(DividendStatus.OFFICIAL).create();

            List<String> handles = handlebysubrequest.get(new String(header));
            if (handles != null) {
                SLF4JLoggerProxy.debug(this,
                        "MarketceteraFeed received response for handle(s): {}", //$NON-NLS-1$
                        handles);
                for (String handle : handles) {
                    dataReceived(handle, dividend);
                }
            }
        }

        private void doupdateIndex(Index index, String symbol, byte[] header) {
            String hdr = new String(header);
            EquityMarketEvent eb = Equity_Quote_Builders.get(symbol);
            if (eb == null) {
                eb = new EquityMarketEvent(symbol, "TO");
                Equity_Quote_Builders.put(symbol, eb);
            }

            // float flast = index.getLast();
       //     if (index.getIsinterim()) {
                TopOfBookEvent ToBE = eb.getTopOfBookEvent(
                        (double) index.getBid(), (double) index.getAsk(), 0, 0);
                TradeEvent tradev = eb.getIndexTradeEvent(index);
                // Convert to Marketcetera quote
                List<String> handles = handlebysubrequest.get(hdr);
                if (handles != null) {
                    SLF4JLoggerProxy
                            .debug(this,
                                    "MarketceteraFeed received response for handle(s): {}", //$NON-NLS-1$
                                    handles);
                    for (String handle : handles) {
                        dataReceived(handle, ToBE);
                        dataReceived(handle, tradev);
                    }
                }
         //   } else {
                //TradeEvent tradev = eb.getIndexTradeEvent(index);
                //List<String> handles = handlebysubrequest.get(hdr);
                //if (handles != null) {
                //    SLF4JLoggerProxy
                 //           .debug(this,
                //                    "MarketceteraFeed received response for handle(s): {}", //$NON-NLS-1$
                //                    handles);
                //    for (String handle : handles) {
                //        dataReceived(handle, tradev);
                //    }
               // }
            //}
        }

        private void doupdateMoc(Moc moc_msg, String symbol, byte[] header) {
            float side = moc_msg.getSide();
            SLF4JLoggerProxy.info(
                    org.marketcetera.core.Messages.USER_MSG_CATEGORY,
                    "{} {} {}", symbol, side < 0 ? "Sell" : "Buy",
                    new DecimalFormat("###,###,###")
                            .format(side < 0 ? (side * -1) : side));
            MocEvent moc = new MocEvent(symbol, side, moc_msg.getVwap());
            byte[] new_req;
            if ( header.length > 2 && header[0] == '1' && header[1] == '4' ) {
                new_req = new byte[header.length];
            } else {
                new_req = new byte[header.length + 1];
            }
            System.arraycopy(header, 0, new_req, 0, header.length);
            new_req[new_req.length - 1] = 30;
            new_req[0]='0';
            new_req[1]='1';
            
            List<String> handles = handlebysubrequest.get(new String(new_req));
            if (handles != null) {
                SLF4JLoggerProxy
                        .debug(this,
                                "Recieved a moc Event received response for handle(s): {}", //$NON-NLS-1$
                                handles);
                for (String handle : handles) {
                    dataReceived(handle, moc);
                }
            }
        }

        private void doupdateStkStatus(StkStatus state, String symbol,
                byte[] header, int stkgrp) {
            INotification notification;
            StkStateEvent sse = new StkStateEvent(symbol, state.getState(),
                    state.getExpectedOpen(), stkgrp);
            // EnumValueDescriptor ev2 = state.getState().getValueDescriptor();
            // only send state once
            // EnumValueDescriptor d = StockStatus.A.getDescriptorForType();
            // header[1] = '1';
            byte[] lthdr = Arrays.copyOf(header, header.length);
            lthdr[0] = 48;
            lthdr[1] = 49;
            List<String> handles = handlebysubrequest.get(new String(lthdr));
            if (handles != null) {
                SLF4JLoggerProxy
                        .debug(this,
                                "Recieved a stockstate Event received response for handle(s): {}", //$NON-NLS-1$
                                handles);
                for (String handle : handles) {
                    dataReceived(handle, sse);
                }
            }
            // notify the desktop etc
            String exch = symbol.endsWith("-V") ? "Venture" : "TSX";
            if (state.getState() != StockStatus.A) { // Authorised
                notification = Notification
                        .high(symbol,
                                "Market State "
                                        + state.getState().name()
                                        + " "
                                        + state.getReason()
                                        + ((state.getExpectedOpen().length() > 0) ? (" and is expected to open " + state
                                                .getExpectedOpen()) : ""), exch);
            } else {
                notification = Notification.high(symbol, "Authorised", exch);
            }
            if (stkgrp == -1)
                NotificationManager.getNotificationManager().publish(
                        notification);
        }

        private void doupdateMktStatus(MarketState state, String symbol,
                byte[] header) {
            mktStateEvent mse = new mktStateEvent(symbol, state.getState());
            // EnumValueDescriptor ev2 = state.getState().getValueDescriptor();
            // only send state once
            // EnumValueDescriptor d = StockStatus.A.getDescriptorForType();
            List<String> handles = handlebysubrequest.get(new String(header));
            if (handles != null) {
                SLF4JLoggerProxy
                        .debug(this,
                                "Recieved a stockstate Event received response for handle(s): {}", //$NON-NLS-1$
                                handles);
                for (String handle : handles) {
                    dataReceived(handle, mse);
                }
            }
        }

        /*
         * private void doupdateStkhalt(HaltResume haltresume_msg, String
         * symbol) { INotification notification;
         * 
         * if (haltresume_msg.getReasonForHalt() != "") { notification =
         * Notification.high(symbol + " Halted ", "Reason - " +
         * haltresume_msg.getReasonForHalt(), "TMX"); SLF4JLoggerProxy.debug(
         * org.marketcetera.core.Messages.USER_MSG_CATEGORY, "{} Halted {} ",
         * symbol, haltresume_msg.getReasonForHalt()); } else {
         * SLF4JLoggerProxy.debug(
         * org.marketcetera.core.Messages.USER_MSG_CATEGORY,
         * "{} Expected to resume {} ", symbol,
         * haltresume_msg.getExpectedOpen()); notification = Notification.high(
         * symbol, new String("Expected to Resume - " +
         * haltresume_msg.getExpectedOpen()), "TMX"); }
         * 
         * NotificationManager.getNotificationManager().publish(notification);
         * 
         * }
         */
        private void doupdateTrade(Trade trade, String symbol, byte[] header) {
            String hdr = new String(header);
            EquityMarketEvent eb = Equity_Quote_Builders.get(symbol);
            if (eb == null) {
                eb = new EquityMarketEvent(symbol, "TO");
                Equity_Quote_Builders.put(symbol, eb);
            }
            TradeEvent tradev = eb.getTradeEvent(trade);
            List<String> handles = handlebysubrequest.get(hdr);
            SLF4JLoggerProxy.debug(this, "Trade {} {}", //$NON-NLS-1$
                    symbol, tradev);
            if (tradev != null && handles != null) {
                SLF4JLoggerProxy.debug(this,
                        "MarketceteraFeed received response for handle(s): {}", //$NON-NLS-1$
                        handles);
                for (String handle : handles) {
                    dataReceived(handle, tradev);
                }
            }
            if (trade.getLast() > 0) {
                // change header to be Stat
                header[0] = '1';
                header[1] = '4';
                doupdateStats(null, symbol, header);
            }
        }

        private void doupdateQuote(Quote quote, String symbol, byte[] header) {
            String hdr = new String(header);
            EquityMarketEvent eb = Equity_Quote_Builders.get(symbol);
            if (eb == null) {
                eb = new EquityMarketEvent(symbol, "TO");
                Equity_Quote_Builders.put(symbol, eb);
            }

            TopOfBookEvent ToBE = eb.getTopOfBookEvent(quote.getBid(),
                    quote.getAsk(), quote.getBidSize(), quote.getAskSize());
            List<String> handles = handlebysubrequest.get(hdr);
            if (handles != null) {
                SLF4JLoggerProxy.debug(this,
                        "MarketceteraFeed received response for handle(s): {}", //$NON-NLS-1$
                        handles);
                for (String handle : handles) {
                    dataReceived(handle, ToBE);
                }
            }
        }

        private void doupdateStats(MktStat stat, String symbol, byte[] header) {
            String hdr = new String(header);
            List<String> handles = handlebysubrequest.get(hdr);
            if (handles == null)
                return;
            EquityMarketEvent eb = Equity_Quote_Builders.get(symbol);
            if (eb == null) {
                eb = new EquityMarketEvent(symbol, "TO");
                Equity_Quote_Builders.put(symbol, eb);
            }

            MarketstatEvent mste = eb.getMktStat(stat, TODAY, YESTERDAY);
            if (stat != null) {
                Moc moc = null;
                StkStatus sstate = null;
                if (stat.getState() != null) {
                    sstate = StkStatus.newBuilder().setState(stat.getState())
                            .setMsgType(Msgs.MsgTypes.STK_STATUS)
                            .setTimeHalted("080000").setExpectedOpen("080000")
                            .setReason("").build();
                    doupdateStkStatus(sstate, symbol, header, stat.getStkgrp());
                }

                if (stat.getMoc() != 0) {
                    moc = Moc.newBuilder().setSide(stat.getMoc())
                            .setMsgType(Msgs.MsgTypes.MOC).build();
                    
                    doupdateMoc(moc, symbol, header);
                }
            }
            if (handles != null) {
                SLF4JLoggerProxy
                        .debug(this,
                                "Recieved a marketstat Event received response for handle(s): {}", //$NON-NLS-1$
                                handles);
                for (String handle : handles) {
                    dataReceived(handle, mste);
                }
            }

        }
    }

    private class FuOpQuoteUpdateWorker implements Runnable {
        private ZMsg message;

        public FuOpQuoteUpdateWorker(ZMsg msg) {
            // super(ZMQFeedModuleFactory.PROVIDER_URN, true);
            message = msg;
        }

        @Override
        public void run() {
            // SLF4JLoggerProxy
            // .info(this, "Message Processor eqty_thread - Message {}",
            // message.toString());
            if (message.contentSize() > 0) {
                byte header[] = message.getFirst().getData();
                try {
                    // message.
                    header = message.getFirst().getData();
                    int lenStr = header.length;
                    String symbol;
                    int mtype = MsgTypes.UNDEFINED_VALUE;
                    try {
                        mtype = Integer.parseInt(new String(Arrays.copyOfRange(
                                header, 0, 2)));
                    } catch (NumberFormatException nfe) {
                    }

                    symbol = new String(Arrays.copyOfRange(header, 3,
                            lenStr - 1));
                    if (mtype == MsgTypes.FUTURE_QUOTE_VALUE) {
                        QtmsgCount.incrementAndGet();
                        long tnow = System.nanoTime();
                        // FutQuote quote = FutQuote.parseFrom(message.getLast()
                        // .getData());
                        FutQuote quote = FutQuote.PARSER.parseFrom(message
                                .getLast().getData());
                        doupdateFutQuote(quote, symbol, header);
                        QtmsgTot.addAndGet(System.nanoTime() - tnow);
                    } else if (mtype == MsgTypes.FUTURE_TRADE_VALUE) {
                        TrdmsgCount.incrementAndGet();
                        long tnow = System.nanoTime();
                        FutTrade trade = FutTrade.PARSER.parseFrom(message
                                .getLast().getData());
                        doupdateFutTrade(trade, symbol, header);
                        TrdmsgTot.addAndGet(System.nanoTime() - tnow);
                    } else if (mtype == MsgTypes.OPTION_TRADE_VALUE) {
                        TrdmsgCount.incrementAndGet();
                        long tnow = System.nanoTime();
                        OptTrade trade = OptTrade.PARSER.parseFrom(message
                                .getLast().getData());
                        doupdateOptTrade(trade, symbol, header);
                        TrdmsgTot.addAndGet(System.nanoTime() - tnow);
                    } else if (mtype == MsgTypes.OPTION_QUOTE_VALUE) {
                        QtmsgCount.incrementAndGet();
                        long tnow = System.nanoTime();
                        OptQuote quote = OptQuote.PARSER.parseFrom(message
                                .getLast().getData());
                        doupdateOptQuote(quote, symbol, header);
                        QtmsgCount.addAndGet(System.nanoTime() - tnow);
                        SLF4JLoggerProxy.debug(ZMQFeed.class, "Recieved {}",
                                symbol);
                    } else if (mtype == MsgTypes.FUTURE_RFQ_VALUE) {
                        FutRFQ frfq = FutRFQ.PARSER.parseFrom(message.getLast()
                                .getData());
                        String rfq = symbol + " - "
                                + Integer.toString(frfq.getSize());
                        doupdateRFQ(rfq);
                        // doupRFQ(
                        // FutRFQ.parseFrom(message.getLast().getData()).get );
                    } else if (mtype == MsgTypes.OPTION_RFQ_VALUE) {
                        OptRFQ rfq = OptRFQ.PARSER.parseFrom(message.getLast()
                                .getData());
                        String srfq = symbol + " - "
                                + Integer.toString(rfq.getSize());
                        doupdateRFQ(srfq);
                    } else if (mtype == MsgTypes.OPTION_STAT_VALUE) {
                        OptStat stat = OptStat.PARSER.parseFrom(message
                                .getLast().getData());
                        doupdateOptStats(stat, symbol, header);
                    } else if (mtype == MsgTypes.FUTURE_STAT_VALUE) {
                        FutStat stat = FutStat.PARSER.parseFrom(message
                                .getLast().getData());
                        doupdateFutStats(stat, symbol, header);
                    } else if (mtype == MsgTypes.FUT_DEPTH_VALUE
                            || mtype == MsgTypes.OPT_DEPTH_VALUE) {
                        QtmsgCount.incrementAndGet();
                        long tnow = System.nanoTime();
                        BDMDepth depth = BDMDepth.PARSER.parseFrom(message
                                .getLast().getData());
                        doupdateDepth(depth, symbol, header);
                        QtmsgCount.addAndGet(System.nanoTime() - tnow);
                        SLF4JLoggerProxy.debug(ZMQFeed.class,
                                "Recieved depth {}", symbol);
                    }

                } catch (InvalidProtocolBufferException e) {
                    SLF4JLoggerProxy
                            .warn(ZMQFeed.class,
                                    "Encountered an Protocol buffer exception {} - {} ",
                                    e, message.getLast());
                }
            }
        }

        private void doupdateRFQ(String RFQ) {
            INotification notification;
            notification = Notification.high("Request For Quote", RFQ, "BDM");
            SLF4JLoggerProxy.debug(
                    org.marketcetera.core.Messages.USER_MSG_CATEGORY, "{} ",
                    RFQ);

            NotificationManager.getNotificationManager().publish(notification);

        }

        private void doupdateDepth(BDMDepth depth, String symbol, byte[] header) {
            Instrument instr = null;
            BDMDepthQuotes book = Depth_Quote_Builders.get(symbol);
            String req = new String(header);
            if (book == null) {
                if (symbol.endsWith("-F")) {
                    String expiry = depth.getExpiry();
                    // instr = new Future(symbol,
                    // expiry.equals("")?"201401":expiry);
                    instr = new Future(symbol, expiry);
                } else {
                    instr = new Option(symbol, depth.getExpiry(),
                            new BigDecimal(depth.getStrike()),
                            symbol.endsWith("-P") ? OptionType.Put
                                    : OptionType.Put);
                }
                book = new BDMDepthQuotes(instr, depth.getIsAmer());
                Depth_Quote_Builders.put(symbol, book);
            }

            List<String> handles = handlebysubrequest.get(req);
            if (handles != null) {
                SLF4JLoggerProxy.debug(this,
                        "MarketceteraFeed received response for handle(s): {}",//$NON-NLS-1$
                        handles);
                List<Depthquote> dqlist = depth.getQuoteList();
                if (dqlist != null) {
                    for (Depthquote dq_i : dqlist) {
                        book.update(dq_i);
                        BidEvent be = book.getBidEvent(dq_i.getLevel());
                        AskEvent ae = book.getAskEvent(dq_i.getLevel());
                        for (String handle : handles) {
                            if (be != null)
                                dataReceived(handle, be);
                            if (ae != null)
                                dataReceived(handle, ae);
                        }
                    }
                }
            }
        }

        private void doupdateFutStats(FutStat stat, String symbol, byte[] header) {
            // Instrument instr = null;
            List<String> handles = handlebysubrequest.get(new String(header));
            if (handles == null)
                return;
            FutureMarketEvent fme = Future_Quote_Builders.get(symbol);
            if (fme == null) {
                fme = new FutureMarketEvent(
                        new Future(symbol, stat.getExpiry()), "BD",
                        (int) stat.getMultiplier());
                Future_Quote_Builders.put(symbol, fme);
            }
            MarketstatEvent mste = fme.getMktStat(stat, TODAY, YESTERDAY);

            if (handles != null) {
                SLF4JLoggerProxy
                        .debug(this,
                                "Recieved a marketstat Event received response for handle(s): {}", //$NON-NLS-1$
                                handles);
                for (String handle : handles) {
                    dataReceived(handle, mste);
                }
            }
        }

        private void doupdateOptStats(OptStat stat, String symbol, byte[] header) {
            List<String> handles = handlebysubrequest.get(new String(header));
            if (handles == null)
                return;
            if (stat != null)
                SLF4JLoggerProxy
                        .debug(this,
                                "Marketstat Event symbol {} close {} low {} high {} close {}", //$NON-NLS-1$
                                symbol, stat.getPdclose(), stat.getLow(),
                                stat.getHigh(), stat.getClose());

            Instrument instr = null;
            OptionMarketEvent fme = Option_Quote_Builders.get(symbol);
            // String req = new String(header);
            if (fme == null) {
                instr = new org.marketcetera.trade.Option(symbol,
                        stat.getExpiry(), new BigDecimal(stat.getStrike()),
                        symbol.endsWith("-P") ? OptionType.Put : OptionType.Put);
                fme = new OptionMarketEvent(instr, "BD", stat.getMultiplier(),
                        stat.getIsAmer());
                Option_Quote_Builders.put(symbol, fme);
            }

            MarketstatEvent mste = fme.getMktStat(stat, TODAY, YESTERDAY);
            if (handles != null) {
                SLF4JLoggerProxy
                        .debug(this,
                                "Recieved a marketstat Event received response for handle(s): {}", //$NON-NLS-1$
                                handles);
                for (String handle : handles) {
                    dataReceived(handle, mste);
                }
            }
        }

        private void doupdateOptQuote(OptQuote quote, String symbol,
                byte[] header) {
            SLF4JLoggerProxy.debug(this,
                    "MarketceteraFeed received Option quote for {}", symbol); //$NON-NLS-1$     
            Instrument instr = null;
            OptionMarketEvent fme = Option_Quote_Builders.get(symbol);
            // String req = new String(header);
            if (fme == null) {
                instr = new org.marketcetera.trade.Option(symbol,
                        quote.getExpiry(), new BigDecimal(quote.getStrike()),
                        symbol.endsWith("-P") ? OptionType.Put : OptionType.Put);
                fme = new OptionMarketEvent(instr, "BD", quote.getMultiplier(),
                        quote.getIsAmer());
                Option_Quote_Builders.put(symbol, fme);
            }
            TopOfBookEvent ToBE = fme.getTopOfBookEvent(quote.getBid(),
                    quote.getAsk(), quote.getBidSize(), quote.getAskSize());

            List<String> handles = handlebysubrequest.get(new String(header));
            if (handles != null) {
                SLF4JLoggerProxy.debug(this,
                        "MarketceteraFeed received response for handle(s): {}",//$NON-NLS-1$
                        handles);
                for (String handle : handles) {
                    dataReceived(handle, ToBE);
                }
            }

        }

        private void doupdateFutQuote(FutQuote quote, String symbol,
                byte[] header) {
            SLF4JLoggerProxy.debug(this,
                    "MarketceteraFeed received Future quote for {}", symbol); //$NON-NLS-1$
            Instrument instr = null;
            FutureMarketEvent fme = Future_Quote_Builders.get(symbol);
            // String req = new String(header);
            if (fme == null) {
                instr = new Future(symbol, quote.getExpiry());
                fme = new FutureMarketEvent(instr, "BD", quote.getMultiplier());
                Future_Quote_Builders.put(symbol, fme);
            }
            TopOfBookEvent ToBE = fme.getTopOfBookEvent(quote.getBid(),
                    quote.getAsk(), quote.getBidSize(), quote.getAskSize());
            List<String> handles = handlebysubrequest.get(new String(header));
            if (handles != null) {
                SLF4JLoggerProxy.debug(this,
                        "MarketceteraFeed received response for handle(s): {}",
                        handles);
                for (String handle : handles) {
                    dataReceived(handle, ToBE);
                }
            }
        }

        private void doupdateFutTrade(FutTrade trade, String symbol,
                byte[] header) {
            SLF4JLoggerProxy.debug(this,
                    "MarketceteraFeed received Future trade for {}", symbol);
            Instrument instr = null;
            FutureMarketEvent fme = Future_Quote_Builders.get(symbol);
            if (fme == null) {
                instr = new Future(symbol, trade.getExpiry());
                fme = new FutureMarketEvent(instr, "BD",
                        (int) trade.getMultiplier());
                Future_Quote_Builders.put(symbol, fme);
            }

            TradeEvent tradev = fme.getTradeEvent(trade);

            List<String> handles = handlebysubrequest.get(new String(header));
            if (handles != null) {
                SLF4JLoggerProxy.debug(this,
                        "MarketceteraFeed received response for handle(s): {}", //$NON-NLS-1$
                        handles);
                for (String handle : handles) {
                    dataReceived(handle, tradev);
                }
            }
            // Future Stat
            header[0] = '3';
            header[1] = '7';
            doupdateFutStats(null, symbol, header);
        }

        private void doupdateOptTrade(OptTrade trade, String symbol,
                byte[] header) {
            SLF4JLoggerProxy.debug(this,
                    "MarketceteraFeed received Option Trade for {}", symbol); //$NON-NLS-1$     

            Instrument instr = null;
            OptionMarketEvent fme = Option_Quote_Builders.get(symbol);
            // String req = new String(header);
            if (fme == null) {
                instr = new org.marketcetera.trade.Option(symbol,
                        trade.getExpiry(), new BigDecimal(trade.getStrike()),
                        symbol.endsWith("-P") ? OptionType.Put : OptionType.Put);
                fme = new OptionMarketEvent(instr, "BD", trade.getMultiplier(),
                        trade.getIsAmer());
                Option_Quote_Builders.put(symbol, fme);
            }

            TradeEvent tradev = fme.getTradeEvent(trade);
            List<String> handles = handlebysubrequest.get(new String(header));
            if (handles != null) {
                SLF4JLoggerProxy.debug(this,
                        "MarketceteraFeed received response for handle(s): {}", //$NON-NLS-1$
                        handles);
                for (String handle : handles) {
                    dataReceived(handle, tradev);
                }
            }
            // Option Stat
            header[0] = '3';
            header[1] = '3';
            doupdateOptStats(null, symbol, header);

        }

    }

    private class ForexQuoteUpdateWorker implements Runnable {
        private ZMsg message;

        public ForexQuoteUpdateWorker(ZMsg msg) {
            // super(ZMQFeedModuleFactory.PROVIDER_URN, true);
            message = msg;
        }

        @Override
        public void run() {
            // SLF4JLoggerProxy
            // .info(this, "Message Processor eqty_thread - Message {}",
            // message.toString());
            if (message.contentSize() > 0) {
                byte header[] = message.getFirst().getData();
                try {
                    // message.pop();
                    header = message.getFirst().getData();
                    while ( header.length ==1 || ((header.length >1)&& header[2] == -117)) {
                        message.pop();
                        header = message.getFirst().getData();
                    }
                    int lenStr = header.length;
                    String symbol;

                    int mtype = MsgTypes.UNDEFINED_VALUE;
                    try {
                        mtype = Integer.parseInt(new String(Arrays.copyOfRange(
                                header, 0, 2)));
                    } catch (NumberFormatException nfe) {
                        nfe.printStackTrace();
                    }

//                    if (mtype == MsgTypes.MOC_VALUE
//                    // || mtype == MsgTypes.HALT_VALUE
//                            || mtype == MsgTypes.DIVIDEND_VALUE) {
//                        symbol = new String(Arrays.copyOfRange(header, 3,
//                                lenStr));
//                    } else {
                        symbol = new String(Arrays.copyOfRange(header, 3,
                                lenStr - 1));
//                    }
                    if (mtype == MsgTypes.FOREX_QUOTE_VALUE) {
                        QtmsgCount.incrementAndGet();
                        long tnow = System.nanoTime();
                        Quote quote = Quote.PARSER.parseFrom(message.getLast()
                                .getData());
                        doupdateQuote(quote, symbol, header);
                        QtmsgTot.addAndGet(System.nanoTime() - tnow);
                    }

                    SLF4JLoggerProxy
                            .debug(ZMQFeed.class, "Recieved {}", symbol);
                } catch (InvalidProtocolBufferException e) {
                    SLF4JLoggerProxy
                            .warn(ZMQFeed.class,
                                    "Encountered an Protocol buffer exception {} - {} ",
                                    e, message.getLast());
                }
            }
            message.destroy();
        }

        private void doupdateQuote(Quote quote, String symbol, byte[] header) {
            String hdr = new String(header);
            ForexMarketEvent eb = Forex_Quote_Builders.get(symbol);
            if (eb == null) {
                eb = new ForexMarketEvent(symbol);
                Forex_Quote_Builders.put(symbol, eb);
            }

            TopOfBookEvent ToBE = eb.getTopOfBookEvent(quote.getBid(),
                    quote.getAsk(), quote.getBidSize(), quote.getAskSize());
            List<String> handles = handlebysubrequest.get(hdr);
            if (handles != null) {
                SLF4JLoggerProxy.debug(this,
                        "MarketceteraFeed received response for handle(s): {}", //$NON-NLS-1$
                        handles);
                for (String handle : handles) {
                    dataReceived(handle, ToBE);
                }
            }
        }
        
    }

    // private class EqtyQuoteUpdateWorker extends Module implements
    // Runnable,DataEmitter {
    private class LimeQuoteUpdateWorker implements Runnable {
        private ZMsg message;

        public LimeQuoteUpdateWorker(ZMsg msg) {
            // super(ZMQFeedModuleFactory.PROVIDER_URN, true);
            message = msg;
        }

        @Override
        public void run() {
            // SLF4JLoggerProxy
            // .info(this, "Message Processor eqty_thread - Message {}",
            // message.toString());
            if (message.contentSize() > 0) {
                byte header[] = message.getFirst().getData();
                try {
                    // message.pop();
                    header = message.getFirst().getData();
                    while ( header.length ==1 || ((header.length >1)&& header[2] == -117)) {
                        message.pop();
                        header = message.getFirst().getData();
                    }
                    int lenStr = header.length;
                    String symbol;

                    int mtype = MsgTypes.UNDEFINED_VALUE;
                    try {
                        mtype = Integer.parseInt(new String(Arrays.copyOfRange(
                                header, 0, 2)));
                    } catch (NumberFormatException nfe) {
                        nfe.printStackTrace();
                    }

                    if (mtype == MsgTypes.MOC_VALUE
                    // || mtype == MsgTypes.HALT_VALUE
                            || mtype == MsgTypes.DIVIDEND_VALUE) {
                        symbol = new String(Arrays.copyOfRange(header, 3,
                                lenStr));
                    } else {
                        symbol = new String(Arrays.copyOfRange(header, 3,
                                lenStr - 1));
                    }
                    if (mtype == MsgTypes.QUOTE_VALUE) {
                        QtmsgCount.incrementAndGet();
                        long tnow = System.nanoTime();
                        Quote quote = Quote.PARSER.parseFrom(message.getLast()
                                .getData());
                        doupdateQuote(quote, symbol, header);
                        QtmsgTot.addAndGet(System.nanoTime() - tnow);
                    } else if (mtype == MsgTypes.TRADE_VALUE) {
                        TrdmsgCount.incrementAndGet();
                        long tnow = System.nanoTime();
                        Trade trade = Trade.PARSER.parseFrom(message.getLast()
                                .getData());
                        doupdateTrade(trade, symbol, header);
                        TrdmsgTot.addAndGet(System.nanoTime() - tnow);
                    } else if (mtype == MsgTypes.INDEX_VALUE) {
                        Index index = Index.PARSER.parseFrom(message.getLast()
                                .getData());
                        doupdateIndex(index, symbol, header); // message.getLast().getData());
                    } else if (mtype == MsgTypes.MOC_VALUE) {
                        Moc moc_msg = Moc.PARSER.parseFrom(message.getLast()
                                .getData());
                        doupdateMoc(moc_msg, symbol, header);
                    } else if (mtype == MsgTypes.STK_STATUS_VALUE) {
                        StkStatus state = StkStatus.PARSER.parseFrom(message
                                .getLast().getData());
                        doupdateStkStatus(state, symbol, header, -1);
                    } else if (mtype == MsgTypes.MKT_STAT_VALUE) {
                        MktStat stat = MktStat.PARSER.parseFrom(message
                                .getLast().getData());
                        doupdateStats(stat, symbol, header);
                    } else if (mtype == MsgTypes.DIVIDEND_VALUE) {
                        Dividend div = Dividend.PARSER.parseFrom(message
                                .getLast().getData());
                        doupdateDividend(div, symbol, header);
                    } else if (mtype == MsgTypes.QF_MKT_STATE_VALUE) {
                        MarketState mste = MarketState.PARSER.parseFrom(message
                                .getLast().getData());
                        doupdateMktStatus(mste, symbol, header);
                    }

                    SLF4JLoggerProxy
                            .debug(ZMQFeed.class, "Recieved {}", symbol);
                } catch (InvalidProtocolBufferException e) {
                    SLF4JLoggerProxy
                            .warn(ZMQFeed.class,
                                    "Encountered an Protocol buffer exception {} - {} ",
                                    e, message.getLast());
                }
            }
            message.destroy();
        }

        private void doupdateDividend(Dividend div, String symbol, byte[] header) {
            int mkrs = div.getMarkers();
            int d2x = mkrs & DivCodes.D_TO_EX_VALUE;
            int mk1 = (mkrs & DivCodes.MRKR1_VALUE) >> DivCodes.L_SHIFT_VALUE;
            mk1 = mk1 > divstr.length ? divstr.length : mk1;
            int mk2 = (mkrs & DivCodes.MRKR2_VALUE) >> 2 * DivCodes.L_SHIFT_VALUE;
            mk2 = mk2 > divstr.length ? divstr.length : mk2;
            int mk3 = (mkrs & DivCodes.MRKR3_VALUE) >> 3 * DivCodes.L_SHIFT_VALUE;
            mk3 = mk3 > divstr.length ? divstr.length : mk3;
            SLF4JLoggerProxy.info(
                    org.marketcetera.core.Messages.USER_MSG_CATEGORY,
                    "{}{} dividend {} ex: {} pay: {} record: {} {}",
                    (d2x == 0 ? "New - " : ""),
                    symbol,
                    div.getDivstr() == "" ? new DecimalFormat("0.0000")
                            .format(div.getDiv()) : div.getDivstr(),
                    "20" + div.getExdate(), "20" + div.getPaydate(),
                    "20" + div.getRecdate(), divstr[DivCodes.valueOf(mk1)
                            .getNumber()]
                            + " "
                            + divstr[DivCodes.valueOf(mk2).getNumber()]
                            + " "
                            + divstr[DivCodes.valueOf(mk3).getNumber()]);
            Equity equity = new Equity(symbol);
            // String exchange = "TO";
            DividendEvent dividend = DividendEventBuilder.dividend()
                    .withEquity(equity).withCurrency("CAD")
                    .withAmount(new BigDecimal(div.getDiv()))
                    .withDeclareDate(div.getDecdate())
                    .withExecutionDate(div.getExdate())
                    .withType(DividendType.CURRENT)
                    .withFrequency(DividendFrequency.ANNUALLY)
                    .withStatus(DividendStatus.OFFICIAL).create();

            List<String> handles = handlebysubrequest.get(new String(header));
            if (handles != null) {
                SLF4JLoggerProxy.debug(this,
                        "MarketceteraFeed received response for handle(s): {}", //$NON-NLS-1$
                        handles);
                for (String handle : handles) {
                    dataReceived(handle, dividend);
                }
            }
        }

        private void doupdateIndex(Index index, String symbol, byte[] header) {
            String hdr = new String(header);
            LimeMarketEvent eb = Lime_Quote_Builders.get(symbol);
            if (eb == null) {
                eb = new LimeMarketEvent(symbol, "US");
                Lime_Quote_Builders.put(symbol, eb);
            }

            // float flast = index.getLast();
       //     if (index.getIsinterim()) {
                TopOfBookEvent ToBE = eb.getTopOfBookEvent(
                        (double) index.getBid(), (double) index.getAsk(), 0, 0);
                TradeEvent tradev = eb.getIndexTradeEvent(index);
                // Convert to Marketcetera quote
                List<String> handles = handlebysubrequest.get(hdr);
                if (handles != null) {
                    SLF4JLoggerProxy
                            .debug(this,
                                    "MarketceteraFeed received response for handle(s): {}", //$NON-NLS-1$
                                    handles);
                    for (String handle : handles) {
                        dataReceived(handle, ToBE);
                        dataReceived(handle, tradev);
                    }
                }
         //   } else {
                //TradeEvent tradev = eb.getIndexTradeEvent(index);
                //List<String> handles = handlebysubrequest.get(hdr);
                //if (handles != null) {
                //    SLF4JLoggerProxy
                 //           .debug(this,
                //                    "MarketceteraFeed received response for handle(s): {}", //$NON-NLS-1$
                //                    handles);
                //    for (String handle : handles) {
                //        dataReceived(handle, tradev);
                //    }
               // }
            //}
        }

        private void doupdateMoc(Moc moc_msg, String symbol, byte[] header) {
            float side = moc_msg.getSide();
            SLF4JLoggerProxy.info(
                    org.marketcetera.core.Messages.USER_MSG_CATEGORY,
                    "{} {} {}", symbol, side < 0 ? "Sell" : "Buy",
                    new DecimalFormat("###,###,###")
                            .format(side < 0 ? (side * -1) : side));
            MocEvent moc = new MocEvent(symbol, side, moc_msg.getVwap());
            byte[] new_req;
            if ( header.length > 2 && header[0] == '1' && header[1] == '4' ) {
                new_req = new byte[header.length];
            } else {
                new_req = new byte[header.length + 1];
            }
            System.arraycopy(header, 0, new_req, 0, header.length);
            new_req[new_req.length - 1] = 30;
            new_req[0]='0';
            new_req[1]='1';
            
            List<String> handles = handlebysubrequest.get(new String(new_req));
            if (handles != null) {
                SLF4JLoggerProxy
                        .debug(this,
                                "Recieved a moc Event received response for handle(s): {}", //$NON-NLS-1$
                                handles);
                for (String handle : handles) {
                    dataReceived(handle, moc);
                }
            }
        }

        private void doupdateStkStatus(StkStatus state, String symbol,
                byte[] header, int stkgrp) {
            INotification notification;
            StkStateEvent sse = new StkStateEvent(symbol, state.getState(),
                    state.getExpectedOpen(), stkgrp);
            // EnumValueDescriptor ev2 = state.getState().getValueDescriptor();
            // only send state once
            // EnumValueDescriptor d = StockStatus.A.getDescriptorForType();
            // header[1] = '1';
            byte[] lthdr = Arrays.copyOf(header, header.length);
            lthdr[0] = 48;
            lthdr[1] = 49;
            List<String> handles = handlebysubrequest.get(new String(lthdr));
            if (handles != null) {
                SLF4JLoggerProxy
                        .debug(this,
                                "Recieved a stockstate Event received response for handle(s): {}", //$NON-NLS-1$
                                handles);
                for (String handle : handles) {
                    dataReceived(handle, sse);
                }
            }
            // notify the desktop etc
            String exch = symbol.endsWith("-V") ? "Venture" : "TSX";
            if (state.getState() != StockStatus.A) { // Authorised
                notification = Notification
                        .high(symbol,
                                "Market State "
                                        + state.getState().name()
                                        + " "
                                        + state.getReason()
                                        + ((state.getExpectedOpen().length() > 0) ? (" and is expected to open " + state
                                                .getExpectedOpen()) : ""), exch);
            } else {
                notification = Notification.high(symbol, "Authorised", exch);
            }
            if (stkgrp == -1)
                NotificationManager.getNotificationManager().publish(
                        notification);
        }

        private void doupdateMktStatus(MarketState state, String symbol,
                byte[] header) {
            mktStateEvent mse = new mktStateEvent(symbol, state.getState());
            // EnumValueDescriptor ev2 = state.getState().getValueDescriptor();
            // only send state once
            // EnumValueDescriptor d = StockStatus.A.getDescriptorForType();
            List<String> handles = handlebysubrequest.get(new String(header));
            if (handles != null) {
                SLF4JLoggerProxy
                        .debug(this,
                                "Recieved a stockstate Event received response for handle(s): {}", //$NON-NLS-1$
                                handles);
                for (String handle : handles) {
                    dataReceived(handle, mse);
                }
            }
        }

        /*
         * private void doupdateStkhalt(HaltResume haltresume_msg, String
         * symbol) { INotification notification;
         * 
         * if (haltresume_msg.getReasonForHalt() != "") { notification =
         * Notification.high(symbol + " Halted ", "Reason - " +
         * haltresume_msg.getReasonForHalt(), "TMX"); SLF4JLoggerProxy.debug(
         * org.marketcetera.core.Messages.USER_MSG_CATEGORY, "{} Halted {} ",
         * symbol, haltresume_msg.getReasonForHalt()); } else {
         * SLF4JLoggerProxy.debug(
         * org.marketcetera.core.Messages.USER_MSG_CATEGORY,
         * "{} Expected to resume {} ", symbol,
         * haltresume_msg.getExpectedOpen()); notification = Notification.high(
         * symbol, new String("Expected to Resume - " +
         * haltresume_msg.getExpectedOpen()), "TMX"); }
         * 
         * NotificationManager.getNotificationManager().publish(notification);
         * 
         * }
         */
        private void doupdateTrade(Trade trade, String symbol, byte[] header) {
            String hdr = new String(header);
            LimeMarketEvent eb = Lime_Quote_Builders.get(symbol);
            if (eb == null) {
                eb = new LimeMarketEvent(symbol, "US");
                Lime_Quote_Builders.put(symbol, eb);
            }
            TradeEvent tradev = eb.getTradeEvent(trade);
            List<String> handles = handlebysubrequest.get(hdr);
            SLF4JLoggerProxy.debug(this, "Trade {} {}", //$NON-NLS-1$
                    symbol, tradev);
            if (tradev != null && handles != null) {
                SLF4JLoggerProxy.debug(this,
                        "MarketceteraFeed received response for handle(s): {}", //$NON-NLS-1$
                        handles);
                for (String handle : handles) {
                    dataReceived(handle, tradev);
                }
            }
            if (trade.getLast() > 0) {
                // change header to be Stat
                header[0] = '1';
                header[1] = '4';
                doupdateStats(null, symbol, header);
            }
        }

        private void doupdateQuote(Quote quote, String symbol, byte[] header) {
            String hdr = new String(header);
            LimeMarketEvent eb = Lime_Quote_Builders.get(symbol);
            if (eb == null) {
                eb = new LimeMarketEvent(symbol, "US");
                Lime_Quote_Builders.put(symbol, eb);
            }

            TopOfBookEvent ToBE = eb.getTopOfBookEvent(quote.getBid(),
                    quote.getAsk(), quote.getBidSize(), quote.getAskSize());
            List<String> handles = handlebysubrequest.get(hdr);
            if (handles != null) {
                SLF4JLoggerProxy.debug(this,
                        "MarketceteraFeed received response for handle(s): {}", //$NON-NLS-1$
                        handles);
                for (String handle : handles) {
                    dataReceived(handle, ToBE);
                }
            }
        }

        private void doupdateStats(MktStat stat, String symbol, byte[] header) {
            String hdr = new String(header);
            List<String> handles = handlebysubrequest.get(hdr);
            if (handles == null)
                return;
            LimeMarketEvent eb = Lime_Quote_Builders.get(symbol);
            if (eb == null) {
                eb = new LimeMarketEvent(symbol, "US");
                Lime_Quote_Builders.put(symbol, eb);
            }

            MarketstatEvent mste = eb.getMktStat(stat, TODAY, YESTERDAY);
            if (stat != null) {
                Moc moc = null;
                StkStatus sstate = null;
                if (stat.getState() != null) {
                    sstate = StkStatus.newBuilder().setState(stat.getState())
                            .setMsgType(Msgs.MsgTypes.STK_STATUS)
                            .setTimeHalted("080000").setExpectedOpen("080000")
                            .setReason("").build();
                    doupdateStkStatus(sstate, symbol, header, stat.getStkgrp());
                }

                if (stat.getMoc() != 0) {
                    moc = Moc.newBuilder().setSide(stat.getMoc())
                            .setMsgType(Msgs.MsgTypes.MOC).build();
                    
                    doupdateMoc(moc, symbol, header);
                }
            }
            if (handles != null) {
                SLF4JLoggerProxy
                        .debug(this,
                                "Recieved a marketstat Event received response for handle(s): {}", //$NON-NLS-1$
                                handles);
                for (String handle : handles) {
                    dataReceived(handle, mste);
                }
            }

        }
    }
    
    /*
     * This class recieves the subscription eqty_req_queue and has a worker pool
     * of data requestor threads which converts from wire to acmes quote object
     * - to marketcetera objec then for each quote, iterates through handle and
     * does datarecieved *
     */
    private class CurrentTickRequestor extends Thread {
        private String req;
        private boolean bisRunning = false;
        ZMQ.Poller items; // = zmqcontext.poller(2);
        ZMQ.Socket req_sock;
        ZMQ.Socket ipc_sock;
        BlockingQueue<String> req_queue;
        BlockingQueue<ZMsg> resp_queue;
        AtomicBoolean bIPCPairrunning;
        private final boolean includeIdentity;
        public final String identity = System.getProperty("user.name") + "|" + getIpv4Address();
        
        private CurrentTickRequestor(ZMQ.Context inzmqcontext,
                String conStrIPC, String conStrTCP,
                BlockingQueue<String> reqqu, BlockingQueue<ZMsg> respqu,
                AtomicBoolean ibool,
                boolean includeIdentity) {
            super("CurrentTickRequestor-" + conStrIPC);
            this.includeIdentity = includeIdentity;
            
            req_queue = reqqu;
            resp_queue = respqu;
            bIPCPairrunning = ibool;
            items = zmqcontext.poller(2);
            req_sock = inzmqcontext.socket(ZMQ.DEALER);
            SLF4JLoggerProxy.info(ZMQFeed.class,
                    "Request Thread Connected to {}", conStrTCP); //$NON-NLS-1$
            req_sock.setSendTimeOut(reqTimeout);
            req_sock.setReceiveTimeOut(reqTimeout);
            req_sock.setLinger(0);
            req_sock.connect(conStrTCP);
            SLF4JLoggerProxy.info(this,
                    "Request Thread Connected to {}", conStrIPC); //$NON-NLS-1$
            ipc_sock = inzmqcontext.socket(ZMQ.PAIR);
            ipc_sock.setLinger(0);
            try {
                while (!bIPCPairrunning.get()) {
                    Thread.sleep(20);
                    SLF4JLoggerProxy.info(this,
                            "Waiting for me to bind to inproc"); //$NON-NLS-1$
                }
            } catch (InterruptedException ie) {
            }
            ipc_sock.connect(conStrIPC);
            SLF4JLoggerProxy.info(this, "Connected to Inproc"); //$NON-NLS-1$
            items.register(req_sock, ZMQ.Poller.POLLIN);

        }

        public boolean isRunning() {
            return bisRunning;

        }

        // @Override
        public void run() {
            long nextHeartbeat = System.currentTimeMillis() + 5000;
            
            SLF4JLoggerProxy.info(this, "Starting subscription handler");
            ZMsg msg;
            bisRunning = true;
            while (isRunning.get()) {
                try {
                    if (!req_queue.isEmpty()) {
                        req = req_queue.take();
                        if (req.charAt(2) != '-') {
                            if (req.charAt(3) == '_' && req.charAt(4) == 'W' && req.charAt(8) == '_' & req.charAt(19) == '_'& req.charAt(14) == 'q') {        
                                final String request = req.substring(0, 2);
                                new Thread()
                                {
                                    @Override
                                    public void run() {
                                        QuotePrimer qp = new QuotePrimer(eqty_sub_queue, "_Warm_up_request_", 10000, Integer.parseInt(request));
                                        qp.run();
                                    }
                                }.start();
                                
                            } else {
                        	
                        	if ( includeIdentity ) {
                        	    req_sock.sendMore(req);
                        	    req_sock.send(identity);
                        	    //System.err.println("Requesting " + req); //$NON-NLS-1$
                        	} else {
                        	    req_sock.send(req);
                        	}
                        	nextHeartbeat = System.currentTimeMillis() + 5000;
                        	SLF4JLoggerProxy.debug(this, "Requesting {}", req); //$NON-NLS-1$
                            }
                        }
//                        if (req != null) {
                            if (req.regionMatches(3, "I-", 0, 2)) {
                                req = String.format("%02d", MsgTypes.INDEX_VALUE)
                                        + req.substring(2);
                            }
                            ipc_sock.send(req, ZMQ.DONTWAIT);
                            req = null;
//                        }
                        SLF4JLoggerProxy.debug(this, "Requesting data for {} ...",
                                req);
                        continue;
                    }
                    if ( nextHeartbeat < System.currentTimeMillis() && includeIdentity ) {
                	nextHeartbeat = System.currentTimeMillis() + 5000;
                	req_sock.sendMore("_HEARTBEAT_");
                	req_sock.send(identity);
                    }
                    items.poll(200); // reqTimeout); // wait 20ms
                    if (items.pollin(0)) {
                        msg = ZMsg.recvMsg(req_sock);
                        try {
                            for (int n = 0; n < msg.size(); n++) {
                                int len = msg.peekFirst().getData().length;
                                if ( n == 0 && len == 10 /* DISCONNECT */ && "DISCONNECT".equals(new String(msg.peekFirst().getData())) ) {
                                    if ( req_queue == lime_req_queue ) {
                                	limehandler.mustexit = true;
                                        lime_sub_queue.put(new ZMsg());
                                    }
                                }
                                if (len < 3
                                        || msg.peekFirst().getData()[2] != 30) {
                                    msg.pop();
                                } else
                                    break;
                            }
                            resp_queue.put(msg);
                            // msg.destroy();
                            SLF4JLoggerProxy.debug(this, "Recieved {}", req); //$NON-NLS-1$
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }
                } catch (InterruptedException ecp) {
                    SLF4JLoggerProxy.debug(this,
                            "Request processor got a wake-up call...");
                }
            }
            req_sock.close();
            ipc_sock.close();
            bisRunning = false;
            SLF4JLoggerProxy.info(this, "Request processor has shut down");
        }

    }

    public enum ZMQ_MKCET_HANDLER_TYPE {
	EQUITY,
	FOREX,
	FUT_OPS,
	LIME,
    }
    
    /*
     * This class recieves the ZMq message and has a worker pool of
     * EqtyQuoteUpdateWorker threads which converts from wire to acmes quote
     * object - to marketcetera objec then for each quote, iterates through
     * handle and does datarecieved *
     */
    private class ZMQtoMkcetHandler extends Thread {
        /*
         * This monitors the worker Threads
         */
        private class Monitor implements Runnable {
            private int seconds;
            private ThreadPoolExecutor executor;
            private long las_msg;
            private String myname;

            public Monitor(ThreadPoolExecutor exec, int delay, String name) {
                executor = exec;
                seconds = delay;
                myname = name;
            }

            @Override
            public void run() {
                while (isRunning.get()) {
                    SLF4JLoggerProxy
                            .info(Monitor.class,
                                    "{} Pool Size:Core {}:{} - Active {}, completed {}, tasks {}, Msgs/Sec {}, Queue {}",
                                    myname, executor.getPoolSize(),
                                    executor.getCorePoolSize(),
                                    executor.getActiveCount(),
                                    executor.getCompletedTaskCount(),
                                    executor.getTaskCount(),
                                    (msgCount.get() - las_msg) / seconds,
                                    eqty_sub_queue.size());
                    long numquotes = QtmsgCount.get();
                    long numtrades = TrdmsgCount.get();
                    // if ( numquotes > 0 && numtrades >0 ){
                    SLF4JLoggerProxy
                            .info(this, "{} Times: Quote - {}, Trade - {}",
                                    myname, numquotes > 0 ? (QtmsgTot.get()
                                            / numquotes * 0.001) : 0,
                                    numtrades > 0 ? (TrdmsgTot.get()
                                            / numtrades * 0.001) : 0);
                    // }
                    las_msg = msgCount.get();
                    try {
                        Thread.sleep(seconds * 1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            }
        }

        private Thread monitor;
        private ThreadFactory threadFactory;
        private ThreadPoolExecutor executorPool;
        private int maxnumThrds;
        private int corenumThrds;
        private boolean bisInit = false;
        private BlockingQueue<ZMsg> qu;
        private String myname;
        private ZMQ_MKCET_HANDLER_TYPE handlerType;
        public boolean mustexit = false;
        
        public ZMQtoMkcetHandler(BlockingQueue<ZMsg> inqu, String name,
                                 ZMQ_MKCET_HANDLER_TYPE handlerType) { // int
            super("ZMQtoMkcetHandler-" + name);
            // min,
            // int
            // max, int
            // timeout) {
            qu = inqu;
            myname = name;
            this.handlerType = handlerType;
            threadFactory = Executors.defaultThreadFactory();
            corenumThrds = Runtime.getRuntime().availableProcessors();
            if (corenumThrds > 4) {
                corenumThrds = 4;
                maxnumThrds = 8;
            } else {
                corenumThrds = 2;
                maxnumThrds = 4;
            }
            BlockingQueue<Runnable> queue = new LinkedBlockingQueue<Runnable>();
            executorPool = new ThreadPoolExecutor(corenumThrds, maxnumThrds,
                    200, TimeUnit.MILLISECONDS, queue, threadFactory);
            // threadFactory, rejectionHandler);
            monitor = new Thread(new Monitor(executorPool, 60, myname), "ZMQtoMkcetHandler-Monitor-" + name);
            monitor.start();
        }

        public boolean isRunning() {
            return bisInit;
        }

        public void cancel() {
            try {
                monitor.join(1000);
            } catch (InterruptedException ie) {

            }
            executorPool.shutdown();
            // interrupt();
        }

        // @Override

        public void run() {
            ZMsg message;
            // /executorPool.execute(new EqtyQuoteUpdateWorker(message));
            SLF4JLoggerProxy.warn(this, "Starting processor");
            bisInit = true;
            while (!mustexit && isRunning.get()) {
                try {
                    message = qu.take();
                    if (message != null && message.peekFirst() != null) {
                	switch (handlerType) {
                	    case EQUITY:
                		executorPool.execute(new EqtyQuoteUpdateWorker(message));
                		SLF4JLoggerProxy.debug(this, "Queue got an Equity message ...");
                		break;
                	    case FUT_OPS:
                		// executorPool.execute(new EqtyQuoteUpdateWorker(
                		// message));
                		executorPool.execute(new FuOpQuoteUpdateWorker(message));
                		SLF4JLoggerProxy.debug(this,"Queue got a Future message ...");
                		break;
                	    case FOREX:
                		executorPool.execute(new ForexQuoteUpdateWorker(message));
                		SLF4JLoggerProxy.debug(this,"Queue got a Forex message ...");
                		break;
                	    case LIME:
                		executorPool.execute(new LimeQuoteUpdateWorker(message));
                		//SLF4JLoggerProxy.info(this, "Queue got a Lime message ...");
                		//System.err.println("Queue got a Lime message");
                		break;
                	}
                    }
                } catch (InterruptedException ecp) {
                    SLF4JLoggerProxy.warn(this,
                            "Blocking Queue... got interrupted");
                    Thread.currentThread().interrupt();
                }
            }
            // stop();
            SLF4JLoggerProxy.info(this, "Quote update Pool has shut down");
            cancel();
            bisInit = false;
        }
    }

    public static String getIpv4Address() {
	String hostname = null;
	try {
	    Enumeration<NetworkInterface> interfaces = NetworkInterface.getNetworkInterfaces();
	    while (interfaces.hasMoreElements()) {
		NetworkInterface nic = interfaces.nextElement();
		Enumeration<InetAddress> addresses = nic.getInetAddresses();
		while (hostname == null && addresses.hasMoreElements()) {
		    InetAddress address = addresses.nextElement();
		    if ( address instanceof Inet4Address && !address.isLoopbackAddress()) {
			hostname = address.getHostName();
		    }
		}
	    }
	} catch (SocketException e) {
	}
	if ( hostname == null )
	    hostname = System.getenv("HOSTNAME");
	if ( hostname == null )
	    hostname = System.getenv("COMPUTERNAME");
	if ( hostname == null )
	    return hostname = "?";
	return hostname;
    }
    
}
