#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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

#include "nat_table.h"

struct table_element *head = NULL;

void insert_element(uint32_t inter_ip,uint32_t trans_ip, uint16_t inter_port, uint16_t trans_port ){
	
	// create a new node 
	struct table_element *create_node = (struct table_element*)malloc(sizeof(struct table_element));
	create_node->inter_ip = inter_ip;
	create_node->trans_ip = trans_ip;
	create_node->inter_port = inter_port;
	create_node->trans_port = trans_port;
	create_node->tcp_state = 0;
	create_node->next = NULL;

	// if head == NULL --> list is empty --> create_node is the first element
	if (head == NULL)
		head = create_node;
	
	// if head != NULL --> list not empty --> find the last element and add create_node to the end
	else{
		struct table_element *current = head;
		while(current->next != NULL)
			current = current->next;
		current->next = create_node;
	}
}

struct table_element* delete_element(uint32_t ip, uint16_t port){

	// if head == NULL --> list is empty 
	if (head == NULL)
		return NULL;

	// find the element that matches the given ip & port
	// iterate from the head to the end
	struct table_element *current = head;
	struct table_element *previous = NULL;
	int find_flag = 0;

	while(current != NULL){
		// if find, current is the matched one
		if ((current->inter_ip == ip && current->inter_port == port) || (current->trans_ip == ip && current->trans_port == port) )
		{
			find_flag = 1;
			break;
		}
		// update previous & current
		previous = current;
		current = current->next;
	}

	// find the element
	if (find_flag == 1)
	{
		// head is the matched one
		if (current == head)
			head = head->next;
		// head is not the matched one
		else
			previous->next = current->next;

		return current;
	}
	else
		return NULL;

}

struct table_element* find_element(uint32_t ip, uint16_t port){
	// if head == NULL --> list is empty 
	if (head == NULL)
		return NULL;

	// find the element that matches the given ip & port
	// use current to iterate from the head to the end
	struct table_element *current = head;

	while(current != NULL){
		// if find, current is the matched one
		if ((current->inter_ip == ip && current->inter_port == port) || (current->trans_ip == ip && current->trans_port == port) )
			return current;
		// update current
		current = current->next;
	}

	// not found --> return NULL
	return NULL;
} 

void print_table(){
	struct table_element *current = head;

	printf("\nTable Display: \n");
	printf("========================================================================\n");
	printf("     Internal IP     Internal Port     Translated IP     Translated Port\n");
	printf("------------------------------------------------------------------------\n");
	if (head == NULL)
	{
		printf("Empty Table!\n");
	}
	else{
		while(current != NULL){
			struct in_addr read;
			read.s_addr = htonl(current->inter_ip);
			printf("%16s,%17d,", (char*)inet_ntoa(read),current->inter_port);
			read.s_addr = htonl(current->trans_ip);
			printf("%17s,%19d\n", (char*)inet_ntoa(read),current->trans_port);

			current = current->next;
		}
	}
	
	printf("========================================================================\n\n");
}

