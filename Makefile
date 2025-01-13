mdb: mdb.c
	gcc mdb.c -o mdb -lelf -lcapstone

clean:
	rm -f mdb