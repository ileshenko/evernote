#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>

#define IPFIX_PROTO
#include "netflow_defs.h"

#define __packed __attribute__((packed))
#define ARR_SZ(arrname) (sizeof(arrname)/sizeof(*(arrname)))
#define NTOP_ENEPRISE_ID 35632

struct netflow_header {
	uint16_t version;
	uint16_t length;
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
	uint16_t field_count;
	uint16_t scope_field_count;
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

	header->version = htobe16(10);
	header->length = 0;
//	header->system_uptime = htobe32(0x00000050);
	header->unix_seconds = htobe32(1458490401);
	header->package_sequence = htobe32(0);
	header->source_id = htobe32(1);

	pos = sizeof(struct netflow_header);
	return header;
}

typedef struct {
	uint16_t field, len;
	uint8_t data[3][32];
} setup_t;

setup_t options_scope[] = {
	{SYSTEM_ID, 4, {{0x12, 0x34, 0x56, 0x78}, {0x12, 0x34, 0x56, 0x78}, {0x12, 0x34, 0x56, 0x78}}}
};

setup_t options[] = {
	{APPLICATION_TAG, APPLICATION_TAG_LEN, {{0x01, 0x00, 0x00, 0x01}, {0x01, 0x00, 0x00, 0x02}, {0x01, 0x00, 0x00, 0x03}}},
	{APPLICATION_NAME, APPLICATION_NAME_LEN, {"Red", "Green", "Blue"}},
	{L7_PROTO | 0x8000, L7_PROTO_LEN, {{0x01, 0x06}, {0x01, 0x07}, {0x01, 0x08}}},
	{L7_PROTO_NAME | 0x8000, L7_PROTO_NAME_LEN, {"Misha", "Masha", "Dasha"}}
};

static void set_opt_template(void)
{
	int i, start;
	struct flowset_header *flowset;
	struct options_template_header *options_template;
	struct template_field *field;
	uint32_t *enterprise_id;

	start = pos;

	flowset = (void*)&accum[pos];
	pos += sizeof(*flowset);
	flowset->id = htobe16(3); //Options Template V10

	options_template = (void*)&accum[pos];
	pos += sizeof(*options_template);

	options_template->template_id = htobe16(256);
	options_template->field_count = 0;
	options_template->scope_field_count = 0;

	for (i = 0; i < ARR_SZ(options_scope); i++)
	{
		field = (void*)&accum[pos];
		pos += sizeof(*field);

		field->type = htobe16(options_scope[i].field);
		field->length = htobe16(options_scope[i].len);
		options_template->field_count++;
		options_template->scope_field_count++;
	}

	for (i = 0; i < ARR_SZ(options); i++)
	{
		field = (void*)&accum[pos];
		pos += sizeof(*field);

		field->type = htobe16(options[i].field);
		field->length = htobe16(options[i].len);
		options_template->field_count++;

		if (!(options[i].field & 0x8000))
			continue;
		enterprise_id = (void*)&accum[pos];
		pos += sizeof(*enterprise_id);
		*enterprise_id = htobe32(NTOP_ENEPRISE_ID);
	}

	flowset->length = pos - start;
	while (flowset->length & 0x3)
	{
		pos++;
		flowset->length++;
	}

	printf("--------------\n len %d fields %d scope fields %d \n============\n",
		flowset->length, options_template->field_count,
		options_template->scope_field_count);

	flowset->length = htobe16(flowset->length);
	options_template->field_count = htobe16(options_template->field_count);
	options_template->scope_field_count = htobe16(options_template->scope_field_count);
}

