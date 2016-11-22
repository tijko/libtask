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

    printf("Attempting to write <%d> bytes\n", TEST_DATA_SIZE); 
    int wbytes = write(test_io_file_write_fd, test_input, TEST_DATA_SIZE);
    printf("<%d> written\n", wbytes);
    close(test_io_file_write_fd);
}

void test_read(void)
{
    char test_output[TEST_DATA_SIZE];
    int test_io_file_read_fd = open(TEST_FILE, O_RDONLY);
    printf("Attempting to read <%d> bytes\n", TEST_DATA_SIZE);
    int rbytes = read(test_io_file_read_fd, test_output, TEST_DATA_SIZE);
    printf("<%d> read\n", rbytes);
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

int main(int argc, char *argv[])
{
    pid_t test_pid;
    struct task *taskobj = init_task();
    struct taskstats *taskstats_obj;

    if (argc > 1)
        test_pid = strtol(argv[1], NULL, 0);
    else {
        test_pid = getpid();
        printf("Process write\n");
        test_write();
        printf("Process read\n");
        test_read();
    }

    printf("Test process id: <%d>\n", test_pid);
    taskstats_obj = get_taskstats(taskobj, test_pid);
    assert(taskstats_obj->ac_pid == test_pid);
    print_taskstats(taskstats_obj);

    unlink(TEST_FILE);
    FREE_TASK(taskobj);
}

