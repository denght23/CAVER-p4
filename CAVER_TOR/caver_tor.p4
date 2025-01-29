#include <core.p4>
#include <tna.p4>

#include "header.p4"
#include "parser.p4"
#include "action.p4"
#include "table.p4"
struct pair {
    bit<16>     first;
    bit<16>     second;
}
control SwitchIngress(
        inout header_t hdr,
        inout metadata_t ig_md,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

    action drop() {
        ig_dprsr_md.drop_ctl = 0x1; // Drop packet.
    }
    action no_action(){

    }
    Hash<bit<8>>(HashAlgorithm_t.CRC8) hash_crc8_low;
    Hash<bit<8>>(HashAlgorithm_t.CRC8) hash_crc8_high;
    action get_flow_id(){
        bit<8> temp1 = (bit<8>)hash_crc8_low.get({hdr.ipv4.dst_ip});
        bit<8> temp2 = (bit<8>)hash_crc8_high.get({hdr.ipv4.src_ip, hdr.udp.src_port, hdr.udp.dst_port});
        ig_md.flow_id[7:0] = temp1;
        ig_md.flow_id[15:8] = temp2;
    }
    table flow_id_table {
        key = {
            ig_md.packet_type: exact;
        }
        actions = {
            get_flow_id;
            no_action;
        }
        default_action = no_action;
        size = 256;
    }
    //FLowTimeTable and FlowMethodTable and FlowPathTable
    Register<bit<32>, bit<16>>(65536) flow_time_reg;//记录每个流最新的包的到达时间（ig_prsr_md.global_tstamp(ns)）
    RegisterAction<bit<32>, bit<16>, bit<1>>(flow_time_reg) flow_time_update_action = {
        void apply(inout bit<32> val, out bit<1> rv) {
            bit<32> current_time = ig_prsr_md.global_tstamp[41:10];
            if (current_time - val > flow_time){
                rv = 1; // new_flow;
            }
            else{
                rv = 0; // old_flow;
            }
            val = current_time;
        }
    };
    action flow_time_update(){
        // bit<8> temp1 = (bit<8>)hash_crc8_low.get({hdr.ipv4.dst_ip});
        // bit<8> temp2 = (bit<8>)hash_crc8_high.get({hdr.ipv4.src_ip, hdr.udp.src_port, hdr.udp.dst_port});
        // ig_md.flow_id[7:0] = temp1;
        // ig_md.flow_id[15:8] = temp2;
        ig_md.new_flow = flow_time_update_action.execute(ig_md.flow_id);
    }
    //简介：
    //处理的包类型：
    //data: key:bit<4> packet_type;
    table flow_time_table {
        key = {
            ig_md.packet_type: exact;
        }
        actions = {
            flow_time_update;
            no_action;
        }
        default_action = no_action;
        size = 256;
    }
    Register<bit<8>, bit<16>>(65536) flow_method_reg;
    RegisterAction<bit<8>, bit<16>, bit<1>>(flow_method_reg) flow_method_update_action = {
        void apply(inout bit<8> val, out bit<1> rv) {
            val = (bit<8>)ig_md.src_route;
        }
    };
    RegisterAction<bit<8>, bit<16>, bit<1>>(flow_method_reg) flow_method_take_action = {
        void apply(inout bit<8> val, out bit<1> rv) {
            rv = (bit<1>)val;
        }
    };
    action flow_method_update(){
        flow_method_update_action.execute(ig_md.flow_id);
    }
    action flow_method_take(){
        ig_md.src_route = flow_method_take_action.execute(ig_md.flow_id);
    }
    //简介：
    //处理的包类型：
    //data(new_flow): flow_method_update;
    //data(old_flow): flow_method_take;
    table flow_method{
        key = {
            ig_md.packet_type: exact;
            ig_md.new_flow: exact;
        }
        actions = {
            flow_method_update;
            flow_method_take;
            no_action;
        }
        default_action = no_action;
        size = 256;
    }

    Register<bit<16>, bit<16>>(65536) flow_path_reg;
    RegisterAction<bit<16>, bit<16>, bit<16>>(flow_path_reg) flow_path_update_action = {
        void apply(inout bit<16> val, out bit<16> rv) {
            val = ig_md.path_choice;
        }
    };
    RegisterAction<bit<16>, bit<16>, bit<16>>(flow_path_reg) flow_path_take_action = {
        void apply(inout bit<16> val, out bit<16> rv) {
            rv = val;
        }
    };
    action flow_path_update(){
        flow_path_update_action.execute(ig_md.flow_id);
    }
    action flow_path_take(){
        ig_md.path_choice = flow_path_take_action.execute(ig_md.flow_id);

        ig_md.dre_port = ig_md.path_choice[7:0];
        hdr.caver_data.setValid();
        hdr.caver_data.src_route = (bit<8>)ig_md.src_route;
        hdr.caver_data.out_port = ig_md.path_choice[15:8];
    }
    action flow_path_ecmp_take(){
        hdr.caver_data.setValid();
        hdr.caver_data.src_route = (bit<8>)ig_md.src_route;
        hdr.caver_data.out_port = 0;
    }
    //简介：
    //处理的包类型：
    //data(new_flow, src_route): flow_path_update;
    //data(old_flow,src_route): flow_path_take;
    //data(old_flow,ecmp): flow_path_ecmp_take;
    table flow_path{
        key = {
            ig_md.packet_type: exact;
            ig_md.new_flow: exact;
            ig_md.src_route: exact;
        }
        actions = {
            flow_path_update;
            flow_path_take;
            flow_path_ecmp_take;
            no_action;
        }
        default_action = no_action;
        size = 256;
    }

    //BestTable
    Register<bit<32>, bit<8>>(8) BestTable_reg; //bit<16>CE_bit<8>outport1_bit<8>outport0
    RegisterAction<bit<32>, bit<8>, bit<32>>(BestTable_reg) BestTable_compare_update_action = {
        void apply(inout bit<32> val, out bit<32> rv) {
            if ((val >= ig_md.before_best_CE_and_Path)) {
                val = ig_md.before_best_CE_and_Path;
            }
            rv = val;
        }
    };
    RegisterAction<bit<32>, bit<8>, bit<32>>(BestTable_reg) BestTable_update_action = {
        void apply(inout bit<32> val, out bit<32> rv) {
            val = ig_md.before_best_CE_and_Path;
            rv = val;
        }
    };
    RegisterAction<bit<32>, bit<8>, bit<8>>(BestTable_reg) BestTable_read_port_action = {
        void apply(inout bit<32> val, out bit<8> rv) {
            rv = val[7:0];
        }
    };
    action check_BestTable(bit<8> host){
        ig_md.after_best_CE_and_Path = BestTable_compare_update_action.execute(host);
        ig_md.after_best_CE = ig_md.after_best_CE_and_Path[31:16];
        ig_md.after_best_Path = ig_md.after_best_CE_and_Path[15:0];
    }
    action update_BestTable(bit<8> host){
        ig_md.after_best_CE_and_Path= BestTable_update_action.execute(host);
        ig_md.after_best_CE = ig_md.after_best_CE_and_Path[31:16];
        ig_md.after_best_Path = ig_md.after_best_CE_and_Path[15:0];
    }
    action read_port_BestTable(bit<8> host){
        ig_md.resubmit_hdr.best_port = BestTable_read_port_action.execute(host);
    }
    //简介：
    //处理的包类型：
    //caver_ack(first): ig_md.port_equal=0,1->read_port_BestTable
    //caver_ack(second): ig_md.port_equal=0->check_BestTable; ig_md.port_equal=1 ->update_BestTable
    //data:ip对应的id
    table BestTable{
        key = {
            hdr.ipv4.src_ip: exact;//Caver_ACK的源ip地址相关的路径
            ig_md.packet_type: exact;
            ig_md.port_equal: exact;
        }
        actions = {
            check_BestTable;
            update_BestTable;
            read_port_BestTable;
            no_action;
        }
        default_action = no_action;
        size = 1024;
    }
    //thresold(judge acceptable)
    Register<bit<16>, bit<8>>(1) TH_reg;
    MathUnit<bit<16>>(MathOp_t.MUL, 6, 5) TH_MUL; //判断的阈值选择的为1.2
    RegisterAction<bit<16>, bit<8>, bit<16>>(TH_reg) TH_action = {
        void apply(inout bit<16> val, out bit<16> rv) {
            // val = TH_MUL.execute(ig_md.after_best_CE);
            // if(val > ig_md.before_goodCE){
            //     rv = 1;
            // }
            // else{
            //     rv = 0;
            // }
            val = TH_MUL.execute(ig_md.after_best_CE);
            rv = val;
        }
    };
    action judge_acceptable(){
        // ig_md.acceptable = TH_action.execute(0);
        ig_md.thresold_CE = TH_action.execute(0);
    }
    //简介：
    //处理的包类型：
    //caver_ack_second
    table judge_acceptable_table{
        key = {
            ig_md.packet_type: exact;
        }
        actions = {
            judge_acceptable;
            no_action;
        }
        default_action = no_action;
        size = 1024;
    }
    //判断后可接收的新路径（before_good_path 或after best path）
    action use_best_path(){
        ig_md.new_path = ig_md.after_best_Path;
    }
    action use_good_path(){
        ig_md.new_path = ig_md.before_good_Path;
    }
    //简介：
    //处理的包类型：
    //caver_ack_second: acceptable == 1->use_good_path; acceptable == 0->use_best_path
    table set_acceptable_path{
        key = {
            ig_md.packet_type: exact;
            // ig_md.delta_CE: ternary;
            ig_md.before_goodCE: exact;
            ig_md.thresold_CE: exact;
        }
        actions = {
            use_best_path;
            use_good_path;
        }
        default_action = use_best_path;
        size = 65536;
    }
    
    //ECMP
    ActionProfile(2048) ipv4_ecmp_ap;
    Hash<bit<8>>(HashAlgorithm_t.CRC16) hash_fn;
    ActionSelector(action_profile = ipv4_ecmp_ap, hash = hash_fn, mode = SelectorMode_t.FAIR, max_group_size = 256,num_groups = 2048) outPort_sel;
    // 注意，这里现在使用的编译器有bug，仅仅修改meta data里的数据时会报错，似乎是因为被错误识别为死代码
    action set_port_ecmp_data(portid_t port, caver_port_t caver_port) {
        ig_tm_md.ucast_egress_port = port;
        ig_md.dre_port = caver_port;
    }
    action set_port_ecmp_ack(portid_t port, caver_port_t caver_port) {
        ig_tm_md.ucast_egress_port = port;
    }
    action resubmit(){
        ig_dprsr_md.resubmit_type = 1;
    }

    table ipv4_lpm {
    key = {
        hdr.ipv4.dst_ip: exact;
        ig_md.packet_type:exact;
        hdr.ipv4.src_ip: selector;
        hdr.ipv4.dst_ip: selector;
        hdr.udp.src_port: selector;
        hdr.udp.dst_port: selector;
        hdr.ipv4.protocol: selector;
    }
        actions = { 
            set_port_ecmp_data;
            set_port_ecmp_ack; 
            resubmit;
            drop; 
        }
        implementation = outPort_sel;
        default_action = drop;
        size = 256;
    }
    action traversePort(portid_t port){
        ig_tm_md.ucast_egress_port = port;
    }
    table caverPort2switchPort{
        key = {
            ig_md.dre_port: exact;
        }
        actions = {
            traversePort;
            drop;
        }
        default_action = drop;
        size = 256;
    }
    //PathTable 
    Register<bit<8>, bit<8>>(8) stack_count_reg; 
    RegisterAction<bit<8>, bit<8>, bit<8>>(stack_count_reg) stack_count_pop_action = {
        void apply(inout bit<8> val, out bit<8> rv) {
            if (val == 0){
                rv = 0;
            }
            else{
                val = val - 1;
                rv = 1;
            }
        }
    };
    RegisterAction<bit<8>, bit<8>, bit<8>>(stack_count_reg) stack_count_push_action = {
        void apply(inout bit<8> val, out bit<8> rv) {
            if (val < 4){
                val = val + 1;
            }
            // rv = val; //push以后的count数量
        }
    };
    action stack_count_push(bit<8> host_id){
        stack_count_push_action.execute(host_id);
    }
    action stack_count_pop(bit<8> host_id){
        ig_md.src_route = (bit<1>)stack_count_pop_action.execute(host_id);
    }
    //简介：
    //处理的包类型：
    //caver_ack_second: 
    //data（and new flow）:
    //host_id:table_ip对应的id； 

    table stack_count_table{
        key = {
            ig_md.packet_type: exact;
            ig_md.new_flow: exact;
            ig_md.table_ip: exact;
        }
        actions = {
            stack_count_push;
            stack_count_pop;
            no_action;
        }
        default_action = no_action;
        size = 32;
    }
    Register<bit<8>, bit<8>>(8) stack_top_reg; //ce_path
    RegisterAction<bit<8>, bit<8>, bit<8>>(stack_top_reg) stack_top_pop_action = {
        void apply(inout bit<8> val, out bit<8> rv) {
           if(val == 0){
                val = 3;
           }else{
            val = val - 1;
            }
            rv = val;
        }
    };
    RegisterAction<bit<8>, bit<8>, bit<8>>(stack_top_reg) stack_top_push_action = {
        void apply(inout bit<8> val, out bit<8> rv) {
           rv = val;
           if (val == 3){
                val = 0;
           }else{
                val = val + 1;
           }
        }
    };

    action stack_top_pop(bit<8> host_id){
        ig_md.delta_index = stack_top_pop_action.execute(host_id);

    }
    action stack_top_push(bit<8> host_id){
        ig_md.delta_index = stack_top_push_action.execute(host_id);
    }
    //简介：
    //处理的包类型：
    //caver_ack_second:(every):stack_top_push;
    //data（and new flow and_src_route）:stack_top_pop;
    //host_id:table_ip对应的id； 
    table stack_top_table{
        key = {
            ig_md.packet_type: exact;
            ig_md.src_route: exact;
            ig_md.new_flow: exact;
            ig_md.table_ip: exact;
        }
        actions = {
            stack_top_pop;
            stack_top_push;
            no_action;
        }
        default_action = no_action;
        size = 64;
    }
    Register<bit<16>, bit<8>>(32) path_reg; //hop1_hop0
    RegisterAction<bit<16>, bit<8>, bit<16>>(path_reg) path_take_action = {
        void apply(inout bit<16> val, out bit<16> rv) {
            rv = val;
        }
    };
    RegisterAction<bit<16>, bit<8>, bit<16>>(path_reg) path_push_action = {
        void apply(inout bit<16> val, out bit<16> rv) {
            val = ig_md.new_path;
        }
    };
    // action path_take(bit<8> host_id, bit<8> base){
    action path_take(bit<8> index){
        // ig_md.index[7:2] = base[7:2];
        // ig_md.index[1:0] = ig_md.delta_index[1:0];
        ig_md.path_choice = path_take_action.execute(index);
        ig_md.dre_port = ig_md.path_choice[7:0];
        // hdr.caver_data.setValid();
        // hdr.caver_data.src_route = (bit<8>)ig_md.src_route;
        // hdr.caver_data.out_port = ig_md.path_choice[15:8];

    }
    // action path_update(bit<8> host_id, bit<8> base){
    action path_update(bit<8> index){
        // ig_md.index[7:2] = base[7:2];
        // ig_md.index[1:0] = ig_md.delta_index[1:0];
        path_push_action.execute(index);
    }

    //简介：
    //处理的包类型：
    //caver_ack_second:(every):path_update;
    //data（and new flow and_src_route）:path_take;
    //host_id:table_ip对应的id； 
    //base: = host_id * 4;
    table path_table{
        key = {
            ig_md.packet_type: exact;
            ig_md.src_route: exact;
            ig_md.new_flow: exact;
            ig_md.table_ip: exact;
            ig_md.delta_index: exact;
        }
        actions = {
            path_take;
            path_update;
            no_action;
        }
        default_action = no_action;
        size = 1024;
    }
    //Dre Table 
    Register<bit<32>, caver_port_t>(8, 0) Dre_time_reg;
    RegisterAction<bit<32>, caver_port_t, bit<8>>(Dre_time_reg) Dre_time_update_action = {
        void apply(inout bit<32> val, out bit<8> rv) {
            bit<32> current_time = ig_prsr_md.global_tstamp[41:10];
            if (current_time - val > dre_time){
                rv = 1;
                val = current_time;
            }
            else{
                rv = 0;
            }
        }
    };
    action Dre_time_update(){
        ig_md.dre_decrease = Dre_time_update_action.execute(ig_md.dre_port);
    }
    //简介：
    //处理的包类型：
    //data: key:bit<4> packet_type;
    //caver_data: key:bit<4> packet_type;
    table Dre_time_table{
        key = {
            ig_md.packet_type: exact;
        }
        actions = {
            Dre_time_update;
            no_action;
        }
        default_action = no_action;
        size = 16;
    }
    Register<bit<32>, bit<8>>(1) Dre_divide_reg; //ce_path
    MathUnit<bit<32>>(MathOp_t.MUL, 1, 12400) Dre_divide_MUL;
    RegisterAction<bit<32>, bit<8>, bit<16>>(Dre_divide_reg) Dre_divide_action = {
        void apply(inout bit<32> val, out bit<16> rv) {
            val = Dre_divide_MUL.execute(ig_md.resubmit_hdr.Dre);
            rv = (bit<16>) val;
            // if(val > 64){
            //     rv = 64;
            // }
            // else{
            //     rv = (bit<16>) val;
            // }
        }
    };
    action path_prepare(){
        ig_md.CE = Dre_divide_action.execute(0);
        ig_md.before_best_CE_and_Path[7:0] = ig_md.resubmit_hdr.port;
        ig_md.before_best_CE_and_Path[15:8] = hdr.caver_ack.best_port;
        ig_md.before_good_Path[7:0] = ig_md.resubmit_hdr.port;
        ig_md.before_good_Path[15:8] = hdr.caver_ack.good_port;
        ig_md.before_goodCE = max(ig_md.CE, hdr.caver_ack.good_CE);

    }
    table ACK_prepare{
        key = {
            ig_md.packet_type: exact;
        }
        actions = {
            path_prepare;
            no_action;
        }
        default_action = no_action;
        size = 16;
    }

    Register<bit<32>, bit<8>>(8, 0) Dre_value_reg;
    MathUnit<bit<32>>(MathOp_t.MUL, 3, 4) Dre_MUL;
    RegisterAction<bit<32>, bit<8>, bit<32>>(Dre_value_reg) Dre_value_update_action = {
        void apply(inout bit<32> val, out bit<32> rv) {
            if (ig_md.dre_decrease == 1){
                val = Dre_MUL.execute(val);
            }
            else{
                val = val + (bit<32>)hdr.udp.hdr_length;
            }
        }
    };
    RegisterAction<bit<32>, bit<8>, bit<32>>(Dre_value_reg) Dre_value_read_action = {
        void apply(inout bit<32> val, out bit<32> rv) {
            rv = val;
        }
    };
    action data_dre(){
        Dre_value_update_action.execute(ig_md.dre_port);
    }
    action ack_dre(){
        // ig_dprsr_md.resubmit_type = 1;
        ig_md.resubmit_hdr.Dre = Dre_value_read_action.execute(ig_md.dre_port);
        ig_md.resubmit_hdr.port = ig_md.dre_port;
    }
    table Dre_table{
        key = {
            ig_md.packet_type: exact;
        }
        actions = {
            data_dre;
            ack_dre;
            no_action;
        }
        default_action = no_action;
        size = 16;
    }
    action inPortToCaverPort_action(caver_port_t port){
        ig_md.dre_port = port;

    }
    //简介：
    //处理的包类型：
    //caver_ACK(first): key:bit<9> ingress port,
    //ig_intr_md.resubmit_flag = 1;
    //
    table inPort2caverPort{
        key = {
            ig_intr_md.ingress_port: exact;
        }
        actions = {
            inPortToCaverPort_action;
            drop;
        }
        size = 256;
        default_action = drop;
    }
    action in_coming_data(){
        //这部分逻辑在path_take中实现
        hdr.infiniband.reserved0 = 1;
        hdr.caver_data.setValid();
        hdr.caver_data.src_route = (bit<8>)ig_md.src_route;
        hdr.caver_data.out_port = ig_md.path_choice[15:8];
    }
    action in_coming_ack(){
        hdr.caver_ack.setValid();
        hdr.caver_ack.best_port = 0;
        hdr.caver_ack.good_port = 0;
        hdr.caver_ack.best_CE = 0;
        hdr.caver_ack.good_CE = 0;

        hdr.infiniband_ack.reserved = 1;

    }
    action in_coming_caver_data(){
        hdr.caver_data.setInvalid();

        hdr.infiniband.reserved0 = 0;
    }
    action in_coming_caver_ack(){
        hdr.caver_ack.setInvalid();

        hdr.infiniband_ack.reserved = 0;
    }
    //简介：
    //处理的包类型：
    //data: in_coming_data;
    //ack: in_coming_ack
    //caver_data: in_coming_caver_data;
    //caver_ack_second: in_coming_caver_ack;

    table header_process{
        key = {
            ig_md.packet_type: exact;
        }
        actions = {
            in_coming_data;
            in_coming_ack;
            in_coming_caver_data;
            in_coming_caver_ack;
            no_action;
        }
        default_action = no_action;
        size = 256;
    }
    action table_ip_is_src_ip(){
        ig_md.table_ip = hdr.ipv4.src_ip;
    }
    action table_ip_is_dst_ip(){
        ig_md.table_ip = hdr.ipv4.dst_ip;
    }
    //简介：
    //处理的包类型：
    //data: table_ip_is_dst_ip;
    //ack: table_ip_is_src_ip;
    //caver_data: table_ip_is_dst_ip;
    //caver_ack_first: table_ip_is_src_ip;
    //caver_ack_second: table_ip_is_src_ip;
    table get_table_ip{
        key = {
            ig_md.packet_type: exact;
        }
        actions = {
            table_ip_is_src_ip;
            table_ip_is_dst_ip;
            no_action;
        }
        default_action = no_action;
        size = 256;
    }
    
    apply {
        if (ig_md.resubmit_hdr.port == ig_md.resubmit_hdr.best_port){
            ig_md.port_equal = 1;
        }
        else{
            ig_md.port_equal = 0;
        }
        get_table_ip.apply();
        // if(ig_intr_md.resubmit_flag == 1){
            ACK_prepare.apply();
            ig_md.before_best_CE_and_Path[31:16] = max(ig_md.CE, hdr.caver_ack.best_CE);
            BestTable.apply();
            judge_acceptable_table.apply();
            // ig_md.delta_CE = ig_md.before_goodCE - ig_md.thresold_CE;
            // ig_md.acceptable = ig_md.delta_CE[15:15];
            // ig_md.delta_CE = max(ig_md.before_goodCE, ig_md.thresold_CE);
            // if (ig_md.delta_CE == ig_md.before_goodCE){
            //     ig_md.acceptable = 0;
            // }
            // else{
            //     ig_md.acceptable = 1;
            // }
            set_acceptable_path.apply();
        // }
        // else{
            inPort2caverPort.apply();
        // }
        // if (ig_md.packet_type == DATA || ig_intr_md.resubmit_flag == 1)
        // {
            flow_id_table.apply();
            flow_time_table.apply();
            stack_count_table.apply();
            stack_top_table.apply();
            path_table.apply();
            flow_method.apply();
            flow_path.apply();
        // }
        if (ig_md.packet_type == DATA && ig_md.src_route == 1){
            caverPort2switchPort.apply();
        }
        else{
            ipv4_lpm.apply();
        }
        header_process.apply();
        // if(ig_intr_md.resubmit_flag == 0){
            Dre_time_table.apply();
            Dre_table.apply();
        // }
    }






    //     get_table_ip.apply();
    //     if(ig_intr_md.resubmit_flag == 1){
    //         ig_md.before_best_CE_and_Path[7:0] = ig_md.resubmit_hdr.port;
    //         ig_md.before_best_CE_and_Path[15:8] = hdr.caver_ack.best_port;
    //         ig_md.before_best_CE_and_Path[31:16] = max(ig_md.resubmit_hdr.CE, hdr.caver_ack.best_CE);
    //         ig_md.before_good_Path[7:0] = ig_md.resubmit_hdr.port;
    //         ig_md.before_good_Path[15:8] = hdr.caver_ack.good_port;
    //         ig_md.before_goodCE = max(ig_md.resubmit_hdr.CE, hdr.caver_ack.good_CE);

    //         BestTable.apply();

    //         judge_acceptable_table.apply();
    //         bit<16>temp = ig_md.before_goodCE - ig_md.thresold_CE;
    //         ig_md.acceptable = temp[15:15];
    //         set_acceptable_path.apply();

    //     }
    //     else{
    //         inPort2caverPort.apply();
    //     }
    //     if (ig_md.packet_type == DATA || ig_intr_md.resubmit_flag == 1)
    //     {
    //         flow_time_table.apply();
    //         stack_count_table.apply();
    //         stack_top_table.apply();
    //         path_table.apply();
    //         flow_method.apply();
    //         flow_path.apply();
    //     }
    //     if (ig_md.packet_type == DATA && ig_md.src_route == 1){
    //         caverPort2switchPort.apply();
    //     }
    //     else{
    //         ipv4_lpm.apply();
    //     }
    //     header_process.apply();
    //     if(ig_intr_md.resubmit_flag == 0){
    //         Dre_time_table.apply();
    //         Dre_table.apply();
    //     }
    // }
}

