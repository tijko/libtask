#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "task.h"


#define TEST_DATA_SIZE 4096
#define TEST_FILE "test_taskio.txt"


void test_write(void)
{
    char test_input[TEST_DATA_SIZE];
    memset(test_input, 'x', TEST_DATA_SIZE);

    int test_io_file_write_fd = open(TEST_FILE, O_WRONLY | O_CREAT);

    int wbytes = write(test_io_file_write_fd, test_input, TEST_DATA_SIZE);
    if (wbytes < 0) {
        perror("test-write");
        exit(1);
    }

    close(test_io_file_write_fd);
}

void test_read(void)
{
    char test_output[TEST_DATA_SIZE];
    int test_io_file_read_fd = open(TEST_FILE, O_RDONLY);

    int rbytes = read(test_io_file_read_fd, test_output, TEST_DATA_SIZE);
    if (rbytes < 0) {
        perror("test-read");
        exit(1);
    }

    close(test_io_file_read_fd);
}

void print_taskstats(struct taskstats *taskstats_obj)
{
    printf("Read-chars <%lld> Write-chars <%lld>\n", taskstats_obj->read_char,
                                                     taskstats_obj->write_char);

    printf("Read-syscalls <%lld> Write-syscalls <%lld>\n", 
                                                 taskstats_obj->read_syscalls, 
                                                 taskstats_obj->write_syscalls);

    printf("Read-bytes <%lld> Write-bytes <%lld>\n",
                                                    taskstats_obj->read_bytes,
                                                  taskstats_obj->write_bytes);
}

char *get_procfs_io_stats(pid_t pid)
{
    char procfs_io_path[256];
    snprintf(procfs_io_path, 255, "/proc/%d/io", pid);

    int open_fd = open(procfs_io_path, O_RDONLY);
    if (open_fd < 0) {
        perror("proc-open");
        exit(1);
    }

    char *io_buf = malloc(1024);

    int rbytes = read(open_fd, io_buf, 1023);
    if (rbytes < 0) {
        perror("proc-read");
        exit(1);
    }

    io_buf[rbytes] = '\0';

    close(open_fd);

    return io_buf;
}

int main(int argc, char *argv[])
{
    pid_t test_pid;
    struct task *taskobj = init_task();
    struct taskstats *taskstats_obj;

    if (argc > 1)
        test_pid = strtol(argv[1], NULL, 0);
    else {
        test_pid = getpid();
        test_write();
        test_read();
    }

    char *io = get_procfs_io_stats(test_pid);
    taskstats_obj = get_taskstats(taskobj, test_pid);

    assert(taskstats_obj->ac_pid == test_pid);

    printf("Test process id: <%d>\n", test_pid);
    print_taskstats(taskstats_obj);

    /*
     * The procfs file /proc/$PID/io has matching fields for i/o to some of the 
     * fields for input output accounting in taskstats.  There can appear to be
     * some discrepancies at first glance but they explained by a couple of
     * factors:
     *
     * The "syscr" and "syscw" in taskstats are both rounded off by & ~(1024 - 1)
     * 
     * The "rchar" and "wchar" in /proc/$PID/io are both affected by *any* 
     * read/write that the process caused.  Which includes any terminal i/o and 
     * for reading any shared objects (whether the calls actually reach the disk 
     * or not). Running strace on the test you will see ~832 bytes read off of 
     * libtask.so, this matches up with the difference between procfs-io and 
     * taskstats.
     *
     * The "read_bytes" and "write_bytes" are the fields in procfs-io which 
     * reflect the *actual* number of bytes the process did cause
     */

    printf("%s", io);
    free(io);

    unlink(TEST_FILE);
    FREE_TASK(taskobj);
}

