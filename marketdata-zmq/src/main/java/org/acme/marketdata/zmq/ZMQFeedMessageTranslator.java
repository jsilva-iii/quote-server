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

import static org.marketcetera.marketdata.Messages.UNSUPPORTED_REQUEST;

import org.marketcetera.core.CoreException;
//import org.marketcetera.marketdata.Content;
import org.marketcetera.marketdata.DataRequestTranslator;
import org.marketcetera.marketdata.MarketDataRequest;
import org.marketcetera.util.log.I18NBoundMessage1P;
import org.marketcetera.util.misc.ClassVersion;

/* $License$ */

/**
 * Bogus feed implementation of {@link DataRequestTranslator}.
 *
 * @author <a href="mailto:colin@marketcetera.com">Colin DuPlantis</a>
 * @version $Id: ZMQFeedMessageTranslator.java 16154 2012-07-14 16:34:05Z colin $
 * @since 0.5.0
 */
@ClassVersion("$Id: ZMQFeedMessageTranslator.java 16154 2012-07-14 16:34:05Z colin $")
public class ZMQFeedMessageTranslator
    implements DataRequestTranslator<MarketDataRequest>
{
    /**
     * static instance
     */
    private static final ZMQFeedMessageTranslator sInstance = new ZMQFeedMessageTranslator();
    /**
     * Gets a <code>ZMQFeedMessageTranslator</code> instance.
     * 
     * @return a <code>ZMQFeedMessageTranslator</code> value
     */
    static ZMQFeedMessageTranslator getInstance()
    {
        return sInstance;
    }
    /**
     * Create a new ZMQFeedMessageTranslator instance.
     *
     */
    private ZMQFeedMessageTranslator()
    {
    }
    /* (non-Javadoc)
     * @see org.marketcetera.marketdata.DataRequestTranslator#translate(org.marketcetera.marketdata.DataRequest)
     */
    @Override
    public MarketDataRequest fromDataRequest(MarketDataRequest inRequest)
            throws CoreException
    {
        if(inRequest.validateWithCapabilities( ZMQFeed.ZMQ_CONTENT )){
        	
            return inRequest;
        }
        throw new CoreException(new I18NBoundMessage1P(UNSUPPORTED_REQUEST,
                                                       String.valueOf(inRequest.getContent())));
    }
}
