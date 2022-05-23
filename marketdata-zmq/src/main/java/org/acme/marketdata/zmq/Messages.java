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
import org.marketcetera.util.log.I18NLoggerProxy;
import org.marketcetera.util.log.I18NMessage0P;
import org.marketcetera.util.log.I18NMessage1P;
import org.marketcetera.util.log.I18NMessageProvider;

/* $License$ */

/**
 * Messages for ZMQFeed plug-in.
 *
 * @author <a href="mailto:colin@marketcetera.com">Colin DuPlantis</a>
 * @version $Id: Messages.java 16154 2012-07-14 16:34:05Z colin $
 * @since 0.6.0
 */
@ClassVersion("$Id: Messages.java 16154 2012-07-14 16:34:05Z colin $") //$NON-NLS-1$
public interface Messages
{
    static final I18NMessageProvider PROVIDER = new I18NMessageProvider("zmq", Messages.class.getClassLoader());  //$NON-NLS-1$

    static final I18NLoggerProxy LOGGER = new I18NLoggerProxy(PROVIDER);
    
    public static final I18NMessage1P UNKNOWN_EVENT_TYPE = new I18NMessage1P(LOGGER,
                                                                             "unknown_event_type"); //$NON-NLS-1$
    public static final I18NMessage1P UNKNOWN_ENTRY_TYPE = new I18NMessage1P(LOGGER,
                                                                             "unknown_entry_type"); //$NON-NLS-1$
    public static final I18NMessage0P PROVIDER_DESCRIPTION = new I18NMessage0P(LOGGER,
                                                                               "provider_description"); //$NON-NLS-1$
    public static final I18NMessage1P UNSUPPORTED_OPTION_SPECIFICATION = new I18NMessage1P(LOGGER,
                                                                                           "unsupported_option_specification"); //$NON-NLS-1$
	public static final I18NMessage0P MISSING_SUB_URL = new I18NMessage0P(LOGGER,  "missing_sub_url"); //$NON-NLS-1$;
	
	public static final I18NMessage0P MISSING_REQ_URL = new I18NMessage0P(LOGGER,  "missing_req_url"); //$NON-NLS-1$;
}
