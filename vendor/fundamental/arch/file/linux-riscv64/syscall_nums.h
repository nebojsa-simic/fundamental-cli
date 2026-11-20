#ifndef FUNDAMENTAL_FILE_SYSCALL_NUMS_LINUX_RISCV64_H
#define FUNDAMENTAL_FILE_SYSCALL_NUMS_LINUX_RISCV64_H

/*
 * Fundamental Library - Linux RISC-V 64 Syscall Numbers
 * 
 * Zero-libc syscall number definitions for RISC-V 64 Linux.
 * Maintained independently from libc to preserve zero-dependency policy.
 * 
 * Source: Linux kernel arch/riscv/include/uapi/asm/unistd.h
 * Note: RISC-V uses the unified syscall table (similar to ARM64)
 */

/* ============================================================================
 * File I/O Syscalls
 * ============================================================================ */
#define SYS_read 63
#define SYS_write 64
#define SYS_open 56
#define SYS_close 57
#define SYS_fstat 80
#define SYS_lseek 62
#define SYS_mmap 222
#define SYS_mprotect 226
#define SYS_munmap 215
#define SYS_fcntl 25
#define SYS_flock 26
#define SYS_fsync 82
#define SYS_fdatasync 83
#define SYS_ftruncate 46
#define SYS_getdents 61
#define SYS_getdents64 61
#define SYS_fadvise 223

/* ============================================================================
 * File Opening Flags (fcntl.h)
 * ============================================================================ */
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0100
#define O_TRUNC 01000
#define O_APPEND 02000
#define O_NONBLOCK 04000
#define O_DSYNC 010000
#define O_DIRECT 040000
#define O_DIRECTORY 0400000
#define O_NOFOLLOW 0400000

/* ============================================================================
 * Memory Protection Flags (sys/mman.h)
 * ============================================================================ */
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define PROT_EXEC 0x4
#define PROT_NONE 0x0

/* ============================================================================
 * Memory Mapping Flags (sys/mman.h)
 * ============================================================================ */
#define MAP_SHARED 0x1
#define MAP_PRIVATE 0x2
#define MAP_FIXED 0x10
#define MAP_ANONYMOUS 0x20
#define MAP_POPULATE 0x80000

/* ============================================================================
 * File Locking Flags (sys/file.h)
 * ============================================================================ */
#define LOCK_SH 1 /* Shared lock */
#define LOCK_EX 2 /* Exclusive lock */
#define LOCK_NB 4 /* Non-blocking */
#define LOCK_UN 8 /* Unlock */

/* ============================================================================
 * Sync Flags (sys/mman.h, unistd.h)
 * ============================================================================ */
#define MS_ASYNC 1 /* Sync memory asynchronously */
#define MS_INVALIDATE 2 /* Invalidate the caches */
#define MS_SYNC 4 /* Synchronous sync */

/* ============================================================================
 * Page Size
 * ============================================================================ */
/* RISC-V supports multiple page sizes: 4KB, 64KB, 2MB, 1GB */
/* This is the minimum; runtime detection recommended */
#define PAGE_SIZE 4096 /* Minimum page size */
#define PAGE_MASK (~(PAGE_SIZE - 1))
#define PAGE_ALIGN(addr) (((addr) + PAGE_SIZE - 1) & PAGE_MASK)

/* ============================================================================
 * io_uring Syscalls
 * ============================================================================ */
#define SYS_io_uring_setup 425
#define SYS_io_uring_enter 426
#define SYS_io_uring_register 427

/* ============================================================================
 * io_uring Operations
 * ============================================================================ */
