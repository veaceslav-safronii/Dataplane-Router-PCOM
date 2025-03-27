#include "queue.h"
#include "lib.h"
#include "protocols.h"
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define ETHERTYPE_IP 0x0800
#define ETHERTYPE_ARP 0x0806
#define PACKET_HDR_LENGTH sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr)
#define ARP_OP_REPLY 2
#define ARP_OP_REQUEST 1

/* Routing table */
struct route_table_entry *rtable;
int rtable_len;

/* Mac table */
struct arp_table_entry *arp_table;
int arp_table_len = 0;



int comparator_prefix (const void *a, const void *b) {
	struct route_table_entry entry1 = *(struct route_table_entry *) a;
	struct route_table_entry entry2 = *(struct route_table_entry *) b;
	if (entry1.prefix > entry2.prefix) {
		return 1;
	} else if (entry1.prefix < entry2.prefix) {
		return -1;
	} else {
		return 0;
	}
}
int comparator_mask (const void *a, const void *b) {
	struct route_table_entry entry1 = *(struct route_table_entry *) a;
	struct route_table_entry entry2 = *(struct route_table_entry *) b;
	return entry2.mask - entry1.mask;
}


/*
 Returns a pointer (eg. &rtable[i]) to the best matching route, or NULL if there
 is no matching route.
*/
struct route_table_entry *get_best_route(uint32_t ip_dest) {
	/* TODO 2.2: Implement the LPM algorithm */
	/* We can iterate through rtable for (int i = 0; i < rtable_len; i++). Entries in
	 * the rtable are in network order already */
	

	int low = 0;
    int high = rtable_len - 1;
    struct route_table_entry *best_entry = NULL;

    while (low <= high) {
        int mid = (high + low) / 2;
        struct route_table_entry *current_entry = &rtable[mid];
        uint32_t current_prefix = current_entry->prefix & current_entry->mask;
        uint32_t ip_prefix = ip_dest & current_entry->mask;

        if (ip_prefix == current_prefix) {
            best_entry = current_entry;
			break;
        } else if (ip_prefix < current_prefix) {
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }
	return best_entry;
}

struct arp_table_entry *get_arp_entry(uint32_t given_ip) {
	/* TODO 2.4: Iterate through the MAC table and search for an entry
	 * that matches given_ip. */

