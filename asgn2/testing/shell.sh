# YIMING TESTS BELOW
# echo Hello World>t1
# head -c 80000 /dev/urandom | od -x > t2
# head -c 200 /dev/urandom > b1


curl -T cmd1.output http://localhost:8080/aaaaaaaaaa -v &
curl -T cmd2.output http://localhost:8080/aaaaaaaaaa -v &
curl -T cmd3.output http://localhost:8080/aaaaaaaaaa -v &
curl http://localhost:8080/aaaaaaaaaa -v &
curl http://localhost:8080/aaaaaaaaaa -v & 
curl http://localhost:8080/bbbbbbbbbb -v &
curl http://localhost:8080/cccccccccc -v &  
curl http://localhost:8080/aaaaaaaaaa -v &
curl -T cmd4.output http://localhost:8080/aaaaaaaaaa -v &
curl -T cmd2.output http://localhost:8080/bbbbbbbbbb -v &
curl -T cmd3.output http://localhost:8080/cccccccccc -v &
curl -T cmd3.output http://localhost:8080/cccccccccc -v &
curl http://localhost:8080/aaaaaaaaaa -v &
curl http://localhost:8080/bbbbbbbbbb -v &
curl http://localhost:8080/cccccccccc -v &
curl http://localhost:8080/dddddddddd -v & 


dd if=/dev/urandom of=d24 count=16 bs=1024 status=none
printf """PUT /13atestNoL HTTP/1.1\r\n\r\n""" \
| cat - d24 > nc-test24.cmds
ncat -d 2 localhost 8080 < nc-test24.cmds

# GET REQUESTS
# curl http://localhost:8080/aaaaaaaaaa -v                    # valid 
# curl http://localhost:8080/zzzzzzzzzz -v                    # nonexistent file
# curl http://localhost:8080/aaaaaaaaa! -v                    # invalid name: non alphanumeric
# curl http://localhost:8080/aaaaaaaaa -v                     # invalid name: <10 ASCII
# curl http://localhost:8080/aaaaaaaaaaa -v                   # invalid name: >10 ASCII

# #PUT REQUESTS
# curl -T cmd1.output http://localhost:8080/aaaaaaaaaa -v     # valid
# curl -T cmd1.output http://localhost:8080/aaaaaaaaa! -v     # invalid name: non alphanumeric
# curl -T cmd1.output http://localhost:8080/aaaaaaaaa -v      # invalid name: <10 ASCII
# curl -T cmd1.output http://localhost:8080/aaaaaaaaaaa -v    # invalid name: >10 ASCII