setup_t data[] = {
	{IN_BYTES, IN_BYTES_LEN, {{0x00, 0x00, 0xbb, 0xcc}, {0x00, 0x00, 0xdd, 0xbb}, {0x00, 0x00, 0xcc, 0xbb}}},
	{IN_PKTS, IN_PKTS_LEN, {{0x00, 0x00, 0x00, 0xcc}, {0x00, 0x00, 0x01, 0xcc}, {0x00, 0x00, 0x0c, 0xcb}}},
	{PROTOCOL, PROTOCOL_LEN, {{6}, {17}, {1}}},
	{L4_SRC_PORT, L4_SRC_PORT_LEN, {{0x12, 0x34}, {0x12, 0x43}, {0x12, 0xc3}}},
	{IPV4_SRC_ADDR, IPV4_SRC_ADDR_LEN, {{0x11, 0x22, 0x33, 0x44}, {0x11, 0x22, 0x33, 0x55}, {0x11, 0x22, 0x44, 0x44}}},
	{L4_DST_PORT, L4_DST_PORT_LEN, {{0x23, 0x45}, {0x23, 0x56}, {0x12, 0x3c}}},
	{IPV4_DST_ADDR, IPV4_DST_ADDR_LEN, {{0x44, 0x33, 0x22, 0x11}, {0x44, 0x33, 0x22, 0x11}, {0x44, 0x33, 0x22, 0x11}}},
	{LAST_SWITCHED, LAST_SWITCHED_LEN, {{0x00, 0x00, 0x00, 0x50}, {0x00, 0x00, 0x002, 0x50}, {0x00, 0x00, 0x002, 0x05}}},
	{FIRST_SWITCHED, FIRST_SWITCHED_LEN, {{0x00, 0x00, 0x00, 0x4d}, {0x00, 0x00, 0x01, 0x4d}, {0x00, 0x00, 0x01, 0x0d}}},
//	{FLOWS, FLOWS_LEN, {{0x00, 0x00, 0x00, 0x01}, {0x00, 0x00, 0x00, 0x10}, {0x00, 0x00, 0x01, 0x00}}},
//	{APPLICATION_NAME,APPLICATION_NAME_LEN, "sdfgsdfgsdfg"},
//	{APPLICATION_TAG, APPLICATION_TAG_LEN, {{0x01, 0x00, 0x00, 47}, {0x01, 0x00, 0x00, 65}, {0x01, 0x00, 0x00, 0}}},
	{L7_PROTO | 0x8000, L7_PROTO_LEN, {{0x00, 0x10}, {0x00, 0x11}, {0x00, 0x12}}},
	{L7_PROTO_NAME | 0x8000, L7_PROTO_NAME_LEN, {"mama", "papa", "ira"}},
};

static void set_data_template(void)
{
	int i, start;
	struct flowset_header *flowset;
	struct data_template_header *data_template;
	struct template_field *field;
	uint32_t *enterprise_id;

	start = pos;

	flowset = (void*)&accum[pos];
	pos += sizeof(*flowset);
	flowset->id = htobe16(2); //Data Template V10

	data_template = (void*)&accum[pos];
	pos += sizeof(*data_template);

	data_template->template_id = htobe16(270);
	data_template->field_count = 0; /*convert endianess at the end */

	for (i = 0; i < ARR_SZ(data); i++)
	{
		field = (void*)&accum[pos];
		pos += sizeof(*field);

		field->type = htobe16(data[i].field);
		field->length = htobe16(data[i].len);
		data_template->field_count++;

		if (!(data[i].field & 0x8000))
			continue;

		enterprise_id = (void*)&accum[pos];
		pos += sizeof(*enterprise_id);
		*enterprise_id = htobe32(NTOP_ENEPRISE_ID);
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
	pos += sizeof(*flowset);
	flowset->id = htobe16(256); /* Options */

	for (j = 0; j<3; j++)
	{
#define WA options_scope
		for (i = 0; i < ARR_SZ(WA); i++)
		{
			memcpy(&accum[pos], WA[i].data[j], WA[i].len);
			pos += WA[i].len;
		}
#undef WA
#define WA options
		for (i = 0; i < ARR_SZ(WA); i++)
		{
			memcpy(&accum[pos], WA[i].data[j], WA[i].len);
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
	pos += sizeof(*flowset);
	flowset->id = htobe16(270); /*Data*/

	for (j = 0; j<3; j++)
	{
#define WA data
		for (i = 0; i < ARR_SZ(WA); i++)
		{
			memcpy(&accum[pos], WA[i].data[j], WA[i].len);
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
	set_data_template();
//	set_options();
	set_data();

	pkt_head->length = htobe16(pos);
	write(fd, accum, pos);

	close(fd);
	return 0;
}
