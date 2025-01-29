#ifndef _SWITCH_CONFIG_H
#define _SWITCH_CONFIG_H

#if __TOFINO_MODE__ == 0
///////////////////////////////////// caver related/////////////////////////////////// 
#define m_Other          0
#define m_DATA           1
#define m_ACK            2
#define m_CAVER_DATA     3
#define m_CAVER_ACK_FIRST 4
#define m_CAVER_ACK_SECOND 5

#define CE_max 64
/////////ip_2_id/////////
typedef struct{
    char* ip;
    uint64_t id;
}ip_id_t;
const ip_id_t ip_id_list[] = {
    {"10.0.0.1", 0}, {"10.0.0.2", 1}, {"10.0.0.3", 2}, {"10.0.0.4", 3},
    {"10.0.0.5", 4}, {"10.0.0.6", 5}, {"10.0.0.7", 6}, {"10.0.0.8", 7} 
};
const size_t IP_ID_SIZE = sizeof(ip_id_list) / sizeof(ip_id_t);

////////inPort2caverPort/////////
typedef struct {
    uint64_t caver_port;     
    uint64_t dp_port; 
} caver_dp_port_t;
const caver_dp_port_t caver_port_list_0[] = {
    {0, 52}, {1, 36}, {2, 20}, {3, 4}, {4, 8}, {5, 16}, {6, 32}, {7, 48}
};
const caver_dp_port_t caver_port_list_1[] = {
    {0,136}, {1, 152}, {2, 168},{3, 184}, {4, 188}, {5, 172}, {6, 148}, {7, 132}
};
const caver_dp_port_t* caver_port_lists[] = {caver_port_list_0, caver_port_list_1};
const size_t CAVER_PORT_SIZES[] = {sizeof(caver_port_list_0) / sizeof(caver_dp_port_t), sizeof(caver_port_list_1) / sizeof(caver_dp_port_t)};
/////////ECMP related/////////
///储存的每个group包含的memeber，其中，（0-7）以及（8-15）member对应的是caver_port_list_0或1中的index对应的端口，分别对应的action为data和ack，16member表示的是resubmit

#define MAX_NESTED_LISTS 11

static const uint32_t nested_list_0[] = {0};
static const uint32_t nested_list_1[] = {1};
static const uint32_t nested_list_2[] = {2};
static const uint32_t nested_list_3[] = {3};
static const uint32_t nested_list_4[] = {4, 5, 6, 7};
static const uint32_t nested_list_5[] = {8};
static const uint32_t nested_list_6[] = {9};
static const uint32_t nested_list_7[] = {10};
static const uint32_t nested_list_8[] = {11};
static const uint32_t nested_list_9[] = {12, 13, 14, 15};
static const uint32_t nested_list_10[] = {16};

static const uint32_t* nested_list[MAX_NESTED_LISTS] = {
    nested_list_0, nested_list_1, nested_list_2, nested_list_3,
    nested_list_4, nested_list_5, nested_list_6, nested_list_7,
    nested_list_8, nested_list_9, nested_list_10
};

static const size_t nested_list_sizes[MAX_NESTED_LISTS] = {
    sizeof(nested_list_0) / sizeof(uint32_t),
    sizeof(nested_list_1) / sizeof(uint32_t),
    sizeof(nested_list_2) / sizeof(uint32_t),
    sizeof(nested_list_3) / sizeof(uint32_t),
    sizeof(nested_list_4) / sizeof(uint32_t),
    sizeof(nested_list_5) / sizeof(uint32_t),
    sizeof(nested_list_6) / sizeof(uint32_t),
    sizeof(nested_list_7) / sizeof(uint32_t),
    sizeof(nested_list_8) / sizeof(uint32_t),
    sizeof(nested_list_9) / sizeof(uint32_t),
    sizeof(nested_list_10) / sizeof(uint32_t)
};


