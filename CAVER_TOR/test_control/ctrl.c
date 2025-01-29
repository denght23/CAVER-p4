#include "headers.h"
#include "switch_config.h"
#include <inttypes.h>
//extern "C" {
//}
#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_DATA(d) *((uint8_t *)&d + 5), *((uint8_t *)&d + 4), \
                    *((uint8_t *)&d + 3), *((uint8_t *)&d + 2), \
                    *((uint8_t *)&d + 1), *((uint8_t *)&d + 0)
#define CHECK_BF_STATUS(func_call)                  \
    do {                                            \
        bf_status = (func_call);                    \
        printf("DEBUG: " #func_call " returned %d\n", bf_status); \
        assert(bf_status == BF_SUCCESS);            \
    } while (0); \

const uint32_t REG_NUM = 1;
const double EXP_PERIOD = 10;



uint32_t ipv4AddrToUint32(const char* ip_str) {
  uint32_t ip_addr;
  if (inet_pton(AF_INET, ip_str, &ip_addr) != 1) {
      fprintf(stderr, "Error: Invalid IP address format.\n");
      return EXIT_FAILURE;
  }

  return ntohl(ip_addr);
  // return ip_addr;
}


// C-style using bf_pm
static void port_setup_25g(const bf_rt_target_t *dev_tgt,
                       const switch_port_t *port_list,
                       const uint8_t port_count) {
    bf_status_t bf_status;

    // Add and enable ports
    for (unsigned int idx = 0; idx < port_count; idx++) {
        bf_pal_front_port_handle_t port_hdl;
        bf_status = bf_pm_port_str_to_hdl_get(dev_tgt->dev_id,
                                              port_list[idx].fp_port,
                                              &port_hdl);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_pm_port_add(dev_tgt->dev_id, &port_hdl,
                                   BF_SPEED_25G, BF_FEC_TYP_NONE);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_pm_port_enable(dev_tgt->dev_id, &port_hdl);
        assert(bf_status == BF_SUCCESS);
        printf("Port %s is enabled successfully!\n", port_list[idx].fp_port);
    }
}

// C-style using bf_pm
static void port_setup_10g(const bf_rt_target_t *dev_tgt,
                       const switch_port_t *port_list,
                       const uint8_t port_count) {
    bf_status_t bf_status;

    // Add and enable ports
    for (unsigned int idx = 0; idx < port_count; idx++) {
        bf_pal_front_port_handle_t port_hdl;
        bf_status = bf_pm_port_str_to_hdl_get(dev_tgt->dev_id,
                                              port_list[idx].fp_port,
                                              &port_hdl);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_pm_port_add(dev_tgt->dev_id, &port_hdl,
                                   BF_SPEED_10G, BF_FEC_TYP_NONE);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_pm_port_enable(dev_tgt->dev_id, &port_hdl);
        assert(bf_status == BF_SUCCESS);
        printf("Port %s is enabled successfully!\n", port_list[idx].fp_port);
    }
}



static void switchd_setup(bf_switchd_context_t *switchd_ctx, const char *prog) {
	char conf_file[256];
	char bf_sysfs_fname[128] = "/sys/class/bf/bf0/device";
    FILE *fd;
    
    printf("%s\n", getenv("SDE_INSTALL"));
	switchd_ctx->install_dir = strdup(getenv("SDE_INSTALL"));
	sprintf(conf_file, "%s%s%s%s", \
	        getenv("SDE_INSTALL"), "/share/p4/targets/tofino/", prog, ".conf");
	switchd_ctx->conf_file = conf_file;
	switchd_ctx->running_in_background = true;
	switchd_ctx->dev_sts_thread = true;
	switchd_ctx->dev_sts_port = 7777; // 9090?
	

//    switchd_ctx->kernel_pkt = true;
	// Determine if kernel mode packet driver is loaded 
    strncat(bf_sysfs_fname, "/dev_add",
            sizeof(bf_sysfs_fname) - 1 - strlen(bf_sysfs_fname));
    printf("bf_sysfs_fname %s\n", bf_sysfs_fname);
    fd = fopen(bf_sysfs_fname, "r");
    if (fd != NULL) {
        // override previous parsing if bf_kpkt KLM was loaded 
        printf("kernel mode packet driver present, forcing kpkt option!\n");
        switchd_ctx->kernel_pkt = true;
        fclose(fd);
    }

    assert(bf_switchd_lib_init(switchd_ctx) == BF_SUCCESS);
    printf("\nbf_switchd is initialized correctly!\n");
}

static void bfrt_setup(const bf_rt_target_t *dev_tgt,
                const bf_rt_info_hdl **bfrt_info,
                const char* prog,
               	bf_rt_session_hdl **session) {
  
 	bf_status_t bf_status;

    // Get bfrtInfo object from dev_id and p4 program name
    bf_status = bf_rt_info_get(dev_tgt->dev_id, prog, bfrt_info);
    assert(bf_status == BF_SUCCESS);
    // Create a session object
    bf_status = bf_rt_session_create(session);
    assert(bf_status == BF_SUCCESS);
    printf("bfrt_info is got and session is created correctly!\n"); 
//	return bf_status;
}


static void losslessTrafficSetUp(const bf_rt_target_t *dev_tgt,
							const switch_port_t* port_list,
							const uint8_t port_count) {
	bf_status_t bf_status;
	
	bf_dev_id_t dev_id = dev_tgt->dev_id; // 0
	uint32_t ppg_cell = 100;
	bf_tm_ppg_hdl  ppg_hdl;
	uint8_t icos_bmap = 0xff;//
	uint32_t skid_cells = 100;
	uint8_t cos_to_icos[8]={0,1,2,3,4,5,6,7}; //TODO: what it means?
	bf_tm_queue_t queue_id = 3;
	uint32_t queue_cells = 100;
	uint8_t queue_count = 8;
	uint8_t queue_mapping[8] = {0,1,2,3,4,5,6,7};
	
	for (uint8_t idx = 0; idx < port_count; idx ++) {
		// Step 1: Map lossless traffic to a PPG handle with a buffer limit

		bf_dev_port_t dev_port; 
		bf_status = bf_pm_port_str_to_dev_port_get(
            dev_id, (char *)port_list[idx].fp_port, &dev_port
        );		
		assert(bf_status == BF_SUCCESS);
		
		bf_status = bf_tm_ppg_allocate(dev_id, dev_port, &ppg_hdl);
		assert(bf_status == BF_SUCCESS);
		
		bf_status = bf_tm_ppg_guaranteed_min_limit_set(dev_id, 
														ppg_hdl, ppg_cell );
		assert(bf_status == BF_SUCCESS);
		// No dynmical buffering or pooling

		// Step 2: Map traffic to an iCoS
		bf_status = bf_tm_ppg_icos_mapping_set(dev_id, ppg_hdl, icos_bmap);
		assert(bf_status == BF_SUCCESS);
		
		// Step 3; Declare buffer and set up pause/PFC generation
		bf_status =	bf_tm_ppg_skid_limit_set(dev_id, ppg_hdl, skid_cells);
		assert(bf_status == BF_SUCCESS);
	
		bf_status = bf_tm_ppg_lossless_treatment_enable(dev_id,
	                                                	 ppg_hdl);
		assert(bf_status == BF_SUCCESS);
	
		// BF_TM_PAUSE_PORT  BF_TM_PAUSE_PFC
		bf_status = bf_tm_port_flowcontrol_mode_set(dev_id, dev_port, BF_TM_PAUSE_PORT);
		assert(bf_status == BF_SUCCESS);
		bf_status = bf_tm_port_pfc_cos_mapping_set(dev_id, dev_port, cos_to_icos);
		assert(bf_status == BF_SUCCESS);
		
		// Step 4: Apply Buffering
		for (uint8_t idx = 0; idx < queue_count; idx ++) {
			bf_status = bf_tm_q_guaranteed_min_limit_set(dev_id, dev_port, queue_mapping[idx], queue_cells);
			assert(bf_status == BF_SUCCESS);
		}		
		
		// Step 5: Allocate queues
		bf_status = bf_tm_port_q_mapping_set(dev_id, dev_port, queue_count, queue_mapping);
		assert(bf_status == BF_SUCCESS);

		// Step 7: Honor pause/PFC events
		
		for (uint8_t idx = 0; idx < queue_count; idx ++) {
			// queue idx has cos idx
			bf_status = bf_tm_q_pfc_cos_mapping_set(dev_id, dev_port, queue_mapping[idx], idx);
			assert(bf_status == BF_SUCCESS);
		}		
		assert(bf_status==BF_SUCCESS);	
		bf_status = bf_tm_port_flowcontrol_rx_set(dev_id, dev_port, BF_TM_PAUSE_PORT);
		assert(bf_status==BF_SUCCESS);
	
		bf_status = bf_pal_port_flow_control_link_pause_set(dev_id, dev_port, 1, 1 );
		assert(bf_status==BF_SUCCESS);
		bf_status = bf_pal_port_flow_control_pfc_set(dev_id, dev_port, 0xff, 0xff);
		//bf_status = bf_pal_port_flow_control_link_pause_set(dev_id, dev_port, 1, 1 );
		assert(bf_status==BF_SUCCESS);
	
		printf("Set up lossless for port %s and dev_port %u\n", port_list[idx].fp_port, dev_port);
	}

}

static void forward_2d_table_setup(const bf_rt_target_t *dev_tgt,
                                const bf_rt_info_hdl *bfrt_info,
                                const bf_rt_table_hdl **forward_table,
                                forward_2d_table_info_t *forward_table_info,
                                const forward_entry_t *forward_list,
                                const uint8_t forward_count) {
    bf_status_t bf_status;

    // Get table object from name
    bf_status = bf_rt_table_from_name_get(bfrt_info,
                                          "SwitchIngress.ti_2d_forward",
                                          forward_table);
    assert(bf_status == BF_SUCCESS);

    // Allocate key and data once, and use reset across different uses
    bf_status = bf_rt_table_key_allocate(*forward_table,
                                         &forward_table_info->key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(*forward_table,
                                          &forward_table_info->data);
    assert(bf_status == BF_SUCCESS);

    // Get field-ids for key field

	bf_status = bf_rt_key_field_id_get(*forward_table, "hdr.ipv4.dst_ip",
										&forward_table_info->kid_ipv4_dst_ip);
    assert(bf_status == BF_SUCCESS);

	bf_status = bf_rt_key_field_id_get(*forward_table, "ig_intr_md.ingress_port",
										&forward_table_info->kid_ingress_port);

    // Get action Ids for action a_unicast
    bf_status = bf_rt_action_name_to_id(*forward_table,
                                        "SwitchIngress.ai_unicast",
                                        &forward_table_info->aid_ai_unicast);
    assert(bf_status == BF_SUCCESS);

    // Get field-ids for data field
    bf_status = bf_rt_data_field_id_with_action_get(
        *forward_table, "port",
        forward_table_info->aid_ai_unicast, &forward_table_info->did_port
    );
    assert(bf_status == BF_SUCCESS);

    //                                       //
    // Set up the multicast for ai_broadcast //
    //                                       //
/*
    bf_status = bf_mc_create_session(&forward_table_info->mc_session);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_mc_mgrp_create(forward_table_info->mc_session,
                                  dev_tgt->dev_id,
                                  BROADCAST_MC_GID,
                                  &forward_table_info->mc_mgrp);
    assert(bf_status == BF_SUCCESS);

    BF_MC_PORT_MAP_INIT(forward_table_info->mc_port_map);
    BF_MC_LAG_MAP_INIT(forward_table_info->mc_lag_map);

    for (unsigned idx = 0; idx < forward_count; idx++) {
        bf_dev_port_t dev_port;
        bf_status = bf_pm_port_str_to_dev_port_get(
            dev_tgt->dev_id, (char *)forward_list[idx].fp_port, &dev_port
        );
        assert(bf_status == BF_SUCCESS);
        BF_MC_PORT_MAP_SET(forward_table_info->mc_port_map, dev_port);
    }

    // Rid set to 0
    bf_status = bf_mc_node_create(forward_table_info->mc_session,
                                  dev_tgt->dev_id, 0,
                                  forward_table_info->mc_port_map,
                                  forward_table_info->mc_lag_map,
                                  &forward_table_info->mc_node);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_mc_associate_node(forward_table_info->mc_session,
                                     dev_tgt->dev_id,
                                     forward_table_info->mc_mgrp,
                                     forward_table_info->mc_node,
                                     false,  0);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_mc_complete_operations(forward_table_info->mc_session);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_mc_destroy_session(forward_table_info->mc_session);
    assert(bf_status == BF_SUCCESS);
*/
}

static void forward_2d_table_entry_add(const bf_rt_target_t *dev_tgt,
                                    const bf_rt_session_hdl *session,
                                    const bf_rt_table_hdl *forward_table,
                                    forward_2d_table_info_t *forward_table_info,
                                    forward_2d_table_entry_t *forward_entry) {
    bf_status_t bf_status;

    // Reset key before use
    bf_rt_table_key_reset(forward_table, &forward_table_info->key);

    // Fill in the Key object
	bf_status = bf_rt_key_field_set_value(forward_table_info->key,
										  forward_table_info->kid_ipv4_dst_ip,
										  forward_entry->ipv4_addr); 
    assert(bf_status == BF_SUCCESS);
	
	bf_status = bf_rt_key_field_set_value(forward_table_info->key,
										  forward_table_info->kid_ingress_port,
										  forward_entry->ingress_port); 
    assert(bf_status == BF_SUCCESS);

    if (strcmp(forward_entry->action, "ai_unicast") == 0) {
        // Reset data before use
        bf_rt_table_action_data_reset(forward_table,
                                      forward_table_info->aid_ai_unicast,
                                      &forward_table_info->data);
        // Fill in the Data object
        bf_status = bf_rt_data_field_set_value(forward_table_info->data,
                                               forward_table_info->did_port,
                                               forward_entry->egress_port);
        assert(bf_status == BF_SUCCESS);
    }
//    else if (strcmp(forward_entry->action, "ai_broadcast") == 0) {
//        bf_rt_table_action_data_reset(forward_table,
//                                      forward_table_info->aid_broadcast,
//                                      &forward_table_info->data);
//    }

    // Call table entry add API
    bf_status = bf_rt_table_entry_add(forward_table, session, dev_tgt,
                                      forward_table_info->key,
                                      forward_table_info->data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
}

static void forward_2d_table_deploy(const bf_rt_target_t *dev_tgt,
                                 const bf_rt_info_hdl *bfrt_info,
                                 const bf_rt_session_hdl *session,
                                 const forward_entry_t *forward_list,
                                 const uint8_t forward_count) {
    bf_status_t bf_status;
    const bf_rt_table_hdl *forward_table = NULL;
    forward_2d_table_info_t forward_table_info;

    // Set up the forward table
    forward_2d_table_setup(dev_tgt, bfrt_info, &forward_table,
                        &forward_table_info, forward_list, forward_count);
    printf("Table forward is set up correctly!\n");

    // Add forward entries
    for (unsigned int idx = 0; idx < forward_count; idx++) {

		forward_2d_table_entry_t forward_entry =  {
			.ingress_port = forward_list[idx].ingress_port,
			.ipv4_addr = ipv4AddrToUint32(forward_list[idx].ipv4_addr),
			.action = "ai_unicast",
			.egress_port = forward_list[idx].egress_port,
		}; 
//        bf_status = bf_pm_port_str_to_dev_port_get(
//            dev_tgt->dev_id,
//            (char *)forward_list[idx].fp_port, &forward_entry.egress_port
//        );
        forward_2d_table_entry_add(dev_tgt, session, forward_table,
                                &forward_table_info, &forward_entry);
        printf("Add entry to unicast packets from port %lu with dstIP %s to port %lu\n",
				forward_list[idx].ingress_port, forward_list[idx].ipv4_addr,
                forward_list[idx].egress_port);
    }

}


static void forward_polling_table_setup(const bf_rt_target_t *dev_tgt,
                                const bf_rt_info_hdl *bfrt_info,
                                const bf_rt_table_hdl **forward_table,
                                forward_polling_table_info_t *forward_table_info) {
    bf_status_t bf_status;

    // Get table object from name
    bf_status = bf_rt_table_from_name_get(bfrt_info,
                                          "SwitchIngress.ti_match_polling_TP",
                                          forward_table);
    assert(bf_status == BF_SUCCESS);

    // Allocate key and data once, and use reset across different uses
    bf_status = bf_rt_table_key_allocate(*forward_table,
                                         &forward_table_info->key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(*forward_table,
                                          &forward_table_info->data);
    assert(bf_status == BF_SUCCESS);

    // Get field-ids for key field

	bf_status = bf_rt_key_field_id_get(*forward_table, "hdr.polling.TP_type",
										&forward_table_info->kid_TP_type);
    assert(bf_status == BF_SUCCESS);

	bf_status = bf_rt_key_field_id_get(*forward_table, "hdr.polling.vf_dst_ip",
										&forward_table_info->kid_vf_dst_ip);
    assert(bf_status == BF_SUCCESS);

	bf_status = bf_rt_key_field_id_get(*forward_table, "ig_intr_md.ingress_port",
										&forward_table_info->kid_ingress_port);

    // Get action Ids for action a_unicast
    bf_status = bf_rt_action_name_to_id(*forward_table,
                                        "SwitchIngress.ai_unicast_polling",
                                        &forward_table_info->aid_ai_unicast);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_rt_action_name_to_id(*forward_table,
                                        "SwitchIngress.ai_broadcast_polling",
                                        &forward_table_info->aid_ai_broadcast);
    assert(bf_status == BF_SUCCESS);
    
	bf_status = bf_rt_action_name_to_id(*forward_table,
                                        "SwitchIngress.ai_drop_polling",
                                        &forward_table_info->aid_ai_drop);
    assert(bf_status == BF_SUCCESS);
    // Get field-ids for data field
    bf_status = bf_rt_data_field_id_with_action_get(
        *forward_table, "port",
        forward_table_info->aid_ai_unicast, &forward_table_info->did_unicast_port
    );
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_rt_data_field_id_with_action_get(
        *forward_table, "port",
        forward_table_info->aid_ai_broadcast, &forward_table_info->did_broadcast_port
    );
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_rt_data_field_id_with_action_get(
        *forward_table, "mc_grp_id",
        forward_table_info->aid_ai_broadcast, &forward_table_info->did_mc_grp_id
    );
    assert(bf_status == BF_SUCCESS);
}

static void forward_polling_table_entry_add(const bf_rt_target_t *dev_tgt,
                                    const bf_rt_session_hdl *session,
                                    const bf_rt_table_hdl *forward_table,
                                    forward_polling_table_info_t *forward_table_info,
                                    forward_polling_table_entry_t *forward_entry) {
    bf_status_t bf_status;

	// Reset key before use
    bf_rt_table_key_reset(forward_table, &forward_table_info->key);

//    // Reset data before use
//    bf_rt_table_action_data_reset(forward_table,
//                                  forward_table_info->aid_ai_unicast,
//                                  &forward_table_info->data);
//    // Reset data before use
//    bf_rt_table_action_data_reset(forward_table,
//                                  forward_table_info->aid_ai_broadcast,
//                                  &forward_table_info->data);
//	printf("DEBUG: forward_entry->action: %s in forward_polling_table_entry_add\n",
//			forward_entry->action);
	printf("DEBUG: Add foward_polling_table_entry: TP_type: %u, ingress_port: %u,"
			" vf_dst_ip: %x, action: %s, to port %u, mc grp id: %u\n",
			forward_entry->polling_TP_type, forward_entry->ingress_port,
			forward_entry->polling_vf_dst_ip, forward_entry->action,
            forward_entry->egress_port, forward_entry->mc_grp_id);

    if (forward_entry->polling_TP_type == POLLING_FLW_TRC ||
		forward_entry->polling_TP_type == POLLING_ALL_TRC ) {
//	    printf("DEBUG: forward_polling_table_entry_add for PLOOING_FLW_TRC\n");
		// Reset key before use
	    //printf("DEBUG: bf_rt_table_key_reset in forward_polling_table_entry_add \n");

		// Fill in the Key object
		bf_status = bf_rt_key_field_set_value(forward_table_info->key,
											  forward_table_info->kid_TP_type,
											  forward_entry->polling_TP_type); 
	    assert(bf_status == BF_SUCCESS);
	
	    //printf("DEBUG: bf_rt_key_field_set_value TP type in forward_polling_table_entry_add \n");
		
		bf_status = bf_rt_key_field_set_value(forward_table_info->key,
											  forward_table_info->kid_ingress_port,
											  forward_entry->ingress_port); 
//		bf_status = bf_rt_key_field_set_value_and_mask(forward_table_info->key,
//											  forward_table_info->kid_ingress_port,
//											  forward_entry->ingress_port, 0x1ff); 
	    assert(bf_status == BF_SUCCESS);
		
		bf_status = bf_rt_key_field_set_value_and_mask(forward_table_info->key,
											  forward_table_info->kid_vf_dst_ip,
											  forward_entry->polling_vf_dst_ip, 0xffffffff); 
	    assert(bf_status == BF_SUCCESS);

	    //printf("DEBUG: bf_rt_key_field_set_value_and_mask in forward_polling_table_entry_add \n");

	    //printf("DEBUG: bf_rt_table_action_data_reset in forward_polling_table_entry_add \n");
        // Fill in the Data object
        if (strcmp(forward_entry->action, "ai_unicast_polling") == 0) {
	        printf("DEBUG:  add ai_unicast_polling\n");
            bf_rt_table_action_data_reset(forward_table,
                                          forward_table_info->aid_ai_unicast,
                                          &forward_table_info->data);

			bf_status = bf_rt_data_field_set_value(forward_table_info->data,
	                                               forward_table_info->did_unicast_port,
	                                               forward_entry->egress_port);
	        assert(bf_status == BF_SUCCESS);
		}
		else if (strcmp(forward_entry->action, "ai_broadcast_polling") == 0) {
	        printf("DEBUG:  add ai_broadcast_polling\n");
            // Reset data before use
            bf_rt_table_action_data_reset(forward_table,
                                          forward_table_info->aid_ai_broadcast,
                                          &forward_table_info->data);

	        bf_status = bf_rt_data_field_set_value(forward_table_info->data,
	                                               forward_table_info->did_broadcast_port,
	                                               forward_entry->egress_port);
	        assert(bf_status == BF_SUCCESS);
	
	        bf_status = bf_rt_data_field_set_value(forward_table_info->data,
	                                               forward_table_info->did_mc_grp_id,
	                                               forward_entry->mc_grp_id);
	        assert(bf_status == BF_SUCCESS);
				
		}
	    //printf("DEBUG: bf_rt_data_field_set_value in forward_polling_table_entry_add \n");
    }
	else if (forward_entry->polling_TP_type == POLLING_PFC_TRC ) {
//	    printf("DEBUG: forward_polling_table_entry_add for POLLING_PFC_TRC\n");

		// Fill in the Key object
		bf_status = bf_rt_key_field_set_value(forward_table_info->key,
											  forward_table_info->kid_TP_type,
											  forward_entry->polling_TP_type); 
	    assert(bf_status == BF_SUCCESS);
	
		bf_status = bf_rt_key_field_set_value(forward_table_info->key,
											  forward_table_info->kid_ingress_port,
											  forward_entry->ingress_port); 
		
//		bf_status = bf_rt_key_field_set_value_and_mask(forward_table_info->key,
//											  forward_table_info->kid_ingress_port,
//											  forward_entry->ingress_port, 0x1ff); 
	    assert(bf_status == BF_SUCCESS);

		// not care mask	
		bf_status = bf_rt_key_field_set_value_and_mask(forward_table_info->key,
											  forward_table_info->kid_vf_dst_ip,
											  forward_entry->polling_vf_dst_ip, 0); 
	    assert(bf_status == BF_SUCCESS);

        // Fill in the Data object
        bf_status = bf_rt_data_field_set_value(forward_table_info->data,
                                               forward_table_info->did_broadcast_port,
                                               forward_entry->egress_port);
        assert(bf_status == BF_SUCCESS);

        bf_status = bf_rt_data_field_set_value(forward_table_info->data,
                                               forward_table_info->did_mc_grp_id,
                                               forward_entry->mc_grp_id);
        assert(bf_status == BF_SUCCESS);
    }

    // Call table entry add API
    bf_status = bf_rt_table_entry_add(forward_table, session, dev_tgt,
                                      forward_table_info->key,
                                      forward_table_info->data);
	//printf("DEBUG: invoking bf_rt_table_entry_add \n");
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
}

void forward_polling_table_deploy(const bf_rt_target_t *dev_tgt,
                                 const bf_rt_info_hdl *bfrt_info,
                                 const bf_rt_session_hdl *session,
                                 const forward_polling_entry_t *forward_list,
                                 const uint8_t forward_count) {
    bf_status_t bf_status;
    const bf_rt_table_hdl *forward_table = NULL;
    forward_polling_table_info_t forward_table_info;

    // Set up the forward table
    forward_polling_table_setup(dev_tgt, bfrt_info, &forward_table,
                        &forward_table_info);
    printf("Table forward_polling is set up correctly!\n");

    // Add forward entries
    for (unsigned int idx = 0; idx < forward_count; idx++) {
		
	    printf("Add foward_polling_list[%u]: TP_type: %u, ingress_port: %lu,"
				" vf_dst_ip: %s, action: %s, to port %lu, mc grp id: %u\n",
				idx,
				forward_list[idx].TP_type, forward_list[idx].ingress_port,
				forward_list[idx].vf_dst_ip_addr, forward_list[idx].action,
	            forward_list[idx].egress_port, forward_list[idx].mc_grp_id);

		if (forward_list[idx].TP_type == POLLING_FLW_TRC ||
			forward_list[idx].TP_type == POLLING_PFC_TRC ||
			forward_list[idx].TP_type == POLLING_ALL_TRC ) { 
			forward_polling_table_entry_t forward_entry =  {
				.polling_TP_type = forward_list[idx].TP_type,
				.ingress_port = forward_list[idx].ingress_port,
				.polling_vf_dst_ip = ipv4AddrToUint32(forward_list[idx].vf_dst_ip_addr),
//				.action = forward_list[idx].action,
				.egress_port = forward_list[idx].egress_port,
				.mc_grp_id = forward_list[idx].mc_grp_id
			}; 
			strncpy(forward_entry.action, forward_list[idx].action, sizeof(forward_entry.action) - 1);
			// make sure ending with 0
			forward_entry.action[sizeof(forward_entry.action) - 1] = '\0';
        	forward_polling_table_entry_add(dev_tgt, session, forward_table,
                                &forward_table_info, &forward_entry);
//	        bf_status = bf_pm_port_str_to_dev_port_get(
//	            dev_tgt->dev_id,
//	            (char *)forward_list[idx].fp_port, &forward_entry.egress_port
//	        );
		
		}
//		else if (forward_list[idx].TP_type == POLLING_ALL_TRC ) {
//		// else if (forward_list[idx].action == "ai_broadcast_polling") {
//			forward_polling_table_entry_t forward_entry =  {
//				.polling_TP_type = forward_list[idx].TP_type,
//				.ingress_port = forward_list[idx].ingress_port,
//				.polling_vf_dst_ip = ipv4AddrToUint32(forward_list[idx].vf_dst_ip_addr),
//				.action = forward_list[idx].action,
//				.egress_port = forward_list[idx].egress_port,
//				.mc_grp_id = forward_list[idx].mc_grp_id
//			}; 
//        	forward_polling_table_entry_add(dev_tgt, session, forward_table,
//                                &forward_table_info, &forward_entry);
//
//		}
//		else if (forward_list[idx].TP_type == POLLING_PFC_TRC ) {
//			forward_polling_table_entry_t forward_entry =  {
//				.polling_TP_type = forward_list[idx].TP_type,
//				.ingress_port = forward_list[idx].ingress_port,
//				.polling_vf_dst_ip = ipv4AddrToUint32(forward_list[idx].vf_dst_ip_addr),
//				.action = forward_list[idx].action,
//				.egress_port = forward_list[idx].egress_port,
//				.mc_grp_id = forward_list[idx].mc_grp_id
//			}; 
//        	forward_polling_table_entry_add(dev_tgt, session, forward_table,
//                                &forward_table_info, &forward_entry);
//		}
    }

}

static void multicast_setup(const bf_rt_target_t *dev_tgt,
							const switch_port_t* port_list,
							const uint8_t port_count,
							const uint16_t mc_grp_id) {
	bf_status_t bf_status;
    bf_mc_session_hdl_t mc_session;
    bf_mc_mgrp_hdl_t mc_mgrp;
    bf_mc_node_hdl_t mc_node;
    bf_mc_port_map_t mc_port_map;
    bf_mc_lag_map_t mc_lag_map;

    bf_status = bf_mc_create_session(&mc_session);
    assert(bf_status == BF_SUCCESS);

    //bf_status = bf_mc_mgrp_create(mc_session, dev_tgt->dev_id,
    //                              SIGNAL_MC_GID, &mc_mgrp);
    bf_status = bf_mc_mgrp_create(mc_session, dev_tgt->dev_id,
                                  mc_grp_id, &mc_mgrp);
    assert(bf_status == BF_SUCCESS);

    BF_MC_PORT_MAP_INIT(mc_port_map);
    BF_MC_LAG_MAP_INIT(mc_lag_map);

//    BF_MC_PORT_MAP_SET(mc_port_map, H1_PORT);	
//    BF_MC_PORT_MAP_SET(mc_port_map, H2_PORT);
    
	for (uint8_t idx = 0; idx < port_count; idx ++){
   		bf_dev_port_t dev_port; 
		bf_status = bf_pm_port_str_to_dev_port_get(
            dev_tgt->dev_id, (char *)port_list[idx].fp_port, &dev_port
        );	     
        BF_MC_PORT_MAP_SET(mc_port_map, dev_port);
        printf(" DEBUG: set multicast for dev_port: %d\n", dev_port);
    }

    // Rid set to 0
    bf_status = bf_mc_node_create(mc_session, dev_tgt->dev_id, 0,  //rid
                                  mc_port_map, mc_lag_map, &mc_node);
    assert(bf_status == BF_SUCCESS);
	
	// Assign node to mc_grp: level1_exclusion_id is not needed here = 0
    bf_status = bf_mc_associate_node(mc_session, dev_tgt->dev_id,
                                     mc_mgrp, mc_node, false,  0);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_mc_complete_operations(mc_session);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_mc_destroy_session(mc_session);
    assert(bf_status == BF_SUCCESS);
    printf("Muticast for %u is set up correctly!\n", mc_grp_id);
}

void multicast_group_setup(const bf_rt_target_t *dev_tgt,
							const switch_port_t* port_list_a,
							const switch_port_t* port_list_b,
							const uint8_t port_count_a,
							const uint8_t port_count_b,
							const uint16_t mc_grp_id_a,
							const uint16_t mc_grp_id_b) {

	if (mc_grp_id_a == mc_grp_id_b){
		printf("ERROR: multicast group id cannot be equal!\n");
		return;
	}	
		
	multicast_setup(dev_tgt, port_list_a, ARRLEN(port_list_a), mc_grp_id_a);
	printf("setup multicast group A successfully\n");
	
	multicast_setup(dev_tgt, port_list_b, ARRLEN(port_list_b), mc_grp_id_b);
	printf("setup multicast group B successfully\n");
	
	return;
}

static void mirrorSetup(const bf_rt_target_t *dev_tgt) {
	p4_pd_status_t pd_status;
	p4_pd_sess_hdl_t mirror_session;
	// p4_pd_dev_target_t pd_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};
	p4_pd_dev_target_t pd_dev_tgt = {dev_tgt->dev_id, dev_tgt->pipe_id};
//	p4_pd_mirror_session_info_t mirror_session_info = {
//		.type        = PD_MIRROR_TYPE_NORM, // Not sure
//        .dir         = PD_DIR_EGRESS,
//        .id          = 2,
//        .egr_port    = CPU_PORT,
//        .egr_port_v  = true,
//        .max_pkt_len = 16384 // Refer to example in Barefoot Academy	
//	};
	p4_pd_mirror_session_info_t mirror_session_info = {
    	PD_MIRROR_TYPE_NORM, // type
    	PD_DIR_EGRESS,       // dir
    	// EGRESS_MIRROR_SID,                   // id
        UPDATE_NOTIFY_MIRROR_SID,  // SESSION ID	
        //H2_PORT,             // egr_port 
		CPU_PORT,            // egr_port = CPU
        true,                // egr_port_v
    	0,                   // egr_port_queue (specified if necessary) 
    	PD_COLOR_GREEN,      // packet_color 
    	0,                   // mcast_grp_a
    	false,               // mcast_grp_a_v
    	0,                   // mcast_grp_b 
    	false,               // mcast_grp_b_v
    	16384,               // max_pkt_len
    	0,                   // level1_mcast_hash 
    	0,                   // level2_mcast_hash
    	0,                   // mcast_l1_xid 
    	0,                   // mcast_l2_xid
    	0,                   // mcast_rid 
    	0,                   // cos 
    	false,               // c2c
    	0,                   // extract_len 
    	0,                   // timeout_usec 
    	NULL,             // int_hdr 
    	0                    // int_hdr_len
	};
	
	pd_status = p4_pd_client_init(&mirror_session);
    assert(pd_status == BF_SUCCESS);

    // p4_pd_mirror_session_create() will enable the session by default
    pd_status = p4_pd_mirror_session_create(mirror_session, pd_dev_tgt,
                                            &mirror_session_info);
    assert(pd_status == BF_SUCCESS);	
	printf("Config Egress mirrror to CPU_PORT: %d, SID: %d\n", CPU_PORT, UPDATE_NOTIFY_MIRROR_SID);
}



static void register_setup(const bf_rt_info_hdl *bfrt_info,
                           const char *reg_name,
                           const char *value_field_name,
                           const bf_rt_table_hdl **reg,
                           register_info_t *reg_info) {
    bf_status_t bf_status;
    char reg_value_field_name[64];
//    printf("Debug: register_setup %s\n", reg_name);
    // Get table object from name
    bf_status = bf_rt_table_from_name_get(bfrt_info, reg_name, reg);
//    printf("Debug: bf_rt_table_from_name_get\n");
    assert(bf_status == BF_SUCCESS);
    
    // Allocate key and data once, and use reset across different uses
    bf_status = bf_rt_table_key_allocate(*reg, &reg_info->key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(*reg, &reg_info->data);
    assert(bf_status == BF_SUCCESS);

    // Get field-ids for key field
    bf_status = bf_rt_key_field_id_get(*reg, "$REGISTER_INDEX",
                                       &reg_info->kid_register_index);
//    printf("Debug: bf_rt_key_field_id_get\n");
    assert(bf_status == BF_SUCCESS);

    // Get field-ids for data field
    strcpy(reg_value_field_name, reg_name);
    if (value_field_name == NULL) {
        strcat(reg_value_field_name, ".f1");
    }
    else {
        strcat(reg_value_field_name, ".");
        strcat(reg_value_field_name, value_field_name);
    }
    bf_status = bf_rt_data_field_id_get(*reg, reg_value_field_name,
                                        &reg_info->did_value);
    assert(bf_status == BF_SUCCESS);
}

static void register_write(const bf_rt_target_t *dev_tgt,
                           const bf_rt_session_hdl *session,
                           const bf_rt_table_hdl *reg,
                           register_info_t *reg_info,
                           register_entry_t *reg_entry) {
    bf_status_t bf_status;

    // Reset key and data before use
    bf_rt_table_key_reset(reg, &reg_info->key);
    bf_rt_table_data_reset(reg, &reg_info->data);

    // Fill in the Key and Data object
    bf_status = bf_rt_key_field_set_value(reg_info->key,
                                          reg_info->kid_register_index,
                                          reg_entry->register_index);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_set_value(reg_info->data,
                                           reg_info->did_value,
                                           reg_entry->value);
    assert(bf_status == BF_SUCCESS);

    // Call table entry add API
    bf_status = bf_rt_table_entry_add(reg, session, dev_tgt,
                                      reg_info->key, reg_info->data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
}

static void register_write_no_wait(const bf_rt_target_t *dev_tgt,
                                   const bf_rt_session_hdl *session,
                                   const bf_rt_table_hdl *reg,
                                   register_info_t *reg_info,
                                   register_entry_t *reg_entry) {
    bf_status_t bf_status;

    // Reset key and data before use
    bf_rt_table_key_reset(reg, &reg_info->key);
    bf_rt_table_data_reset(reg, &reg_info->data);

    // Fill in the Key and Data object
    bf_status = bf_rt_key_field_set_value(reg_info->key,
                                          reg_info->kid_register_index,
                                          reg_entry->register_index);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_set_value(reg_info->data,
                                           reg_info->did_value,
                                           reg_entry->value);
    assert(bf_status == BF_SUCCESS);

    // Call table entry add API
    bf_status = bf_rt_table_entry_add(reg, session, dev_tgt,
                                      reg_info->key, reg_info->data);
    assert(bf_status == BF_SUCCESS);
    // bf_status = bf_rt_session_complete_operations(session);
    // assert(bf_status == BF_SUCCESS);
}

static void register_read(const bf_rt_target_t *dev_tgt,
                          const bf_rt_session_hdl *session,
                          const bf_rt_table_hdl *reg,
                          register_info_t *reg_info,
                          register_entry_t *reg_entry) {
    bf_status_t bf_status;

    // Reset key and data before use
    bf_rt_table_key_reset(reg, &reg_info->key);
    bf_rt_table_data_reset(reg, &reg_info->data);

    // Fill in the Key object
    bf_status = bf_rt_key_field_set_value(reg_info->key,
                                          reg_info->kid_register_index,
                                          reg_entry->register_index);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_set_value(reg_info->data,
                                           reg_info->did_value,
                                           reg_entry->value);
    assert(bf_status == BF_SUCCESS);

    // Call table entry add API
    bf_status = bf_rt_table_entry_get(reg, session, dev_tgt, reg_info->key,
                                      reg_info->data, ENTRY_READ_FROM_HW);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);

    // Get the real values in the Data object
    // Notice: I don't know whether bf_rt_data_field_get_value_u64_array works
    // fine here, instead bf_rt_data_field_get_value_u64_array.
    bf_status = bf_rt_data_field_get_value_u64_array_size(
        reg_info->data, reg_info->did_value, &reg_entry->value_array_size
    );
    assert(bf_status == BF_SUCCESS);
    if (reg_entry->value_array) {
        printf("Debug: Free reg_entry->value_array\n");
        free(reg_entry->value_array);
    }
    reg_entry->value_array = (uint64_t *)malloc
                             (reg_entry->value_array_size * sizeof(uint64_t));
    bf_status = bf_rt_data_field_get_value_u64_array(reg_info->data,
                                                     reg_info->did_value,
                                                     reg_entry->value_array);
    assert(bf_status == BF_SUCCESS);
}


static void telemetry_pkt_num_setup(const bf_rt_info_hdl *bfrt_info,
                               telemetry_pkt_num_t *pkt_num_reg, uint32_t win_idx) {

    // Set up the pkt num register win0-3
//    for (int i = 0; i < 4; i ++ ) {
//        char reg_name[100];
//        sprintf(reg_name, "SwitchEgress.re_telemetry_pkt_num_win%d", i);
//  
//        register_setup(bfrt_info, reg_name, 
//                    NULL, &pkt_num_reg[i]->reg, &pkt_num_reg[i]->reg_info);
//        printf("Finished set up Register Name: %s\n", reg_name);
//    }
      char reg_name[100];
      sprintf(reg_name, "SwitchEgress.re_telemetry_pkt_num_win%d", win_idx);
  
      register_setup(bfrt_info, reg_name, 
                    NULL, &pkt_num_reg->reg, &pkt_num_reg->reg_info);
      printf("Finished set up Register Name: %s\n", reg_name);
}

static void telemetry_paused_num_setup(const bf_rt_info_hdl *bfrt_info,
                               			telemetry_paused_num_t *paused_num_reg,
										uint32_t win_idx) {

    // Set up the paused num register win0-3
    char reg_name[100];
    sprintf(reg_name, "SwitchEgress.re_telemetry_paused_num_win%d", win_idx);
  
    register_setup(bfrt_info, reg_name, 
                    NULL, &paused_num_reg->reg, &paused_num_reg->reg_info);
    printf("Finished set up Register Name: %s\n", reg_name);
        
}

static void telemetry_egress_port_setup(const bf_rt_info_hdl *bfrt_info,
                               telemetry_egress_port_t *egress_port_reg) {

    // Set up the egress port register win0-3
    char reg_name[100];
    sprintf(reg_name, "SwitchEgress.re_telemetry_egress_port");
  
    printf("Set up Register Name: %s\n", reg_name);
    register_setup(bfrt_info, reg_name, 
                NULL, &egress_port_reg->reg, &egress_port_reg->reg_info);
}

void telemetry_pkt_num_read(const bf_rt_target_t *dev_tgt,
								   const bf_rt_session_hdl *session,
								   telemetry_pkt_num_t *telemetry_pkt_num_ptr,
								  // const bf_rt_table_hdl *reg,
								  // const uint8_t win_idx,
								   const uint32_t reg_idx) {
//pkt_num_reg[table_idx]->reg, idx

	bf_status_t bf_status;
    register_entry_t reg_entry;
    reg_entry.value_array = NULL;
    //memset(&reg_info, 0, sizeof(register_info_t));
    reg_entry.register_index = reg_idx;
    printf("Debug:  reg_idx %u, ", reg_idx);
    register_read(dev_tgt, session, telemetry_pkt_num_ptr->reg,
				 &telemetry_pkt_num_ptr->reg_info, &reg_entry);
    printf("Value %lx\n", reg_entry.value_array[2]);
}	

void telemetry_paused_num_read(const bf_rt_target_t *dev_tgt,
								   const bf_rt_session_hdl *session,
								   telemetry_paused_num_t *telemetry_paused_num_ptr,
								   const uint32_t reg_idx,
								   FILE* file) {

	bf_status_t bf_status;
    register_entry_t reg_entry;
    reg_entry.value_array = NULL;
    //memset(&reg_info, 0, sizeof(register_info_t));
    reg_entry.register_index = reg_idx;
    register_read(dev_tgt, session, telemetry_paused_num_ptr->reg,
				 &telemetry_paused_num_ptr->reg_info, &reg_entry);
//    if (filename != NULL) {
//        //print to file 
//        FILE *file = fopen(filename, "w");
//        if (file == NULL) {
//            perror("Failed to open file");
//            return;
//        }
//        fprintf(file, "Debug: reg_idx %u\n", reg_idx);
//        fprintf(file, "Value %lx\n", reg_entry.value_array[2]);
//        fclose(file);
//    }
// TODO: check non-zero value
	if (reg_entry.value_array[2] == 0)
	{	// zero value, not care
		return;
	}
// end TODO
	if (file != NULL) {
        fprintf(file, "Debug: reg_idx %u\n", reg_idx);
        fprintf(file, "Value %lx\n", reg_entry.value_array[2]);
    }
	else {
      //print to std  
		printf("Debug: reg_idx %u\n", reg_idx);
      	printf("Value %lx\n", reg_entry.value_array[2]);
  	}
//	printf("Debug:  reg_idx %u, ", reg_idx);
//    printf("Value %lx\n", reg_entry.value_array[2]);

}

static void re_lock_flag_setup(const bf_rt_info_hdl *bfrt_info,
                               re_lock_flag_t *re_lock_flag) {

    register_setup(bfrt_info, "SwitchEgress.re_lock_flag", 
                NULL, &re_lock_flag->reg, &re_lock_flag->reg_info);
    printf("Finished set up Register Name: SwitchEgress.re_lock_flag\n");
}
void re_lock_flag_read(const bf_rt_target_t *dev_tgt,
							   const bf_rt_session_hdl *session,
                               re_lock_flag_t *re_lock_flag) {

	bf_status_t bf_status;
    register_entry_t reg_entry;
    reg_entry.value_array = NULL;
    //memset(&reg_info, 0, sizeof(register_info_t));
    reg_entry.register_index = 0;
    printf("Debug: re_lock_flag reg_idx %u, ", reg_entry.register_index);
    register_read(dev_tgt, session, re_lock_flag->reg,
				 &re_lock_flag->reg_info, &reg_entry);
    printf("Value %lx\n", reg_entry.value_array[2]);
    
}

void re_lock_flag_write_to_completion(const bf_rt_target_t *dev_tgt,
							   const bf_rt_session_hdl *session,
                               re_lock_flag_t *re_lock_flag, uint8_t value) {

    printf("Debug: start write re_lock_flag: set up reg_entry\n");
    register_entry_t reg_entry;
    reg_entry.register_index = 0;
    reg_entry.value = value;

//    if (&reg_entry.value==NULL){
//        printf("null reg_entry_value\n");
//    }
//    else {
//        printf("reg_entry value not null\n");
//    }

//    printf("new reg_entry.value: %u\n", reg_entry.value);
//    printf("Debug: write re_lock_flag reg_idx %u, ", reg_entry.register_index);
    register_write(dev_tgt, session, re_lock_flag->reg,
				 &re_lock_flag->reg_info, &reg_entry);
    printf("Register re_lock_flag is set to "
                "%u correctly!\n", reg_entry.value);
}


int create_update_polling_channel(update_polling_channel_t *channel) {
	struct ifreq cpuif_req;
	struct sockaddr_ll sock_addr;
    int sock_addrlen = sizeof(sock_addr);
	char cpuif_name[IFNAMSIZ];

    /* Get interface name */
    strcpy(cpuif_name, CPUIF_NAME);

    channel->sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETHER_TYPE_POLLING));
    /* Open RAW socket to send on */
	if (channel->sockfd == -1) {
	    perror("socket");
        return -1;
	}

    memset(&cpuif_req, 0, sizeof(struct ifreq));
    strncpy(cpuif_req.ifr_name, cpuif_name, IFNAMSIZ - 1);
    ioctl(channel->sockfd, SIOCGIFFLAGS, &cpuif_req);
    cpuif_req.ifr_flags |= IFF_PROMISC;
    ioctl(channel->sockfd, SIOCSIFFLAGS, &cpuif_req);

	if (setsockopt(channel->sockfd, SOL_SOCKET, SO_BINDTODEVICE,
                   cpuif_name, IFNAMSIZ - 1) == -1)	{
		perror("SO_BINDTODEVICE");
		close(channel->sockfd);
        return -1;
	}

	/* Construct the Ethernet header */
	memset(channel->recvbuf, 0, PKTBUF_SIZE);

    return 0;

}

//ECMP related
/* Function to print polling_h details */
static void set_ecmp_table(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session)
{


    bf_status_t bf_status;

    // Get table object from name
    bf_rt_table_hdl *for_table;
    bf_rt_table_hdl *ap_table;
    bf_rt_table_hdl *sel_table;
    
    bf_rt_table_key_hdl *ap_key;
    bf_rt_table_data_hdl *ap_data;
    bf_rt_table_key_hdl *sel_key;
    bf_rt_table_data_hdl *sel_data;


    bf_rt_table_key_hdl *for_key;
    bf_rt_table_data_hdl *for_data;
    bf_rt_id_t ap_key_id;
    bf_rt_id_t ap_action_id;
    bf_rt_id_t ap_data_id;
    bf_rt_id_t sel_key_id;
    bf_rt_id_t sel_action_id;
    bf_rt_id_t sel_data_id;
    bf_rt_id_t for_key_id;
    bf_rt_id_t for_action_id;
    bf_rt_id_t for_data_id;

    bf_status = bf_rt_table_from_name_get(bfrt_info,
                                          "SwitchIngress.ipv4_lpm",
                                          &for_table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_from_name_get(bfrt_info,
                                          "SwitchIngress.ipv4_ecmp_ap",
                                          &ap_table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_from_name_get(bfrt_info,
                                          "SwitchIngress.ipv4_ecmp",
                                          &sel_table);
    assert(bf_status == BF_SUCCESS);
    // Set actioon profile table
    bf_status = bf_rt_table_key_allocate(ap_table, &ap_key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(ap_table, &ap_data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(ap_table, "$ACTION_MEMBER_ID",
                                       &ap_key_id);
    assert(bf_status == BF_SUCCESS);
    // Get action Ids for action ai_ipv4_forward
    bf_status = bf_rt_action_name_to_id(ap_table, "SwitchIngress.set_port",
                                        &ap_action_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_with_action_get(ap_table, "port", ap_action_id, &ap_data_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(ap_table, &ap_key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(ap_key, ap_key_id, 11);// memeber id 11
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(ap_table,ap_action_id,&ap_data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_set_value(ap_data, ap_data_id, 11); //outport 11
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(ap_table, session, dev_tgt, ap_key, ap_data);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_rt_table_key_reset(ap_table, &ap_key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(ap_key, ap_key_id, 12);// memeber id 12
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(ap_table,ap_action_id,&ap_data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_set_value(ap_data, ap_data_id, 12); //outport 12
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(ap_table, session, dev_tgt, ap_key, ap_data);
    assert(bf_status == BF_SUCCESS);
    //set action selector table
    bf_status = bf_rt_table_key_allocate(sel_table, &sel_key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(sel_table, &sel_data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(sel_table, "$SELECTOR_GROUP_ID",
                                       &sel_key_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_get(sel_table, "$MAX_GROUP_SIZE",
                                        &sel_data_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(sel_key, sel_key_id, 1); //group id 1
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_set_value(sel_data, sel_data_id, 200);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_get(sel_table, "$ACTION_MEMBER_ID", &sel_data_id);
    assert(bf_status == BF_SUCCESS);
    uint32_t val[] = {12, 11};
    bf_status = bf_rt_data_field_set_value_array(sel_data, sel_data_id, val, 2);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_get(sel_table, "$ACTION_MEMBER_STATUS", &sel_data_id);
    assert(bf_status == BF_SUCCESS);
    bool bool_val[] = {true, true};
    bf_status = bf_rt_data_field_set_value_bool_array(sel_data, sel_data_id, bool_val, 2);
    assert(bf_status == BF_SUCCESS);  
    bf_status = bf_rt_table_entry_add(sel_table, session, dev_tgt, sel_key, sel_data);
    assert(bf_status == BF_SUCCESS); 
    //set forward table
    bf_status = bf_rt_table_key_allocate(for_table, &for_key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(for_table, &for_data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(for_table, "ig_intr_md.ingress_port", &for_key_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_get(for_table, "$SELECTOR_GROUP_ID", &for_data_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(for_key, for_key_id, 10); //入端口设为10
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_set_value(for_data, for_data_id, 1);// 匹配的组为1
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(for_table, session, dev_tgt, for_key, for_data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("ECMP table is deployed correctly!\n");
}


static void set_forward_table(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session)
{
    printf("start to set up forward table\n");
    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *table;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data;
    bf_rt_id_t key_id;
    bf_rt_id_t action_id;
    bf_rt_id_t data_id;
    bf_status = bf_rt_table_from_name_get(bfrt_info,
                                          "SwitchIngress.forward_table",
                                          &table);
    assert(bf_status == BF_SUCCESS);
    printf("find table SwitchIngress.forward_table\n");
    // Allocate key and data once, and use reset across different uses
    bf_status = bf_rt_table_key_allocate(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data);
    assert(bf_status == BF_SUCCESS);
    printf("find key and data\n");
    // Get field-ids for key field
    bf_status = bf_rt_key_field_id_get(table, "ig_intr_md.ingress_port",
                                       &key_id);
    assert(bf_status == BF_SUCCESS);
    printf("find key_id\n");
    // Get action Ids for action ai_ipv4_forward
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.forward",
                                        &action_id);
    assert(bf_status == BF_SUCCESS);
    printf("find action_id\n");
    // Get field-ids for data field
    bf_status = bf_rt_data_field_id_with_action_get(
        table, "port",
        action_id,
        &data_id);
    assert(bf_status == BF_SUCCESS);
    printf("find data_id\n");

    bf_rt_table_key_reset(table, &key);
    bf_status = bf_rt_key_field_set_value(key, key_id, 10);
    printf("set key value\n");
    assert(bf_status == BF_SUCCESS);
    bf_rt_table_action_data_reset(table,action_id,&data);
    printf("reset data\n");
    bf_status = bf_rt_data_field_set_value(data, data_id, 12);
    printf("set data value\n");
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data);
    assert(bf_status == BF_SUCCESS);
    printf("add entry\n");
    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);

    printf("forward table is deployed correctly!\n");
}

void set_inPort2caverPort(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){

    printf("start to set up inPort2caverPort table\n");


    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *table;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data;
    bf_rt_id_t key_id;
    bf_rt_id_t action_id;
    bf_rt_id_t data_id;
    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.inPort2caverPort", &table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_scope_attributes_allocate(table, &table_attributes);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(table_attributes, false);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_attributes_set(table, session, dev_tgt, table_attributes);
    assert(bf_status == BF_SUCCESS);
    printf("set table attributes\n");


    bf_status = bf_rt_table_key_allocate(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_intr_md.ingress_port", &key_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.inPortToCaverPort_action", &action_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id, &data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_with_action_get(table, "port", action_id, &data_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id,&data);
    assert(bf_status == BF_SUCCESS);
    printf("find key_id, action_id, data_id\n");

    printf("pipeline id: %d\n", dev_tgt->pipe_id);
    const caver_dp_port_t* group = caver_port_lists[dev_tgt->pipe_id];
    size_t size = CAVER_PORT_SIZES[dev_tgt->pipe_id];

    uint64_t caver_port;
    uint64_t dp_port;

    for (size_t i = 0; i < size; i++) {
        printf("Caver_port: %" PRIu64 ", DP_port: %" PRIu64 "\n", group[i].caver_port, group[i].dp_port);
        caver_port = group[i].caver_port;
        dp_port = group[i].dp_port;
        bf_status = bf_rt_key_field_set_value(key, key_id, dp_port);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(data, data_id, caver_port);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data);
        assert(bf_status == BF_SUCCESS);
    }

    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("inPort2caverPort table is deployed correctly!\n");
}
void set_flow_id_table(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){
    printf("start to set up flow_id_table\n");


    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *table;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data;
    bf_rt_id_t key_id;
    bf_rt_id_t action_id;
    bf_rt_id_t data_id;
    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.flow_id_table", &table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_scope_attributes_allocate(table, &table_attributes);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(table_attributes, false);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_attributes_set(table, session, dev_tgt, table_attributes);
    assert(bf_status == BF_SUCCESS);
    printf("set table attributes\n");


    bf_status = bf_rt_table_key_allocate(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.packet_type", &key_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.get_flow_id", &action_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id, &data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id,&data);
    assert(bf_status == BF_SUCCESS);
    printf("find key_id, action_id, data_id\n");      

    bf_status = bf_rt_key_field_set_value(key, key_id, m_DATA);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data);
    assert(bf_status == BF_SUCCESS);    
    printf("add entry\n");  

    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("flow_id_table is deployed correctly!\n");                
}
void set_flow_time_table(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){
    printf("start to set up flow_time_table\n");


    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *table;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data;
    bf_rt_id_t key_id;
    bf_rt_id_t action_id;
    bf_rt_id_t data_id;
    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.flow_time_table", &table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_scope_attributes_allocate(table, &table_attributes);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(table_attributes, false);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_attributes_set(table, session, dev_tgt, table_attributes);
    assert(bf_status == BF_SUCCESS);
    printf("set table attributes\n");


    bf_status = bf_rt_table_key_allocate(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.packet_type", &key_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.flow_time_update", &action_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id, &data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id,&data);
    assert(bf_status == BF_SUCCESS);
    printf("find key_id, action_id, data_id\n");      

    bf_status = bf_rt_key_field_set_value(key, key_id, m_DATA);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data);
    assert(bf_status == BF_SUCCESS);    
    printf("add entry\n");  

    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("flow_time_table is deployed correctly!\n");                
}


void set_BestTable(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){
    printf("start to set up BestTable\n");


    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *table;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data_check;
    bf_rt_table_data_hdl *data_update;
    bf_rt_table_data_hdl *data_read;
    bf_rt_id_t key_id_ip;
    bf_rt_id_t key_id_type;
    bf_rt_id_t key_id_equal;
    bf_rt_id_t action_id_check;
    bf_rt_id_t action_id_update;
    bf_rt_id_t action_id_read;
    bf_rt_id_t data_id_check;
    bf_rt_id_t data_id_update;
    bf_rt_id_t data_id_read;
    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.BestTable", &table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_scope_attributes_allocate(table, &table_attributes);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(table_attributes, false);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_attributes_set(table, session, dev_tgt, table_attributes);
    assert(bf_status == BF_SUCCESS);
    printf("set table attributes\n");


    bf_status = bf_rt_table_key_allocate(table, &key);
    assert(bf_status == BF_SUCCESS);
    // bf_status = bf_rt_table_data_allocate(table, &data);
    // assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "hdr.ipv4.src_ip", &key_id_ip);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.packet_type", &key_id_type);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.port_equal", &key_id_equal);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.check_BestTable", &action_id_check);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.update_BestTable", &action_id_update);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.read_port_BestTable", &action_id_read);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_check, &data_check);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_update, &data_update);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_read, &data_read);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_with_action_get(table, "host", action_id_check, &data_id_check);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_with_action_get(table, "host", action_id_update, &data_id_update);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_with_action_get(table, "host", action_id_read, &data_id_read);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_check,&data_check);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_update,&data_update);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_read,&data_read);
    assert(bf_status == BF_SUCCESS);
    printf("find key_id, action_id, data_id\n");     

    uint32_t ip;
    uint64_t id;
    for (int i = 0; i < IP_ID_SIZE; i++) {
        ip = ipv4AddrToUint32(ip_id_list[i].ip);
        id = ip_id_list[i].id;
        bf_status = bf_rt_key_field_set_value(key, key_id_ip, ip);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(key, key_id_type, m_CAVER_ACK_SECOND);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(key, key_id_equal, 0);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(data_check, data_id_check, id);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_check);
        assert(bf_status == BF_SUCCESS);    

        bf_status = bf_rt_key_field_set_value(key, key_id_ip, ip);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(key, key_id_type, m_CAVER_ACK_SECOND);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(key, key_id_equal, 1);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(data_update, data_id_update, id);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_update);
        assert(bf_status == BF_SUCCESS); 

        bf_status = bf_rt_key_field_set_value(key, key_id_ip, ip);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(key, key_id_type, m_CAVER_ACK_FIRST);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(key, key_id_equal, 0);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(data_read, data_id_read, id);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_read);
        assert(bf_status == BF_SUCCESS);    

        bf_status = bf_rt_key_field_set_value(key, key_id_ip, ip);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(key, key_id_type, m_CAVER_ACK_FIRST);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(key, key_id_equal, 1);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(data_read, data_id_read, id);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_read);
        assert(bf_status == BF_SUCCESS);    
    }
    printf("add entry\n");
    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("BestTable is deployed correctly!\n");                
}

void set_judge_acceptable_table(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){
    printf("start to set up judge_acceptable_table\n");


    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *table;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data;
    bf_rt_id_t key_id;
    bf_rt_id_t action_id;
    bf_rt_id_t data_id;
    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.judge_acceptable_table", &table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_scope_attributes_allocate(table, &table_attributes);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(table_attributes, false);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_attributes_set(table, session, dev_tgt, table_attributes);
    assert(bf_status == BF_SUCCESS);
    printf("set table attributes\n");


    bf_status = bf_rt_table_key_allocate(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.packet_type", &key_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.judge_acceptable", &action_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id, &data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id,&data);
    assert(bf_status == BF_SUCCESS);
    printf("find key_id, action_id, data_id\n");      

    bf_status = bf_rt_key_field_set_value(key, key_id, m_CAVER_ACK_SECOND);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data);
    assert(bf_status == BF_SUCCESS);    
    printf("add entry\n");  

    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("judge_acceptable_table is deployed correctly!\n");                
}

void set_set_acceptable_path(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){
    printf("start to set up set_acceptable_path\n");


    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *table;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data_best;
    bf_rt_table_data_hdl *data_good;
    bf_rt_id_t key_id_type;
    bf_rt_id_t key_id_good;
    bf_rt_id_t key_id_threshold;
    bf_rt_id_t action_id_best;
    bf_rt_id_t action_id_good;
    bf_rt_id_t data_id_best;
    bf_rt_id_t data_id_good;
    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.set_acceptable_path", &table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_scope_attributes_allocate(table, &table_attributes);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(table_attributes, false);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_attributes_set(table, session, dev_tgt, table_attributes);
    assert(bf_status == BF_SUCCESS);
    printf("set table attributes\n");


    bf_status = bf_rt_table_key_allocate(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data_best);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data_good);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.packet_type", &key_id_type);
    assert(bf_status == BF_SUCCESS);
    // bf_status = bf_rt_key_field_id_get(table, "ig_md.delta_CE", &key_id_acc);
    // assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.before_goodCE", &key_id_good);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.thresold_CE", &key_id_threshold);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.use_best_path", &action_id_best);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.use_good_path", &action_id_good);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_best, &data_best);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_good, &data_good);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_best,&data_best);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_good,&data_good);
    assert(bf_status == BF_SUCCESS);
    printf("find key_id, action_id, data_id\n");      

    for (int good = 0; good < CE_max; good++){
        for (int threshold = 0; threshold < good; threshold++){
            bf_status = bf_rt_key_field_set_value(key, key_id_type, m_CAVER_ACK_SECOND);
            assert(bf_status == BF_SUCCESS);
            bf_status = bf_rt_key_field_set_value(key, key_id_good, good);
            assert(bf_status == BF_SUCCESS);
            bf_status = bf_rt_key_field_set_value(key, key_id_threshold, threshold);
            assert(bf_status == BF_SUCCESS);
            bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_best);
            assert(bf_status == BF_SUCCESS);
        }
    }
    for (int good = 0; good < CE_max; good++){
        for (int threshold = good; threshold < CE_max * 1.2; threshold++){
            bf_status = bf_rt_key_field_set_value(key, key_id_type, m_CAVER_ACK_SECOND);
            assert(bf_status == BF_SUCCESS);
            bf_status = bf_rt_key_field_set_value(key, key_id_good, good);
            assert(bf_status == BF_SUCCESS);
            bf_status = bf_rt_key_field_set_value(key, key_id_threshold, threshold);
            assert(bf_status == BF_SUCCESS);
            bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_good);
            assert(bf_status == BF_SUCCESS);
        }
    }
    // bf_status = bf_rt_key_field_set_value(key, key_id_type, m_CAVER_ACK_SECOND);
    // assert(bf_status == BF_SUCCESS);
    // bf_status = bf_rt_key_field_set_value_and_mask(key, key_id_acc, 0x8000, 0x8000);//acceptable->use_good_path
    // assert(bf_status == BF_SUCCESS);
    // bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_good);
    // assert(bf_status == BF_SUCCESS);

    // bf_status = bf_rt_key_field_set_value(key, key_id_type, m_CAVER_ACK_SECOND);
    // assert(bf_status == BF_SUCCESS);
    // bf_status = bf_rt_key_field_set_value_and_mask(key, key_id_acc, 0x0000, 0x8000);//ig_md.before_goodCE - ig_md.thresold_CE最高位是0，说明goodCE大于阈值，此时应该用bestpath
    // assert(bf_status == BF_SUCCESS);
    // bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_best);
    // assert(bf_status == BF_SUCCESS);  
    // printf("add entry\n");  

    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("set_acceptable_path is deployed correctly!\n");                
}

void set_stack_count_table(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){
    printf("start to set up stack_count_table\n");


    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *table;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data_push;
    bf_rt_table_data_hdl *data_pop;
    bf_rt_id_t key_id_type;
    bf_rt_id_t key_id_new_flow;
    bf_rt_id_t key_id_ip;
    bf_rt_id_t action_id_push;
    bf_rt_id_t action_id_pop;
    bf_rt_id_t data_id_push;
    bf_rt_id_t data_id_pop;
    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.stack_count_table", &table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_scope_attributes_allocate(table, &table_attributes);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(table_attributes, false);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_attributes_set(table, session, dev_tgt, table_attributes);
    assert(bf_status == BF_SUCCESS);
    printf("set table attributes\n");


    bf_status = bf_rt_table_key_allocate(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data_push);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data_pop);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.packet_type", &key_id_type);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.new_flow", &key_id_new_flow);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.table_ip", &key_id_ip);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.stack_count_push", &action_id_push);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.stack_count_pop", &action_id_pop);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_push, &data_push);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_pop, &data_pop);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_with_action_get(table, "host_id", action_id_push, &data_id_push);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_with_action_get(table, "host_id", action_id_pop, &data_id_pop);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_push,&data_push);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_pop,&data_pop);
    assert(bf_status == BF_SUCCESS);
    printf("find key_id, action_id, data_id\n");      
    //for data;
    uint32_t ip;
    uint64_t id;
    for (int i = 0; i < IP_ID_SIZE; i++) {
        ip = ipv4AddrToUint32(ip_id_list[i].ip);
        id = ip_id_list[i].id;
        bf_status = bf_rt_key_field_set_value(key, key_id_ip, ip);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(key, key_id_type, m_DATA);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(key, key_id_new_flow, 1);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(data_pop, data_id_pop, id);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_pop);
        assert(bf_status == BF_SUCCESS);    
    }
    //for ack;
    for (int i = 0; i < IP_ID_SIZE; i++) {
        ip = ipv4AddrToUint32(ip_id_list[i].ip);
        id = ip_id_list[i].id;
        bf_status = bf_rt_key_field_set_value(key, key_id_ip, ip);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(key, key_id_type, m_CAVER_ACK_SECOND);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(key, key_id_new_flow, 1);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(data_push, data_id_push, id);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_push);
        assert(bf_status == BF_SUCCESS);    
    }
    for (int i = 0; i < IP_ID_SIZE; i++) {
        ip = ipv4AddrToUint32(ip_id_list[i].ip);
        id = ip_id_list[i].id;
        bf_status = bf_rt_key_field_set_value(key, key_id_ip, ip);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(key, key_id_type, m_CAVER_ACK_SECOND);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(key, key_id_new_flow, 0);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(data_push, data_id_push, id);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_push);
        assert(bf_status == BF_SUCCESS);    
    }
    printf("add entry\n");  

    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("stack_count_table is deployed correctly!\n");                
}


void set_stack_top_table(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){
    printf("start to set up stack_top_table\n");


    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *table;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data_push;
    bf_rt_table_data_hdl *data_pop;
    bf_rt_id_t key_id_type;
    bf_rt_id_t key_id_new_flow;
    bf_rt_id_t key_id_ip;
    bf_rt_id_t key_id_src_route;
    bf_rt_id_t action_id_push;
    bf_rt_id_t action_id_pop;
    bf_rt_id_t data_id_push;
    bf_rt_id_t data_id_pop;
    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.stack_top_table", &table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_scope_attributes_allocate(table, &table_attributes);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(table_attributes, false);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_attributes_set(table, session, dev_tgt, table_attributes);
    assert(bf_status == BF_SUCCESS);
    printf("set table attributes\n");


    bf_status = bf_rt_table_key_allocate(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data_push);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data_pop);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.packet_type", &key_id_type);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.new_flow", &key_id_new_flow);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.table_ip", &key_id_ip);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.src_route", &key_id_src_route);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.stack_top_push", &action_id_push);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.stack_top_pop", &action_id_pop);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_push, &data_push);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_pop, &data_pop);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_with_action_get(table, "host_id", action_id_push, &data_id_push);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_with_action_get(table, "host_id", action_id_pop, &data_id_pop);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_push,&data_push);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_pop,&data_pop);
    assert(bf_status == BF_SUCCESS);
    printf("find key_id, action_id, data_id\n");      
    //for data;
    uint32_t ip;
    uint64_t id;
    for (int i = 0; i < IP_ID_SIZE; i++) {
        ip = ipv4AddrToUint32(ip_id_list[i].ip);
        id = ip_id_list[i].id;
        bf_status = bf_rt_key_field_set_value(key, key_id_ip, ip);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(key, key_id_type, m_DATA);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(key, key_id_new_flow, 1);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(key, key_id_src_route, 1);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(data_pop, data_id_pop, id);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_pop);
        assert(bf_status == BF_SUCCESS);    
    }
    //for ack;
    for (int i = 0; i < IP_ID_SIZE; i++) {
        ip = ipv4AddrToUint32(ip_id_list[i].ip);
        id = ip_id_list[i].id;
        for (int j = 0; j < 2; j++){
            for (int k = 0; k < 1; k++){
                bf_status = bf_rt_key_field_set_value(key, key_id_ip, ip);
                assert(bf_status == BF_SUCCESS);
                bf_status = bf_rt_key_field_set_value(key, key_id_type, m_CAVER_ACK_SECOND);
                assert(bf_status == BF_SUCCESS);
                bf_status = bf_rt_key_field_set_value(key, key_id_new_flow, j);
                assert(bf_status == BF_SUCCESS);
                bf_status = bf_rt_key_field_set_value(key, key_id_src_route, k);
                assert(bf_status == BF_SUCCESS);
                bf_status = bf_rt_data_field_set_value(data_push, data_id_push, id);
                assert(bf_status == BF_SUCCESS);
                bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_push);
                assert(bf_status == BF_SUCCESS);  
            }
        }
    }
    printf("add entry\n");  

    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("stack_top_table is deployed correctly!\n");                
}

