# quote-server
A high performance, low latency quote server for Toronto & Montreal Exchangess quote server 
publishing quotes to Marketcetera trading platform. This was written in Apr 2012. 
The server processes multicast traffic, as broadcast by the Exchanges and using 
Google Protocol Buffers publishes to Marketcetera Java clients. 

Technolologies used in this server are:
  ZeroMq - https://zeromq.org/
  lebev - http://software.schmorp.de/pkg/libev.html
  tommyDS - http://tommyds.sourceforge.net
  Google Protocol Buffers - https://developers.google.com/protocol-buffers
  Marketcetera - https://www.marketcetera.com/
  
I haven't maintained the code since it was used in production from 2012-2014. For support, 
please see above links and contact the individuals directly. Although this code is shared
as is, all original ownership and credit remains with the original owners (feel free to 
contact me, if any work is not correctly attributed to you), namely the above any addtions/derivations
are my own own work.