	/* We can iterate thrpigh the mac_table for (int i = 0; i <
	 * mac_table_len; i++) */
	for (int i = 0; i < arp_table_len; i++) {
		if (arp_table[i].ip == given_ip) {
			return &arp_table[i];
		}
	}
	return NULL;
}

void arp_request(struct route_table_entry *route) {
	char packet[MAX_PACKET_LEN];
	struct ether_header *arp_eth_hdr = (struct ether_header *) packet;
	struct arp_header *arp_hdr = (struct arp_header *)(packet + sizeof(struct ether_header));

	hwaddr_aton("FF:FF:FF:FF:FF:FF", arp_eth_hdr->ether_dhost);
	get_interface_mac(route->interface, arp_eth_hdr->ether_shost);

	arp_eth_hdr->ether_type = htons(ETHERTYPE_ARP);

    arp_hdr->htype = htons(1);
    arp_hdr->ptype = htons(0x0800);
    arp_hdr->hlen = 6;
    arp_hdr->plen = 4;
    arp_hdr->op = htons(ARP_OP_REQUEST);
    get_interface_mac(route->interface, arp_hdr->sha);
	arp_hdr->spa = inet_addr(get_interface_ip(route->interface));
	hwaddr_aton("00:00:00:00:00:00", arp_hdr->tha);
	arp_hdr->tpa = route->next_hop;

	send_to_link(route->interface, packet, sizeof(struct ether_header) + sizeof(struct arp_header));

}

void arp_reply(int interface, uint32_t router_ip, char *buf) {
	struct ether_header *eth_hdr = (struct ether_header *) buf;
	struct arp_header *arp_hdr = (struct arp_header *)(buf + sizeof(struct ether_header));

	char packet[MAX_PACKET_LEN];
	struct ether_header *reply_eth_hdr = (struct ether_header *) packet;
	struct arp_header *reply_arp_hdr = (struct arp_header *)(packet + sizeof(struct ether_header));

	reply_eth_hdr->ether_type = htons(ETHERTYPE_ARP);
	get_interface_mac(interface, reply_eth_hdr->ether_shost);
	memcpy(reply_eth_hdr->ether_dhost, eth_hdr->ether_shost, 6);

    reply_arp_hdr->htype = htons(1);
    reply_arp_hdr->ptype = htons(0x0800);
    reply_arp_hdr->hlen = 6;
    reply_arp_hdr->plen = 4;
	
	reply_arp_hdr->op = htons(ARP_OP_REPLY);

	memcpy(reply_arp_hdr->tha, arp_hdr->sha, 6);
	get_interface_mac(interface, reply_arp_hdr->sha);

	reply_arp_hdr->tpa = arp_hdr->spa;
	reply_arp_hdr->spa = router_ip;

	send_to_link(interface, packet, sizeof(struct ether_header) + sizeof(struct arp_header));
}

void icmp_handler(int interface, uint32_t router_ip, uint8_t type, char* buf, u_int16_t recv_packet_len) {
	struct ether_header *eth_hdr = (struct ether_header *) buf;
	struct iphdr *ip_hdr = (struct iphdr *)(buf + sizeof(struct ether_header));


	char packet[MAX_PACKET_LEN];
	struct ether_header *eth_hdr_icmp = (struct ether_header *) packet;
	
	get_interface_mac(interface, eth_hdr_icmp->ether_shost);
	memcpy(eth_hdr_icmp->ether_dhost, eth_hdr->ether_shost, 6);

	eth_hdr_icmp->ether_type = htons(ETHERTYPE_IP);

	struct iphdr *ip_hdr_icmp = (struct iphdr *)(packet + sizeof(struct ether_header));
	uint16_t length;
	uint16_t payload_len;

	if (type == 3 || type == 11) {
		payload_len = sizeof(struct iphdr) + 8;
		length = PACKET_HDR_LENGTH + payload_len;
	} else {
		payload_len = recv_packet_len - PACKET_HDR_LENGTH;
		length = recv_packet_len;
	}

	ip_hdr_icmp->tos = 0;
	ip_hdr_icmp->frag_off = 0;
	ip_hdr_icmp->version = 4;
	ip_hdr_icmp->ihl = 5;
	ip_hdr_icmp->id = htons(1);
	ip_hdr_icmp->ttl = 255;
	ip_hdr_icmp->tot_len = htons(length - sizeof(struct ether_header));
	ip_hdr_icmp->daddr = ip_hdr->saddr;
	ip_hdr_icmp->saddr = router_ip;
	ip_hdr_icmp->protocol = IPPROTO_ICMP;
	ip_hdr_icmp->check = 0;
	ip_hdr_icmp->check = htons(checksum((uint16_t *)ip_hdr_icmp, sizeof(struct iphdr)));

	struct icmphdr *icmp_hdr = (struct icmphdr *)(packet + sizeof(struct ether_header) + sizeof(struct iphdr));
	icmp_hdr->code = 0;
	icmp_hdr->type = type;
	icmp_hdr->checksum = 0;

	if (type == 3 || type == 11) {
		memcpy(icmp_hdr + 1, ip_hdr, payload_len);
	} else {
		memcpy(icmp_hdr + 1, ip_hdr + sizeof(struct icmphdr), payload_len);
		icmp_hdr->un.echo.id = ((struct icmphdr*)(ip_hdr + 1))->un.echo.id;
		icmp_hdr->un.echo.sequence = ((struct icmphdr*)(ip_hdr + 1))->un.echo.sequence;
	}

	icmp_hdr->checksum = htons(checksum((uint16_t *)icmp_hdr, sizeof(struct icmphdr) + payload_len));

	send_to_link(interface, packet, length);
}

int main(int argc, char *argv[])
{
	char buf[MAX_PACKET_LEN];

	// Do not modify this line
	init(argc - 2, argv + 2);

	/* Code to allocate the MAC and route tables */
	rtable = malloc(sizeof(struct route_table_entry) * 100000);
	/* DIE is a macro for sanity checks */
	DIE(rtable == NULL, "memory");

	arp_table = malloc(sizeof(struct arp_table_entry) * 100);
	DIE(arp_table == NULL, "memory");
	
	rtable_len = read_rtable(argv[1], rtable);
	// arp_table_len = parse_arp_table("arp_table.txt", arp_table);

	qsort((void *)rtable, rtable_len, sizeof(struct route_table_entry), comparator_prefix);
	qsort((void *)rtable, rtable_len, sizeof(struct route_table_entry), comparator_mask);

	while (1) {

		int interface;
		size_t len;

		interface = recv_from_any_link(buf, &len);
		DIE(interface < 0, "recv_from_any_links");

		struct ether_header *eth_hdr = (struct ether_header *) buf;
		/* Note that packets received are in network order,
		any header field which has more than 1 byte will need to be conerted to
		host order. For example, ntohs(eth_hdr->ether_type). The oposite is needed when
		sending a packet on the link, */
		struct iphdr *ip_hdr = (struct iphdr *)(buf + sizeof(struct ether_header));
		struct arp_header *arp_hdr = (struct arp_header *)(buf + sizeof(struct ether_header));

		uint32_t router_ip = inet_addr(get_interface_ip(interface));
		struct arp_table_entry *arp_entry = NULL;

		Queue queue = createQueue();

		if (ntohs(eth_hdr->ether_type) != ETHERTYPE_IP && ntohs(eth_hdr->ether_type) != ETHERTYPE_ARP) {
			printf("Ignored non-IPv4 and non-ARP packet \n");
			printf("Protocol %x\n", ntohs(eth_hdr->ether_type));
			continue;
		}
		if (ntohs(eth_hdr->ether_type) == ETHERTYPE_IP) {
			if (checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)) != 0) {
				printf("Invalid cheksum\n");
				continue;
			}

			if (ip_hdr->daddr == router_ip) {
				icmp_handler(interface, router_ip, 0, buf, len);
				continue;
			}

			struct route_table_entry * best_route = get_best_route(ip_hdr->daddr);
			// printf("Best route: %d", best_route->next_hop);
			if (best_route == NULL) {
				printf("No route found\n");
				icmp_handler(interface, router_ip, 3, buf, len);
				continue;
			}
			
			if (ip_hdr->ttl > 1) {
				ip_hdr->ttl--;
				ip_hdr->check = 0;
				ip_hdr->check = htons(checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)));
				printf("Check %x\n", ip_hdr->check);
			} else {
				printf("TTL exceded\n");
				icmp_handler(interface, router_ip, 11, buf, len);
				continue;
			}
			
			arp_entry = get_arp_entry(best_route->next_hop);
			if (arp_entry == NULL) {
				enQueue(queue, buf, len, best_route->next_hop);
				arp_request(best_route);
				continue;
			}

			get_interface_mac(best_route->interface, eth_hdr->ether_shost);
			memcpy(eth_hdr->ether_dhost, arp_entry->mac, 6);
			send_to_link(best_route->interface, buf, len);
		}
		
		if (ntohs(arp_hdr->op) == ARP_OP_REQUEST) {
			if (arp_hdr->tpa == router_ip) {
				arp_reply(interface, router_ip, buf);
				continue;
			}
		}

		if (ntohs(arp_hdr->op) == ARP_OP_REPLY) {
			arp_entry = malloc(sizeof(struct arp_table_entry));
			arp_entry->ip = arp_hdr->spa;
			memcpy(arp_entry->mac, arp_hdr->sha, 6);

			if (get_arp_entry(arp_entry->ip) == NULL) {
				arp_table[arp_table_len].ip = arp_entry->ip;
				memcpy(arp_table[arp_table_len].mac, arp_entry->mac, 6);
				arp_table_len++;
			}

			if (isQueueEmpty(queue)) {
				continue;
			}

			for (QNode iter = queue->front; iter != NULL; iter = iter->next) {
				if (iter->ip_dest == arp_entry->ip) {
					eth_hdr = (struct ether_header *)iter->packet;
					memcpy(eth_hdr->ether_dhost, arp_entry->mac, 6);
					struct route_table_entry *best_route = get_best_route(iter->ip_dest);
					
					get_interface_mac(best_route->interface, eth_hdr->ether_shost);
					send_to_link(best_route->interface, iter->packet, iter->len);
					deQueue(queue, iter);
				}
			}
		}
	}
}