void set_path_table(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){
    printf("start to set up path_table\n");


    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *table;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data_take;
    bf_rt_table_data_hdl *data_update;
    bf_rt_id_t key_id_type;
    bf_rt_id_t key_id_new_flow;
    bf_rt_id_t key_id_ip;
    bf_rt_id_t key_id_src_route;
    bf_rt_id_t key_id_index;
    bf_rt_id_t action_id_take;
    bf_rt_id_t action_id_update;
    bf_rt_id_t data_id_take_index;
    bf_rt_id_t data_id_update_index;
    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.path_table", &table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_scope_attributes_allocate(table, &table_attributes);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(table_attributes, false);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_attributes_set(table, session, dev_tgt, table_attributes);
    assert(bf_status == BF_SUCCESS);
    printf("set table attributes\n");


    bf_status = bf_rt_table_key_allocate(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data_take);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data_update);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.packet_type", &key_id_type);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.new_flow", &key_id_new_flow);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.table_ip", &key_id_ip);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.src_route", &key_id_src_route);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.delta_index", &key_id_index);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.path_take", &action_id_take);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.path_update", &action_id_update);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_take, &data_take);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_update, &data_update);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_with_action_get(table, "index", action_id_take, &data_id_take_index);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_with_action_get(table, "index", action_id_update, &data_id_update_index);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_take,&data_take);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_update,&data_update);
    assert(bf_status == BF_SUCCESS);
    printf("find key_id, action_id, data_id\n");      
    //for data;
    uint32_t ip;
    uint64_t id;
    for (int i = 0; i < IP_ID_SIZE; i++) {
        ip = ipv4AddrToUint32(ip_id_list[i].ip);
        id = ip_id_list[i].id;
        for (int j = 0; j < 4; j++){
            bf_status = bf_rt_key_field_set_value(key, key_id_ip, ip);
            assert(bf_status == BF_SUCCESS);
            bf_status = bf_rt_key_field_set_value(key, key_id_type, m_DATA);
            assert(bf_status == BF_SUCCESS);
            bf_status = bf_rt_key_field_set_value(key, key_id_new_flow, 1);
            assert(bf_status == BF_SUCCESS);
            bf_status = bf_rt_key_field_set_value(key, key_id_src_route, 1);
            assert(bf_status == BF_SUCCESS);
            bf_status = bf_rt_key_field_set_value(key, key_id_index, j);
            assert(bf_status == BF_SUCCESS);
            bf_status = bf_rt_data_field_set_value(data_take, data_id_take_index, id * 4 + j);
            assert(bf_status == BF_SUCCESS);
            bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_take);
            assert(bf_status == BF_SUCCESS);    
        }
    }
    //for ack;
    for (int i = 0; i < IP_ID_SIZE; i++) {
        ip = ipv4AddrToUint32(ip_id_list[i].ip);
        id = ip_id_list[i].id;
        for (int j = 0; j < 2; j++){
            for (int k = 0; k < 2; k++){
                for (int m = 0; m < 4; m++){
                    bf_status = bf_rt_key_field_set_value(key, key_id_ip, ip);
                    assert(bf_status == BF_SUCCESS);
                    bf_status = bf_rt_key_field_set_value(key, key_id_type, m_CAVER_ACK_SECOND);
                    assert(bf_status == BF_SUCCESS);
                    bf_status = bf_rt_key_field_set_value(key, key_id_new_flow, j);
                    assert(bf_status == BF_SUCCESS);
                    bf_status = bf_rt_key_field_set_value(key, key_id_src_route, k);
                    assert(bf_status == BF_SUCCESS);
                    bf_status = bf_rt_key_field_set_value(key, key_id_index, m);
                    assert(bf_status == BF_SUCCESS);
                    bf_status = bf_rt_data_field_set_value(data_update, data_id_update_index, id * 4 + m);
                    assert(bf_status == BF_SUCCESS);
                    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_update);
                    assert(bf_status == BF_SUCCESS);  
                }
            }
        }
    }
    printf("add entry\n");  

    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("path_table is deployed correctly!\n");                
}

