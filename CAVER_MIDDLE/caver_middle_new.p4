#include <core.p4>
#include <tna.p4>

#include "header.p4"
#include "parser.p4"
#include "action.p4"
#include "table.p4"

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
    // // 功能：将switch的ingress port id转换成为caver_port_t
    action inPortToCaverPort_action(caver_port_t port){
        ig_md.dre_port = port;
        // ig_md.port_equal = ig_md.dre_port - ig_md.resubmit_hdr.best_port;
        ig_md.before_best_CE_and_Path[7:0] = ig_md.dre_port;
        ig_md.before_good_CE_and_Path[7:0] = ig_md.dre_port;
        ig_md.before_best_CE_and_Path[15:8] = 0;
        ig_md.before_good_CE_and_Path[15:8] = 0;
    }
    table inPort2caverPort{
        key = {
            ig_intr_md.ingress_port: exact;
            ig_md.packet_type: exact;
        }
        actions = {
            inPortToCaverPort_action;
            no_action;
        }
        size = 256;
        default_action = no_action;
    }

    action traversePort(portid_t port){
        ig_tm_md.ucast_egress_port = port;
    }
    table caverPort2switchPort{
        key = {
            ig_md.src_route: exact;
            hdr.caver_data.out_port: exact;
        }
        actions = {
            traversePort;
            no_action;
        }
        default_action = no_action;
        size = 256;
    }


    //Dre Table 
    Register<bit<32>, caver_port_t>(4, 0) Dre_time_reg;
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

    Register<bit<32>, bit<8>>(4, 0) Dre_value_reg;
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
        ig_md.Dre = Dre_value_read_action.execute(ig_md.dre_port);
        ig_md.CE = (bit<16>)(ig_md.Dre >> 12);
        ig_md.before_best_CE_and_Path[31:16] = ig_md.CE;
        ig_md.before_good_CE_and_Path[31:16] = ig_md.CE;
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

    Register<bit<32>, bit<8>>(1) Dre_divide_reg; //ce_path
    MathUnit<bit<32>>(MathOp_t.MUL, 1, 8192) Dre_divide_MUL;
    RegisterAction<bit<32>, bit<8>, bit<16>>(Dre_divide_reg) Dre_divide_action = {
        void apply(inout bit<32> val, out bit<16> rv) {
            val = Dre_divide_MUL.execute(ig_md.Dre);
            rv = (bit<16>) val;
        }
    };
    action path_prepare(){
        ig_md.CE = Dre_divide_action.execute(0);
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
        hdr.caver_ack.best_port = ig_md.after_best_CE_and_Path[7:0];
        hdr.caver_ack.best_CE = ig_md.after_best_CE_and_Path[31:16];
    }
    action update_BestTable(bit<8> host){
        ig_md.after_best_CE_and_Path= BestTable_update_action.execute(host);
        ig_md.after_best_CE = ig_md.after_best_CE_and_Path[31:16];
        ig_md.after_best_Path = ig_md.after_best_CE_and_Path[15:0];
        hdr.caver_ack.best_port = ig_md.after_best_CE_and_Path[7:0];
        hdr.caver_ack.best_CE = ig_md.after_best_CE_and_Path[31:16];
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
            val = TH_MUL.execute(ig_md.after_best_CE);
            rv = val;
        }
    };
    action judge_acceptable(){
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

    action use_best_path(){
        ig_md.new_path = ig_md.after_best_CE_and_Path;
    }
    action use_good_path(){
        ig_md.new_path = ig_md.before_good_CE_and_Path;
    }
    //简介：
    //处理的包类型：
    //caver_ack_second: acceptable == 1->use_good_path; acceptable == 0->use_best_path
    table set_acceptable_path{
        key = {
            ig_md.packet_type: exact;
            // ig_md.acceptable: exact;
            // ig_md.delta_CE: ternary;
            ig_md.CE : exact;
            ig_md.thresold_CE : exact;
        }
        actions = {
            use_best_path;
            use_good_path;
            // no_action;
        }
        // const entries = {
        //     (CAVER_ACK_second, 	_, _, 0 .. 65535, 0 .. 65535) : ae_read_lock_flag();

        // }
        default_action = use_best_path;
        size = 65535;
    }

    Register<bit<32>, bit<8>>(8) GoodTable_reg; //ce_path
    RegisterAction<bit<32>, bit<8>, bit<32>>(GoodTable_reg) GoodTable_update_action = {
        void apply(inout bit<32> val, out bit<32> rv) {
           rv = val;
           val = ig_md.new_path;
        }
    };
    action update_good_GoodTable(bit<8> host){
        ig_md.after_good_CE_and_Path = GoodTable_update_action.execute(host);
        hdr.caver_ack.good_port = ig_md.after_good_CE_and_Path[7:0];
        hdr.caver_ack.good_CE = ig_md.after_good_CE_and_Path[31:16];
    }
    table GoodTable{
        key = {
            hdr.ipv4.src_ip: exact;
            ig_md.packet_type: exact;
        }
        actions = {
            update_good_GoodTable;
            no_action;
        }
        default_action = no_action;
        size = 1024;
    }



    // 执行结束后效果： 若为ACK，则meta的port内存的是ingress对应的8位port_id，否则未定义；
    apply {
            if(hdr.caver_data.isValid()){
                ig_md.dre_port = hdr.caver_data.out_port;
                ig_md.src_route = hdr.caver_data.src_route;
            }
            caverPort2switchPort.apply();
            if(ig_md.src_route == 0){
                ipv4_lpm.apply();
            }
            inPort2caverPort.apply();
            if(ig_md.dre_port == ig_md.resubmit_hdr.best_port){
                ig_md.port_equal = 1;
            }
            else{
                ig_md.port_equal = 0;
            }
            Dre_time_table.apply();
            Dre_table.apply();
            BestTable.apply();
            judge_acceptable_table.apply();
            // ig_md.delta_CE = ig_md.CE - ig_md.thresold_CE;
            // ig_md.delta_CE = max(ig_md.CE, ig_md.thresold_CE);
            // if (ig_md.delta_CE == ig_md.CE){
            //     ig_md.acceptable = 0;
            // }
            // else{
            //     ig_md.acceptable = 1;
            // }
            set_acceptable_path.apply();
            GoodTable.apply();
    }
}

control Caver_Middle_Deparser(packet_out pkt,
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


Pipeline(Caver_Middle_Parser(),
         SwitchIngress(),
         Caver_Middle_Deparser(),
         ECNParser(),
         ECNEgress(),
         SwitchEgressDeparser()) pipe;

Switch(pipe) main;
