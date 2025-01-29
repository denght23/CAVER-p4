#ifndef _ACTION_
#define _ACTION_
#include "header.p4"
#include <core.p4>
#include <tna.p4>

// Register<bit<32>, bit<32>>(1, 0) roce_data_counter;
// Register<bit<32>, bit<32>>(1, 0) roce_ack_counter;
// RegisterAction<bit<32>, bit<32>, bit<32>>(roce_data_counter) roce_data_counter_action = {
//     void apply(inout bit<32> val, out bit<32> rv) {
//         val = val + 1;
//     }
// };
// RegisterAction<bit<32>, bit<32>, bit<32>>(roce_ack_counter) roce_ack_counter_action = {
//     void apply(inout bit<32> val, out bit<32> rv) {
//         val = val + 1;
//     }
// };

#endif /* _ACTION_ */