#define IORING_OP_NOP 0
#define IORING_OP_READV 1
#define IORING_OP_WRITEV 2
#define IORING_OP_FSYNC 3
#define IORING_OP_READ_FIXED 4
#define IORING_OP_WRITE_FIXED 5
#define IORING_OP_POLL_ADD 6
#define IORING_OP_POLL_REMOVE 7
#define IORING_OP_SYNC_FILE_RANGE 8
#define IORING_OP_SENDMSG 9
#define IORING_OP_RECVMSG 10
#define IORING_OP_TIMEOUT 11
#define IORING_OP_TIMEOUT_REMOVE 12
#define IORING_OP_ACCEPT 13
#define IORING_OP_ASYNC_CANCEL 14
#define IORING_OP_LINK_TIMEOUT 15
#define IORING_OP_CONNECT 16
#define IORING_OP_FALLOCATE 17
#define IORING_OP_OPENAT 18
#define IORING_OP_CLOSE 19
#define IORING_OP_FILES_UPDATE 20
#define IORING_OP_STATX 21
#define IORING_OP_READ 22
#define IORING_OP_WRITE 23
#define IORING_OP_FADVISE 24
#define IORING_OP_FADVISE64 24
#define IORING_OP_MADVISE 25
#define IORING_OP_SEND 26
#define IORING_OP_RECV 27
#define IORING_OP_OPENAT2 28
#define IORING_OP_EPOLL_CTL 29
#define IORING_OP_SPLICE 30
#define IORING_OP_PROVIDE_BUFFERS 31
#define IORING_OP_REMOVE_BUFFERS 32
#define IORING_OP_TEE 33
#define IORING_OP_SHUTDOWN 34
#define IORING_OP_RENAMEAT 35
#define IORING_OP_UNLINKAT 36
#define IORING_OP_MKDIRAT 37
#define IORING_OP_SYMLINKAT 38
#define IORING_OP_LINKAT 39
#define IORING_OP_MSG_RING 40
#define IORING_OP_FSETXATTR 41
#define IORING_OP_SETXATTR 42
#define IORING_OP_FGETXATTR 43
#define IORING_OP_GETXATTR 44
#define IORING_OP_SOCKET 45
#define IORING_OP_URING_CMD 46

/* ============================================================================
 * io_uring Enter Flags
 * ============================================================================ */
#define IORING_ENTER_GETEVENTS 0x01
#define IORING_ENTER_SQ_WAKEUP 0x02
#define IORING_ENTER_SQ_WAIT 0x04
#define IORING_ENTER_EXT_ARG 0x08

/* ============================================================================
 * io_uring SQE Flags
 * ============================================================================ */
#define IOSQE_FIXED_FILE (1U << 0)
#define IOSQE_IO_DRAIN (1U << 1)
#define IOSQE_IO_LINK (1U << 2)
#define IOSQE_IO_HARDLINK (1U << 3)
#define IOSQE_ASYNC (1U << 4)
#define IOSQE_BUFFER_SELECT (1U << 5)
#define IOSQE_CQE_SKIP_SUCCESS (1U << 6)

/* ============================================================================
 * io_uring CQE Flags
 * ============================================================================ */
#define IORING_CQE_F_BUFFER (1U << 0)
#define IORING_CQE_F_MORE (1U << 1)
#define IORING_CQE_F_SOCK_NONZERO (1U << 2)
#define IORING_CQE_F_NOTIF (1U << 3)

/* ============================================================================
 * io_uring Fsync Flags
 * ============================================================================ */
#define IORING_FSYNC_DATASYNC (1U << 0)

/* ============================================================================
 * Poll Events
 * ============================================================================ */
#define SYS_poll 209
#define POLLIN 0x0001
#define POLLOUT 0x0004
#define POLLERR 0x0008
#define POLLHUP 0x0010
#define POLLNVAL 0x0020

/* ============================================================================
 * Inotify Syscalls (for file notifications)
 * ============================================================================ */
#define SYS_inotify_init 269
#define SYS_inotify_add_watch 268
#define SYS_inotify_rm_watch 270

/* ============================================================================
 * Inotify Events
 * ============================================================================ */
#define IN_ACCESS 0x00000001
#define IN_MODIFY 0x00000002
#define IN_ATTRIB 0x00000004
#define IN_CLOSE_WRITE 0x00000008
#define IN_CLOSE_NOWRITE 0x00000010
#define IN_CLOSE (IN_CLOSE_WRITE | IN_CLOSE_NOWRITE)
#define IN_OPEN 0x00000020
#define IN_MOVED_FROM 0x00000040
#define IN_MOVED_TO 0x00000080
#define IN_MOVE (IN_MOVED_FROM | IN_MOVED_TO)
#define IN_CREATE 0x00000100
#define IN_DELETE 0x00000200
#define IN_DELETE_SELF 0x00000400
#define IN_MOVE_SELF 0x00000800
#define IN_ALL_EVENTS 0x00000fff
#define IN_UNMOUNT 0x00002000
#define IN_Q_OVERFLOW 0x00004000
#define IN_IGNORED 0x00008000

#endif /* FUNDAMENTAL_FILE_SYSCALL_NUMS_LINUX_RISCV64_H */
