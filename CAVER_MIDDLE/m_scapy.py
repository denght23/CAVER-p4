from scapy.all import *
from scapy.packet import Packet, bind_layers
from scapy.fields import BitField, ByteField, ShortField, IntField
import random
import time
class IB_BTH_H(Packet):
    name = "IB_BTH_H"
    fields_desc = [
        ByteField("opcode", 0),                  # 8 bits
        ByteField("flags", 0),                  # 8 bits
        ShortField("partition_key", 0),         # 16 bits
        BitField("reserved0", 0, 8),            # RC reserved0 (8 bits)
        BitField("destination_qp", 0, 24),      # 24 bits
        BitField("ack_request", 0, 1),          # 1 bit
        BitField("reserved1", 0, 7),            # RC reserved1 (7 bits)
        BitField("packet_seqnum", 0, 24)        # 24 bits
    ]
class IB_AETH_H(Packet):
    name = "IB_AETH_H"
    fields_desc = [
        BitField("reserved", 0, 1),           # 1 bit reserved
        BitField("opcode", 0, 2),             # 2 bits opcode
        BitField("error_code", 0, 5),         # 5 bits error_code
        BitField("msg_seq_number", 0, 8)      # 8 bits msg_seq_number
    ]
class CAVER_DATA_H(Packet):
    name = "CAVER_DATA_H"
    fields_desc = [
        ByteField("out_port", 0), 
        ByteField("src_route", 0)
    ]
class CAVER_ACK_H(Packet):
    name = "CAVER_ACK_H"
    fields_desc = [
        ByteField("best_port", 0),      # 8 bits best_port
        ByteField("good_port", 0),     # 8 bits good_port
        ShortField("best_CE", 0),      # 16 bits best_CE
        ShortField("good_CE", 0)       # 16 bits good_CE
    ]
    
def send_roce_data_pacekts(ifaces, dst_ips):
    for iface in ifaces:
        for dst_ip in dst_ips:
            sport = random.randint(500, 10000)
            udp_layer = UDP(sport=sport, dport=4791)
            payload = Raw(load="This is a UDP packet")
            ip_layer = IP(dst=dst_ip)
            ib_bth = IB_BTH_H(opcode=0x1, flags=0x0, partition_key=0x0, reserved0 = 0, destination_qp=0x0, ack_request=0x0, packet_seqnum=0x0)
            pkt = Ether() / ip_layer / udp_layer / ib_bth /payload
            sendp(pkt, iface=iface)
            # time.sleep(0.1)

def send_roce_ack_pacekts(ifaces, src_ip, dst_ips):
    for iface in ifaces:
        for dst_ip in dst_ips:
            sport = random.randint(500, 10000)
            udp_layer = UDP(sport=sport, dport=4791)
            payload = Raw(load="This is a UDP packet")
            ip_layer = IP(dst=dst_ip, src = src_ip)
            ib_bth = IB_BTH_H(opcode=0x11, flags=0x0, partition_key=0x0, destination_qp=0x0, ack_request=0x0, packet_seqnum=0x0)
            ib_ack = IB_AETH_H(reserved=0x0, opcode=0x0, error_code=0x0, msg_seq_number=0x0)
            pkt = Ether() / ip_layer / udp_layer / ib_bth / ib_ack /payload
            sendp(pkt, iface=iface)
            # time.sleep(0.1)

def send_caver_data_pacekts(ifaces, dst_ips, outport, srcroute):
    for iface in ifaces:
        for dst_ip in dst_ips:
            sport = random.randint(500, 10000)
            udp_layer = UDP(sport=sport, dport=4791)
            payload = Raw(load="This is a UDP packet")
            ip_layer = IP(dst=dst_ip)
            ib_bth = IB_BTH_H(opcode=0x1, flags=0x0, partition_key=0x0, reserved0 = 1, destination_qp=0x0, ack_request=0x0, packet_seqnum=0x0)
            caver_data = CAVER_DATA_H(out_port=outport, src_route=srcroute)
            pkt = Ether() / ip_layer / udp_layer / ib_bth/caver_data /payload
            sendp(pkt, iface=iface)
            print(time.time())
            # time.sleep(0.1)

def send_caver_ack_pacekts(ifaces, src_ip, dst_ips, bestPort, goodPort, bestCE, goodCE):
    for iface in ifaces:
        for dst_ip in dst_ips:
            sport = random.randint(500, 10000)
            udp_layer = UDP(sport=sport, dport=4791)
            payload = Raw(load="This is a UDP packet")
            ip_layer = IP(dst=dst_ip, src = src_ip)
            ib_bth = IB_BTH_H(opcode=0x11, flags=0x0, partition_key=0x0, destination_qp=0x0, ack_request=0x0, packet_seqnum=0x0)
            ib_ack = IB_AETH_H(reserved=0x1, opcode=0x0, error_code=0x0, msg_seq_number=0x0)
            caver_ack = CAVER_ACK_H(best_port=bestPort, good_port=goodPort, best_CE=bestCE, good_CE=goodCE)
            pkt = Ether() / ip_layer / udp_layer / ib_bth / ib_ack / caver_ack /payload
            sendp(pkt, iface=iface)
            # time.sleep(0.1)

