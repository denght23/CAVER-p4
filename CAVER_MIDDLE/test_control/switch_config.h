#ifndef _SWITCH_CONFIG_H
#define _SWITCH_CONFIG_H

#if __TOFINO_MODE__ == 0
////////////////////Caver_related////////////////////
#define m_Other          0
#define m_DATA           1
#define m_ACK            2
#define m_CAVER_DATA     3
#define m_CAVER_ACK_FIRST 4
#define m_CAVER_ACK_SECOND 5


#define CE_max 64 //C
typedef struct{
    char* ip;
    uint64_t id;
}ip_id_t;
const ip_id_t ip_id_list[] = {
    {"10.0.0.1", 0}, {"10.0.0.2", 1}, {"10.0.0.3", 2}, {"10.0.0.4", 3},
    {"10.0.0.5", 4}, {"10.0.0.6", 5}, {"10.0.0.7", 6}, {"10.0.0.8", 7} 
};
const size_t IP_ID_SIZE = sizeof(ip_id_list) / sizeof(ip_id_t);


typedef struct {
    uint64_t caver_port;     
    uint64_t dp_port; 
} caver_dp_port_t;
const caver_dp_port_t caver_port_list_0[] = {
    {0, 52}, {1, 36}, {2, 20}, {3, 4}
};
const caver_dp_port_t caver_port_list_1[] = {
    {0,136}, {1, 152}, {2, 168},{3, 184}
};
const caver_dp_port_t* caver_port_lists[] = {caver_port_list_0, caver_port_list_1};
const size_t CAVER_PORT_SIZES[] = {sizeof(caver_port_list_0) / sizeof(caver_dp_port_t), sizeof(caver_port_list_1) / sizeof(caver_dp_port_t)};

//记录每个group对应的member的list
#define MAX_NESTED_LISTS 5

static const uint32_t nested_list_0[] = {0, 1};
static const uint32_t nested_list_1[] = {2, 3};
static const uint32_t nested_list_2[] = {4, 5};
static const uint32_t nested_list_3[] = {6, 7};
static const uint32_t nested_list_4[] = {8};

static const uint32_t* nested_list[MAX_NESTED_LISTS] = {
    nested_list_0, nested_list_1, nested_list_2, nested_list_3, nested_list_4
};

static const size_t nested_list_sizes[MAX_NESTED_LISTS] = {
    sizeof(nested_list_0) / sizeof(uint32_t),
    sizeof(nested_list_1) / sizeof(uint32_t),
    sizeof(nested_list_2) / sizeof(uint32_t),
    sizeof(nested_list_3) / sizeof(uint32_t),
    sizeof(nested_list_4) / sizeof(uint32_t)

};


static const bool nested_bool_list_0[] = {true, true};
static const bool nested_bool_list_1[] = {true, true};
static const bool nested_bool_list_2[] = {true, true};
static const bool nested_bool_list_3[] = {true, true};
static const bool nested_bool_list_4[] = {true};

static const bool* nested_bool_list[MAX_NESTED_LISTS] = {
    nested_bool_list_0, nested_bool_list_1, nested_bool_list_2, nested_bool_list_3,
    nested_bool_list_4
};

static const size_t nested_bool_list_sizes[MAX_NESTED_LISTS] = {
    sizeof(nested_bool_list_0) / sizeof(bool),
    sizeof(nested_bool_list_1) / sizeof(bool),
    sizeof(nested_bool_list_2) / sizeof(bool),
    sizeof(nested_bool_list_3) / sizeof(bool),
    sizeof(nested_bool_list_4) / sizeof(bool)
};

const uint32_t* get_list(size_t index, size_t* size) {
    *size = nested_list_sizes[index]; // 返回当前列表大小
    return nested_list[index]; // 返回指向数组的指针
}

