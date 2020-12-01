# Multithreading testing with and without redundancy

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


####################################################################
# General error testing
####################################################################

# GET REQUESTS
curl http://localhost:8080/aaaaaaaaaa -v                    # valid 
curl http://localhost:8080/zzzzzzzzzz -v                    # nonexistent file
curl http://localhost:8080/aaaaaaaaa! -v                    # invalid name: non alphanumeric
curl http://localhost:8080/aaaaaaaaa -v                     # invalid name: <10 ASCII
curl http://localhost:8080/aaaaaaaaaaa -v                   # invalid name: >10 ASCII

# #PUT REQUESTS
curl -T cmd1.output http://localhost:8080/aaaaaaaaaa -v     # valid
curl -T cmd1.output http://localhost:8080/aaaaaaaaa! -v     # invalid name: non alphanumeric
curl -T cmd1.output http://localhost:8080/aaaaaaaaa -v      # invalid name: <10 ASCII
curl -T cmd1.output http://localhost:8080/aaaaaaaaaaa -v    # invalid name: >10 ASCII



