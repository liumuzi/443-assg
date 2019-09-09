#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h> // required by "netfilter.h"
#include <arpa/inet.h> // required by ntoh[s|l]()
#include <signal.h> // required by SIGINT
#include <string.h> // required by strerror()
#include <sys/time.h> // required by gettimeofday()
#include <time.h> // required by nanosleep()
#include <errno.h> // required by errno
# include <pthread.h>
#include <netinet/ip.h>        // required by "struct iph"
#include <netinet/tcp.h>    // required by "struct tcph"
#include <netinet/udp.h>    // required by "struct udph"
#include <netinet/ip_icmp.h>    // required by "struct icmphdr"

#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>


#include "checksum.h"
#include "nat_table.h"
#define BUF_SIZE 1024*1024


unsigned int subnet_mask;
uint32_t public_network;
uint32_t local_network;
int port_list[2001];

int is_terminated(uint16_t FIN, uint16_t ACK, uint16_t RST, uint32_t internal_ip, uint16_t internal_port);
void terminate(uint32_t internal_ip, uint16_t internal_port);

void print_ip(int ip)
{
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;   
    printf("ip: %d.%d.%d.%d\n", bytes[3], bytes[2], bytes[1], bytes[0]);        
}

// ????????????? what if no port is available?????????
int find_port(){
	for (int i = 0; i < 2001; ++i)
	{
		if (port_list[i] == 0)
		{
			port_list[i] = 1;
			return i;
		}
	}
	return 3000;
	
}

static int Callback(struct nfq_q_handle *myQueue, struct nfgenmsg *msg,
    struct nfq_data *pkt, void *cbData) {

	//printf("******** inside callback function ********\n");

    // Get the id in the queue
    unsigned int id = 0;
    struct nfqnl_msg_packet_hdr *header;
    if (header = nfq_get_msg_packet_hdr(pkt)) 
    	id = ntohl(header->packet_id);
    //printf("packet: id: %d\n", id);

    // Access IP Packet
    unsigned char *pktData;
    int ip_pkt_len = nfq_get_payload(pkt, (unsigned char**)&pktData);
    struct iphdr *ipHeader = (struct iphdr*)pktData;
	
	// check whether TCP packet
    if (ipHeader->protocol != IPPROTO_TCP) {
    	fprintf(stderr, "hacknfqueue does not verdict for non-TCP packet!");
    	exit(-1);
    }

    // Access TCP Packet
    struct tcphdr *tcph = (struct tcphdr*)((unsigned char*)pktData+(ipHeader->ihl<<2));


    // Get source & destination ip & port 
    uint32_t source_ip = ntohl(ipHeader->saddr);
    uint32_t destination_ip = ntohl(ipHeader->daddr);
    uint16_t source_port = ntohs(tcph->source);
    uint16_t destination_port = ntohs(tcph->dest);
    
    /**debug: note the type**/
    //printf("source_ip: \n");
    //print_ip(source_ip);
    //printf("destination_ip\n");
    //print_ip(destination_ip);
    //printf("source port: %d\t destination port: %d\n\n", ntohs(tcph->source), ntohs(tcph->dest));
    /********/

    // Access TCP flags
    uint16_t SYN = tcph->syn;
    uint16_t ACK = tcph->ack;
    uint16_t FIN = tcph->fin;
    uint16_t RST = tcph->rst;

    // create a node to get the return element from nat_table.c functions
    struct table_element *create_node = (struct table_element*)malloc(sizeof(struct table_element));
    uint32_t internal_ip;
    uint16_t internal_port;

    if ((source_ip & subnet_mask) == local_network) {
		// outbound traffic
    	printf(" *****Outbound Packet*****\n");
        printf("tcp flags: SYN %d, ACK %d, FIN %d, RST %d\n", SYN,  ACK,  FIN,  RST);

        internal_ip = source_ip;
        internal_port = source_port;

    	create_node = find_element(source_ip, source_port);
        uint32_t trans_ip;
        uint16_t trans_port;

    	if ( create_node != NULL)
    	{
    		//printf("Outbound Packet Element Found\n");

            // new source ip & port are the one stored in the table that we found
            trans_ip = create_node->trans_ip;
            trans_port = create_node->trans_port;

    	}
    	else{
    		printf("Outbound Packet Element NOT FOUND!\n");

            // if SYN packet, add new element into table
    		if (SYN == 1)
    		{
    			printf("SYN Packet, create a new element in table\n");
    			uint16_t min_port = (uint16_t)find_port();
    			//printf("new port: %d\n", min_port);

                // new source ip & port are the public_network & min_port
                trans_ip = public_network;
                trans_port = min_port + 10000;

                // add new element into table
    			insert_element(source_ip, trans_ip, source_port, trans_port);
    			print_table();
    		}
            // if not SYN packet, drop
    		else
    		{   // not find the element in the table & not a syn element --> drop
    			printf("NON-SYN Packet, drop it\n");
                print_ip(source_ip);
                print_ip(destination_ip);
                print_table();
                if(is_terminated(FIN, ACK, RST, internal_ip, internal_port)) terminate(internal_ip, internal_port);
                printf(" *****End of Packet: DROP*****\n");
    			return nfq_set_verdict(myQueue, id, NF_DROP, 0, NULL);
    		}
    	}
        // deal with find & not find (syn) situations
        // update packet source ip & port
        ipHeader->saddr = htonl(trans_ip);
        tcph->source = htons(trans_port);

        // update ip checksum & tcp checksum
        tcph->check = tcp_checksum(pktData);
        ipHeader->check = ip_checksum(pktData);
        if(is_terminated(FIN, ACK, RST, internal_ip, internal_port)) terminate(internal_ip, internal_port);
        printf(" *****End of Packet: ACCEPT*****\n");
        return nfq_set_verdict(myQueue, id, NF_ACCEPT, ip_pkt_len, pktData);

	} 
    else {
        // inbound traffic
		printf(" *****Inbound Packet*****\n");
        printf("tcp flags: SYN %d, ACK %d, FIN %d, RST %d\n", SYN,  ACK,  FIN,  RST);

        internal_ip = destination_ip;
        internal_port = destination_port;

        create_node = find_element(destination_ip, destination_port);
        if ( create_node != NULL){
            //printf("Inbound Packet Element Found\n");
            uint32_t origin_ip = create_node->inter_ip;
            uint16_t origin_port = create_node->inter_port;
            
            // update packet source ip & port
            ipHeader->daddr = htonl(origin_ip);
            tcph->dest = htons(origin_port);

            // update ip checksum & tcp checksum
            tcph->check = tcp_checksum(pktData);
            ipHeader->check = ip_checksum(pktData);
            //print_table();
            if(is_terminated(FIN, ACK, RST, internal_ip, internal_port)) {
                //printf("**********terminate********\n");
                terminate(internal_ip, internal_port);
                //printf("after terminate\n");
            }
            printf(" *****End of Packet: ACCEPT*****\n");
            return nfq_set_verdict(myQueue, id, NF_ACCEPT, ip_pkt_len, pktData);

        }
        else{
            // not find the element in the table --> drop
            //printf("Inbound Packet Element Not Found, drop it\n");
            if(is_terminated(FIN, ACK, RST, internal_ip, internal_port)) terminate(internal_ip, internal_port);
            printf(" *****End of Packet: DROP*****\n");
            return nfq_set_verdict(myQueue, id, NF_DROP, 0, NULL);
        }

	}

}


