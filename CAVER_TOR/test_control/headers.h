#ifndef _HEADERS_H
#define _HEADERS_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <bf_rt/bf_rt_init.h>
#include <bf_rt/bf_rt_common.h>
#include <bf_rt/bf_rt_table_key.h>
#include <bf_rt/bf_rt_table_data.h>
#include <bf_rt/bf_rt_table.h>
#include <bf_rt/bf_rt_session.h>
#include <bf_switchd/bf_switchd.h>
#include <tofino/pdfixed/pd_conn_mgr.h>
#include <tofino/pdfixed/pd_mirror.h>
#include <bf_pm/bf_pm_intf.h>
#include <mc_mgr/mc_mgr_intf.h>
#include <tofino/bf_pal/bf_pal_port_intf.h>
#include <traffic_mgr/traffic_mgr_types.h>
#include <traffic_mgr/traffic_mgr_ppg_intf.h>
#include <traffic_mgr/traffic_mgr_port_intf.h>
#include <traffic_mgr/traffic_mgr_q_intf.h>

// for channel
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
#include <unistd.h>
//#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/in6.h>
#include <linux/ipv6.h>
#include <linux/icmp.h>
#include <linux/tcp.h>
#include <sys/socket.h>
//#include <time.h>

#include "../config.h"

#define ARRLEN(arr) sizeof(arr)/sizeof(arr[0])
#if __TOFINO_MODE__ == 0
const char *P4_PROG_NAME = "caver_tor";
static const char CPUIF_NAME[] = "bf_pci0";
#else
const char *P4_PROG_NAME = "rocesw_mode";
// static const char CPUIF_NAME[] = "veth251";
static const char CPUIF_NAME[] = "veth251";
#endif

/*
PORT |MAC |D_P|P/PT|SPEED  |FEC |RDY|ADM|OPR|LPBK    |FRAMES RX       |FRAMES TX       |E
-----+----+---+----+-------+----+---+---+---+--------+----------------+----------------+-
1/0  |23/0|128|3/ 0|40G    |NONE|YES|DIS|DWN|  NONE  |               0|               0|
2/0  |22/0|136|3/ 8|40G    |NONE|YES|DIS|DWN|  NONE  |               0|               0|
3/0  |21/0|144|3/16|40G    |NONE|YES|DIS|DWN|  NONE  |               0|               0|
4/0  |20/0|152|3/24|40G    |NONE|YES|DIS|DWN|  NONE  |               0|               0|
7/0  |17/0|176|3/48|40G    |NONE|YES|DIS|DWN|  NONE  |               0|               0|
8/0  |16/0|184|3/56|40G    |NONE|YES|DIS|DWN|  NONE  |               0|               0|
9/0  |15/0| 60|1/60|40G    |NONE|NO |DIS|DWN|  NONE  |               0|               0|
10/0 |14/0| 52|1/52|40G    |NONE|NO |DIS|DWN|  NONE  |               0|               0|
11/0 |13/0| 44|1/44|40G    |NONE|YES|DIS|DWN|  NONE  |               0|               0|
12/0 |12/0| 36|1/36|40G    |NONE|YES|DIS|DWN|  NONE  |               0|               0|
13/0 |11/0| 28|1/28|40G    |NONE|NO |DIS|DWN|  NONE  |               0|               0|
14/0 |10/0| 20|1/20|40G    |NONE|NO |DIS|DWN|  NONE  |               0|               0|
15/0 | 9/0| 12|1/12|40G    |NONE|NO |DIS|DWN|  NONE  |               0|               0|
16/0 | 8/0|  4|1/ 4|40G    |NONE|NO |DIS|DWN|  NONE  |               0|               0|
17/0 | 7/0|  0|1/ 0|40G    |NONE|NO |DIS|DWN|  NONE  |               0|               0|
18/0 | 6/0|  8|1/ 8|40G    |NONE|NO |DIS|DWN|  NONE  |               0|               0|
19/0 | 5/0| 16|1/16|40G    |NONE|NO |DIS|DWN|  NONE  |               0|               0|
20/0 | 4/0| 24|1/24|40G    |NONE|NO |DIS|DWN|  NONE  |               0|               0|
21/0 | 3/0| 32|1/32|40G    |NONE|NO |DIS|DWN|  NONE  |               0|               0|
22/0 | 2/0| 40|1/40|40G    |NONE|NO |DIS|DWN|  NONE  |               0|               0|
23/0 | 1/0| 48|1/48|40G    |NONE|NO |DIS|DWN|  NONE  |               0|               0|
24/0 | 0/0| 56|1/56|40G    |NONE|NO |DIS|DWN|  NONE  |               0|               0|
25/0 |31/0|188|3/60|40G    |NONE|NO |DIS|DWN|  NONE  |               0|               0|
26/0 |30/0|180|3/52|40G    |NONE|NO |DIS|DWN|  NONE  |               0|               0|
27/0 |29/0|172|3/44|40G    |NONE|NO |DIS|DWN|  NONE  |               0|               0|
28/0 |28/0|164|3/36|40G    |NONE|NO |DIS|DWN|  NONE  |               0|               0|
29/0 |26/0|148|3/20|40G    |NONE|NO |DIS|DWN|  NONE  |               0|               0|
30/0 |27/0|156|3/28|40G    |NONE|NO |DIS|DWN|  NONE  |               0|               0|
31/0 |24/0|132|3/ 4|40G    |NONE|YES|DIS|DWN|  NONE  |               0|               0|
32/0 |25/0|140|3/12|40G    |NONE|YES|DIS|DWN|  NONE  |               0|               0|
33/0 |32/0| 64|1/64|40G    |NONE|YES|DIS|DWN|  NONE  |               0|               0|
*/
typedef struct switch_port_s {
    char fp_port[5];
} switch_port_t;

