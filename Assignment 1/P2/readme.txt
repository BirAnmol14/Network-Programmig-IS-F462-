Compile using 
    make

Execute 
    server using    
        ./clustershell_server
    client using 
        ./clustershell_client

shell executable is used by clustershell_client, no need to run separately

Multiple clients can be executed on the same machine using separate terminals, if config file 
has sufficient duplicate IP addresses to accomodate them.

Config file format: (only used by clustershell_server)
<IP address for n1>
<IP address for n2>
...
<IP address for nN>


