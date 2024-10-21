CC := gcc
CFLAGS := -g -ggdb -Wall -Wextra -O2 -static -DDEBUG -Isrc

SRCS := utils.c kcov.c forksrv.c siphash.c namespace.c rle.c fuzz_main.c fs/fs_fuzz.c
SRCS := $(addprefix src/, $(SRCS))

.PHONY: all
all: fuzz_fs gen_fs

fuzz_fs: $(SRCS)
	$(CC) $(CFLAGS) $^ -o $@

gen_fs: src/fs/fs_gen.c
	$(CC) $(CFLAGS) $< -o $@

.PHONY: format
format:
	clang-format -i src/*.c src/fs/*.c src/*.h

.PHONY: clean
clean:
	rm -f fuzz_fs gen_fs

