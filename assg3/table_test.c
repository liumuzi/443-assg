#include <stdio.h>
#include <stdlib.h>
#include "nat_table.c"
#include <netinet/in.h>

int main(void){
	struct table_element *create_node = (struct table_element*)malloc(sizeof(struct table_element));
	uint32_t inter_ip = 100;
	uint32_t trans_ip = 200;
	uint16_t inter_port = 1000;
	uint16_t trans_port = 2000;
	insert_element(inter_ip,trans_ip,inter_port,trans_port);

	inter_ip = 300;
	trans_ip = 400;
	inter_port = 3000;
	trans_port = 4000;
	insert_element(inter_ip,trans_ip,inter_port,trans_port);

	inter_ip = 100;
	trans_ip = 200;
	inter_port = 3000;
	trans_port = 4000;
	insert_element(inter_ip,trans_ip,inter_port,trans_port);
	print_table();

	printf("delete: \n");
	struct table_element *dele_node = (struct table_element*)malloc(sizeof(struct table_element));
	dele_node = delete_element(100,1000);
	print_table();
	printf("delete:\n");
	dele_node = delete_element(200,4000);
	print_table();

	return 0;
}