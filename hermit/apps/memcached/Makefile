.PHONY: module
module: memcached
	echo '/memcached: $${MODULE_DIR}/memcached' > usr.manifest
	ldd memcached | grep libevent | \
		sed 's/ *[^ ] *\(.*\) => \(.*\) .*/\/\1: \2/' \
		>> usr.manifest
	: > bootfs.manifest

src:
	./GET

CC=gcc

sources := assoc.c cache.c daemon.c hash.c items.c memcached.c slabs.c \
	stats.c thread.c util.c jenkins_hash.c murmur3_hash.c \
	test_thread.c zipf/zipf.c zipf/rand.c

sources := $(addprefix src/, $(sources))

$(sources): src

CFLAGS = -O2 -std=gnu99 -fpie -rdynamic -DHAVE_CONFIG_H -D_GNU_SOURCE
LDFLAGS = -pie

CFLAGS += -g -ggdb

# To use a shared object instead of PIE (e.g., we needed to do this because
# of issue #689), use the following instead:
#CFLAGS = -std=gnu99 -fpic -rdynamic -DHAVE_CONFIG_H
#LDFLAGS = -shared

memcached: $(sources)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(sources) -levent -lpthread -lm -DSLEEP_TIME=$(stime)

clean:
	rm -f bootfs.manifest usr.manifest module.pyc memcached
