package org.acme.marketdata.zmq;

import static org.junit.Assert.assertTrue;

import org.acme.marketdata.zmq.Messages;
import org.junit.Test;
import org.marketcetera.util.l10n.MessageComparator;

/* $License$ */

/**
 * Tests the messages for the Bogus Market Data Feed.
 *
 * @author <a href="mailto:colin@marketcetera.com">Colin DuPlantis</a>
 * @version $Id: MessagesTest.java 16154 2012-07-14 16:34:05Z colin $
 * @since 2.1.0
 */
public class MessagesTest
{
    @Test
    public void messagesMatch()
        throws Exception
    {
        MessageComparator comparator = new MessageComparator(Messages.class);
        assertTrue(comparator.getDifferences(),
                   comparator.isMatch());
    }
}