static const bool nested_bool_list_0[] = {true};
static const bool nested_bool_list_1[] = {true};
static const bool nested_bool_list_2[] = {true};
static const bool nested_bool_list_3[] = {true};
static const bool nested_bool_list_4[] = {true, true, true, true};
static const bool nested_bool_list_5[] = {true};
static const bool nested_bool_list_6[] = {true};
static const bool nested_bool_list_7[] = {true};
static const bool nested_bool_list_8[] = {true};
static const bool nested_bool_list_9[] = {true, true, true, true};
static const bool nested_bool_list_10[] = {true};

static const bool* nested_bool_list[MAX_NESTED_LISTS] = {
    nested_bool_list_0, nested_bool_list_1, nested_bool_list_2, nested_bool_list_3,
    nested_bool_list_4, nested_bool_list_5, nested_bool_list_6, nested_bool_list_7,
    nested_bool_list_8, nested_bool_list_9, nested_bool_list_10
};

static const size_t nested_bool_list_sizes[MAX_NESTED_LISTS] = {
    sizeof(nested_bool_list_0) / sizeof(bool),
    sizeof(nested_bool_list_1) / sizeof(bool),
    sizeof(nested_bool_list_2) / sizeof(bool),
    sizeof(nested_bool_list_3) / sizeof(bool),
    sizeof(nested_bool_list_4) / sizeof(bool),
    sizeof(nested_bool_list_5) / sizeof(bool),
    sizeof(nested_bool_list_6) / sizeof(bool),
    sizeof(nested_bool_list_7) / sizeof(bool),
    sizeof(nested_bool_list_8) / sizeof(bool),
    sizeof(nested_bool_list_9) / sizeof(bool),
    sizeof(nested_bool_list_10) / sizeof(bool)
};

const uint32_t* get_list(size_t index, size_t* size) {
    *size = nested_list_sizes[index]; // 返回当前列表大小
    return nested_list[index]; // 返回指向数组的指针
}

const bool* get_bool_list(size_t index, size_t* size) {
    *size = nested_bool_list_sizes[index]; // 返回布尔列表的大小
    return nested_bool_list[index]; // 返回指向布尔数组的指针
}


const ip_id_t ip_group_data_list_0[] = {
    {"10.0.0.1", 0}, {"10.0.0.2", 1}, {"10.0.0.3", 2}, {"10.0.0.4", 3},
    {"10.0.0.5", 4}, {"10.0.0.6", 4}, {"10.0.0.7", 4}, {"10.0.0.8", 4} 
};
const ip_id_t ip_group_data_list_1[] = {
    {"10.0.0.1", 4}, {"10.0.0.2", 4}, {"10.0.0.3", 4}, {"10.0.0.4", 4},
    {"10.0.0.5", 0}, {"10.0.0.6", 1}, {"10.0.0.7", 2}, {"10.0.0.8", 3} 
};
const ip_id_t* ip_group_data_lists[] = {ip_group_data_list_0, ip_group_data_list_1};

const ip_id_t ip_group_ack_list_0[] = {
    {"10.0.0.1", 5}, {"10.0.0.2", 6}, {"10.0.0.3", 7}, {"10.0.0.4", 8},
    {"10.0.0.5", 9}, {"10.0.0.6", 9}, {"10.0.0.7", 9}, {"10.0.0.8", 9} 
};
const ip_id_t ip_group_ack_list_1[] = {
    {"10.0.0.1", 9}, {"10.0.0.2", 9}, {"10.0.0.3", 9}, {"10.0.0.4", 9},
    {"10.0.0.5", 5}, {"10.0.0.6", 6}, {"10.0.0.7", 7}, {"10.0.0.8", 8} 
};
const ip_id_t* ip_group_ack_lists[] = {ip_group_ack_list_0, ip_group_ack_list_1};
const size_t ip_SIZE = sizeof(ip_group_ack_list_1) / sizeof(ip_id_t);


