#pragma once
#include <rdma/rdma_cma.h>
#include <arpa/inet.h>

/* Constants */

#ifndef REGION_SIZE_GB
#define REGION_SIZE_GB ((size_t)8)
#endif

#define BACKLOG_REQUESTS 10

#define CQE_NUM 10

#define MAX_SEND_WR_NUM 16
#define MAX_RECV_WR_NUM 16
#define MAX_SEND_SGE_NUM 1
#define MAX_RECV_SGE_NUM 1
#define MAX_REGION_NUM 16 /* Up to MAX_REGION_NUM * REGION_SIZE_GB remote memory */

#define CQ_QP_IDLE 0
#define CQ_QP_BUSY 1
#define CQ_QP_DOWN 2

enum rdma_queue_type {
    QP_STORE,
    QP_LOAD_SYNC,
    QP_LOAD_ASYNC,
    NUM_QP_TYPE
};

enum rdma_message_type {
    DONE = 1,
    GOT_CHUNKS,
    GOT_SINGLE_CHUNK,
    FREE_SIZE,
    EVICT,

    ACTIVITY,
    STOP,

    REQUEST_CHUNKS,
    REQUEST_SINGLE_CHUNK,
    QUERY,

    AVAILABLE_TO_QUERY
};

enum server_states {
    S_WAIT,
    S_BIND,
    S_DONE
};

enum send_states {
    SS_INIT,
    SS_MR_SENT,
    SS_STOP_SENT,
    SS_DONE_SENT
};

enum recv_states {
    RS_INIT,
    RS_STOPPED_RECV,
    RS_DONE_RECV
};

struct rdma_dev{
    struct ibv_context *ctx;
    struct ibv_pd *pd;
};

struct rdma_queue{
    struct rdma_cm_id *cm_id;

    struct ibv_cq *cq;
    struct ibv_qp *qp;

    uint8_t connected;

    int q_index;
    enum rdma_queue_type type;
    struct context *rdma_session;
};

struct rdma_message{
    uint64_t buf[MAX_REGION_NUM];
    uint64_t mapped_size[MAX_REGION_NUM];
    uint32_t rkey[MAX_REGION_NUM];
    int mapped_chunk;

    enum rdma_message_type type;
};

struct rdma_mem_pool{
    char *heap_start;
    int region_num;

    struct ibv_mr *mr_buffer[MAX_REGION_NUM];
    char *region_list[MAX_REGION_NUM];
    size_t region_mappped_size[MAX_REGION_NUM];
    int cache_status[MAX_REGION_NUM];
};

struct context{
    struct rdma_dev *dev;
    struct rdma_queue *queues;

    struct ibv_comp_channel *comp_channel;
    pthread_t cq_poller_thread; /* One thread to poll the cq for 2-sided RDMA */

    /* Resource for 2-sided RDMA */
    struct rdma_message *recv_msg;
    struct ibv_mr *recv_mr;

    struct rdma_message *send_msg;
    struct ibv_mr *send_mr;

    struct rdma_mem_pool *mem_pool;

    /* states */
    int connected;
    server_states server_state;
};

/**
 * Define tools
 */
void die(const char *reason);

#define ntohll(x)                                                              \
  (((uint64_t)(ntohl((int)((x << 32) >> 32))) << 32) |                         \
   (unsigned int)ntohl(((int)(x >> 32))))

#define TEST_NZ(x)                                                             \
  do {                                                                         \
    if ((x))                                                                   \
      die("error: " #x " failed (returned non-zero).");                        \
  } while (0)
#define TEST_Z(x)                                                              \
  do {                                                                         \
    if (!(x))                                                                  \
      die("error: " #x " failed (returned zero/null).");                       \
  } while (0)