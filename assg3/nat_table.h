struct table_element
{
	uint32_t inter_ip;
	uint32_t trans_ip;
	uint16_t inter_port;
	uint16_t trans_port;
	int tcp_state;
	struct table_element *next;

};

void insert_element(uint32_t inter_ip,uint32_t trans_ip, uint16_t inter_port, uint16_t trans_port );
struct table_element* delete_element(uint32_t ip, uint16_t port);
struct table_element* find_element(uint32_t ip, uint16_t port);
void print_table();