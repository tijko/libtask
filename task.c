#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "task.h"


inline static void *parse_task_req_response(char *taskmsg, int request, 
                                            size_t nla_data_size)
{
    struct nlmsg *nlmsg_obj = (struct nlmsg *) taskmsg;

    if (nlmsg_obj->nlhdr.nlmsg_type == NLMSG_ERROR)
        TASK_ERROR("parse-task-req-response-nlmsg-error");

    size_t msg_length = nlmsg_obj->nlhdr.nlmsg_len - NLMSG_LEN;
    struct nlattr *nla = (struct nlattr *) (taskmsg + NLMSG_LEN);

    void *nla_data = NULL;

    while (msg_length) {

        if (nla->nla_type == request) {
            nla_data = malloc(nla_data_size);
            memcpy(nla_data, NLA_DATA(nla), nla_data_size);
            break;
        }

        size_t nla_aligned_len = nla->nla_type == TASKSTATS_TYPE_AGGR_PID ?
                                 NLA_HDRLEN : NLMSG_ALIGN(nla->nla_len);
        msg_length -= nla_aligned_len;
        nla = (struct nlattr *) ((char *) nla + nla_aligned_len);
    }

    return nla_data;
}

inline static void set_netlink_address(struct sockaddr_nl *nladdr)
{
    memset(nladdr, 0, NL_ADDR_SIZE);
    nladdr->nl_family = AF_NETLINK;
}

inline static void *rcv_task_req_response(struct task *taskobj, int request,
                                          size_t nla_data_len)
{
    taskobj->msg.msg_iov->iov_len = NL_MSG_SIZE;
    
    if (recvmsg(taskobj->nlconn_obj.conn_fd, &(taskobj->msg), 0) < 0)
        TASK_ERROR("rcv-task-req-response-recv");

    return parse_task_req_response((char *) &(taskobj->nlmsg_obj), request,
                                    nla_data_len);
}

inline static void *snd_task_req(struct task *taskobj, int request, 
                                 size_t nla_data_len)
{
    if (sendmsg(taskobj->nlconn_obj.conn_fd, &(taskobj->msg), 0) < 0)
        TASK_ERROR("snd-task-req-sendmsg");
    
    return rcv_task_req_response(taskobj, request, nla_data_len);
}

inline static void build_task_req(struct nlmsg *nlmsg_obj, int nl_type, int cmd,
                                  int nla_type, size_t data_len, void *data)
{
    nlmsg_obj->nlhdr.nlmsg_type = nl_type; 
    nlmsg_obj->nlhdr.nlmsg_flags = NLM_F_REQUEST;
    nlmsg_obj->nlhdr.nlmsg_len = NLMSG_LEN;
    nlmsg_obj->gnlhdr.cmd = cmd;
    nlmsg_obj->gnlhdr.version = 0x1;

    struct nlattr *nlattr_obj = GENLMSG_DATA(&(nlmsg_obj->gnlhdr));
    nlattr_obj->nla_type = nla_type;
    nlattr_obj->nla_len = NLA_LENGTH(data_len);
    memcpy(NLA_DATA(nlattr_obj), data, data_len);
    nlattr_obj->nla_len = NLA_HDRLEN + data_len;
    nlmsg_obj->nlhdr.nlmsg_len += NLMSG_ALIGN(nlattr_obj->nla_len);
}

inline static void build_task_family_id_req(struct task *taskobj)
{
    struct nlmsg *nlmsg_obj = &(taskobj->nlmsg_obj);

    size_t family_name_length = strlen(TASKSTATS_GENL_NAME) + 1;
    build_task_req(nlmsg_obj, GENL_ID_CTRL, CTRL_CMD_GETFAMILY,
              CTRL_ATTR_FAMILY_NAME, family_name_length, TASKSTATS_GENL_NAME);
}

inline static void create_conn(struct task *taskobj)
{
    taskobj->nlconn_obj.conn_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
    if (taskobj->nlconn_obj.conn_fd < 0)
        TASK_ERROR("create-conn-socket");
     
    set_netlink_address(&(taskobj->nlconn_obj.nladdr));
    if (bind(taskobj->nlconn_obj.conn_fd, (struct sockaddr *) 
             &(taskobj->nlconn_obj.nladdr), NL_ADDR_SIZE))
        TASK_ERROR("create-conn-bind");
}

struct taskstats *get_taskstats(struct task *taskobj, pid_t pid)
{
    if (!taskobj)
        TASK_ERROR("get-taskstats");

    struct nlmsg *nlmsg_obj = &(taskobj->nlmsg_obj);

    build_task_req(nlmsg_obj, taskobj->nlconn_obj.gnl_family_id, 
                   TASKSTATS_CMD_GET, TASKSTATS_CMD_ATTR_PID, 
                   sizeof(int), (void *) &pid);

    return (struct taskstats *) snd_task_req(taskobj, TASKSTATS_TYPE_STATS, 
                                             sizeof(struct taskstats));
}

struct task *init_task(void)
{
    struct task *new_task = calloc(1, sizeof *new_task);

    if (!new_task)
        TASK_ERROR("init-calloc");

    create_conn(new_task);

    new_task->msg.msg_name = &(new_task->nlconn_obj.nladdr);
    new_task->msg.msg_namelen = NL_ADDR_SIZE;

    build_task_family_id_req(new_task);
    new_task->vec.iov_base = &(new_task->nlmsg_obj);
    new_task->vec.iov_len = new_task->nlmsg_obj.nlhdr.nlmsg_len;

    new_task->msg.msg_iov = &(new_task->vec);
    new_task->msg.msg_iovlen = 1;

    void *family_id = snd_task_req(new_task, CTRL_ATTR_FAMILY_ID, sizeof(int)); 

    if (!family_id) 
        TASK_ERROR("get-task-family-rcv");

    new_task->nlconn_obj.gnl_family_id = *(int *) family_id;
    free(family_id);

    return new_task;
}

