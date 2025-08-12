#include <iostream>
#include "rdma_server.hpp"
/* Mostly follow the hermit style */
size_t region_num;
int online_cores;
int rdma_num_queues;

struct context *global_rdma_ctx = NULL;
int rdma_queue_count = 0;

inline enum rdma_queue_type get_qp_type(int idx) {
  unsigned type = idx / online_cores;

  if (type < NUM_QP_TYPE) {
    return (enum rdma_queue_type)type;
  } else {
    std::cerr<<"wrong rdma queue type: "<<idx / online_cores << std::endl;
    return QP_STORE;
  }

  return QP_STORE;
}

int main(int argc, char* argv[]){
    struct sockaddr_in6 addr;
    char ip_str[INET_ADDRSTRLEN];
    uint16_t port = 0;
    int mem_size_in_gb;

    struct rdma_cm_id *listener = NULL;
    struct rdma_event_channel *ec = NULL;
    struct rdma_cm_event *event = NULL;

    if (argc < 5) {
        std::cout<<"Usage: "<<argv[0]<<" <ip> <port> <memory size in GB> <#(cores) on CPU server>"<<std::endl;   
        exit(-1);
    }
    strcpy(ip_str, argv[1]);
    port = atoi(argv[2]);
    mem_size_in_gb = atoi(argv[3]);
    region_num = mem_size_in_gb / REGION_SIZE_GB;
    online_cores = atoi(argv[4]);
    rdma_num_queues = online_cores * NUM_QP_TYPE;

    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, ip_str, &addr.sin6_addr);
    addr.sin6_port = htons(port);

    std::cerr<<__func__<<" trying to bind to "<<ip_str<<":"<<port<<std::endl;

    ec = rdma_create_event_channel();
    if (!ec) {
        std::cerr<<"rdma_create_event_channel failed " <<(void *)ec<<std::endl; 
    }

    if (rdma_create_id(ec, &listener, NULL, RDMA_PS_TCP)){
        std::cerr<<"rdma_create_id failed"<<std::endl;
    }

    if (rdma_bind_addr(listener, reinterpret_cast<struct sockaddr*>(&addr))){
        std::cerr<<"rdma_bin_addr failed"<<std::endl;
    }

    if (rdma_listen(listener, BACKLOG_REQUESTS)) {
        std::cerr<<"rdma_listen failed"<<std::endl;
    }

    port = ntohs(rdma_get_src_port(listener));
    std::cerr<<"listening on port "<<port<<std::endl;

    global_rdma_ctx = reinterpret_cast<struct context *>(calloc(1, sizeof(struct context)));
    global_rdma_ctx->queues = reinterpret_cast<struct rdma_queue *>(calloc(rdma_num_queues, sizeof(struct rdma_queue)));
    global_rdma_ctx ->connected = 0;
    global_rdma_ctx ->server_state = S_WAIT;

    init_memory_pool(global_rdma_ctx);

    /* Get event from channel */
    while (rdma_get_cm_event(ec, &event) == 0){
        struct rdma_cm_event event_copy;
        memcpy(&event_copy, event, sizeof(*event));
        rdma_ack_cm_event(event);

        if(on_cm_event(&event_copy))
            break;
    }


    std::cerr<<__func__ << " RDMA cma thread exit"<<std::endl;
    rdma_destroy_event_channel(ec);
}

/* simply init each memory region by malloc */
void init_memory_pool(struct context *rdma_ctx){
    rdma_ctx ->mem_pool = reinterpret_cast<struct rdma_mem_pool *>(calloc(1, sizeof(struct rdma_mem_pool)));

    size_t region_size = REGION_SIZE_GB * 1024 * 1024 * 1024;
    size_t heap_size = region_size * region_num;
    
    rdma_ctx -> mem_pool -> heap_start = reinterpret_cast<char*> (malloc(heap_size));
    rdma_ctx -> mem_pool -> region_num = region_num;

    for (size_t i = 0; i < region_num; i++){
        rdma_ctx -> mem_pool->region_list[i] = rdma_ctx -> mem_pool -> heap_start + i * region_size;
        rdma_ctx -> mem_pool->region_mappped_size[i] = region_size;
        rdma_ctx -> mem_pool->cache_status[i] = -1;
    }
}

int on_cm_event(struct rdma_cm_event *event){
    int r = 0;
    struct rdma_queue *queue  = reinterpret_cast<struct rdma_queue*>(event -> id ->context);

    if (event -> event == RDMA_CM_EVENT_CONNECT_REQUEST){
        r = on_connect_request(event->id);
    }else if (event -> event == RDMA_CM_EVENT_ESTABLISHED){
        r = rdma_connected(queue);
    }else if (event -> event == RDMA_CM_EVENT_DISCONNECTED){

    }
    else {
        die("on_cm_event: unknown event");
    }
    return r;
}