const bool* get_bool_list(size_t index, size_t* size) {
    *size = nested_bool_list_sizes[index]; // 返回布尔列表的大小
    return nested_bool_list[index]; // 返回指向布尔数组的指针
}
//每个目的ip对应的group的id
const ip_id_t ip_group_data_list_0[] = {
    {"10.0.0.1", 0}, {"10.0.0.2", 0}, {"10.0.0.3", 0}, {"10.0.0.4", 0},
    {"10.0.0.5", 1}, {"10.0.0.6", 1}, {"10.0.0.7", 1}, {"10.0.0.8", 1} 
};
const ip_id_t ip_group_data_list_1[] = {
    {"10.0.0.1", 0}, {"10.0.0.2", 0}, {"10.0.0.3", 0}, {"10.0.0.4", 0},
    {"10.0.0.5", 1}, {"10.0.0.6", 1}, {"10.0.0.7", 1}, {"10.0.0.8", 1} 
};
const ip_id_t ip_group_ack_list_0[] = {
    {"10.0.0.1", 2}, {"10.0.0.2", 2}, {"10.0.0.3", 2}, {"10.0.0.4", 2},
    {"10.0.0.5", 3}, {"10.0.0.6", 3}, {"10.0.0.7", 3}, {"10.0.0.8", 3} 
};
const ip_id_t ip_group_ack_list_1[] = {
    {"10.0.0.1", 2}, {"10.0.0.2", 2}, {"10.0.0.3", 2}, {"10.0.0.4", 2},
    {"10.0.0.5", 3}, {"10.0.0.6", 3}, {"10.0.0.7", 3}, {"10.0.0.8", 3} 
};
const ip_id_t* ip_group_data_lists[] = {ip_group_data_list_0, ip_group_data_list_1};
const ip_id_t* ip_group_ack_lists[] = {ip_group_ack_list_0, ip_group_ack_list_1};
const size_t ip_SIZE = sizeof(ip_group_ack_list_1) / sizeof(ip_id_t);



//init_BestTable&GoodTable
const ip_id_t init_path_0[] = {
    {"10.0.0.1", 0}, {"10.0.0.2", 1}, {"10.0.0.3", 0}, {"10.0.0.4", 1},
    {"10.0.0.5", 2}, {"10.0.0.6", 3}, {"10.0.0.7", 2}, {"10.0.0.8", 3} 
    // 2
};
const ip_id_t init_path_1[] = {
    {"10.0.0.1", 1}, {"10.0.0.2", 0}, {"10.0.0.3", 1}, {"10.0.0.4", 0},
    {"10.0.0.5", 3}, {"10.0.0.6", 2}, {"10.0.0.7", 3}, {"10.0.0.8", 2} 
};
const ip_id_t* init_path_lists[] = {init_path_0, init_path_1};
const size_t INIT_PATH_SIZES[] = {sizeof(init_path_0) / sizeof(ip_id_t), sizeof(init_path_1) / sizeof(ip_id_t)};


// /////ECN related///////
#define DCQCN_K_MIN 1250         // 100KB
#define DCQCN_K_MAX 3000         // 240KB
#define DCQCN_P_MAX 0.2          // 20%
#define QDEPTH_RANGE_MAX (1 << 19)
#define SEED_RANGE_MAX 256       // Random number range ~ [0, 255] (8 bits)
#define SEED_K_MAX ((int)(DCQCN_P_MAX * SEED_RANGE_MAX)) // 52
#define QDEPTH_STEPSIZE ((DCQCN_K_MAX - DCQCN_K_MIN) / SEED_K_MAX) // 72







const switch_port_t PORT_LIST[] = {
	{"10/0"}, {"12/0"}, {"14/0"}, {"16/0"}, // pipe 0 
	{"2/0"}, {"4/0"}, {"6/0"}, {"8/0"}, 
};
const switch_port_t PORT_LIST_A[] = {
	{"10/0"}, {"12/0"}, {"14/0"}, {"16/0"}, // pipe 0 
};
const uint16_t DEV_PORT_LIST_A[] = {
	52, 36, 20, 4
};
const switch_port_t PORT_LIST_B[] = {
	{"2/0"}, {"4/0"}, {"6/0"}, {"8/0"}, 
};
const uint16_t DEV_PORT_LIST_B[] = {
	136, 152, 168, 184
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
