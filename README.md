## About
This repository contains the HTTP Server project written in C but submitted as CPP files named httpserver.cpp. Design.pdf is design document of each assignment. Each .docx file describes the scope of the assignment. Each assignment builds off the previous one.

## Assignment Summary
1. **asgn1** is a single threaded HTTP server that responds to GET/PUT requests to read and write files in the directory of httpserver.cpp.
2.  **asgn2** adds redudancy and multithreading to the HTTP server. Redundancy is implemented using 3 live copies and multithreading uses mutex locks and semaphores.
3. **asgn3** adds backup and recovery of files used in the HTTP server. Uses UNIX timestamp to chronologically sort backups. 
