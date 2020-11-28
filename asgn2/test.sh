# YIMING TEST CASE?
echo Hello World > SmallFile1
head -c 80000 /dev/urandom | od -x >LargeFile1
head -c 200 /dev/urandom > SmBinFile1

./httpserver localhost 8001 -N threads &
(curl -T t1 http://localhost:8001/Small12345 -v & \
curl -T t2 http://localhost:8001/Large12345 -v)

./httpserver localhost 8003 -N threads -r &
(curl -T t1 http://localhost:8003/Small12345 -v & \
curl -T b1 http://localhost:8003/binaryfile -v)

./httpserver localhost 8005 -N threads &
(curl http://localhost:8005/SmallFile1 -v > out1 & \
curl http://localhost:8005/LargeFile1 -v > out2)

./httpserver localhost 8004 -N threads -r &
(curl http://localhost:8004/SmallFile1 -v > out3 & \
curl http://localhost:8004/SmBinFile1 -v > out4)

