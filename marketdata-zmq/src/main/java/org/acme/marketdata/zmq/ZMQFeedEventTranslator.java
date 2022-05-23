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

import java.util.Arrays;
import java.util.List;

import org.apache.commons.lang.ObjectUtils;
import org.marketcetera.core.ClassVersion;
import org.marketcetera.core.CoreException;
import org.marketcetera.event.Event;
import org.marketcetera.event.EventTranslator;
import org.marketcetera.event.UnsupportedEventException;
import org.marketcetera.util.log.I18NBoundMessage1P;

/* $License$ */

/**
 * Bogus feed {@link EventTranslator} instance.
 *
 * @author <a href="mailto:colin@marketcetera.com">Colin DuPlantis</a>
 * @version $Id: ZMQFeedEventTranslator.java 16154 2012-07-14 16:34:05Z colin $
 * @since 0.5.0
 */
@ClassVersion("$Id: ZMQFeedEventTranslator.java 16154 2012-07-14 16:34:05Z colin $")
public class ZMQFeedEventTranslator
        implements EventTranslator, Messages
{
    /* (non-Javadoc)
     * @see org.marketcetera.event.IEventTranslator#translate(java.lang.Object)
     */
    public List<Event> toEvent(Object inData,
                                   String inHandle)
            throws CoreException
    {
        if(!(inData instanceof Event)) {
            throw new UnsupportedEventException(new I18NBoundMessage1P(UNKNOWN_EVENT_TYPE,
                                                                       ObjectUtils.toString(inData,
                                                                                            null)));
        }
        Event event = (Event)inData;
        return Arrays.asList(new Event[] { event } );
    }
    /* (non-Javadoc)
     * @see org.marketcetera.event.IEventTranslator#translate(org.marketcetera.event.EventBase)
     */
    public Object fromEvent(Event inEvent)
            throws CoreException
    {
        throw new UnsupportedOperationException();
    }
    /**
     * static instance of <code>ZMQFeedEventTranslator</code>
     */
    private static final ZMQFeedEventTranslator sInstance = new ZMQFeedEventTranslator();
    /**
     * Gets a <code>ZMQFeedEventTranslator</code> instance.
     * 
     * @return a <code>ZMQFeedEventTranslator</code> instance
     */
    static ZMQFeedEventTranslator getInstance()
    {
        return sInstance;
    }
}
