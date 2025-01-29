#ifndef _PARSER_
#define _PARSER_
#include <core.p4>
#include <tna.p4>
#include "header.p4"
#include "util.p4"

parser Caver_Middle_Parser(
        packet_in pkt, 
        out header_t hdr,
        out metadata_t ig_md,
        out ingress_intrinsic_metadata_t ig_intr_md) {
    //parser结束后：通过ig_md.packet_type判断数据包类型（data，ACK， caverdata， caver_ack(1/2), others）
    //resubmit的数据在ig_md.resubmit_hdr里面；
    //通过infiniband的reserved0判断是否为caver_data；
    //通过infiniband_ack的reserved判断是否为caver_ack；
    state start {
        ig_md.packet_type = Other;
        ig_md.before_best_CE_and_Path = 0;
        ig_md.before_good_CE_and_Path = 0;
        ig_md.dre_port = 0;
        ig_md.src_route = 0;
        pkt.extract(ig_intr_md);
        transition select(ig_intr_md.resubmit_flag) {
            1 : parse_resubmit;
            0 : parse_port_metadata;
        }
    }
    state parse_resubmit {
        pkt.extract(ig_md.resubmit_hdr);
        pkt.extract(hdr.ethernet);
        pkt.extract(hdr.ipv4);
        pkt.extract(hdr.udp);
        pkt.extract(hdr.infiniband);
        pkt.extract(hdr.infiniband_ack);
        pkt.extract(hdr.caver_ack);
        ig_md.packet_type = CAVER_ACK_second;
        transition accept;
    }

    state parse_port_metadata {
        pkt.advance(PORT_METADATA_SIZE);
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
            ETHERTYPE_IPV4 : parse_ipv4;
            ETHERTYPE_ARP : parse_arp;
            default: accept;
        }
    }

    state parse_arp {
        pkt.extract(hdr.arp);
        transition accept;
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            IP_PROTOCOLS_UDP : parse_udp;
            IP_PROTOCOLS_TCP : parse_tcp;
            default: accept;
        }
    }

    state parse_udp{
        pkt.extract(hdr.udp);
        transition select(hdr.udp.dst_port) {
            ROCE_V2: parse_roce;
            default: accept;
        }
    }
    state parse_tcp{
        pkt.extract(hdr.tcp);
        transition accept;
    }
    state parse_roce{
        ig_md.opcode = pkt.lookahead<bit<8>>()[7:0];
        transition select(ig_md.opcode){
            RC_IBV_OPCODE_ACKNOWLEDGE: parse_roce_ack;
            default: parse_roce_data;
        }
    }
    state parse_roce_data{
        pkt.extract(hdr.infiniband);
        transition select(hdr.infiniband.reserved0){
            1: parse_roce_caver_data;
            default: parse_roce_raw_data;
        }
    }
    state parse_roce_raw_data{
        ig_md.packet_type = DATA;
        transition accept;
    }
    state parse_roce_caver_data{
        pkt.extract(hdr.caver_data);
        ig_md.packet_type = CAVER_DATA;
        transition accept;
    }

    state parse_roce_ack{
        pkt.extract(hdr.infiniband);
        pkt.extract(hdr.infiniband_ack);
        transition select(hdr.infiniband_ack.reserved){
            1: parse_roce_caver_ack;
            default: parse_roce_raw_ack;
        }
    }
    state parse_roce_raw_ack{
        ig_md.packet_type = ACK;
        transition accept;
    }
    state parse_roce_caver_ack{
        pkt.extract(hdr.caver_ack);
        ig_md.packet_type = CAVER_ACK_first;
        transition accept;
    }
}
parser SimpleIngressParser(
        packet_in pkt,
        out header_t hdr,
        out metadata_t ig_md,
        out ingress_intrinsic_metadata_t ig_intr_md) {

    TofinoIngressParser() tofino_parser;    
    state start {
        tofino_parser.apply(pkt, ig_intr_md);
        transition accept;
    }  
}

