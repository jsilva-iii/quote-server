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


import javax.management.MXBean;

import org.marketcetera.marketdata.AbstractMarketDataModuleMXBean;
import org.marketcetera.module.DisplayName;
import org.marketcetera.util.misc.ClassVersion;

/* $License$ */

/**
 * Provides an MX interface for the Yahoo market data feed.
 *
 * @author <a href="mailto:colin@marketcetera.com">Colin DuPlantis</a>
 * @version $Id: ZMQFeedMXBean.java 16154 2012-07-14 16:34:05Z colin $
 * @since 2.1.4
 */
@MXBean(true)
@DisplayName("Management Interface for the ZMQ Marketdata Feed")
@ClassVersion("$Id: ZMQFeedMXBean.java 16154 2012-07-14 16:34:05Z colin $")
public interface ZMQFeedMXBean
        extends AbstractMarketDataModuleMXBean
{
    /**
     * Returns the URL that describes the location of the ZMQ server.
     *
     * @return a <code>String</code> value
     */
    @DisplayName("Equity subscription URL")
    public String getEQTYSubURL();
    /**
     * Returns the URL that describes the location of the Future ZMQ server.
     *
     * @return a <code>String</code> value
     */
    @DisplayName("Future/Option subscription URL")
    public String getFUOPSubURL();
    /**
     * Returns the URL that describes the location of the ZMQ server.
     *
     * @return a <code>String</code> value
     */
    @DisplayName("Forex subscription URL")
    public String getForexSubURL();
    /**
     * Returns the URL that describes the location of the ZMQ server.
     *
     * @return a <code>String</code> value
     */
    @DisplayName("Lime subscription URL")
    public String getLimeSubURL();
//    /**
//     * Returns the URL that describes the location of the ZMQ server.
//     *
//     * @return a <code>String</code> value
//     */
//    @DisplayName("Equity request URL")
//    public String getEQTYReqURL();
//    /**
//     * Returns the URL that describes the location of the ZMQ server.
//     *
//     * @return a <code>String</code> value
//     */
//    @DisplayName("Futures/Options request URL")
//    public String getFUOPReqURL();
//    /**
//     * Returns the URL that describes the location of the ZMQ server.
//     *
//     * @return a <code>String</code> value
//     */
//    @DisplayName("Forex request URL")
//    public String getForexReqURL();
    /**
     * Sets the URL that describes the location of the Equity Exchange server.
     *
     * @param inURL a <code>String</code> value
     */
    @DisplayName("Equity subscription URL")
    public void setEQTYSubURL(@DisplayName("Equity subscription URL")
                       String inURL);
    /**
     * Sets the URL that describes the location of the ZMQ Exchange server.
     *
     * @param inURL a <code>String</code> value
     */
    @DisplayName("Future/Option subscription URL")
    public void setFUOPSubURL(@DisplayName("Future/Option subscription URL")
                       String inURL);
    /**
     * Sets the URL that describes the location of the Forex server.
     *
     * @param inURL a <code>String</code> value
     */
    @DisplayName("Forex subscription URL")
    public void setForexSubURL(@DisplayName("Forex subscription URL")
                       String inURL);
    /**
     * Sets the URL that describes the location of the Lime server.
     *
     * @param inURL a <code>String</code> value
     */
    @DisplayName("Lime subscription URL")
    public void setLimeSubURL(@DisplayName("Lime subscription URL")
                       String inURL);
//    /**
//     * Sets the URL that describes the location of the ZMQ Exchange server.
//     *
//     * @param inURL a <code>String</code> value
//     */
//    @DisplayName("Equity request URL")
//    public void setEQTYReqURL(@DisplayName("The request URL")
//                       String inURL);
//    @DisplayName("Futures/Options request URL")
//    public void setFUOPReqURL(@DisplayName("Futures/Options request URL")
//                       String inURL);
//    @DisplayName("Forex request URL")
//    public void setForexReqURL(@DisplayName("Forex request URL")
//                       String inURL);
}
