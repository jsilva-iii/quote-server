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

import org.marketcetera.core.CoreException;
import org.marketcetera.core.NoMoreIDsException;
import org.marketcetera.marketdata.AbstractMarketDataFeedFactory;
import org.marketcetera.marketdata.FeedException;
import org.marketcetera.util.misc.ClassVersion;

 /* $License$ */

/**
 * {@link ZMQFeed} constructor factory.
 *
 * @author <a href="mailto:colin@marketcetera.com">Colin DuPlantis</a>
 * @version $Id: ZMQFeedFactory.java 16154 2012-07-14 16:34:05Z colin $
 * @since 0.5.0
 */
@ClassVersion("$Id: ZMQFeedFactory.java 16154 2012-07-14 16:34:05Z colin $")  //$NON-NLS-1$
public class ZMQFeedFactory 
    extends AbstractMarketDataFeedFactory<ZMQFeed,ZMQFeedCredentials> 
{
    private final static ZMQFeedFactory sInstance = new ZMQFeedFactory();
    public static ZMQFeedFactory getInstance()
    {
        return sInstance;
    }
	public String getProviderName()
	{
		return "TSX/BDM L1"; //$NON-NLS-1$
	}
    /* (non-Javadoc)
     * @see org.marketcetera.marketdata.IMarketDataFeedFactory#getMarketDataFeed()
     */
    @Override
    public ZMQFeed getMarketDataFeed()
            throws CoreException
    {
        try {
            return ZMQFeed.getInstance(getProviderName());
        } catch (NoMoreIDsException e) {
            throw new FeedException(e);
        }
    }
    
}
