## Overview
This fuzzer is a feedback fuzzer based on kAFL. It is designed to fuzz binary blobs such as filesystem images with feedback. KCOV is used for feedback generation, KASAN is used for instrumentation, and AFL is used for mutation and feedback evaluation.

## Customizing the binary blob kernel fuzzer
To customize the fuzzer for arbitrary blobs, override the FUZZER_FUNCTION
prototype in the common.h and set the preprocessor define of FUZZ_FUNCITON
to a function that accepts a char buffer pointer and integer length. In 
addition, generate an initial set of binary blobs. The FUZZ_FUNCTION needs
to feed the corrupted binary blobs to the kernel.

A general outline of tasks to perform feedback fuzzing in the kernel are
below. More detail on each stage are below this list.

1. Create a FUZZ_FUNCTION that takes a binary blob (char\*) and length.
    a. The function should feed the binary blob to the kernel.
2. Update common.h to point to your function.
3. Build a kernel and emulate it using Qemu or Virtio or UML.
    a. Enable KASAN
    b. Enable KCOV on the subsystem you are fuzzing.
4. Build AFLplusplus.
5. Generate an initial corpus of binary blobs.
6. Put AFLplusplus, your fuzzer binary, the inital corpus in the emulated
   and instrumented kernel.
7. Run

## Building Linux Kernel to fuzz on desktop
These are general build instructions and should be customized to suit
your fuzzing needs. First, you will need Linux kernel.

    git clone git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git
    cd linux

Then, you will need a .config file. It's your choice which one will
you use. One option is to reuse the config that shipped with your
distribution:

    make defconfig
    make kvmconfig
    ./scripts/config \
        -e EARLY_PRINTK \
        -e 64BIT \
        -e BPF -d EMBEDDED -d EXPERT \
        -e INOTIFY_USER \
        -e ACPI \
        -e BLK_DEV_INITRD \
        -e SYSVIPC \
        -e CC_OPTIMIZE_FOR_PERFORMANCE \
        -d CC_OPTIMIZE_FOR_SIZE

Then, enable KCOV, but disable KCOV_INSTRUMENT_ALL:

    ./scripts/config \
        -e KCOV \
        -d KCOV_INSTRUMENT_ALL \
        -e KCOV_ENABLE_COMPARISONS

Enable KCOV in a subdirectory via:

    find fs -name Makefile \
        | xargs -L1 -I {} bash -c 'echo "KCOV_INSTRUMENT := y" >> {}'

Apply the recommended toggles by Syzkaller, see https://github.com/google/syzkaller/blob/master/docs/linux/kernel_configs.md :

    ./scripts/config \
        -e DEBUG_FS -e DEBUG_INFO \
        -e KALLSYMS -e KALLSYMS_ALL \
        -e NAMESPACES -e UTS_NS -e IPC_NS -e PID_NS -e NET_NS -e USER_NS \
        -e CGROUP_PIDS -e MEMCG -e CONFIGFS_FS -e SECURITYFS \
        -e KASAN -e KASAN_INLINE -e WARNING \
        -e FAULT_INJECTION -e FAULT_INJECTION_DEBUG_FS \
        -e FAILSLAB -e FAIL_PAGE_ALLOC \
        -e FAIL_MAKE_REQUEST -e FAIL_IO_TIMEOUT -e FAIL_FUTEX \
        -e LOCKDEP -e PROVE_LOCKING \
        -e DEBUG_ATOMIC_SLEEP \
        -e PROVE_RCU -e DEBUG_VM \
        -e REFCOUNT_FULL -e FORTIFY_SOURCE \
        -e HARDENED_USERCOPY -e LOCKUP_DETECTOR \
        -e SOFTLOCKUP_DETECTOR -e HARDLOCKUP_DETECTOR \
        -e BOOTPARAM_HARDLOCKUP_PANIC \
        -e DETECT_HUNG_TASK -e WQ_WATCHDOG \
        --set-val DEFAULT_HUNG_TASK_TIMEOUT 140 \
        --set-val RCU_CPU_STALL_TIMEOUT 100 \
        -e UBSAN \
        -d RANDOMIZE_BASE

