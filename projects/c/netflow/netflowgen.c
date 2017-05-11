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

#if 0
#define NETFLOW_FIELDS_NUM 13

struct eznetflow_template_flowset {
	uint16_t 					flowset_id;
	uint16_t 					length;
//	struct eznetflow_template 	templates[1];
} __packed;

struct eznetflow_data_flow {
			uint32_t 	first_switched;
			uint32_t 	last_switched;
			char	 	protocol;
			uint32_t 	src_ip;
			uint32_t 	dst_ip;
			uint16_t 	src_port;
			uint16_t 	dst_port;
			uint32_t 	in_bytes;
			uint32_t 	in_pkts;
			//uint32_t 	flows;
			uint32_t 	application_tag;
			char 		application_name[APPLICATION_NAME_LEN];
			//uint32_t 	flow_start_sec;
			//uint32_t 	flow_end_sec;
			uint16_t 	l7_protocol_tag;
			char 		l7_protocol_name[APPLICATION_NAME_LEN];
} __packed;

struct eznetflow_data_flowset {
	union
	{
		char				raw_data[4+sizeof(struct eznetflow_data_flow)];

		struct
		{
			uint16_t 					flowset_id;
			uint16_t 					length;
			struct eznetflow_data_flow	flows[1];
		};
	};
};

//instance
struct eznetflow {
	uint32_t						boot_time;      /* Time when netflow_create() was called. */
	uint32_t						linux_boot_time;      /* Time when netflow_create() was called. */
	//shared counters. assuming it is updated by one thread only. not synced
	uint16_t 						netflow_count;//count netflow packages
	uint16_t 						template_flowset_count;//count template flowsets. value is 0-255
	uint16_t 						data_flowset_count;//count data flowsets. value is grater then 255
};

struct eznetflow_working_area {
	char							data[sizeof(struct eznetflow_header)+sizeof(struct eznetflow_template_flowset)+sizeof(struct eznetflow_data_flowset)];
	struct ezdp_rtc 				rtc_info;
};
#endif

static unsigned char accum[1500];
static int pos;

static struct netflow_header *set_header(void)
{
	struct netflow_header *header = (void*)&accum[0];

	header->version = htobe16(9);
	header->count = 0; //save in host endian untill end
	header->system_uptime = htobe32(0x00000050);
	header->unix_seconds = htobe32(1458490401);
	header->package_sequence = htobe32(0);
	header->source_id = htobe32(1);

	pos = sizeof(struct netflow_header);
	return header;
}

typedef struct {
	uint16_t field, len;
	uint8_t value[3][32];
} setup_t;

setup_t options_scope[] = {
	{SYSTEM_ID, 4, {{0x12, 0x34, 0x56, 0x78}, {0x12, 0x34, 0x56, 0x78}, {0x12, 0x34, 0x56, 0x78}}}
};

setup_t options[] = {
	{APPLICATION_TAG, APPLICATION_TAG_LEN, {{0x01, 0x00, 0x00, 0x01}, {0x01, 0x00, 0x00, 0x02}, {0x01, 0x00, 0x00, 0x03}}},
	{APPLICATION_NAME, APPLICATION_NAME_LEN, {"Red", "Green", "Blue"}},
//	{L7_PROTO, L7_PROTO_LEN, {{0x01, 0x06}, {0x01, 0x07}, {0x01, 0x08}}},
//	{L7_PROTO_NAME, L7_PROTO_NAME_LEN, {"Misha", "Masha", "Dasha"}}
};

static void set_opt_template(void)
{
	int i, start;
	struct flowset_header *flowset;
	struct options_template_header *options_template;
	struct template_field *field;

	start = pos;

	flowset = (void*)&accum[pos];
	pos += sizeof (*flowset);
	flowset->id = htobe16(1); //Options Template V9

	options_template = (void*)&accum[pos];
	pos += sizeof (*options_template);

	options_template->template_id = htobe16(256);
	options_template->option_scope_length = 0;
	options_template->option_length = 0;

	for (i = 0; i < ARR_SZ(options_scope); i++)
	{
		field = (void*)&accum[pos];
		pos += sizeof (*field);

		field->type = htobe16(options_scope[i].field);
		field->length = htobe16(options_scope[i].len);
		options_template->option_scope_length += sizeof (*field);
	}

	for (i = 0; i < ARR_SZ(options); i++)
	{
		field = (void*)&accum[pos];
		pos += sizeof (*field);

		field->type = htobe16(options[i].field);
		field->length = htobe16(options[i].len);
		options_template->option_length += sizeof (*field);
	}

	flowset->length = pos - start;
	while (flowset->length & 0x3)
	{
		pos++;
		flowset->length++;
	}

	printf("--------------\n len %d scope len %d opt let %d \n============\n",
		flowset->length, options_template->option_scope_length,
		options_template->option_length);

	flowset->length = htobe16(flowset->length);
	options_template->option_scope_length = htobe16(options_template->option_scope_length);
	options_template->option_length = htobe16(options_template->option_length);
}

