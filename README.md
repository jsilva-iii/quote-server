# quote-server
A high performance, low latency quote server for Toronto & Montreal Exchangess quote server 
publishing quotes to Marketcetera trading platform. This was written in Apr 2012. 
The server processes multicast traffic, as broadcast by the Exchanges and using 
Google Protocol Buffers publishes to Marketcetera Java clients. 

Technolologies used in this server are:
  ZeroMq - https://zeromq.org/
  libev - http://software.schmorp.de/pkg/libev.html
  tommyDS - http://tommyds.sourceforge.net
  Google Protocol Buffers - https://developers.google.com/protocol-buffers
  Marketcetera - https://www.marketcetera.com/
  Lime Financial - https://lime.co/
  
I haven't maintained the code since it was used in production from 2012-2014. For support
on any of the above technology, please see above links and contact the authors directly.
Although this code is shared as is, all original ownership and credit remains with the 
original owners - feel free to contact me, if any work I have included, has not been correctly 
attributed to you. Any addtions/derivations are my own own work.

The Market Data Adapter for Marketcetera can handle Futures, Options, Equities, Forex and Lime
Market Data, from Lime Financial

