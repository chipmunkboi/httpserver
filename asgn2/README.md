<p>Michelle Yeung (myyeung)<br>
Jeffrey Zhang (jzhan182)<br>
asgn2<br>
README.md</p>

**Files submitted:**
1. httpserver.cpp
2. DESIGN.pdf
3. Makefile
4. README.md

**To run:**
1. "make"
2. "httpserver [hostname/IP address] [port number] [-N] [# of worker threads] [-r]"
    - port number argument is optional; default is 80
    - "-N" & "# of worker threads" is optional; default number is 4
    - "-r" is optional; flag toggles redundancy on/off

**Known limitations/issues:**<br>
A limitation with the code is that if the request header contains
"Content-Length: x", the code may not be able to extract it 100% of the 
time. This is because of the way we recv and parse the buffer. If the max
buffer size happens to be reached with only part of content-length present 
(e.g. "Content-Length: 10" when the request specifies "Content-Length: 100"),
it will not be detected and the file will will not receive and write all the
bytes that it should.

Another issue is if "Content-Length" in the header is overridden to a number
that is greater than the actual content length of the file, PUT will infinitely
loop. This is due to the way we recv and write the buffer. Everytime we write 
bytes, we subtract that from the header-specified content-length. If the content-
length is too large, we will never recv enough bytes to exit the loop.
