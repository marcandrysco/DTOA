src/fp.o: src/fp.c
	$(bmake_PRECC)
	$(CC) -fpic $(CFLAGS) -c $< -o $@

src/errol.o: src/errol.c
	$(bmake_PRECC)
	$(CC) -fpic $(CFLAGS) -c $< -o $@

liberrol.a: src/fp.o src/errol.o
	$(AR) $@ $^

liberrol.so.1.0.0: src/fp.o src/errol.o
	$(LD) $^ -lm $(LDFLAGS) -shared -Wl,-soname,liberrol.so.1 -o $@
	ln -fs liberrol.so.1.0.0 liberrol.so.1
	ln -fs liberrol.so.1.0.0 liberrol.so

bmake_all: liberrol.a liberrol.so.1.0.0

bmake_test:

bmake_install: liberrol.a liberrol.so.1.0.0
	install --mode 0644 -D liberrol.a "/usr/local/lib/liberrol.a"
	install --mode 0755 -D liberrol.so.1.0.0 "/usr/local/lib/liberrol.so.1.0.0"
	ln -fs liberrol.so.1.0.0 "/usr/local/lib/liberrol.so"
	ln -fs liberrol.so.1.0.0 "/usr/local/lib/liberrol.so.1"

bmake_clean:
	rm -rf config.status config.log src/fp.o src/errol.o liberrol.a liberrol.so.1.0.0 liberrol.so.1 liberrol.so src/.deps/* src/.deps/*

bmake_dist:
	if [ -e errol-1.0.0 ] ; then rm -rf errol-1.0.0 ; fi ; mkdir errol-1.0.0
	cp --parents configure Makefile.in $(wildcard mktests/* sources user.mk) src/common.h src/fp.h src/fp.c src/errol.h src/errol.c errol-1.0.0
	tar -zcf errol-1.0.0.tar.gz errol-1.0.0/
	rm -rf errol-1.0.0

sinclude src/.deps/* src/.deps/*

.PNOHY: bmake_install bmake_clean
