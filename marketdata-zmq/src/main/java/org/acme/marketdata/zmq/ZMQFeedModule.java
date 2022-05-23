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

import org.apache.commons.lang.StringUtils;
import org.apache.commons.lang.Validate;
import org.marketcetera.core.CoreException;
import org.marketcetera.marketdata.AbstractMarketDataModule;
import org.acme.marketdata.zmq.Messages;
import org.acme.marketdata.zmq.ZMQFeed;
import org.acme.marketdata.zmq.ZMQFeedMXBean;
import org.marketcetera.module.DisplayName;
import org.marketcetera.util.misc.ClassVersion;

//import javax.management.AttributeChangeNotification;
//import org.marketcetera.marketdata.AbstractMarketDataModuleMXBean;

/* $License$ */

/**
 * StrategyAgent module for {@link ZMQFeed}.
 * <p>
 * Module Features
 * <table>
 * <tr>
 * <th>Factory:</th>
 * <td>{@link ZMQFeedModuleFactory}</td>
 * </tr>
 * <tr>
 * <th colspan="2">See {@link AbstractMarketDataModule parent} for module
 * features.</th>
 * </tr>
 * </table>
 * 
 * @author <a href="mailto:colin@marketcetera.com">Colin DuPlantis</a>
 * @version $Id: ZMQFeedModule.java 16154 2012-07-14 16:34:05Z colin $
 * @since 1.0.0
 */
@ClassVersion("$Id: ZMQFeedModule.java 16154 2012-07-14 16:34:05Z colin $")//$NON-NLS-1$
public final class ZMQFeedModule extends
		AbstractMarketDataModule<ZMQFeedToken, ZMQFeedCredentials>

implements ZMQFeedMXBean {

	/**
	 * the underlying feed
	 */
	// private final ZMQFeed feed;
	/**
	 * the URL at which ZMQ provides the data
	 */
	private volatile String tsx_sub_url = "tcp://192.168.10.12:5536"; //$NON-NLS-1$ 
	private volatile String bdm_sub_url = "tcp://192.168.10.14:5538"; //$NON-NLS-1$ 
	private volatile String forex_sub_url = "tcp://192.168.10.14:5538"; //$NON-NLS-1$ 
	private volatile String lime_sub_url = "tcp://192.168.14:5540"; //$NON-NLS-1$ 
	
	/**
	 * Create a new BogusFeedEmitter instance.
	 * 
	 * @throws CoreException
	 */
	ZMQFeedModule() throws CoreException {
		super(ZMQFeedModuleFactory.INSTANCE_URN, ZMQFeedFactory.getInstance()
				.getMarketDataFeed());
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.marketcetera.marketdata.AbstractMarketDataModule#getCredentials()
	 */
	@Override
	protected final ZMQFeedCredentials getCredentials() throws CoreException {
		return ZMQFeedCredentials.getInstance(getEQTYSubURL(), getFUOPSubURL(), getForexSubURL(), getLimeSubURL());
	}

	@Override
	public String getEQTYSubURL() { return tsx_sub_url; }
	@Override
	public String getFUOPSubURL() { return bdm_sub_url; }
	@Override
	public String getForexSubURL() { return forex_sub_url; }
	@Override
	public String getLimeSubURL() { return lime_sub_url; }
//	@Override
//	public String getEQTYReqURL() { return tsx_req_url; }
//	@Override
//	public String getFUOPReqURL() { return bdm_req_url; }
//	@Override
//	public String getForexReqURL() { return forex_req_url; }

	@Override
	public void setEQTYSubURL(
			@DisplayName("Equity subscription URL (protocol://IP:port)") String inURL) {
		tsx_sub_url = StringUtils.trimToNull(inURL);
		Validate.notNull(tsx_sub_url, Messages.MISSING_SUB_URL.getText());
	}
	
	@Override
	public void setFUOPSubURL(
			@DisplayName("Futures/options subscription URL (protocol://IP:port)") String inURL) {
		bdm_sub_url = StringUtils.trimToNull(inURL);
		Validate.notNull(bdm_sub_url, Messages.MISSING_SUB_URL.getText());
	}
	
	@Override
	public void setForexSubURL(
			@DisplayName("Forex subscription URL (protocol://IP:port)") String inURL) {
		forex_sub_url = StringUtils.trimToNull(inURL);
		Validate.notNull(forex_sub_url, Messages.MISSING_SUB_URL.getText());
	}
	@Override
	public void setLimeSubURL(
			@DisplayName("Lime subscription URL (protocol://IP:port)") String inURL) {
		lime_sub_url = StringUtils.trimToNull(inURL);
		Validate.notNull(lime_sub_url, Messages.MISSING_SUB_URL.getText());
	}

//	@Override
//	public void setEQTYReqURL(
//			@DisplayName("Equity request URL (protocol://IP:port)") String inURL) {
//		tsx_req_url = StringUtils.trimToNull(inURL);
//		Validate.notNull(tsx_req_url, Messages.MISSING_REQ_URL.getText());
//	}
//	
//	@Override
//	public void setFUOPReqURL(
//			@DisplayName("The request URL (protocol://IP:port)") String inURL) {
//		bdm_req_url = StringUtils.trimToNull(inURL);
//		Validate.notNull(bdm_req_url, Messages.MISSING_REQ_URL.getText());
//	}
//
//	@Override
//	public void setForexReqURL(
//			@DisplayName("Forex request URL (protocol://IP:port)") String inURL) {
//		forex_req_url = StringUtils.trimToNull(inURL);
//		Validate.notNull(forex_req_url, Messages.MISSING_REQ_URL.getText());
//	}

	//public static ZMQFeedCredentials getInstance(String tsx_sub_url, String tsx_req_url, String bdm_sub_url, String bdm_req_url)
	//		throws FeedException {
	//	// getEQTYSubURL(), getEQTYReqURL(), getFUOPSubURL(), getFUOPReqURL()
	//	return new ZMQFeedCredentials(tsx_sub_url, tsx_req_url, bdm_sub_url, bdm_req_url);
	//}

}
