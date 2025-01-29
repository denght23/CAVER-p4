#ifndef _HEADER_
#define _HEADER_

typedef bit<48> mac_addr_t;
typedef bit<16> ether_type_t;
typedef bit<8> ip_protocol_t;
typedef bit<32> ipv4_addr_t;
typedef bit<9>   portid_t;       
typedef bit<8> caver_port_t;  
typedef bit<8> CE_t;  
const ether_type_t ETHERTYPE_IPV4 = 16w0x0800;
const ether_type_t ETHERTYPE_ARP = 16w0x0806;

const ip_protocol_t IP_PROTOCOLS_ICMP = 1;
const ip_protocol_t IP_PROTOCOLS_TCP = 6;
const ip_protocol_t IP_PROTOCOLS_UDP = 17;

const bit<16> ROCE_V2 = 4791;

// infiniband opcode
const bit<8> RC_IBV_OPCODE_ACKNOWLEDGE  = 0x11;

const bit<4> Other = 0x0;
const bit<4> DATA = 0x1;
const bit<4> ACK = 0x2;
const bit<4> CAVER_DATA = 0x3;
const bit<4> CAVER_ACK_first = 0x4;
const bit<4> CAVER_ACK_second = 0x5;

const bit<32> Path_Choice_Num = 0x4;

const bit<32> dre_time = 0x5; // us
const bit<32> flow_time = 50; // 50us




header ethernet_h {
    mac_addr_t dst_mac;
    mac_addr_t src_mac;
    ether_type_t ether_type;
}

header ipv4_h {
    bit<4> version;
    bit<4> ihl;
    bit<6> dscp;
    bit<2> ecn;
    bit<16> total_len;
    bit<16> identification;
    bit<3> flags;
    bit<13> frag_offset;
    bit<8> ttl;
    bit<8> protocol;
    bit<16> checksum;
    ipv4_addr_t src_ip;
    ipv4_addr_t dst_ip;
}


header arp_h {
    bit<16> hw_type;
    bit<16> proto_type;
    bit<8> hw_addr_len;
    bit<8> proto_addr_len;
    bit<16> opcode;
    // ...
}

header udp_h {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> hdr_length;
    bit<16> checksum;
}

header tcp_h {
    bit<16> src_port;
    bit<16> dst_port;
    bit<32> seq_no;
    bit<32> ack_no;
    bit<4> data_offset;
    bit<4> res;
    bit<8> flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgent_ptr;
}

header ib_bth_h {
    bit<8> opcode;
    bit<8> flags;  /** NOTE: "flags" field is used for REPLY INIT's ECN (0x3) between SrcToR/DstToR. No effect on RDMA. */
    bit<16> partition_key; 

    /*--- RC reserved0 (8 bits) ----*/
    bit<8> reserved0;  
    /*---------------------*/

    bit<24> destination_qp;
    bit<1> ack_request; 

    /*--- RC reserved1 (7 bits)----*/
    bit<7> reserved1;
    /*---------------------*/

    bit<24> packet_seqnum;
}

header ib_aeth_h {
    bit<1> reserved;
    bit<2> opcode;      // (0: ACK, 3: NACK)
    bit<5> error_code;  // (PSN SEQ ERROR)
    bit<8> msg_seq_number;
}
header caver_data{
    bit<8> out_port;
    bit<8> src_route;
}
header caver_ack{
    bit<8> best_port;
    bit<8> good_port;
    bit<16> best_CE;
    bit<16> good_CE;
}
header test{
    bit<16> temp;
}
struct header_t {
    ethernet_h ethernet;
    ipv4_h ipv4;
    arp_h arp;
    tcp_h tcp;
    udp_h udp;
    ib_bth_h infiniband;
    ib_aeth_h infiniband_ack;
    caver_data caver_data;
    caver_ack caver_ack;
    test test;

    // Add more headers here.
}
header resubmit_t {
    bit<8> port;
    bit<32> Dre;
    bit<8> best_port;
    bit<16> f1;
}

struct metadata_t {
    // Add metadata fields here.
    resubmit_t resubmit_hdr;
    bit<32> before_best_CE_and_Path;
    bit<32> after_best_CE_and_Path;
    bit<16> after_best_CE; // after_best_CE_and_Path[31:16]
    bit<16> after_best_Path; // after_best_CE_and_Path[15:0]
    bit<16> before_best_Path;
    bit<16> before_good_Path;//ACK中的good_path和本地的结合以后的path，outport1_outport0
    bit<16> before_goodCE;
    bit<4> packet_type;
    bit<8> opcode;
    bit<16> CE;
    bit<8> ack_port;
    bit<8> dre_port;
    bit<8> dre_decrease;
    bit<32> table_ip;//数据包的话是目的ip，ack的话是源ip，用于表项匹配
    bit<16> thresold_CE;
    bit<16> delta_CE;
    bit<1> acceptable;//判断收到的goodpath是否是一个可以接收的goodpath
    bit<16> new_path; //判断后可接收的新路径（before_good_path 或after best path）
    bit<16> flow_id; // flow id,hash冲突时保证此时目的地相同
    bit<1> new_flow; // 判断是不是一个新的flow
    bit<1> src_route;
    bit<8> delta_index;
    bit<16> path_choice; //outport1_outport0
    bit<8> index;//查path表的索引
    bit<8> port_equal;
}

// struct metadata_t {
//     // Add metadata fields here.
//     bit<32> before_best_CE_and_Path;
//     bit<32> after_best_CE_and_Path;
//     bit<16> before_best_Path;
//     bit<16> after_best_CE; // after_best_CE_and_Path[31:16]
//     bit<16> after_best_Path; // after_best_CE_and_Path[15:0]
//     bit<16> before_good_Path;//ACK中的good_path和本地的结合以后的path，outport1_outport0
//     bit<16> before_goodCE;
//     bit<16> thresold_CE;
//     bit<1> acceptable;//判断收到的goodpath是否是一个可以接收的goodpath
//     bit<4> packet_type;
//     bit<8> opcode;
//     bit<1> src_route;
//     bit<8> index;
//     bit<16> path_choice; //outport1_outport0
//     bit<16> new_path; //判断后可接收的新路径（before_good_path 或after best path）
//     bit<8> delta_index;
//     bit<16> CE;
//     bit<8> dre_port;
//     resubmit_t resubmit_hdr;
//     bit<1> dre_decrease;
//     bit<1> new_flow; // 判断是不是一个新的flow
//     bit<16> flow_id; // flow id,hash冲突时保证此时目的地相同
//     bit<32> table_ip;//数据包的话是目的ip，ack的话是源ip，用于表项匹配
// }

struct egress_metadata_t{  
      bit<8>dcqcn_prob_output;
      bit<8> dcqcn_random_number;
}
struct empty_header_t {}

struct empty_metadata_t {}



#endif /* _HEADER_ */