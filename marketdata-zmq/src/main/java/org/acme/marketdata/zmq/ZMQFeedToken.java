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

import org.marketcetera.core.ClassVersion;
import org.marketcetera.marketdata.AbstractMarketDataFeedToken;
import org.marketcetera.marketdata.MarketDataFeedTokenSpec;

/**
 * Token for {@link ZMQFeed}.
 *
 * @author <a href="mailto:colin@marketcetera.com">Colin DuPlantis</a>
 * @version $Id: ZMQFeedToken.java 16154 2012-07-14 16:34:05Z colin $
 * @since 0.5.0
 */
@ClassVersion("$Id: ZMQFeedToken.java 16154 2012-07-14 16:34:05Z colin $") //$NON-NLS-1$
public class ZMQFeedToken
        extends AbstractMarketDataFeedToken<ZMQFeed>
{
    static ZMQFeedToken getToken(MarketDataFeedTokenSpec inTokenSpec,
                                   ZMQFeed inFeed) 
    {
        return new ZMQFeedToken(inTokenSpec,
                                  inFeed);
    }   
    /**
     * Create a new ZMQFeedToken instance.
     */
    private ZMQFeedToken(MarketDataFeedTokenSpec inTokenSpec,
                           ZMQFeed inFeed) 
    {
        super(inTokenSpec,
              inFeed);
    }
    public String toString()
    {
        return String.format("TL1 FeedToken(%s)", //$NON-NLS-1$
                             getStatus());
    }
}
