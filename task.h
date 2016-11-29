#ifndef LIBTASK
#define LIBTASK


#include <sys/uio.h>
#include <sys/socket.h>
#include <linux/genetlink.h>
#include <linux/taskstats.h>

#define MAX_NLMSG 1024

#define NLMSG_LEN NLMSG_ALIGN(GENL_HDRLEN + NLMSG_HDRLEN)
#define GENLMSG_DATA(gnlhdr) (struct nlattr *) ((char *) gnlhdr + GENL_HDRLEN)

#define NLA_DATA(msg) (void *) ((char *) msg + NLA_HDRLEN)
#define NLA_LENGTH(len) NLA_HDRLEN + len


struct nlmsg {
    struct nlmsghdr nlhdr;
    struct genlmsghdr gnlhdr;
    char nla_buffer[MAX_NLMSG];
};

#define NL_MSG_SIZE sizeof(struct nlmsg)
#define NL_ADDR_SIZE sizeof(struct sockaddr_nl)

struct task {
    struct {
        struct sockaddr_nl nladdr;
        int conn_fd;
        int gnl_family_id;
    } nlconn_obj;
    struct msghdr msg;
    struct iovec vec;
    struct nlmsg nlmsg_obj;
};

struct task *init_task(void);

struct taskstats *get_taskstats(struct task *taskobj, pid_t pid);

#define FREE_TASK(t)     \
    if (t) free(t)       \

#define TASK_ERROR(func) \
    do {                 \
        perror(func);    \
        exit(1);         \
    } while (1)          \

                         
#endif