control Caver_ToR_Deparser(packet_out pkt,
        inout header_t hdr,
        in metadata_t ig_md,
        in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {
    Resubmit() resubmit;
    apply {
        if(ig_dprsr_md.resubmit_type == 1){
            resubmit.emit(ig_md.resubmit_hdr);
        }
        pkt.emit(hdr);
    }
}


parser ECNParser(
        packet_in pkt,
        out header_t hdr,
        out egress_metadata_t eg_md,
        out egress_intrinsic_metadata_t eg_intr_md){

    TofinoEgressParser() tofino_parser;

    state start {        
        pkt.extract(eg_intr_md);
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
            default: accept;
        }
    }

    state parse_udp{
        pkt.extract(hdr.udp);
        transition accept;
    }

}


control ECNEgress(
        inout header_t hdr,
        inout egress_metadata_t eg_md,
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_intr_prsr_md,
        inout egress_intrinsic_metadata_for_deparser_t eg_intr_dprsr_md,
        inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md) {
    
	action dcqcn_mark_probability(bit<8> value) {
		eg_md.dcqcn_prob_output = value;
	}

	table dcqcn_get_ecn_probability {
		key = {
			eg_intr_md.deq_qdepth : range; // 19 bits
		}
		actions = {
			dcqcn_mark_probability;
		}
		const default_action = dcqcn_mark_probability(0); // default: no ecn mark
		size = 1024;
	}
	Random<bit<8>>() random;  // random seed for sampling
	action dcqcn_get_random_number(){
		eg_md.dcqcn_random_number = random.get();
	}

	action nop(){}

    action dcqcn_check_ecn_marking() {
		hdr.ipv4.ecn = 0b11;
        hdr.ipv4.checksum = hdr.ipv4.checksum - 1;
	}
	
	table dcqcn_compare_probability {
		key = {
			eg_md.dcqcn_prob_output : exact;
			eg_md.dcqcn_random_number : exact;
		}
		actions = {
			dcqcn_check_ecn_marking;
			@defaultonly nop;
		}
		const default_action = nop();
		size = 65536;
	}

    apply {
        if(hdr.ipv4.ecn == 0b01 || hdr.ipv4.ecn == 0b10){
            if(hdr.udp.isValid()){
                dcqcn_get_ecn_probability.apply();
                dcqcn_get_random_number();
                dcqcn_compare_probability.apply();
            }
        }
    }
}

Pipeline(Caver_ToR_Parser(),
         SwitchIngress(),
         Caver_ToR_Deparser(),
         ECNParser(),
         ECNEgress(),
         SwitchEgressDeparser()) pipe;

Switch(pipe) main;