int main(int argc, char** argv) {

	// Check the no. of arguments
	if(argc != 4){
		fprintf(stderr, "Usage: %s <PUBLIC IP> <INTERNAL IP> <MASK>\n", argv[0]);
		exit(1);
	}

	//read parameters from arguments
	char *public_ip = argv[1];
	char *internal_ip = argv[2];
	char *mask = argv[3];

	// initialize the port_list, port_list[i] is the status for the 10000+i port
	for (int i = 0; i < 2001; ++i)
		port_list[i] = 0;
	

	// generate subnet mask
	int mask_int= atoi(mask);
	subnet_mask= (0xffffffff << (32 - mask_int));

	// inet_aton, inet_ntoa: https://beej.us/guide/bgnet/html/multi/inet_ntoaman.html
	// sockaddr_in, in_addr: https://www.gta.ufrj.br/ensino/eel878/sockets/sockaddr_inman.html
	// htons(), htonl(), ntohs(), ntohl(): https://beej.us/guide/bgnet/html/multi/htonsman.html
	
	// store this IP address in internal_temp:
	struct in_addr public_temp;
	inet_aton(public_ip, &public_temp);
  	public_network = ntohl(public_temp.s_addr);

	struct in_addr internal_temp;
	inet_aton(internal_ip, &internal_temp);
	local_network = ntohl(internal_temp.s_addr) & subnet_mask;
	
	
	// print the ip for check
	//char *internal_str = inet_ntoa(internal_temp); 
	//printf("local_network: %s\n", internal_str);

	// Get a queue connection handle from the module
	struct nfq_handle *nfqHandle;
	if (!(nfqHandle = nfq_open())) {
		fprintf(stderr, "Error in nfq_open()\n");
		exit(-1);
  	}

  	if (nfq_unbind_pf(nfqHandle, AF_INET) < 0) {
    	fprintf(stderr, "Error: nfq_unbind_pf()\n");
    	exit(1);
    }

    // Unbind the handler from processing any IP packets
    if (nfq_unbind_pf(nfqHandle, AF_INET) < 0) {
    	printf("Error\n");
    	fprintf(stderr, "Error in nfq_unbind_pf()\n");
    	exit(1);
    }

    // Bind this handler to process IP packets...
	if (nfq_bind_pf(nfqHandle, AF_INET) < 0) {
		fprintf(stderr, "Error in nfq_bind_pf()\n");
		exit(1);
	}

    // Install a callback on queue 0
    struct nfq_q_handle *nfQueue;
    if (!(nfQueue = nfq_create_queue(nfqHandle,  0, &Callback, NULL))) {
    	fprintf(stderr, "Error in nfq_create_queue()\n");
    	exit(1);
    }

    // nfq_set_mode: I want the entire packet 
    if(nfq_set_mode(nfQueue, NFQNL_COPY_PACKET, BUF_SIZE) < 0) {
    	fprintf(stderr, "Error in nfq_set_mode()\n");
    	exit(1);
    }

    //set max queue length
    if(nfq_set_queue_maxlen(nfQueue, 60000) < 0){
    	fprintf(stderr, "Error in nfq_set_queue_maxlength()\n");
    	exit(1);
    }

    // struct nfnl_handle *netlinkHandle;
    // netlinkHandle = nfq_nfnlh(nfqHandle);
    // int fd;
    // fd = nfnl_fd(netlinkHandle);

    // nfq_fd(): http://charette.no-ip.com:81/programming/doxygen/netfilter/group__Queue.html
 	// int nfq_fd(struct nfq_handle *h)
	// {
	//     return nfnl_fd(nfq_nfnlh(h));
	// }
    int fd;
    fd = nfq_fd(nfqHandle);;

    int res;
    char buf[BUF_SIZE];
    int packetCnt = 0;

    while ((res = recv(fd, buf, BUF_SIZE, 0)) && res >= 0) {
    	nfq_handle_packet(nfqHandle, buf, res);
        printf("Packet # %d handled.\n", packetCnt);
        packetCnt ++;
    }
    printf("While loop ends. res = %d\n", res);

    if (res < 0)
    {
        fprintf(stderr, "recv: %s (%d)\n", strerror(errno), errno);
        exit(0);
    }

    nfq_destroy_queue(nfQueue);
    printf("NFQUEUE destroyed.\n");
    nfq_close(nfqHandle);
    printf("NF HANDLER closed.\n");

}


