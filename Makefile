all: client_udp.out server_udp.out

client_udp.out: client_udp.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDLIBS)

server_udp.out: server_udp.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDLIBS)

client_udp.o: client_udp.c
	$(CC) -c -o $@ $< $(CFLAGS)

server_udp.o: server_udp.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm *.out *.o
