mdb: mdb.c
	gcc mdb.c -o mdb -lelf -lcapstone
	gcc -fno-pie -no-pie -o binaries/debug-me binaries/debug-me.c

clean:
	rm -f mdb
	rm -f binaries/debug-me