typedef struct switch_s {
    bf_rt_target_t dev_tgt_0;
    bf_rt_target_t dev_tgt_1;
    bf_rt_session_hdl *session;
//    probe_period_t period_reg;
//    probe_port_stride_t stride_reg;
//    probe_port_t port_reg;
//    probe_ip_range_table_t pipr_table_t0;
//    probe_ip_range_table_t pipr_table_t1;
} switch_t;

typedef struct forward_entry_s{
	uint64_t ingress_port;
	const char* ipv4_addr;
	// char ipv4_addr[20];
	uint64_t egress_port;
} forward_entry_t;

// helper struct for forward polling table entry
typedef struct forward_polling_entry_s{
	uint32_t TP_type;
	uint64_t ingress_port;
	const char* vf_dst_ip_addr;
	// char ipv4_addr[20];
	const char* action;
	uint64_t egress_port;
	uint16_t mc_grp_id;
} forward_polling_entry_t;

typedef struct forward_2d_table_info_s {
    // Key field ids
    bf_rt_id_t kid_ipv4_dst_ip;
    bf_rt_id_t kid_ingress_port;
    // Action Ids
    bf_rt_id_t aid_ai_unicast;
    bf_rt_id_t aid_drop;
    // Data field Ids for ai_unicast
    bf_rt_id_t did_port;
    // Key and Data objects
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data;
} forward_2d_table_info_t;

typedef struct forward_table_info_s {
    // Key field ids
    bf_rt_id_t kid_dst_mac;
    // Action Ids
    bf_rt_id_t aid_unicast;
    bf_rt_id_t aid_broadcast;
    bf_rt_id_t aid_drop;
    // Data field Ids for ai_unicast
    bf_rt_id_t did_port;
    // Key and Data objects
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data;
    // Multicast info
    bf_mc_session_hdl_t mc_session;
    bf_mc_mgrp_hdl_t mc_mgrp;
    bf_mc_node_hdl_t mc_node;
    bf_mc_port_map_t mc_port_map;
    bf_mc_lag_map_t mc_lag_map;
} forward_table_info_t;

typedef struct forward_2d_table_entry_s {
    // Key value
    bf_dev_port_t ingress_port;
    uint32_t ipv4_addr;
    
    // Match length (for LPM)
    //uint16_t match_length;
    // Action
    char action[20];
    // Data value
    bf_dev_port_t egress_port;
} forward_2d_table_entry_t;

typedef struct forward_table_entry_s {
    // Key value
    uint64_t dst_mac;
    // Match length (for LPM)
    uint16_t match_length;
    // Action
    char action[16];
    // Data value
    bf_dev_port_t egress_port;
} forward_table_entry_t;