void set_flow_method_table(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){
    printf("start to set up flow_method_table\n");


    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *table;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data_take;
    bf_rt_table_data_hdl *data_update;

    bf_rt_id_t key_id_type;
    bf_rt_id_t key_id_new_flow;

    bf_rt_id_t action_id_take;
    bf_rt_id_t action_id_update;
    bf_rt_id_t data_id_take;
    bf_rt_id_t data_id_update;
    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.flow_method", &table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_scope_attributes_allocate(table, &table_attributes);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(table_attributes, false);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_attributes_set(table, session, dev_tgt, table_attributes);
    assert(bf_status == BF_SUCCESS);
    printf("set table attributes\n");


    bf_status = bf_rt_table_key_allocate(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data_take);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data_update);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.packet_type", &key_id_type);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.new_flow", &key_id_new_flow);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.flow_method_update", &action_id_update);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.flow_method_take", &action_id_take);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_update, &data_update);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_take, &data_take);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_take,&data_take);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_update,&data_update);
    assert(bf_status == BF_SUCCESS);
    printf("find key_id, action_id, data_id\n");      

    bf_status = bf_rt_key_field_set_value(key, key_id_type, m_DATA);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(key, key_id_new_flow, 0);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_take);//old_flow take methods from flow_method_table
    assert(bf_status == BF_SUCCESS);    
    printf("add entry\n");  

    bf_status = bf_rt_key_field_set_value(key, key_id_type, m_DATA);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(key, key_id_new_flow, 1);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_update);//new_flow update flow methods
    assert(bf_status == BF_SUCCESS);    
    printf("add entry\n");  

    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("flow_method_table is deployed correctly!\n");                
}