Then, ensure the virtme options are enabled, as recommended in https://github.com/amluto/virtme :

    ./scripts/config \
        -e VIRTIO -e VIRTIO_PCI -e VIRTIO_MMIO \
        -e NET -e NET_CORE -e NETDEVICES -e NETWORK_FILESYSTEMS \
        -e INET -e NET_9P -e NET_9P_VIRTIO -e 9P_FS \
        -e VIRTIO_NET -e VIRTIO_CONSOLE \
        -e DEVTMPFS -e SCSI_VIRTIO -e BINFMT_SCRIPT -e TMPFS \
        -e UNIX -e TTY -e VT -e UNIX98_PTYS -e WATCHDOG -e WATCHDOG_CORE \
        -e I6300ESB_WDT \
        -e BLOCK -e SCSI_gLOWLEVEL -e SCSI -e SCSI_VIRTIO \
        -e BLK_DEV_SD -e VIRTIO_BALLOON \
        -d CMDLINE_OVERRIDE \
        -d UEVENT_HELPER \
        -d EMBEDDED -d EXPERT \
        -d MODULE_SIG_FORCE

Ensure the .config is coherent:

    make olddefconfig

And build it:

    make KBUILD_BUILD_TIMESTAMP="" bzImage -j4

## Building AFL

To show pretty console AFL requires a tty terminal. We're going to run
it in qemu/kvm environment - the virtio console is fake. In order to
get AFL console we need to patch it:

    git clone https://github.com/vanhauser-thc/AFLplusplus.git
    make afl-fuzz

## Fetching virtme

We also need virtme, run:

    git clone https://github.com/amluto/virtme.git

No installation needed.

## Building fuzz_fs binary

Next, we need to build the shim binary that will live between AFL and
Kernel. Just type make:

    make

Now you should see "./fuzz_fs" program ready for action.

```.txt
$  ./fuzz_fs --help
Usage:

    ./fuzz_fs [options]

Options:

  -v --verbose       Print stuff to stderr
  -r --one-run       Exit after first data read
  -d --dump          Dump KCOV offsets
  -k --no-kcov       Don't attempt to run KCOV
  -n --netns=N       Set up new a namespace every N tests
  -m --dmesg=FILE    Copy /dev/kmsg into a file
```

## Putting it together

To run AFL you need to first prepare input corpus:

    mkdir inp
    echo "hello world" > inp/01.txt

Then we can run:

    stty rows 128 cols 80
    sudo ./virtme/virtme-run \
	--rw --pwd \
	--kimg linux/arch/x86/boot/bzImage \
	--memory 512M \
	--script-sh "echo core > /proc/sys/kernel/core_pattern; ./AFLplusplus/afl-fuzz -i ./inp -o ./out -- ./fuzz_fs --dmesg dmesg.txt --netns=1"

## Additional fuzz_fs toggles

If you want to improve the AFL "stability" score run the tested
`fuzz_fs` binary with the namespace options such as "--netns=1" option.
This will create a dedicated network namespace for each test,
ensuring tests won't have lasting side effects, but will slow down the
fuzzing.

Once you found an interesting code path, you may want to investigate
it. Run within virtme environment:

```
$ ./fuzz_fs --one-run --dump < test_case.bin | head
[-] Running outside of AFL
0xffffffff81a0642d
0xffffffff81a062ac
0xffffffff81a062c4
0xffffffff81a05cd9
0xffffffff81a05cf3
0xffffffff81a05d0f
```

This produces %rip offsets from the covered code. To decode these
offsets into code blocks, run `addr2line`:

```
$ ./fuzz_fs --dump --one-run < test_case.bin \
    | addr2line -e ./linux/vmlinux
[-] Running outside of AFL
linux/net/socket.c:1514
linux/net/socket.c:1500
linux/net/socket.c:1502
linux/net/socket.c:1353
linux/net/socket.c:1355
```