setup_t data[] = {
	{FIRST_SWITCHED, FIRST_SWITCHED_LEN, {{0x00, 0x00, 0x00, 0x4d}, {0x00, 0x00, 0x01, 0x4d}, {0x00, 0x00, 0x01, 0x0d}}},
	{LAST_SWITCHED, LAST_SWITCHED_LEN, {{0x00, 0x00, 0x00, 0x50}, {0x00, 0x00, 0x002, 0x50}, {0x00, 0x00, 0x002, 0x05}}},
	{PROTOCOL, PROTOCOL_LEN, {{6}, {17}, {1}}},
	{IPV4_SRC_ADDR, IPV4_SRC_ADDR_LEN, {{0x11, 0x22, 0x33, 0x44}, {0x11, 0x22, 0x33, 0x55}, {0x11, 0x22, 0x44, 0x44}}},
	{IPV4_DST_ADDR, IPV4_DST_ADDR_LEN, {{0x44, 0x33, 0x22, 0x11}, {0x44, 0x33, 0x22, 0x11}, {0x44, 0x33, 0x22, 0x11}}},
	{L4_SRC_PORT, L4_SRC_PORT_LEN, {{0x12, 0x34}, {0x12, 0x43}, {0x12, 0xc3}}},
	{L4_DST_PORT, L4_DST_PORT_LEN, {{0x23, 0x45}, {0x23, 0x56}, {0x12, 0x3c}}},
	{IN_BYTES, IN_BYTES_LEN, {{0x00, 0x00, 0xbb, 0xcc}, {0x00, 0x00, 0xdd, 0xbb}, {0x00, 0x00, 0xcc, 0xbb}}},
	{IN_PKTS, IN_PKTS_LEN, {{0x00, 0x00, 0x00, 0xcc}, {0x00, 0x00, 0x01, 0xcc}, {0x00, 0x00, 0x0c, 0xcb}}},
	{FLOWS, FLOWS_LEN, {{0x00, 0x00, 0x00, 0x01}, {0x00, 0x00, 0x00, 0x10}, {0x00, 0x00, 0x01, 0x00}}},
//	{APPLICATION_NAME,APPLICATION_NAME_LEN, "sdfgsdfgsdfg"},
//	{APPLICATION_TAG, APPLICATION_TAG_LEN, {{0x01, 0x00, 0x00, 47}, {0x01, 0x00, 0x00, 65}, {0x01, 0x00, 0x00, 0}}},
	{L7_PROTO, L7_PROTO_LEN, {{0x00, 68}, {0x00, 69}, {0x00, 70}}},
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
	printf("--------------\ncnt %d, len %d\n============\n", data_template->field_count, flowset->length);
	flowset->length = htobe16(flowset->length);
	data_template->field_count = htobe16(data_template->field_count);
}

static void set_options(void)
{
	int i, j, start;
	struct flowset_header *flowset;

	start = pos;

	flowset = (void*)&accum[pos];
	pos += sizeof (*flowset);
	flowset->id = htobe16(256); /* Options */

	for (j = 0; j<3; j++)
	{
#define WA options_scope
		for (i = 0; i < ARR_SZ(WA); i++)
		{
			memcpy(&accum[pos], WA[i].value[j], WA[i].len);
			pos += WA[i].len;
		}
#undef WA
#define WA options
		for (i = 0; i < ARR_SZ(WA); i++)
		{
			memcpy(&accum[pos], WA[i].value[j], WA[i].len);
			pos += WA[i].len;
		}
#undef WA
	}
	flowset->length = pos - start;

	while (flowset->length & 0x3)
	{
		pos++;
		flowset->length++;
	}

	printf("--------------\nopt len %d, pos %d\n============\n", flowset->length, pos);
	flowset->length = htobe16(flowset->length);
}

static void set_data(void)
{
	int i, j, start;
	struct flowset_header *flowset;

	start = pos;

	flowset = (void*)&accum[pos];
	pos += sizeof (*flowset);
	flowset->id = htobe16(259); /*Data*/

	for (j = 0; j<3; j++)
	{
#define WA data
		for (i = 0; i < ARR_SZ(WA); i++)
		{
			memcpy(&accum[pos], WA[i].value[j], WA[i].len);
			pos += WA[i].len;
		}
#undef WA
	}
	flowset->length = pos - start;

	while (flowset->length & 0x3)
	{
		pos++;
		flowset->length++;
	}


	flowset->length = pos-start;
	printf("--------------\ndata len %d, pos %d\n============\n", flowset->length, pos);
	flowset->length = htobe16(flowset->length);
}

int main(void)
{
	int fd = open("rename_me.bin", O_WRONLY | O_CREAT | O_APPEND, 0666);
	struct netflow_header *pkt_head;


	pkt_head = set_header();
//	set_opt_template();
//	pkt_head->count++;


	set_data_template();
	pkt_head->count++;

//	set_options();
//	pkt_head->count++;

	set_data();
	pkt_head->count++;

	pkt_head->count = htobe16(pkt_head->count);
	write(fd, accum, pos);

	close(fd);
	return 0;



	
}
