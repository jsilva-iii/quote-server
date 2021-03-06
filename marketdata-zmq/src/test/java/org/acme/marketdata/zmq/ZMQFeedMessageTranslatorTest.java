package org.acme.marketdata.zmq;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

import java.util.Collections;
import java.util.EnumSet;
import java.util.Set;

import org.acme.marketdata.zmq.ZMQFeedMessageTranslator;
import org.marketcetera.marketdata.Content;
import org.marketcetera.marketdata.DataRequestTranslator;
import org.marketcetera.marketdata.MarketDataMessageTranslatorTestBase;
import org.marketcetera.marketdata.MarketDataRequest;

/* $License$ */

/**
 * Tests {@link ZMQFeedMessageTranslator}.
 *
 * @author <a href="mailto:colin@marketcetera.com">Colin DuPlantis</a>
 * @version $Id: ZMQFeedMessageTranslatorTest.java 16154 2012-07-14 16:34:05Z colin $
 * @since 1.5.0
 */
public class ZMQFeedMessageTranslatorTest
    extends MarketDataMessageTranslatorTestBase<MarketDataRequest>
{
    /* (non-Javadoc)
     * @see org.marketcetera.marketdata.MarketDataMessageTranslatorTestBase#getCapabilities()
     */
    @Override
    protected Set<Content> getCapabilities()
    {
        return Collections.unmodifiableSet(EnumSet.complementOf(EnumSet.of(Content.BBO10)));
    }
    /* (non-Javadoc)
     * @see org.marketcetera.marketdata.MarketDataMessageTranslatorTestBase#getTranslator()
     */
    @Override
    protected DataRequestTranslator<MarketDataRequest> getTranslator()
    {
        return ZMQFeedMessageTranslator.getInstance();
    }
    /* (non-Javadoc)
     * @see org.marketcetera.marketdata.MarketDataMessageTranslatorTestBase#verifyResponse(java.lang.Object, java.lang.String, org.marketcetera.marketdata.MarketDataRequest.Content, org.marketcetera.marketdata.MarketDataRequest.Type, java.lang.String[])
     */
    @Override
    protected void verifyResponse(MarketDataRequest inActualResponse,
                                  String inExpectedExchange,
                                  Content[] inExpectedContent,
                                  String[] inExpectedSymbols)
            throws Exception
    {
        assertEquals(inExpectedExchange == null || inExpectedExchange.isEmpty() ? null : inExpectedExchange,
                inActualResponse.getExchange());
        assertArrayEquals(inExpectedContent,
                          inActualResponse.getContent().toArray(new Content[inActualResponse.getContent().size()]));
        assertArrayEquals(inExpectedSymbols,
                          inActualResponse.getSymbols().toArray(new String[0]));
    }
}
