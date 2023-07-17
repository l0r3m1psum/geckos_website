CC = cc
CFLAGS = -I/usr/local/include -static -Wall -Wextra -Wpedantic -Wshadow -std=c11 -march=native -DNDEBUG
LDFLAGS = -L/usr/local/lib/ -lsqlite3 -pthread -lm

SITE_DIR = /var/www/htdocs/website/geckos

all: gecko catalog

deploy:
	rsync -r * root@l0r3m1p5um.com:/root/geckos

install: gecko catalog
	# TODO: maybe add -s flag to strip
	install -g www -o www -m 0500 gecko catalog $(SITE_DIR)

.c:
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

clean:
	rm -rf catalog gecko catalog.dSYM gecko.dSYM a.out

SANITIZE = -fsanitize=address,undefined,signed-integer-overflow,null,alignment -fno-omit-frame-pointer #-fsanitize=memory

macos:
	cc $(SANITIZE) -g -lsqlite3 gecko.c -o gecko
	cc $(SANITIZE) -g -lsqlite3 catalog.c -o catalog

# ASAN_OPTIONS=detect_leaks=1 QUERY_STRING=id=7 gecko
# ASAN_OPTIONS=detect_leaks=1 catalog
