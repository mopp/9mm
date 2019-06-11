9mm: 9mm.c

test: 9mm
	./9mm -test
	./test.sh

clean:
	rm -f 9cc *.o *~ tmp*