void set_flow_path(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){
    printf("start to set up flow_path\n");


    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *table;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data_take;
    bf_rt_table_data_hdl *data_update;
    bf_rt_table_data_hdl *data_take_ecmp;

    bf_rt_id_t key_id_type;
    bf_rt_id_t key_id_new_flow;
    bf_rt_id_t key_id_srcRoute;

    bf_rt_id_t action_id_take;
    bf_rt_id_t action_id_update;
    bf_rt_id_t action_id_take_ecmp;
    bf_rt_id_t data_id_take;
    bf_rt_id_t data_id_update;
    bf_rt_id_t data_id_take_ecmp;
    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.flow_path", &table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_scope_attributes_allocate(table, &table_attributes);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(table_attributes, false);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_attributes_set(table, session, dev_tgt, table_attributes);
    assert(bf_status == BF_SUCCESS);
    printf("set table attributes\n");


    bf_status = bf_rt_table_key_allocate(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data_take);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data_update);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data_take_ecmp);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.packet_type", &key_id_type);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.new_flow", &key_id_new_flow);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.src_route", &key_id_srcRoute);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.flow_path_update", &action_id_update);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.flow_path_take", &action_id_take);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.flow_path_ecmp_take", &action_id_take_ecmp);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_update, &data_update);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_take, &data_take);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_take_ecmp, &data_take_ecmp);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_take,&data_take);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_update,&data_update);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_take_ecmp,&data_take_ecmp);
    assert(bf_status == BF_SUCCESS);
    printf("find key_id, action_id, data_id\n");      

    bf_status = bf_rt_key_field_set_value(key, key_id_type, m_DATA);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(key, key_id_new_flow, 0);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(key, key_id_srcRoute, 1);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_take);//old_flow take paths  from flow_path
    assert(bf_status == BF_SUCCESS);    
    printf("add entry\n");  

    bf_status = bf_rt_key_field_set_value(key, key_id_type, m_DATA);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(key, key_id_new_flow, 1);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(key, key_id_srcRoute, 1);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_update); //new_flow update paths in the flow_path
    assert(bf_status == BF_SUCCESS);    
    printf("add entry\n");  

    bf_status = bf_rt_key_field_set_value(key, key_id_type, m_DATA);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(key, key_id_new_flow, 1);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(key, key_id_srcRoute, 0);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_take_ecmp); //old_flow take ecmp paths
    assert(bf_status == BF_SUCCESS);    
    printf("add entry\n");  

    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("flow_path is deployed correctly!\n");                
}