// /////ECN related///////
#define DCQCN_K_MIN 1250         // 100KB
#define DCQCN_K_MAX 3000         // 240KB
#define DCQCN_P_MAX 0.2          // 20%
#define QDEPTH_RANGE_MAX (1 << 19)
#define SEED_RANGE_MAX 256       // Random number range ~ [0, 255] (8 bits)
#define SEED_K_MAX ((int)(DCQCN_P_MAX * SEED_RANGE_MAX)) // 52
#define QDEPTH_STEPSIZE ((DCQCN_K_MAX - DCQCN_K_MIN) / SEED_K_MAX) // 72



//init_BestTable
const ip_id_t init_path_0[] = {
    {"10.0.0.1", 0}, {"10.0.0.2", 0}, {"10.0.0.3", 0}, {"10.0.0.4", 0},
    {"10.0.0.5", 516}, {"10.0.0.6", 773}, {"10.0.0.7", 518}, {"10.0.0.8", 775} 
};
const ip_id_t init_path_1[] = {
    {"10.0.0.1", 4}, {"10.0.0.2", 261}, {"10.0.0.3", 6}, {"10.0.0.4", 263},
    {"10.0.0.5", 0}, {"10.0.0.6", 0}, {"10.0.0.7", 0}, {"10.0.0.8", 0} 
};
const ip_id_t* init_path_lists[] = {init_path_0, init_path_1};
const size_t INIT_PATH_SIZES[] = {sizeof(init_path_0) / sizeof(ip_id_t), sizeof(init_path_1) / sizeof(ip_id_t)};





const switch_port_t PORT_LIST_UP[] = {
    {"18/0"}, {"20/0"}, {"22/0"}, {"24/0"}, // pipe 0 
    {"26/0"}, {"28/0"}, {"30/0"}, {"32/0"},  // pipe 1
};

const switch_port_t PORT_LIST_DOWN[] = {
    {"10/0"}, {"12/0"}, {"14/0"}, {"16/0"}, // pipe 0 
    {"2/0"}, {"4/0"}, {"6/0"}, {"8/0"},  // pipe 1
};

const switch_port_t PORT_LIST[] = {
	{"10/0"}, {"12/0"}, {"14/0"}, {"16/0"}, {"18/0"}, {"20/0"}, {"22/0"}, {"24/0"}, // pipe 0 
	{"2/0"}, {"4/0"}, {"6/0"}, {"8/0"}, {"26/0"}, {"28/0"}, {"30/0"}, {"32/0"},  // pipe 1
};
const switch_port_t PORT_LIST_A[] = {
	{"10/0"}, {"12/0"}, {"14/0"}, {"16/0"}, {"18/0"}, {"20/0"}, {"22/0"}, {"24/0"}, // pipe 0 
};
const uint16_t DEV_PORT_LIST_A[] = {
	52, 36, 20, 4, 8, 16, 32, 48,
};
const switch_port_t PORT_LIST_B[] = {
	{"2/0"}, {"4/0"}, {"6/0"}, {"8/0"}, {"26/0"}, {"28/0"}, {"30/0"}, {"32/0"},  // pipe 1
};
const uint16_t DEV_PORT_LIST_B[] = {
	136, 152, 168, 184, 188, 172, 148, 132,
};

