#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <endian.h>

#include "netflow_defs.h"

#define __packed __attribute__((packed))
#define ARR_SZ(arrname) (sizeof(arrname)/sizeof(*(arrname))) 

struct netflow_header {
	uint16_t version;
	uint16_t count;
	uint32_t system_uptime;
	uint32_t unix_seconds;
	uint32_t package_sequence;
	uint32_t source_id;
} __packed;

struct flowset_header {
	uint16_t id;
	uint16_t length;
} __packed;

struct options_template_header {
	uint16_t template_id;
	uint16_t option_scope_length;
	uint16_t option_length;
} __packed;


struct data_template_header {
	uint16_t template_id;
	uint16_t field_count;
} __packed;

struct template_field {
	uint16_t type;
	uint16_t length;
} __packed;


static unsigned char accum[1500];
static int pos;

static struct netflow_header *set_header(void)
{
	struct netflow_header *header = (void*)&accum[0];
	static uint32_t package_sequence = 0;

	header->version = htobe16(9);
	header->count = 0; //save in host endian untill end
	header->system_uptime = htobe32(0x00000050);

	header->unix_seconds = htobe32(time(NULL));
	header->package_sequence = htobe32(package_sequence++);
	header->source_id = htobe32(1);

	pos = sizeof(struct netflow_header);
	return header;
}

typedef struct {
	uint16_t field, len;
	uint8_t value[3][32];
} setup_t;


setup_t data[] = {
	{PROTOCOL, PROTOCOL_LEN, {{6}, {17}, {1}}},
	{IPV4_SRC_ADDR, IPV4_SRC_ADDR_LEN, {{0x11, 0x22, 0x33, 0x44}, {0x11, 0x22, 0x33, 0x55}, {0x11, 0x22, 0x44, 0x44}}},
	{IPV4_DST_ADDR, IPV4_DST_ADDR_LEN, {{0x44, 0x33, 0x22, 0x11}, {0x44, 0x33, 0x22, 0x11}, {0x44, 0x33, 0x22, 0x11}}},
	{L4_SRC_PORT, L4_SRC_PORT_LEN, {{0x12, 0x34}, {0x12, 0x43}, {0x12, 0xc3}}},
	{L4_DST_PORT, L4_DST_PORT_LEN, {{0x23, 0x45}, {0x23, 0x56}, {0x12, 0x3c}}},
	{IN_BYTES, IN_BYTES_LEN, {{0x00, 0x00, 0x01, 0xcc}, {0x00, 0x00, 0x01, 0xbb}, {0x00, 0x00, 0x01, 0xbb}}},
	{IN_PKTS, IN_PKTS_LEN, {{0x00, 0x00, 0x00, 0xcc}, {0x00, 0x00, 0x00, 0xcc}, {0x00, 0x00, 0x00, 0xcb}}},
	{FLOWS, FLOWS_LEN, {{0x00, 0x00, 0x00, 0x01}, {0x00, 0x00, 0x00, 0x10}, {0x00, 0x00, 0x01, 0x00}}},
	{APPLICATION_TAG, APPLICATION_TAG_LEN, {{0x00, 0x00, 0x03, 0xe9}, {0x00, 0x00, 0x03, 0xea}, {0x00, 0x00, 0x03, 0xeb}}},
};

static void set_data_template(void)
{
	int i, start;
	struct flowset_header *flowset;
	struct data_template_header *data_template;
	struct template_field *field;

	start = pos;

	flowset = (void*)&accum[pos];
	pos += sizeof (*flowset);
	flowset->id = htobe16(0); //Data Template V9

	data_template = (void*)&accum[pos];
	pos += sizeof (*data_template);

	data_template->template_id = htobe16(259);
	data_template->field_count = 0; /*convert endianess at the end */

	for (i = 0; i < ARR_SZ(data); i++)
	{
		field = (void*)&accum[pos];
		pos += sizeof (*field);

		field->type = htobe16(data[i].field);
		field->length = htobe16(data[i].len);
		data_template->field_count++;
	}

	flowset->length = pos-start;
//	printf("--------------\ncnt %d, len %d\n============\n", data_template->field_count, flowset->length);
	flowset->length = htobe16(flowset->length);
	data_template->field_count = htobe16(data_template->field_count);
}

static void set_data(int random_val)
{
	int i, j, start;
	struct flowset_header *flowset;

	start = pos;

	flowset = (void*)&accum[pos];
	pos += sizeof (*flowset);
	flowset->id = htobe16(259); /*Data*/

	j = random_val%3;
	for (i = 0; i < ARR_SZ(data); i++)
	{
		if (data[i].field == IN_BYTES)
			data[i].value[j][3] = (random_val & 0xff00)>>8;

		if (data[i].field == IN_PKTS)
			data[i].value[j][3] = (random_val & 0xff0000)>>16;

		memcpy(&accum[pos], data[i].value[j], data[i].len);
		pos += data[i].len;
	}

	flowset->length = pos - start;

	while (flowset->length & 0x3)
	{
		pos++;
		flowset->length++;
	}


	flowset->length = pos-start;
//	printf("--------------\ndata len %d, pos %d\n============\n", flowset->length, pos);
	flowset->length = htobe16(flowset->length);
}

int main(void)
{
	struct netflow_header *pkt_head;

	srand(time(NULL));
	for (;;)
	{
		pkt_head = set_header();

		set_data_template();
		pkt_head->count++;

		set_data(rand());
		pkt_head->count++;

		pkt_head->count = htobe16(pkt_head->count);
		write(1, accum, pos);
		sleep(1);
	}


	return 0;



	
}