void set_caverPort2switchPort(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){

    printf("start to set up caverPort2switchPort table\n");


    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *table;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data;
    bf_rt_id_t key_id;
    bf_rt_id_t action_id;
    bf_rt_id_t data_id;
    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.caverPort2switchPort", &table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_scope_attributes_allocate(table, &table_attributes);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(table_attributes, false);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_attributes_set(table, session, dev_tgt, table_attributes);
    assert(bf_status == BF_SUCCESS);
    printf("set table attributes\n");


    bf_status = bf_rt_table_key_allocate(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.dre_port", &key_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.traversePort", &action_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id, &data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_with_action_get(table, "port", action_id, &data_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id,&data);
    assert(bf_status == BF_SUCCESS);
    printf("find key_id, action_id, data_id\n");

    printf("pipeline id: %d\n", dev_tgt->pipe_id);
    const caver_dp_port_t* group = caver_port_lists[dev_tgt->pipe_id];
    size_t size = CAVER_PORT_SIZES[dev_tgt->pipe_id];

    uint64_t caver_port;
    uint64_t dp_port;

    for (size_t i = 0; i < size; i++) {
        printf("Caver_port: %" PRIu64 ", DP_port: %" PRIu64 "\n", group[i].caver_port, group[i].dp_port);
        caver_port = group[i].caver_port;
        dp_port = group[i].dp_port;
        bf_status = bf_rt_key_field_set_value(key, key_id, caver_port);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(data, data_id, dp_port);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data);
        assert(bf_status == BF_SUCCESS);
    }

    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("caverPort2switchPort table is deployed correctly!\n");
}

