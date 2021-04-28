Compile using
    make

Start server using
    ./prefork_server <minSpareServers> <maxSpareServers> <maxRequestsPerChild>
    
Terminate server using 
    [Ctrl+\]

Test using 
    httperf --port=31415 --num-conns=50 --rate=5

-------------------------------------------------------------------------------
Sample run:

$ ./prefork_server 17 19 3
minSpareServers: 17
maxSpareServers: 19
maxRequestsPerChild: 3
Listening on port 31415...
Creating server process pool...
[P]: (spawn) 31 processes, 0 clients, 31 idle
[P]: (kill) 19 processes, 0 clients, 19 idle
[4960]: Accepted Connection (127.0.0.1:56314)
[4961]: Accepted Connection (127.0.0.1:56316)
[4962]: Accepted Connection (127.0.0.1:56318)
[4963]: Accepted Connection (127.0.0.1:56320)
[4964]: Accepted Connection (127.0.0.1:56322)
[4965]: Accepted Connection (127.0.0.1:56324)
[4960]: Accepted Connection (127.0.0.1:56326)
[4961]: Accepted Connection (127.0.0.1:56328)
[P]: (spawn) 20 processes, 3 clients, 17 idle
[4962]: Accepted Connection (127.0.0.1:56330)
[4963]: Accepted Connection (127.0.0.1:56332)
[4964]: Accepted Connection (127.0.0.1:56334)
[4965]: Accepted Connection (127.0.0.1:56336)
[4960]: Accepted Connection (127.0.0.1:56338)
[P]: (spawn) 21 processes, 4 clients, 17 idle
[P]: (kill) 20 processes, 1 clients, 19 idle
[4961]: Accepted Connection (127.0.0.1:56340)
[4962]: Accepted Connection (127.0.0.1:56342)
[4963]: Accepted Connection (127.0.0.1:56344)
[4964]: Accepted Connection (127.0.0.1:56346)
[4965]: Accepted Connection (127.0.0.1:56348)
[P]: (spawn) 23 processes, 6 clients, 17 idle
[P]: (kill) 20 processes, 1 clients, 19 idle
[4966]: Accepted Connection (127.0.0.1:56350)
[4968]: Accepted Connection (127.0.0.1:56352)
[4969]: Accepted Connection (127.0.0.1:56354)
[4970]: Accepted Connection (127.0.0.1:56356)
[4971]: Accepted Connection (127.0.0.1:56358)
[P]: (spawn) 21 processes, 4 clients, 17 idle
[P]: (recycle) 21 processes, 5 clients, 16 idle
[P]: (recycle) 21 processes, 4 clients, 17 idle
[P]: (recycle) 21 processes, 3 clients, 18 idle
[P]: (recycle) 21 processes, 2 clients, 19 idle
[4966]: Accepted Connection (127.0.0.1:56360)
[4968]: Accepted Connection (127.0.0.1:56362)
[4969]: Accepted Connection (127.0.0.1:56364)
[4970]: Accepted Connection (127.0.0.1:56366)
[4972]: Accepted Connection (127.0.0.1:56368)
[4971]: Accepted Connection (127.0.0.1:56370)
[4966]: Accepted Connection (127.0.0.1:56372)
[4968]: Accepted Connection (127.0.0.1:56374)
[4969]: Accepted Connection (127.0.0.1:56376)
[4970]: Accepted Connection (127.0.0.1:56378)
[P]: (spawn) 24 processes, 6 clients, 18 idle
[P]: (recycle) 24 processes, 6 clients, 18 idle
[P]: (recycle) 24 processes, 5 clients, 19 idle
[P]: (kill) 22 processes, 3 clients, 19 idle
[P]: (kill) 21 processes, 2 clients, 19 idle
[P]: (spawn) 22 processes, 5 clients, 17 idle
[P]: (kill) 20 processes, 1 clients, 19 idle
[4972]: Accepted Connection (127.0.0.1:56382)
[4971]: Accepted Connection (127.0.0.1:56384)
[4973]: Accepted Connection (127.0.0.1:56386)
[4974]: Accepted Connection (127.0.0.1:56388)
[4975]: Accepted Connection (127.0.0.1:56390)
[P]: (spawn) 23 processes, 5 clients, 18 idle
[P]: (recycle) 23 processes, 4 clients, 19 idle
[P]: (recycle) 23 processes, 3 clients, 20 idle
[P]: (recycle) 23 processes, 2 clients, 21 idle
[P]: (recycle) 23 processes, 1 clients, 22 idle
[P]: (kill) 21 processes, 2 clients, 19 idle
[P]: (spawn) 22 processes, 5 clients, 17 idle
[4976]: Accepted Connection (127.0.0.1:56392)
[4972]: Accepted Connection (127.0.0.1:56394)
[4977]: Accepted Connection (127.0.0.1:56396)
[4973]: Accepted Connection (127.0.0.1:56398)
[4974]: Accepted Connection (127.0.0.1:56400)
[4975]: Accepted Connection (127.0.0.1:56402)
[P]: (spawn) 23 processes, 6 clients, 17 idle
[P]: (recycle) 23 processes, 3 clients, 20 idle
[P]: (kill) 22 processes, 3 clients, 19 idle
[4976]: Accepted Connection (127.0.0.1:56406)
[4978]: Accepted Connection (127.0.0.1:56408)
[4977]: Accepted Connection (127.0.0.1:56412)
[4973]: Accepted Connection (127.0.0.1:56414)
[4974]: Accepted Connection (127.0.0.1:56416)
[4975]: Accepted Connection (127.0.0.1:56418)
[P]: (spawn) 23 processes, 6 clients, 17 idle
[P]: (recycle) 23 processes, 4 clients, 19 idle
[P]: (kill) 22 processes, 3 clients, 19 idle
[P]: (recycle) 22 processes, 2 clients, 20 idle
[P]: (kill) 21 processes, 2 clients, 19 idle
[P]: (recycle) 21 processes, 1 clients, 20 idle
[P]: (kill) 20 processes, 1 clients, 19 idle
[P]: (recycle) 20 processes, 0 clients, 20 idle
[P]: (kill) 19 processes, 0 clients, 19 idle
^C[P]: 19 processes, 0 clients
[4976]: (Idle) handled 2 clients
[4977]: (Idle) handled 2 clients
[4978]: (Idle) handled 1 clients
[4979]: (Idle) handled 0 clients
[5008]: (Idle) handled 0 clients
[5014]: (Idle) handled 0 clients
[5015]: (Idle) handled 0 clients
[5016]: (Idle) handled 0 clients
[5017]: (Idle) handled 0 clients
[5018]: (Idle) handled 0 clients
[5019]: (Idle) handled 0 clients
[5025]: (Idle) handled 0 clients
[5026]: (Idle) handled 0 clients
[5027]: (Idle) handled 0 clients
[5029]: (Idle) handled 0 clients
[5030]: (Idle) handled 0 clients
[5033]: (Idle) handled 0 clients
[5034]: (Idle) handled 0 clients
[5036]: (Idle) handled 0 clients

-------------------------------------------------------------------------------
httperf Report:

$ httperf --port=31415 --num-conns=50 --rate=5

Total: connections 50 requests 50 replies 50 test-duration 10.801 s

Connection rate: 4.6 conn/s (216.0 ms/conn, <=6 concurrent connections)
Connection time [ms]: min 1000.2 avg 1000.2 max 1000.5 median 1000.5 stddev 0.0
Connection time [ms]: connect 0.0
Connection length [replies/conn]: 1.000

Request rate: 4.6 req/s (216.0 ms/req)
Request size [B]: 62.0

Reply rate [replies/s]: min 4.0 avg 4.5 max 5.0 stddev 0.7 (2 samples)
Reply time [ms]: response 1000.2 transfer 0.0
Reply size [B]: header 19.0 content 0.0 footer 0.0 (total 19.0)
Reply status: 1xx=0 2xx=50 3xx=0 4xx=0 5xx=0

CPU time [s]: user 3.75 system 7.04 (user 34.7% system 65.2% total 99.9%)
Net I/O: 0.4 KB/s (0.0*10^6 bps)

Errors: total 0 client-timo 0 socket-timo 0 connrefused 0 connreset 0
Errors: fd-unavail 0 addrunavail 0 ftab-full 0 other 0