typedef struct forward_polling_table_entry_s {
    // Key value
    uint16_t polling_TP_type;
    bf_dev_port_t ingress_port;
	uint32_t polling_vf_dst_ip;
    
    // Match length (for LPM)
    //uint16_t match_length;
    // Action
    char action[64];
    // Data value
    bf_dev_port_t egress_port;
	uint16_t mc_grp_id;

} forward_polling_table_entry_t;

typedef struct forward_polling_table_info_s {
    // Key field ids
	bf_rt_id_t kid_TP_type;
    bf_rt_id_t kid_ingress_port;
	bf_rt_id_t kid_vf_dst_ip;
    // Action Ids
    bf_rt_id_t aid_ai_unicast;
    bf_rt_id_t aid_ai_broadcast;
    bf_rt_id_t aid_ai_drop;
    // Data field Ids for ai_unicast
    bf_rt_id_t did_unicast_port;
    bf_rt_id_t did_broadcast_port;
	bf_rt_id_t did_mc_grp_id;
    // Key and Data objects
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data;
    // Multicast info
//    bf_mc_session_hdl_t mc_session;
//    bf_mc_mgrp_hdl_t mc_mgrp;
//    bf_mc_node_hdl_t mc_node;
//    bf_mc_port_map_t mc_port_map;
//    bf_mc_lag_map_t mc_lag_map;
} forward_polling_table_info_t;


typedef struct register_info_s {
    // Key field ids
    bf_rt_id_t kid_register_index;
    // Data field Ids for register table
    bf_rt_id_t did_value;
    // Key and Data objects
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data;
} register_info_t;

typedef struct register_entry_s {
    // Key value
    uint32_t register_index;
    // Data value
    uint32_t value;
    uint32_t value_array_size;
    uint64_t *value_array;
} register_entry_t;


typedef struct re_lock_flag_s {
    const bf_rt_table_hdl *reg;
    register_info_t reg_info;
    register_entry_t entry;
} re_lock_flag_t;

typedef struct telemtry_pkt_num_s {
    const bf_rt_table_hdl *reg;
    register_info_t reg_info;
    register_entry_t entry;
} telemetry_pkt_num_t;

typedef struct telemtry_pkt_num_table {
    const bf_rt_table_hdl *reg;
    register_info_t reg_info;
} telemetry_pkt_num_table_t;



typedef struct telemtry_paused_num_s {
    const bf_rt_table_hdl *reg;
    register_info_t reg_info;
    register_entry_t entry;
} telemetry_paused_num_t;

typedef struct telemtry_egress_port_s {
    const bf_rt_table_hdl *reg;
    register_info_t reg_info;
    register_entry_t entry;
} telemetry_egress_port_t;

typedef struct telemtry_src_ip_s {
    const bf_rt_table_hdl *reg;
    register_info_t reg_info;
    register_entry_t entry;
} telemtry_src_ip_t;

typedef struct telemtry_dst_ip_s {
    const bf_rt_table_hdl *reg;
    register_info_t reg_info;
    register_entry_t entry;
} telemtry_dst_ip_t;

typedef struct telemtry_src_port_s {
    const bf_rt_table_hdl *reg;
    register_info_t reg_info;
    register_entry_t entry;
} telemtry_src_port_t;

typedef struct telemtry_dst_port_s {
    const bf_rt_table_hdl *reg;
    register_info_t reg_info;
    register_entry_t entry;
} telemtry_dst_port_t;

static void register_setup(const bf_rt_info_hdl *bfrt_info,
                           const char *reg_name,
                           const char *value_field_name,
                           const bf_rt_table_hdl **reg,
                           register_info_t *reg_info);
static void register_write(const bf_rt_target_t *dev_tgt,
                           const bf_rt_session_hdl *session,
                           const bf_rt_table_hdl *reg,
                           register_info_t *reg_info,
                           register_entry_t *reg_entry);
static void register_write_no_wait(const bf_rt_target_t *dev_tgt,
                                   const bf_rt_session_hdl *session,
                                   const bf_rt_table_hdl *reg,
                                   register_info_t *reg_info,
                                   register_entry_t *reg_entry);