static void set_ipv4_lpm(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){
   
    printf("start to set up ipv4_lpm table\n");   
    bf_status_t bf_status;

    bf_rt_table_hdl *ap_table;
    bf_rt_table_hdl *sel_table;
    bf_rt_table_hdl *for_table;

    bf_rt_table_key_hdl *ap_key;
    bf_rt_table_data_hdl *ap_data_data;
    bf_rt_table_data_hdl *ap_data_ack;
    bf_rt_table_data_hdl *ap_data_resubmit;

    bf_rt_table_key_hdl *sel_key;
    bf_rt_table_data_hdl *sel_data;

    bf_rt_table_key_hdl *for_key;
    bf_rt_table_data_hdl *for_data;

    bf_rt_id_t ap_key_id;

    bf_rt_id_t ap_action_id_data;
    bf_rt_id_t ap_data_id_data_dp_port;
    bf_rt_id_t ap_data_id_data_caver_port;

    bf_rt_id_t ap_action_id_ack;
    bf_rt_id_t ap_data_id_ack_dp_port;
    bf_rt_id_t ap_data_id_ack_caver_port;

    bf_rt_id_t ap_action_id_resubmit;

    bf_rt_id_t sel_key_id;
    bf_rt_id_t sel_action_id;
    bf_rt_id_t sel_data_id;

    bf_rt_id_t for_key_id_ip;
    bf_rt_id_t for_key_id_type;

    bf_rt_id_t for_action_id;
    bf_rt_id_t for_data_id;



    bf_rt_table_attributes_hdl *ap_table_attributes;
    bf_rt_table_attributes_hdl *sel_table_attributes;
    bf_rt_table_attributes_hdl *for_table_attributes;

    //set_ap_table
    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.ipv4_ecmp_ap", &ap_table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.outPort_sel", &sel_table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.ipv4_lpm", &for_table);
    assert(bf_status == BF_SUCCESS);

    // bf_status = bf_rt_table_entry_scope_attributes_allocate(ap_table, &ap_table_attributes);
    // assert(bf_status == BF_SUCCESS);
    // bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(ap_table_attributes, false);
    // assert(bf_status == BF_SUCCESS);
    // bf_status = bf_rt_table_attributes_set(ap_table, session, dev_tgt, ap_table_attributes);
    // assert(bf_status == BF_SUCCESS);
    printf("set ap_table attributes\n");
    // bf_status = bf_rt_table_entry_scope_attributes_allocate(sel_table, &sel_table_attributes);
    // assert(bf_status == BF_SUCCESS);
    // bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(sel_table_attributes, false);
    // assert(bf_status == BF_SUCCESS);
    // bf_status = bf_rt_table_attributes_set(sel_table, session, dev_tgt, sel_table_attributes);
    // assert(bf_status == BF_SUCCESS);
    printf("set sel_table attributes\n");
    bf_status = bf_rt_table_entry_scope_attributes_allocate(for_table, &for_table_attributes);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(for_table_attributes, false);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_attributes_set(for_table, session, dev_tgt, for_table_attributes);
    assert(bf_status == BF_SUCCESS);
    printf("set for_table attributes\n");

    bf_status = bf_rt_table_key_allocate(ap_table, &ap_key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(ap_table, &ap_data_data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(ap_table, &ap_data_ack);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(ap_table, &ap_data_resubmit);
    assert(bf_status == BF_SUCCESS);
    printf("ap_table_allocate\n");

    bf_status = bf_rt_key_field_id_get(ap_table, "$ACTION_MEMBER_ID",&ap_key_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(ap_table, "SwitchIngress.set_port_ecmp_data", &ap_action_id_data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_with_action_get(ap_table, "port", ap_action_id_data, &ap_data_id_data_dp_port);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_with_action_get(ap_table, "caver_port", ap_action_id_data, &ap_data_id_data_caver_port);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(ap_table, &ap_key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(ap_table,ap_action_id_data,&ap_data_data);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_rt_action_name_to_id(ap_table, "SwitchIngress.set_port_ecmp_ack", &ap_action_id_ack);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_with_action_get(ap_table, "port", ap_action_id_ack, &ap_data_id_ack_dp_port);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_with_action_get(ap_table, "caver_port", ap_action_id_ack, &ap_data_id_ack_caver_port);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(ap_table, &ap_key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(ap_table,ap_action_id_ack,&ap_data_ack);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_rt_action_name_to_id(ap_table, "SwitchIngress.resubmit", &ap_action_id_resubmit);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(ap_table, &ap_key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(ap_table,ap_action_id_resubmit,&ap_data_resubmit);
    assert(bf_status == BF_SUCCESS);
    printf("get ap_table key_id, action_id, data_id\n");


    const caver_dp_port_t* group = caver_port_lists[dev_tgt->pipe_id];
    size_t size = CAVER_PORT_SIZES[dev_tgt->pipe_id];

    uint64_t caver_port;
    uint64_t dp_port;

    for (size_t i = 0; i < size; i++) {
        printf("Caver_port: %" PRIu64 ", DP_port: %" PRIu64 "\n", group[i].caver_port, group[i].dp_port);
        caver_port = group[i].caver_port;
        dp_port = group[i].dp_port;

        bf_status = bf_rt_key_field_set_value(ap_key, ap_key_id, i);// memeber id
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(ap_data_data, ap_data_id_data_dp_port, dp_port);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(ap_data_data, ap_data_id_data_caver_port, caver_port); 
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_table_entry_add(ap_table, session, dev_tgt, ap_key, ap_data_data);
        assert(bf_status == BF_SUCCESS);

        bf_status = bf_rt_key_field_set_value(ap_key, ap_key_id, size + i);// memeber id
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(ap_data_ack, ap_data_id_ack_dp_port, dp_port);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(ap_data_ack, ap_data_id_ack_caver_port, caver_port); 
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_table_entry_add(ap_table, session, dev_tgt, ap_key, ap_data_ack);
        assert(bf_status == BF_SUCCESS);
    }
    printf("add ap_table set_port entry\n");

    bf_status = bf_rt_key_field_set_value(ap_key, ap_key_id, 2 * size);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(ap_table, session, dev_tgt, ap_key, ap_data_resubmit);
    assert(bf_status == BF_SUCCESS);
    printf("add ap_table resubmit entry\n");

    //selector table
    bf_status = bf_rt_table_key_allocate(sel_table, &sel_key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(sel_table, &sel_data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(sel_table, "$SELECTOR_GROUP_ID", &sel_key_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(sel_table, &sel_key);
    assert(bf_status == BF_SUCCESS);
    printf("get sel_table key_id\n");

    for (size_t i = 0; i < MAX_NESTED_LISTS; ++i) {
        size_t size = 0;
        const uint32_t* val = get_list(i, &size); // 获取子列表的指针和大小
        const bool* bool_val = get_bool_list(i, &size);
        bf_status = bf_rt_key_field_set_value(sel_key, sel_key_id, i);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_id_get(sel_table, "$MAX_GROUP_SIZE", &sel_data_id);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(sel_data, sel_data_id, 20);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_id_get(sel_table, "$ACTION_MEMBER_ID", &sel_data_id);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value_array(sel_data, sel_data_id, val, size);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_id_get(sel_table, "$ACTION_MEMBER_STATUS", &sel_data_id);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value_bool_array(sel_data, sel_data_id, bool_val, size);
        assert(bf_status == BF_SUCCESS);  
        bf_status = bf_rt_table_entry_add(sel_table, session, dev_tgt, sel_key, sel_data);
        assert(bf_status == BF_SUCCESS); 
    }
    printf("set sel_table entry\n");
    //forward table
    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.ipv4_lpm", &for_table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_allocate(for_table, &for_key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(for_table, &for_data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(for_table, "hdr.ipv4.dst_ip", &for_key_id_ip);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(for_table, "ig_md.packet_type", &for_key_id_type);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_get(for_table, "$SELECTOR_GROUP_ID", &for_data_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(for_table, &for_key);
    assert(bf_status == BF_SUCCESS);
    uint32_t ip;
    uint64_t group_id;
    const ip_id_t* m_list_for_data = ip_group_data_lists[dev_tgt->pipe_id];
    for (uint64_t i = 0; i < ip_SIZE; i++){
        ip = ipv4AddrToUint32(m_list_for_data[i].ip);
        group_id = m_list_for_data[i].id;
        bf_status = bf_rt_key_field_set_value(for_key, for_key_id_ip, ip);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(for_key, for_key_id_type, m_DATA);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(for_data, for_data_id, group_id);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_table_entry_add(for_table, session, dev_tgt, for_key, for_data);
        assert(bf_status == BF_SUCCESS);

        bf_status = bf_rt_key_field_set_value(for_key, for_key_id_ip, ip);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(for_key, for_key_id_type, m_CAVER_DATA);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(for_data, for_data_id, group_id);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_table_entry_add(for_table, session, dev_tgt, for_key, for_data);
        assert(bf_status == BF_SUCCESS);
    }
    const ip_id_t* m_list_for_ack = ip_group_ack_lists[dev_tgt->pipe_id];
    for (uint64_t i = 0; i < ip_SIZE; i++){
        ip = ipv4AddrToUint32(m_list_for_ack[i].ip);
        group_id = m_list_for_ack[i].id;
        bf_status = bf_rt_key_field_set_value(for_key, for_key_id_ip, ip);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(for_key, for_key_id_type, m_ACK);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(for_data, for_data_id, group_id);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_table_entry_add(for_table, session, dev_tgt, for_key, for_data);
        assert(bf_status == BF_SUCCESS);

        bf_status = bf_rt_key_field_set_value(for_key, for_key_id_ip, ip);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(for_key, for_key_id_type, m_CAVER_ACK_SECOND);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(for_data, for_data_id, group_id);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_table_entry_add(for_table, session, dev_tgt, for_key, for_data);
        assert(bf_status == BF_SUCCESS);

        bf_status = bf_rt_key_field_set_value(for_key, for_key_id_ip, ip);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_key_field_set_value(for_key, for_key_id_type, m_CAVER_ACK_FIRST);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(for_data, for_data_id, 10);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_table_entry_add(for_table, session, dev_tgt, for_key, for_data);
        assert(bf_status == BF_SUCCESS);        
    }

    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("ipv4_lpm table is deployed correctly!\n");
}
void set_header_process(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){

    printf("start to set up header_process table\n");


    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *table;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data_ack;
    bf_rt_table_data_hdl *data_data;
    bf_rt_table_data_hdl *data_caver_ack;
    bf_rt_table_data_hdl *data_caver_data;
    bf_rt_id_t key_id;
    bf_rt_id_t action_id_ack;
    bf_rt_id_t action_id_data;
    bf_rt_id_t action_id_caver_ack;
    bf_rt_id_t action_id_caver_data;
    bf_rt_id_t data_id_ack;
    bf_rt_id_t data_id_data;
    bf_rt_id_t data_id_caver_ack;
    bf_rt_id_t data_id_caver_data;

    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.header_process", &table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_scope_attributes_allocate(table, &table_attributes);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(table_attributes, false);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_attributes_set(table, session, dev_tgt, table_attributes);
    assert(bf_status == BF_SUCCESS);
    printf("set table attributes\n");


    bf_status = bf_rt_table_key_allocate(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.packet_type", &key_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.in_coming_ack", &action_id_ack);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_ack, &data_ack);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_ack,&data_ack);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(key, key_id, m_ACK);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_ack);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.in_coming_data", &action_id_data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_data, &data_data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(key, key_id, m_DATA);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_data);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.in_coming_caver_data", &action_id_caver_data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_caver_data, &data_caver_data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(key, key_id, m_CAVER_DATA);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_caver_data);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.in_coming_caver_ack", &action_id_caver_ack);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_caver_ack, &data_caver_ack);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(key, key_id, m_CAVER_ACK_SECOND);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_caver_ack);
    assert(bf_status == BF_SUCCESS);


    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("header_process table is deployed correctly!\n");
}

void set_Dre_time_table(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){

    printf("start to set up Dre_time_table table\n");


    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *table;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data;

    bf_rt_id_t key_id;
    bf_rt_id_t action_id;
    bf_rt_id_t data_id;
    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.Dre_time_table", &table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_scope_attributes_allocate(table, &table_attributes);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(table_attributes, false);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_attributes_set(table, session, dev_tgt, table_attributes);
    assert(bf_status == BF_SUCCESS);
    printf("set table attributes\n");


    bf_status = bf_rt_table_key_allocate(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.packet_type", &key_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.Dre_time_update", &action_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id, &data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id,&data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(key, key_id, m_DATA);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(key, key_id, m_CAVER_DATA);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("Dre_time_table table is deployed correctly!\n");
}

void set_Dre_table(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){

    printf("start to set up Dre_table table\n");


    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *table;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data_data;
    bf_rt_table_data_hdl *data_ack;

    bf_rt_id_t key_id;
    bf_rt_id_t action_id_data;
    bf_rt_id_t action_id_ack;
    bf_rt_id_t data_id_data;
    bf_rt_id_t data_id_ack;
    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.Dre_table", &table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_scope_attributes_allocate(table, &table_attributes);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(table_attributes, false);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_attributes_set(table, session, dev_tgt, table_attributes);
    assert(bf_status == BF_SUCCESS);
    printf("set table attributes\n");


    bf_status = bf_rt_table_key_allocate(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.packet_type", &key_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.ack_dre", &action_id_ack);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.data_dre", &action_id_data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_ack, &data_ack);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_data, &data_data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_ack,&data_ack);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_data,&data_data);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_rt_key_field_set_value(key, key_id, m_DATA);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(key, key_id, m_CAVER_ACK_FIRST);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_ack);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(key, key_id, m_CAVER_DATA);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_data);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("Dre_table table is deployed correctly!\n");
}

void set_get_table_ip(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){

    printf("start to set up get_table_ip table\n");


    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *table;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data_data;
    bf_rt_table_data_hdl *data_ack;

    bf_rt_id_t key_id;
    bf_rt_id_t action_id_data;
    bf_rt_id_t action_id_ack;
    bf_rt_id_t data_id_data;
    bf_rt_id_t data_id_ack;

    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.get_table_ip", &table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_scope_attributes_allocate(table, &table_attributes);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(table_attributes, false);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_attributes_set(table, session, dev_tgt, table_attributes);
    assert(bf_status == BF_SUCCESS);
    printf("set table attributes\n");


    bf_status = bf_rt_table_key_allocate(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.packet_type", &key_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.table_ip_is_src_ip", &action_id_ack);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.table_ip_is_dst_ip", &action_id_data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_ack, &data_ack);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id_data, &data_data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_ack,&data_ack);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id_data,&data_data);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_rt_key_field_set_value(key, key_id, m_DATA);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_data);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_rt_key_field_set_value(key, key_id, m_CAVER_DATA);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_data);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_rt_key_field_set_value(key, key_id, m_ACK);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_ack);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_rt_key_field_set_value(key, key_id, m_CAVER_ACK_FIRST);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_ack);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_rt_key_field_set_value(key, key_id, m_CAVER_ACK_SECOND);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data_ack);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("get_table_ip table is deployed correctly!\n");
}


void set_ACK_prepare(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){

    printf("start to set up ACK_prepare table\n");


    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *table;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data;

    bf_rt_id_t key_id;
    bf_rt_id_t action_id;
    bf_rt_id_t data_id;
    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.ACK_prepare", &table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_scope_attributes_allocate(table, &table_attributes);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(table_attributes, false);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_attributes_set(table, session, dev_tgt, table_attributes);
    assert(bf_status == BF_SUCCESS);
    printf("set table attributes\n");


    bf_status = bf_rt_table_key_allocate(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "ig_md.packet_type", &key_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "SwitchIngress.path_prepare", &action_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id, &data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id,&data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_set_value(key, key_id, m_CAVER_ACK_SECOND);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data);
    assert(bf_status == BF_SUCCESS);

    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("ACK_prepare table is deployed correctly!\n");
}

void init_BestTable(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){

    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *reg;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data;

    bf_rt_id_t key_id;
    bf_rt_id_t key_id_type;
    bf_rt_id_t action_id;
    bf_rt_id_t data_id;
    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.BestTable_reg", &reg);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_allocate(reg, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(reg, &data);
    assert(bf_status == BF_SUCCESS);
    printf("set table allocate\n");

    bf_status = bf_rt_key_field_id_get(reg, "$REGISTER_INDEX", &key_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_get(reg, "SwitchIngress.BestTable_reg.f1", &data_id);
    assert(bf_status == BF_SUCCESS);
    bf_rt_table_key_reset(reg, &key);
    assert(bf_status == BF_SUCCESS);
    bf_rt_table_data_reset(reg, &data);
    assert(bf_status == BF_SUCCESS);

    uint32_t ip;
    uint64_t id;
    const ip_id_t* group = init_path_lists[dev_tgt->pipe_id];
    for (int i = 0; i < INIT_PATH_SIZES[dev_tgt->pipe_id]; i++) {
        ip = ipv4AddrToUint32(group[i].ip);
        id = group[i].id;
        printf("host_id: %d,ip: %d, id: %d\n", i, ip, id);
        bf_status = bf_rt_key_field_set_value(key, key_id, i);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(data, data_id, id);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_table_entry_add(reg, session, dev_tgt, key, data);
        assert(bf_status == BF_SUCCESS);    
        printf("add entry\n");
    }
    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
}
// ECN related 
void set_dcqcn_get_ecn_probability(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){
    printf("start to set up dcqcn_get_ecn_probability\n");


    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *table;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data;
    bf_rt_id_t key_id;
    bf_rt_id_t action_id;
    bf_rt_id_t data_id;
    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "ECNEgress.dcqcn_get_ecn_probability", &table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_scope_attributes_allocate(table, &table_attributes);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(table_attributes, false);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_attributes_set(table, session, dev_tgt, table_attributes);
    assert(bf_status == BF_SUCCESS);
    printf("set table attributes\n");


    bf_status = bf_rt_table_key_allocate(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "eg_intr_md.deq_qdepth", &key_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "ECNEgress.dcqcn_mark_probability", &action_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_with_action_get(table, "value", action_id, &data_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id, &data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id,&data);
    assert(bf_status == BF_SUCCESS);
    printf("find key_id, action_id, data_id\n");      

    bf_status = bf_rt_key_field_set_value_range(key, key_id, 0, DCQCN_K_MIN);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_set_value(data, data_id, 0);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data);
    assert(bf_status == BF_SUCCESS);    
    printf("add entry\n");  

    int last_range = DCQCN_K_MIN;
    for (int i = 1; i < SEED_K_MAX; i++) {
        float probability = ((float)i / SEED_RANGE_MAX) * 100.0;
        printf("DCQCN Table -- Adding qdepth:[%d, %d] -> probability:%.2f%% (%d/%d)\n",
               last_range, last_range + QDEPTH_STEPSIZE - 1, probability, i, SEED_RANGE_MAX);
        
        bf_status = bf_rt_key_field_set_value_range(key, key_id, last_range, last_range + QDEPTH_STEPSIZE);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(data, data_id, i);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data);
        assert(bf_status == BF_SUCCESS);    
        printf("add entry\n"); 
        last_range += QDEPTH_STEPSIZE;
    }

    bf_status = bf_rt_key_field_set_value_range(key, key_id, last_range, QDEPTH_RANGE_MAX-1);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_set_value(data, data_id, SEED_RANGE_MAX - 1);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data);
    assert(bf_status == BF_SUCCESS);    
    printf("add entry\n");  

    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("flow_time_table is deployed correctly!\n");                
}

void set_dcqcn_compare_probability(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){
    printf("start to set up dcqcn_compare_probability\n");


    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *table;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data;
    bf_rt_id_t key_id_random;
    bf_rt_id_t key_id_prob;
    bf_rt_id_t action_id;
    bf_rt_id_t data_id;
    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "ECNEgress.dcqcn_compare_probability", &table);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_entry_scope_attributes_allocate(table, &table_attributes);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_attributes_entry_scope_symmetric_mode_set(table_attributes, false);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_attributes_set(table, session, dev_tgt, table_attributes);
    assert(bf_status == BF_SUCCESS);
    printf("set table attributes\n");


    bf_status = bf_rt_table_key_allocate(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(table, &data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "eg_md.dcqcn_prob_output", &key_id_prob);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_key_field_id_get(table, "eg_md.dcqcn_random_number", &key_id_random);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_action_name_to_id(table, "ECNEgress.dcqcn_check_ecn_marking", &action_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_allocate(table, action_id, &data);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_reset(table, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_action_data_reset(table,action_id,&data);
    assert(bf_status == BF_SUCCESS);
    printf("find key_id, action_id, data_id\n");      


    for (int prob_output = 1; prob_output < SEED_K_MAX; prob_output++) {
        for (int random_number = 0; random_number < SEED_RANGE_MAX; random_number++) {
            if (random_number < prob_output) {
                printf("Comparison Table -- ECN Marking for Random Number %d, Output Value %d\n",
                       random_number, prob_output);
                bf_status = bf_rt_key_field_set_value(key, key_id_prob, prob_output);
                assert(bf_status == BF_SUCCESS);
                bf_status = bf_rt_key_field_set_value(key, key_id_random, random_number);
                assert(bf_status == BF_SUCCESS);
                bf_status = bf_rt_table_entry_add(table, session, dev_tgt, key, data);
            
            }
        }
    }
    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
    printf("flow_time_table is deployed correctly!\n");                
}



void test_init_Dre_value_table(const bf_rt_target_t *dev_tgt,
                               const bf_rt_info_hdl *bfrt_info,
                               const bf_rt_session_hdl *session){

    bf_status_t bf_status;;

    // Get table object from name
    bf_rt_table_hdl *reg;
    bf_rt_table_key_hdl *key;
    bf_rt_table_data_hdl *data;

    bf_rt_id_t key_id;
    bf_rt_id_t action_id;
    bf_rt_id_t data_id;
    bf_rt_table_attributes_hdl *table_attributes;

    bf_status = bf_rt_table_from_name_get(bfrt_info, "SwitchIngress.Dre_value_reg", &reg);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_key_allocate(reg, &key);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_table_data_allocate(reg, &data);
    assert(bf_status == BF_SUCCESS);
    printf("set table allocate\n");

    bf_status = bf_rt_key_field_id_get(reg, "$REGISTER_INDEX", &key_id);
    assert(bf_status == BF_SUCCESS);
    bf_status = bf_rt_data_field_id_get(reg, "SwitchIngress.Dre_value_reg.f1", &data_id);
    assert(bf_status == BF_SUCCESS);
    bf_rt_table_key_reset(reg, &key);
    assert(bf_status == BF_SUCCESS);
    bf_rt_table_data_reset(reg, &data);
    assert(bf_status == BF_SUCCESS);

    for (int i = 0; i < 8; i++) {
        bf_status = bf_rt_key_field_set_value(key, key_id, i);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_data_field_set_value(data, data_id, 400000);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_rt_table_entry_add(reg, session, dev_tgt, key, data);
        assert(bf_status == BF_SUCCESS);    
        printf("add entry\n");
    }
    bf_status = bf_rt_session_complete_operations(session);
    assert(bf_status == BF_SUCCESS);
}




int main(void) {
    bf_status_t bf_status;

	int imap_status = 0;
    switch_t iswitch;

	bf_switchd_context_t *switchd_ctx;
    bf_rt_target_t *dev_tgt_0 = &iswitch.dev_tgt_0;
    bf_rt_target_t *dev_tgt_1 = &iswitch.dev_tgt_1;
    const bf_rt_info_hdl *bfrt_info = NULL;
    bf_rt_session_hdl **session = &iswitch.session;

    dev_tgt_0->dev_id = 0;
    dev_tgt_0->pipe_id = 0;
    dev_tgt_1->dev_id = 0;
    dev_tgt_1->pipe_id = 1;
	
	// Initialize and set the bf_switchd
    switchd_ctx = (bf_switchd_context_t *)
                  calloc(1, sizeof(bf_switchd_context_t));
    if (switchd_ctx == NULL) {
        printf("Cannot allocate switchd context\n");
        return -1;
    }	

    switchd_setup(switchd_ctx, P4_PROG_NAME);
    printf("\nbf_switchd is initialized successfully!\n");

    // Get BfRtInfo and create the bf_runtime session
    bfrt_setup(dev_tgt_0, &bfrt_info, P4_PROG_NAME, session);
    printf("bfrtInfo is got and session is created successfully!\n");

    bfrt_setup(dev_tgt_1, &bfrt_info, P4_PROG_NAME, session);
    printf("bfrtInfo is got and session is created successfully!\n");

	// Set up the portable using C bf_pm api, instead of BF_RT CPP
	// port_setup(dev_tgt_0, PORT_LIST, ARRLEN(PORT_LIST));	
    port_setup_25g(dev_tgt_0, PORT_LIST_DOWN, ARRLEN(PORT_LIST_DOWN));
    // port_setup_25g(dev_tgt_0, PORT_LIST_UP, ARRLEN(PORT_LIST_UP));
    port_setup_10g(dev_tgt_0, PORT_LIST_UP, ARRLEN(PORT_LIST_UP));
    printf("$PORT table is set up successfully!\n");
    // port_setup(dev_tgt_1, PORT_LIST, ARRLEN(PORT_LIST));
    port_setup_25g(dev_tgt_1, PORT_LIST_DOWN, ARRLEN(PORT_LIST_DOWN));
    // port_setup_25g(dev_tgt_1, PORT_LIST_UP, ARRLEN(PORT_LIST_UP));
    port_setup_10g(dev_tgt_1, PORT_LIST_UP, ARRLEN(PORT_LIST_UP));
    printf("$PORT table is set up successfully!\n");

    set_inPort2caverPort(dev_tgt_0, bfrt_info, *session);
    set_inPort2caverPort(dev_tgt_1, bfrt_info, *session);

    set_flow_id_table(dev_tgt_0, bfrt_info, *session);
    set_flow_id_table(dev_tgt_1, bfrt_info, *session);

    set_flow_time_table(dev_tgt_0, bfrt_info, *session);
    set_flow_time_table(dev_tgt_1, bfrt_info, *session);

    set_BestTable(dev_tgt_0, bfrt_info, *session);
    set_BestTable(dev_tgt_1, bfrt_info, *session);

    set_judge_acceptable_table(dev_tgt_0, bfrt_info, *session);
    set_judge_acceptable_table(dev_tgt_1, bfrt_info, *session);

    set_set_acceptable_path(dev_tgt_0, bfrt_info, *session);
    set_set_acceptable_path(dev_tgt_1, bfrt_info, *session);

    set_stack_count_table(dev_tgt_0, bfrt_info, *session);
    set_stack_count_table(dev_tgt_1, bfrt_info, *session);

    set_stack_top_table(dev_tgt_0, bfrt_info, *session);
    set_stack_top_table(dev_tgt_1, bfrt_info, *session);

    set_path_table(dev_tgt_0, bfrt_info, *session);
    set_path_table(dev_tgt_1, bfrt_info, *session);

    set_flow_method_table(dev_tgt_0, bfrt_info, *session);
    set_flow_method_table(dev_tgt_1, bfrt_info, *session);

    set_flow_path(dev_tgt_0, bfrt_info, *session);
    set_flow_path(dev_tgt_1, bfrt_info, *session);

    set_caverPort2switchPort(dev_tgt_0, bfrt_info, *session);
    set_caverPort2switchPort(dev_tgt_1, bfrt_info, *session);

    set_ipv4_lpm(dev_tgt_0, bfrt_info, *session);
    set_ipv4_lpm(dev_tgt_1, bfrt_info, *session);

    set_header_process(dev_tgt_0, bfrt_info, *session);
    set_header_process(dev_tgt_1, bfrt_info, *session);

    set_Dre_time_table(dev_tgt_0, bfrt_info, *session);
    set_Dre_time_table(dev_tgt_1, bfrt_info, *session);

    set_Dre_table(dev_tgt_0, bfrt_info, *session);
    set_Dre_table(dev_tgt_1, bfrt_info, *session);

    set_get_table_ip(dev_tgt_0, bfrt_info, *session);
    set_get_table_ip(dev_tgt_1, bfrt_info, *session);

    set_ACK_prepare(dev_tgt_0, bfrt_info, *session);
    set_ACK_prepare(dev_tgt_1, bfrt_info, *session);
    

    init_BestTable(dev_tgt_0, bfrt_info, *session);
    init_BestTable(dev_tgt_1, bfrt_info, *session);

    set_dcqcn_get_ecn_probability(dev_tgt_0, bfrt_info, *session);
    set_dcqcn_get_ecn_probability(dev_tgt_1, bfrt_info, *session);

    set_dcqcn_compare_probability(dev_tgt_0, bfrt_info, *session);
    set_dcqcn_compare_probability(dev_tgt_1, bfrt_info, *session);

    test_init_Dre_value_table(dev_tgt_0, bfrt_info, *session);
    test_init_Dre_value_table(dev_tgt_1, bfrt_info, *session);


	while(1) {
       sleep(1);
    }

    return bf_status;

}