/* return 1 if the nat table entry should be deleted, return 0 otherwise
/* 1. check RST
/* 2. check the progress of 4-way handshake
/* 3. update tcp_state in nat table */
int is_terminated(uint16_t FIN, uint16_t ACK, uint16_t RST, uint32_t internal_ip, uint16_t internal_port){
    //printf("Checking termination...\n");
    struct table_element* e = find_element(internal_ip, internal_port);
	
    if (RST)
    {
        e->tcp_state = 4;
        return 1;
    }

    if (!FIN && !ACK)
        return 0;
    else if (!FIN && ACK)
        if (e->tcp_state%2==1)
            e->tcp_state ++;
        else
            return 0;
    else if (FIN && !ACK)
    {
        e->tcp_state ++;
    }
    else if (FIN && ACK)
    {
        if (e->tcp_state%2==1)
            e->tcp_state += 2;
        else
            e->tcp_state ++;

        //e->tcp_state ? e->tcp_state += 2 : e->tcp_state += 1;//add 1 if it is the first FIN, add (FIN+ACK) otherwise
    }

    //printf("TCP_STATE: %d\n", e->tcp_state);
    if (e->tcp_state == 4)
        return 1;
    else if (e->tcp_state > 4 || e->tcp_state < 0)
    {
        printf("ERROR: tcp_state = %d\n", e->tcp_state);
        exit(-1);
    }
    else
        return 0;
}


/* actual termination */
void terminate(uint32_t internal_ip, uint16_t internal_port)
{
    //printf("Flow terminated! (");
    print_ip(internal_ip);
    //printf(", port: %d)\n", internal_port);

    struct table_element *node = delete_element(internal_ip, internal_port);
    port_list[node->trans_port-10000]=0;
    print_table();
}