static void register_read(const bf_rt_target_t *dev_tgt,
                          const bf_rt_session_hdl *session,
                          const bf_rt_table_hdl *reg,
                          register_info_t *reg_info,
                          register_entry_t *reg_entry);
/*
re_initial_tstamp
re_last_epoch_idx
re_pkt_last_timer
re_pause_timer
re_flow_epoch_no_win0-3
re_port_epoch_no_win0-3
re_port_meter_win0-3
re_lock_flag
re_port_paused_num_win0

re_telemetry_pkt_num_win0-3
re_telemetry_paused_num_win0-3
re_telemetry_egress_port
re_telemetry_dst_ip
re_telemetry_src_ip
re_telemetry_dst_port
re_telemetry_src_port
*/

//static void telemetry_pkt_num_setup(const bf_rt_info_hdl *bfrt_info,
//                               telemetry_pkt_num_t **pkt_num_reg);
static void telemetry_pkt_num_setup(const bf_rt_info_hdl *bfrt_info,
                               telemetry_pkt_num_t *pkt_num_reg, uint32_t win_idx);

static void telemetry_paused_num_setup(const bf_rt_info_hdl *bfrt_info,
                               telemetry_paused_num_t *paused_num_reg, uint32_t win_idx); 

static void telemetry_egress_port_setup(const bf_rt_info_hdl *bfrt_info,
                               telemetry_egress_port_t *egress_port_reg); 

static void re_lock_flag_setup(const bf_rt_info_hdl *bfrt_info,
                               re_lock_flag_t *re_lock_flag);


void re_lock_flag_read(const bf_rt_target_t *dev_tgt,
							   const bf_rt_session_hdl *session,
                               re_lock_flag_t *re_lock_flag);

void re_lock_flag_write_to_completion(const bf_rt_target_t *dev_tgt,
							   const bf_rt_session_hdl *session,
                               re_lock_flag_t *re_lock_flag, uint8_t value);

void re_lock_flag_write_no_wait(const bf_rt_target_t *dev_tgt,
							   const bf_rt_session_hdl *session,
                               re_lock_flag_t *re_lock_flag);

void telemetry_pkt_num_read(const bf_rt_target_t *dev_tgt,
								   const bf_rt_session_hdl *session,
								   telemetry_pkt_num_t *telemetry_pkt_num_ptr,
								   const uint32_t reg_idx);

void telemetry_paused_num_read(const bf_rt_target_t *dev_tgt,
								   const bf_rt_session_hdl *session,
								   telemetry_paused_num_t *telemetry_paused_num_ptr,
								   const uint32_t reg_idx,
									FILE* file);



// ---------Channel ---------------
// track event id
uint8_t used_flag[EVENT_ID_MAX];

typedef struct {
//    uint8_t TP_type: 2;
//    uint8_t padding_0: 6;
	uint8_t TP_type_w_padding_0; // 1bytes for TPtype and padding
    uint8_t bytes_ingress[2];  // 2bytes for ingress_port and padding_1
    uint8_t bytes_egress[2];   // 2bytes for egress_port and padding_2
    uint8_t event_id;
    uint32_t vf_src_ip;
    uint32_t vf_dst_ip;
    uint8_t vf_protocol;
    uint16_t vf_src_port;
    uint16_t vf_dst_port;
} __attribute__((packed)) polling_h;

typedef struct update_polling_channel_s {
	int sockfd;
	char recvbuf[PKTBUF_SIZE];
//    uint8_t  probe_table;
//    uint16_t pipr_idx;
    uint16_t egress_port;
	polling_h *polling;
} update_polling_channel_t;



int create_update_polling_channel(update_polling_channel_t *channel);
int recv_update_polling(update_polling_channel_t *channel);
int process_event_id(uint8_t event_id) {
	printf(" Debug: Processing event id %u\n", event_id);
    if (used_flag[event_id] == 1) {
        printf("Duplicate event ID %d detected, dropping packet.\n", event_id);
        return 1;
    } else {
        used_flag[event_id] = 1;  // 
        printf("New event ID %d received and processed.\n", event_id);
        if (event_id == 255) {
            memset(used_flag, 0, sizeof(used_flag));
            printf("Resetting event ID tracking array.\n");
        }
        return 0;
    }
}





#endif