###### Send a UDP packet to 10.1.0.1-10.1.0.4
###### through veth12 interface
def send_udp_packets(iface, dst_ips, dport):
    for dst_ip in dst_ips:
        sport = random.randint(500, 10000)
        udp_layer = UDP(sport=sport, dport=dport)
        payload = Raw(load="This is a UDP packet")
        ip_layer = IP(dst=dst_ip)
        pkt = Ether() / ip_layer / udp_layer / payload
        sendp(pkt, iface=iface)
        # 加入延时，防止接收方接收不到数据
        # time.sleep(0.1)

################ test parser ################
# # roce_data
# send_roce_data_pacekts(["veth12"], ["10.1.0.1"])
# # roce_ack
# send_roce_ack_pacekts(["veth12"], ["10.1.0.2"])
# # caver_data
# send_caver_data_pacekts(["veth12"], ["10.1.0.3"], 0, 0)
# send_caver_data_pacekts(["veth12"], ["10.1.0.3"], 0, 0)
# # caver_ack
# send_caver_ack_pacekts(["veth12"], "10.0.0.4", ["10.1.0.4"], 0, 0, 0, 0)
# send_caver_ack_pacekts(["veth12"], "10.0.0.4", ["10.1.0.4"], 0, 0, 0, 0)


# # 测试一下caver_data的转发
# # srcroute
# send_caver_data_pacekts(["veth10"], ["10.1.0.3"], 2, 1)
# send_caver_data_pacekts(["veth12"], ["10.1.0.3"], 3, 1)

# send_caver_data_pacekts(["veth22"], ["10.1.0.3"], 2, 1)
# send_caver_data_pacekts(["veth24"], ["10.1.0.3"], 3, 1)
# # ECMP
# send_caver_data_pacekts(["veth10"], ["10.1.0.3"], 2, 0)
# send_caver_data_pacekts(["veth12"], ["10.1.0.3"], 3, 0)

# send_caver_data_pacekts(["veth22"], ["10.1.0.3"], 2, 0)
# send_caver_data_pacekts(["veth24"], ["10.1.0.3"], 3, 0)



# # 测试bestTable能否根据port的关系判断更新
# # send_caver_ack_pacekts(["veth14"], "10.1.0.1", ["10.0.0.4"], 0, 0, 0, 0)
send_caver_ack_pacekts(["veth22"], "10.0.0.1", ["10.1.0.4"], 0, 0, 0, 0)
send_caver_ack_pacekts(["veth24"], "10.0.0.1", ["10.1.0.4"], 0, 0, 0, 0)

send_caver_ack_pacekts(["veth26"], "10.1.0.2", ["10.0.0.1"], 0, 0, 0, 0)
send_caver_ack_pacekts(["veth28"], "10.1.0.2", ["10.0.0.1"], 0, 0, 0, 0)

################ pipeline0 test ################
# # Send packets through veth12 interface
# send_udp_packets('veth12', ["10.1.0.1", "10.1.0.2", "10.1.0.3", "10.1.0.4"], 91)
# # Send packets through veth10 interface
# send_udp_packets('veth10', ["10.1.0.1", "10.1.0.2", "10.1.0.3", "10.1.0.4"], 91)
# # Send packets through veth14 interface
# send_udp_packets('veth14', ["10.0.0.1", "10.0.0.2", "10.0.0.3", "10.0.0.4"], 90)
# # Send packets through veth16 interface
# send_udp_packets('veth16', ["10.0.0.1", "10.0.0.2", "10.0.0.3", "10.0.0.4"], 90)
# ################ pipeline1 test ################
# send_udp_packets('veth22', ["10.1.0.1", "10.1.0.2", "10.1.0.3", "10.1.0.4"], 91)
# # Send packets through veth10 interface
# send_udp_packets('veth24', ["10.1.0.1", "10.1.0.2", "10.1.0.3", "10.1.0.4"], 91)
# # Send packets through veth14 interface
# send_udp_packets('veth26', ["10.0.0.1", "10.0.0.2", "10.0.0.3", "10.0.0.4"], 90)
# # Send packets through veth16 interface
# send_udp_packets('veth28', ["10.0.0.1", "10.0.0.2", "10.0.0.3", "10.0.0.4"], 90)

