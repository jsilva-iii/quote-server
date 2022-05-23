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
// Toronto TL1 and Montreak HSVF depth 

package org.acme.marketdata.zmq;

//import static org.marketcetera.marketdata.marketcetera.Messages.TARGET_COMP_ID_REQUIRED;

import org.apache.commons.lang.StringUtils;
import org.apache.commons.lang.Validate;
import org.marketcetera.core.ClassVersion;
import org.marketcetera.marketdata.AbstractMarketDataFeedURLCredentials;
import org.marketcetera.marketdata.FeedException;


/* $License$ */

/**
 * Credentials implementation for {@link ZMQFeed}.
 * 
 * @author <a href="mailto:colin@marketcetera.com">Colin DuPlantis</a>
 * @since 0.5.0
 */
@ClassVersion("$Id: ZMQFeedCredentials.java 16154 2012-07-14 16:34:05Z colin $") //$NON-NLS-1$
public class ZMQFeedCredentials
	extends AbstractMarketDataFeedURLCredentials
{
    private String mEQTYSubURL;
    private String mEQTYReqURL;
    private String mFUOPSubURL;
    private String mFUOPReqURL;
    private String mForexSubURL;
    private String mForexReqURL;
    private String mLimeSubURL;
    private String mLimeReqURL;
	
    /**
     * Retrieves an instance of <code>ZMQFeedCredentials</code>.
     * @param string2 
     * @param string 
     * 
     * @return a <code>ZMQFeedCredentials</code> value
     * @throws FeedException if an error occurs while retrieving the credentials object
     */
    public static ZMQFeedCredentials getInstance(String ineqtypuburl, String infuoppuburl, String inForexPubURL, String inLimePubUrl) 
        throws FeedException
    {
        return new ZMQFeedCredentials( ineqtypuburl,  infuoppuburl, inForexPubURL, inLimePubUrl);
    }
    
    private ZMQFeedCredentials(String ineqtypuburl, String infuoppuburl, String inForexPubURL, String inLimePubURL) throws FeedException{
    	super(ineqtypuburl);

    	// EQUITY
    	mEQTYSubURL = StringUtils.trimToNull(ineqtypuburl);
    	Validate.notNull(mEQTYSubURL, Messages.MISSING_SUB_URL.getText());
	if ( mEQTYSubURL == null )
	    throw new FeedException(Messages.MISSING_SUB_URL);
    	
	mEQTYReqURL = convertPubSubToReqRep(mEQTYSubURL);
	if ( mEQTYReqURL == null )
	    throw new FeedException(Messages.MISSING_REQ_URL);
    	
    	// futures and options
	mFUOPSubURL = StringUtils.trimToNull(infuoppuburl);
	if ( mFUOPSubURL == null )
	    throw new FeedException(Messages.MISSING_SUB_URL);

	mFUOPReqURL = convertPubSubToReqRep(mFUOPSubURL);
	if ( mFUOPReqURL == null )
	    throw new FeedException(Messages.MISSING_REQ_URL);

	// FOREX
	mForexSubURL = StringUtils.trimToNull(inForexPubURL);
	if ( mForexSubURL == null )
	    throw new FeedException(Messages.MISSING_SUB_URL);

	mForexReqURL = convertPubSubToReqRep(mForexSubURL);
	if ( mForexReqURL == null )
	    throw new FeedException(Messages.MISSING_REQ_URL);

	// FOREX
	mLimeSubURL = StringUtils.trimToNull(inLimePubURL);
	if ( mLimeSubURL == null )
	    throw new FeedException(Messages.MISSING_SUB_URL);

	mLimeReqURL = convertPubSubToReqRep(mLimeSubURL);
	if ( mLimeReqURL == null )
	    throw new FeedException(Messages.MISSING_REQ_URL);
    }

    /**
     * 
     * @param url Must be of the pattern: &lt;something&gt;:port
     * @return
     */
    private String convertPubSubToReqRep(String url) {
	if ( url == null )
	    return null;
	
	if ( url.toLowerCase().startsWith("tcp://") ) {
	    int colon = url.lastIndexOf(':');
	    if ( colon < 0 )
		return null;
	    
	    String lhs = url.substring(0, colon);
	    String rhs = url.substring(colon + 1, url.length());
	    
	    int port;
	    try { port = Integer.parseInt(rhs); } catch (NumberFormatException e) { return null; }
	    if ( port <= 0 )
		return null;
	    return lhs + ":" + (port + 1);
	}
	
	if ( url.toLowerCase().startsWith("ipc://") ) {
	    if ( !url.endsWith("_P") )
		return null;
	    return url.substring(0, url.length() - 1) + "C";
	}

	return null;
    }
    
    public String getEqtySubURL()  { return mEQTYSubURL; }
    public String getEqtyReqURL()  { return mEQTYReqURL; }
    public String getFuOpSubURL()  { return mFUOPSubURL; }
    public String getFuOpReqURL()  { return mFUOPReqURL; }
    public String getForexSubURL() { return mForexSubURL; }
    public String getForexReqURL() { return mForexReqURL; }
    public String getLimeSubURL()  { return mLimeSubURL; }
    public String getLimeReqURL()  { return mLimeReqURL; }

}