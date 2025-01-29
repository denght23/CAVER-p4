#ifndef _TABLE_
#define _TABLE_

#include <core.p4>
#include <tna.p4>
#include "header.p4"
#include "parser.p4"

// action drop(){
//     ig_dprsr_md.drop_ctl = 0x1;
// }

// action forward(portid_t port) {
//     ig_tm_md.ucast_egress_port = port;
// }

// table forward_table {
//     key = {
//         hdr.ipv4.dst_ip : exact;
//     }

//     actions = {
//         forward;
//         drop;
//     }

//     const default_action = drop;
//     size = 1024;
// }



#endif /* _TABLE_ */