/* Connect a qp */
int on_connect_request(struct rdma_cm_id *id){
    struct rdma_conn_param cm_params;
    struct rdma_queue *queue;
    struct ibv_qp_init_attr qp_attr;

    queue = &(global_rdma_ctx->queues[rdma_queue_count]);
    queue -> q_index = rdma_queue_count;
    queue -> type = get_qp_type(rdma_queue_count);
    rdma_queue_count ++;
    queue -> cm_id = id;  /* Associate the queue with the cm_id of the new event */

    std::cerr<<__func__<< "rdma_queue["<<rdma_queue_count -1 <<"] received connection request"<<std::endl;

    /* Initialize pd cq and poll_cq trd. poll_cq trd is used to establish 
       the connection with the computer server */

    /* pd comp_channel cq and poll_cq are only created once */
    if (global_rdma_ctx -> dev == NULL){
        global_rdma_ctx -> dev = reinterpret_cast<struct rdma_dev*> (calloc(1, sizeof(struct rdma_dev)));
        global_rdma_ctx -> dev -> ctx = id -> verbs; 
        /* Alloc pd */
        TEST_Z(global_rdma_ctx -> dev -> pd = ibv_alloc_pd(id -> verbs)); 
        /* Create comp_channel */
        TEST_Z(global_rdma_ctx -> comp_channel = ibv_create_comp_channel(id ->verbs));
        /* Create cq */
        TEST_Z(queue -> cq = ibv_create_cq(id -> verbs, CQE_NUM, NULL, global_rdma_ctx -> comp_channel, 0));

        TEST_NZ(ibv_req_notify_cq(queue-> cq, 0));

        /* Register buffers for 2-sided RDMA */
        global_rdma_ctx -> send_msg = reinterpret_cast<struct rdma_message*>(calloc(1, 
        sizeof(struct rdma_message)));
        global_rdma_ctx -> recv_msg = reinterpret_cast<struct rdma_message*>(calloc(1, 
        sizeof(struct rdma_message)));
        TEST_Z(global_rdma_ctx -> send_mr = ibv_reg_mr(global_rdma_ctx -> dev-> pd, 
        global_rdma_ctx -> send_msg, sizeof(struct rdma_message), 
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ));
        TEST_Z(global_rdma_ctx -> recv_mr = ibv_reg_mr(global_rdma_ctx -> dev-> pd, 
        global_rdma_ctx -> recv_msg, sizeof(struct rdma_message), 
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ));

        TEST_NZ(pthread_create(&global_rdma_ctx->cq_poller_thread, NULL, poll_cq, NULL));
    }

    /* Build qp attribute necessary to create a qp */
    memset(&qp_attr, 0, sizeof(qp_attr));
    qp_attr.send_cq = queue -> cq; /* optional. if NULL, cm will create a cq and comp_channel */
    qp_attr.recv_cq = queue -> cq;
    qp_attr.qp_type = IBV_QPT_RC;
    qp_attr.cap.max_recv_wr = MAX_RECV_WR_NUM;
    qp_attr.cap.max_send_wr = MAX_SEND_WR_NUM;
    qp_attr.cap.max_recv_sge = MAX_RECV_SGE_NUM;
    qp_attr.cap.max_send_sge = MAX_SEND_SGE_NUM;

    /* Create new qp under the same pd */
    TEST_NZ(rdma_create_qp(queue->cm_id, global_rdma_ctx->dev->pd, &qp_attr));
    queue -> qp = queue -> cm_id -> qp;
    queue->cm_id -> context = queue;
    queue->rdma_session = global_rdma_ctx; /* Use the same global context */ 

    /* Build connection parameter */
    memset(&cm_params, 0, sizeof(cm_params));
    cm_params.initiator_depth = cm_params.responder_resources + 1;
    cm_params.rnr_retry_count = 7; /* Infinite retry */

    /* Accept */
    TEST_NZ(rdma_accept(id, &cm_params));
    queue->connected = 1;

    std::cerr<<__func__<< "rdma_queue["<<rdma_queue_count -1 <<"] sends ACCEPT back to compute server"<<std::endl;

    return 0;
}

int rdma_connected(struct rdma_queue *queue){
    struct context *rdma_session = reinterpret_cast<struct context *>(queue->rdma_session);
    if (rdma_session -> connected == 0){
        rdma_session -> connected = 1;
        std::cerr<<__func__ << " connection build. Register heap as RDMA buffer" <<std::endl;
        for (size_t i = 0; i < region_num; i ++){
            /* Register mr_buffer */
            TEST_Z(rdma_session -> mem_pool -> mr_buffer[i] = ibv_reg_mr(
                rdma_session -> dev -> pd, rdma_session->mem_pool -> region_list[i],
                (size_t)rdma_session->mem_pool->region_mappped_size[i],
                IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | 
                IBV_ACCESS_REMOTE_READ));
        }
    }

    rdma_session->send_msg->type = AVAILABLE_TO_QUERY;
    send_message(queue);
    post_receives(queue);
    return 0;
}

void send_message(struct rdma_queue* queue){
    struct ibv_send_wr wr, *bad_wr = NULL;
    struct ibv_sge sge;
    struct context *rdma_session = queue->rdma_session;
    memset(&wr, 0, sizeof(wr));
    wr.wr_id = (uintptr_t)queue; /* When we receive, we can use wr_id field to get the queue back */ 
    wr.opcode = IBV_WR_SEND;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED;

    sge.addr = (uintptr_t)rdma_session -> send_msg;
    
}

void post_receives(struct rdma_queue* queue){

}

void *poll_cq(void* ctx){
    struct ibv_cq *cq;
    struct ibv_wc wc;
    while (1){
        TEST_NZ(ibv_get_cq_event(global_rdma_ctx -> comp_channel, &cq, &ctx));
        ibv_ack_cq_events(cq, 1);
        TEST_NZ(ibv_req_notify_cq(cq, 0));
        while (ibv_poll_cq(cq, 1, &wc))
            handle_cqe(&wc);
    }
    return NULL;
}

void handle_cqe(struct ibv_wc *wc){

}