control SwitchIngressDeparser(
        packet_out pkt,
        inout header_t hdr,
        in metadata_t ig_md,
        in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {

    apply {
        pkt.emit(hdr);
    }
}

// parser Caver_middle_EgressParser(
//         packet_in pkt,
//         out header_t hdr,
//         out egress_metadata_t eg_md,
//         out egress_intrinsic_metadata_t eg_intr_md){

//     TofinoEgressParser() tofino_parser;

//     state start {
//         eg_md.dre_decrease = 0;
//         eg_md.packet_type = Other;
//         tofino_parser.apply(pkt, eg_intr_md);
//         transition parse_bridge_hdr;
//     }
//     state parse_bridge_hdr {
//         pkt.extract(eg_md.bridge_hdr);
//         transition parse_ethernet;
//     }

//     state parse_ethernet {
//         pkt.extract(hdr.ethernet);
//         transition select(hdr.ethernet.ether_type) {
//             ETHERTYPE_IPV4 : parse_ipv4;
//             ETHERTYPE_ARP : parse_arp;
//             default: accept;
//         }
//     }

//     state parse_arp {
//         pkt.extract(hdr.arp);
//         transition accept;
//     }

//     state parse_ipv4 {
//         pkt.extract(hdr.ipv4);
//         transition select(hdr.ipv4.protocol) {
//             IP_PROTOCOLS_UDP : parse_udp;
//             IP_PROTOCOLS_TCP : parse_tcp;
//             default: accept;
//         }
//     }

//     state parse_udp{
//         pkt.extract(hdr.udp);
//         transition select(hdr.udp.dst_port) {
//             ROCE_V2: parse_roce;
//             default: accept;
//         }
//     }
//     state parse_tcp{
//         pkt.extract(hdr.tcp);
//         transition accept;
//     }
//     state parse_roce{
//         eg_md.opcode = pkt.lookahead<bit<8>>()[7:0];
//         transition select(eg_md.opcode){
//             RC_IBV_OPCODE_ACKNOWLEDGE: parse_roce_ack;
//             default: parse_roce_data;
//         }
//     }
//     state parse_roce_data{
//         pkt.extract(hdr.infiniband);
//         transition select(hdr.infiniband.reserved0){
//             1: parse_roce_caver_data;
//             default: parse_roce_raw_data;
//         }
//     }
//     state parse_roce_raw_data{
//         eg_md.packet_type = DATA;
//         transition accept;
//     }
//     state parse_roce_caver_data{
//         pkt.extract(hdr.caver_data);
//         eg_md.packet_type = CAVER_DATA;
//         transition accept;
//     }

//     state parse_roce_ack{
//         pkt.extract(hdr.infiniband);
//         pkt.extract(hdr.infiniband_ack);
//         transition select(hdr.infiniband_ack.reserved){
//             1: parse_roce_caver_ack;
//             default: parse_roce_raw_ack;
//         }
//     }
//     state parse_roce_raw_ack{
//         eg_md.packet_type = ACK;
//         transition accept;
//     }
//     state parse_roce_caver_ack{
//         pkt.extract(hdr.caver_ack);
//         eg_md.packet_type = CAVER_ACK_first;
//         transition accept;
//     }
// }

parser SwitchEgressParser(
        packet_in pkt,
        out header_t hdr,
        out egress_metadata_t eg_md,
        out egress_intrinsic_metadata_t eg_intr_md){

    TofinoEgressParser() tofino_parser;

    state start {
        tofino_parser.apply(pkt, eg_intr_md);
        transition accept;
    }
}
control SwitchEgress(
        inout header_t hdr,
        inout egress_metadata_t eg_md,
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_intr_prsr_md,
        inout egress_intrinsic_metadata_for_deparser_t eg_intr_dprsr_md,
        inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md) {
    apply {}
}
control SwitchEgressDeparser(
        packet_out pkt,
        inout header_t hdr,
        in egress_metadata_t eg_md,
        in egress_intrinsic_metadata_for_deparser_t eg_intr_dprsr_md) {
    apply {
        pkt.emit(hdr);
    }
}

#endif /* _PARSER_ */