const forward_entry_t FORWARD_LIST[] = {
    {44, "172.17.1.106", 36},
    {44, "172.17.2.103", 52},
    {44, "172.17.3.104", 52},
    {44, "172.17.2.108", 52},
    {44, "172.17.3.109", 52},
    {36, "172.17.1.101", 44},
    {36, "172.17.2.103", 52},
    {36, "172.17.3.104", 52},
    {36, "172.17.2.108", 52},
    {36, "172.17.3.109", 52},
    {52, "172.17.1.101", 44},
    {52, "172.17.1.106", 36},
    {132, "172.17.3.104", 148},
    {132, "172.17.2.108", 140},
    {132, "172.17.3.109", 156},
    {132, "172.17.1.101", 164},
    {132, "172.17.1.106", 164},
    {140, "172.17.2.103", 132},
    {140, "172.17.3.104", 148},
    {140, "172.17.3.109", 156},
    {140, "172.17.1.101", 164},
    {140, "172.17.1.106", 164},
    {148, "172.17.2.103", 132},
    {148, "172.17.2.108", 140},
    {148, "172.17.3.109", 156},
    {148, "172.17.1.101", 164},
    {148, "172.17.1.106", 164},
    {156, "172.17.2.103", 132},
    {156, "172.17.3.104", 148},
    {156, "172.17.2.108", 140},
    {156, "172.17.1.101", 164},
    {156, "172.17.1.106", 164},
    {164, "172.17.2.103", 132},
    {164, "172.17.2.108", 140},
    {164, "172.17.3.104", 148},
    {164, "172.17.3.109", 156},
};
const forward_polling_entry_t FORWARD_POLLING_LIST[] = {
    {1, 44, "172.17.1.106", "ai_unicast_polling", 36, 0},
    {3, 44, "172.17.1.106", "ai_broadcast_polling", 36, POLLING_MC_GID_A},
    {1, 44, "172.17.2.103", "ai_unicast_polling", 52, 0},
    {3, 44, "172.17.2.103", "ai_broadcast_polling", 52, POLLING_MC_GID_A},
    {1, 44, "172.17.3.104", "ai_unicast_polling", 52, 0},
    {3, 44, "172.17.3.104", "ai_broadcast_polling", 52, POLLING_MC_GID_A},
    {1, 44, "172.17.2.108", "ai_unicast_polling", 52, 0},
    {3, 44, "172.17.2.108", "ai_broadcast_polling", 52, POLLING_MC_GID_A},
    {1, 44, "172.17.3.109", "ai_unicast_polling", 52, 0},
    {3, 44, "172.17.3.109", "ai_broadcast_polling", 52, POLLING_MC_GID_A},
    {1, 36, "172.17.1.101", "ai_unicast_polling", 44, 0},
    {3, 36, "172.17.1.101", "ai_broadcast_polling", 44, POLLING_MC_GID_A},
    {1, 36, "172.17.2.103", "ai_unicast_polling", 52, 0},
    {3, 36, "172.17.2.103", "ai_broadcast_polling", 52, POLLING_MC_GID_A},
    {1, 36, "172.17.3.104", "ai_unicast_polling", 52, 0},
    {3, 36, "172.17.3.104", "ai_broadcast_polling", 52, POLLING_MC_GID_A},
    {1, 36, "172.17.2.108", "ai_unicast_polling", 52, 0},
    {3, 36, "172.17.2.108", "ai_broadcast_polling", 52, POLLING_MC_GID_A},
    {1, 36, "172.17.3.109", "ai_unicast_polling", 52, 0},
    {3, 36, "172.17.3.109", "ai_broadcast_polling", 52, POLLING_MC_GID_A},
    {1, 52, "172.17.1.101", "ai_unicast_polling", 44, 0},
    {3, 52, "172.17.1.101", "ai_broadcast_polling", 44, POLLING_MC_GID_A},
    {1, 52, "172.17.1.106", "ai_unicast_polling", 36, 0},
    {3, 52, "172.17.1.106", "ai_broadcast_polling", 36, POLLING_MC_GID_A},
    {1, 132, "172.17.3.104", "ai_unicast_polling", 148, 0},
    {3, 132, "172.17.3.104", "ai_broadcast_polling", 148, POLLING_MC_GID_B},
    {1, 132, "172.17.2.108", "ai_unicast_polling", 140, 0},
    {3, 132, "172.17.2.108", "ai_broadcast_polling", 140, POLLING_MC_GID_B},
    {1, 132, "172.17.3.109", "ai_unicast_polling", 156, 0},
    {3, 132, "172.17.3.109", "ai_broadcast_polling", 156, POLLING_MC_GID_B},
    {1, 132, "172.17.1.101", "ai_unicast_polling", 164, 0},
    {3, 132, "172.17.1.101", "ai_broadcast_polling", 164, POLLING_MC_GID_B},
    {1, 132, "172.17.1.106", "ai_unicast_polling", 164, 0},
    {3, 132, "172.17.1.106", "ai_broadcast_polling", 164, POLLING_MC_GID_B},
    {1, 140, "172.17.2.103", "ai_unicast_polling", 132, 0},
    {3, 140, "172.17.2.103", "ai_broadcast_polling", 132, POLLING_MC_GID_B},
    {1, 140, "172.17.3.104", "ai_unicast_polling", 148, 0},
    {3, 140, "172.17.3.104", "ai_broadcast_polling", 148, POLLING_MC_GID_B},
    {1, 140, "172.17.3.109", "ai_unicast_polling", 156, 0},
    {3, 140, "172.17.3.109", "ai_broadcast_polling", 156, POLLING_MC_GID_B},
    {1, 140, "172.17.1.101", "ai_unicast_polling", 164, 0},
    {3, 140, "172.17.1.101", "ai_broadcast_polling", 164, POLLING_MC_GID_B},
    {1, 140, "172.17.1.106", "ai_unicast_polling", 164, 0},
    {3, 140, "172.17.1.106", "ai_broadcast_polling", 164, POLLING_MC_GID_B},
    {1, 148, "172.17.2.103", "ai_unicast_polling", 132, 0},
    {3, 148, "172.17.2.103", "ai_broadcast_polling", 132, POLLING_MC_GID_B},
    {1, 148, "172.17.2.108", "ai_unicast_polling", 140, 0},
    {3, 148, "172.17.2.108", "ai_broadcast_polling", 140, POLLING_MC_GID_B},
    {1, 148, "172.17.3.109", "ai_unicast_polling", 156, 0},
    {3, 148, "172.17.3.109", "ai_broadcast_polling", 156, POLLING_MC_GID_B},
    {1, 148, "172.17.1.101", "ai_unicast_polling", 164, 0},
    {3, 148, "172.17.1.101", "ai_broadcast_polling", 164, POLLING_MC_GID_B},
    {1, 148, "172.17.1.106", "ai_unicast_polling", 164, 0},
    {3, 148, "172.17.1.106", "ai_broadcast_polling", 164, POLLING_MC_GID_B},
    {1, 156, "172.17.2.103", "ai_unicast_polling", 132, 0},
    {3, 156, "172.17.2.103", "ai_broadcast_polling", 132, POLLING_MC_GID_B},
    {1, 156, "172.17.3.104", "ai_unicast_polling", 148, 0},
    {3, 156, "172.17.3.104", "ai_broadcast_polling", 148, POLLING_MC_GID_B},
    {1, 156, "172.17.2.108", "ai_unicast_polling", 140, 0},
    {3, 156, "172.17.2.108", "ai_broadcast_polling", 140, POLLING_MC_GID_B},
    {1, 156, "172.17.1.101", "ai_unicast_polling", 164, 0},
    {3, 156, "172.17.1.101", "ai_broadcast_polling", 164, POLLING_MC_GID_B},
    {1, 156, "172.17.1.106", "ai_unicast_polling", 164, 0},
    {3, 156, "172.17.1.106", "ai_broadcast_polling", 164, POLLING_MC_GID_B},
    {1, 164, "172.17.2.103", "ai_unicast_polling", 132, 0},
    {3, 164, "172.17.2.103", "ai_broadcast_polling", 132, POLLING_MC_GID_B},
    {1, 164, "172.17.2.108", "ai_unicast_polling", 140, 0},
    {3, 164, "172.17.2.108", "ai_broadcast_polling", 140, POLLING_MC_GID_B},
    {1, 164, "172.17.3.104", "ai_unicast_polling", 148, 0},
    {3, 164, "172.17.3.104", "ai_broadcast_polling", 148, POLLING_MC_GID_B},
    {1, 164, "172.17.3.109", "ai_unicast_polling", 156, 0},
    {3, 164, "172.17.3.109", "ai_broadcast_polling", 156, POLLING_MC_GID_B},
    {2, 52, "0.0.0.0", "ai_broadcast_polling", 511, POLLING_MC_GID_A},
    {2, 44, "0.0.0.0", "ai_broadcast_polling", 511, POLLING_MC_GID_A},
    {2, 36, "0.0.0.0", "ai_broadcast_polling", 511, POLLING_MC_GID_A},
    {2, 4, "0.0.0.0", "ai_broadcast_polling", 511, POLLING_MC_GID_A},
    {2, 164, "0.0.0.0", "ai_broadcast_polling", 511, POLLING_MC_GID_B},
    {2, 148, "0.0.0.0", "ai_broadcast_polling", 511, POLLING_MC_GID_B},
    {2, 156, "0.0.0.0", "ai_broadcast_polling", 511, POLLING_MC_GID_B},
    {2, 132, "0.0.0.0", "ai_broadcast_polling", 511, POLLING_MC_GID_B},
    {2, 140, "0.0.0.0", "ai_broadcast_polling", 511, POLLING_MC_GID_B},
};
#else
const switch_port_t  PORT_LIST[] = {
    {"1/0"}, {"2/0"}, {"3/0"}, 
    {"4/0"}, {"5/0"}, {"6/0"}, // {"7/0"}, {"8/0"}, // pipe of mode
};
const forward_entry_t FORWARD_LIST[] = {
//    {5, "172.17.3.104", 6},
//    {5, "172.17.2.108", 7},
//    {5, "172.17.3.109", 8},
//    {6, "172.17.2.103", 5},
//    {6, "172.17.2.108", 7},
//    {6, "172.17.3.109", 8},
//    {7, "172.17.2.103", 5},
//    {7, "172.17.3.104", 6},
//    {7, "172.17.3.109", 8},
//    {8, "172.17.2.103", 5},
//    {8, "172.17.3.104", 6},
//    {8, "172.17.2.108", 7},
//    {16, "172.17.3.104", 20},
//    {16, "172.17.2.108", 24},
//    {16, "172.17.3.109", 28},
//    {20, "172.17.2.103", 16},
//    {20, "172.17.2.108", 24},
//    {20, "172.17.3.109", 28},
//    {24, "172.17.2.103", 16},
//    {24, "172.17.3.104", 20},
//    {24, "172.17.3.109", 28},
//    {28, "172.17.2.103", 16},
//    {28, "172.17.3.104", 20},
//    {28, "172.17.2.108", 24},
    {0, "172.17.3.104", 4},
    {0, "172.17.2.108", 8},
    {0, "172.17.3.109", 12},
    {4, "172.17.2.103", 0},
    {4, "172.17.2.108", 8},
    {4, "172.17.3.109", 12},
    {8, "172.17.2.103", 0},
    {8, "172.17.3.104", 4},
    {8, "172.17.3.109", 12},
    {12, "172.17.2.103", 0},
    {12, "172.17.3.104", 4},
    {12, "172.17.2.108", 8},
};
const forward_polling_entry_t FORWARD_POLLING_LIST[] = {
    {1, 0, "172.17.2.108", "ai_unicast_polling", 8, 0},
    {3, 0, "172.17.2.108", "ai_broadcast_polling", 8, SIGNAL_MC_GID},
    {1, 0, "172.17.3.104", "ai_unicast_polling", 4, 0},
    {3, 0, "172.17.3.104", "ai_broadcast_polling", 4, SIGNAL_MC_GID},
    {2, 0, "0.0.0.0", "ai_broadcast_polling", 511, SIGNAL_MC_GID},
};
#endif

#endif
