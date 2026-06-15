#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STM8_FLASH_START 0x8000u
#define STM8_FLASH_END   0x9FFFu
#define STM8_FLASH_SIZE  (STM8_FLASH_END - STM8_FLASH_START + 1u)
#define MAX_LINE_LEN 512

static int hex_value(int c)
{
	if ((c >= '0') && (c <= '9')) {return c - '0';}
	if ((c >= 'A') && (c <= 'F')) {return c - 'A' + 10;}
	if ((c >= 'a') && (c <= 'f')) {return c - 'a' + 10;}
	return -1;
}

static int parse_hex_byte(const char *s, uint8_t *out)
{
	int hi = hex_value((unsigned char)s[0]);
	int lo = hex_value((unsigned char)s[1]);
	if ((hi < 0) || (lo < 0)) {return 0;}
	*out = (uint8_t)((hi << 4) | lo);
	return 1;
}

static unsigned address_len_for_type(char type)
{
	switch (type) {
		case '0':
		case '1':
		case '5':
		case '9':
			return 2;
		case '2':
		case '8':
			return 3;
		case '3':
		case '7':
			return 4;
		default:
			return 0;
	}
}

int main(int argc, char **argv)
{
	FILE *fp;
	char line[MAX_LINE_LEN];
	uint8_t seen[STM8_FLASH_SIZE];
	uint8_t data[STM8_FLASH_SIZE];
	unsigned long line_no = 0;
	unsigned long data_records = 0;
	unsigned long total_bytes = 0;
	unsigned long consistent_overlaps = 0;
	uint32_t min_addr = 0xFFFFFFFFu;
	uint32_t max_addr = 0;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <file.s19>\n", argv[0]);
		return 2;
	}

	memset(seen,0,sizeof(seen));
	memset(data,0,sizeof(data));

	fp = fopen(argv[1],"rb");
	if (!fp) {
		perror(argv[1]);
		return 2;
	}

	while (fgets(line,sizeof(line),fp)) {
		size_t len;
		unsigned byte_count;
		unsigned address_len;
		unsigned data_len;
		unsigned checksum_pos;
		unsigned i;
		uint8_t value;
		uint8_t checksum;
		uint32_t address = 0;
		uint32_t sum = 0;
		char type;

		line_no++;
		len = strcspn(line,"\r\n");
		line[len] = '\0';
		if (len == 0) {continue;}

		if ((len < 4) || (line[0] != 'S')) {
			printf("FAIL line %lu: invalid S-record prefix\n", line_no);
			fclose(fp);
			return 1;
		}

		type = line[1];
		address_len = address_len_for_type(type);
		if (address_len == 0) {
			printf("FAIL line %lu: unsupported S-record type S%c\n", line_no, type);
			fclose(fp);
			return 1;
		}

		if (!parse_hex_byte(&line[2],&value)) {
			printf("FAIL line %lu: invalid byte count\n", line_no);
			fclose(fp);
			return 1;
		}
		byte_count = value;
		sum += value;

		if (len != (size_t)(4 + byte_count * 2)) {
			printf("FAIL line %lu: length mismatch\n", line_no);
			fclose(fp);
			return 1;
		}

		if (byte_count < address_len + 1) {
			printf("FAIL line %lu: byte count too small\n", line_no);
			fclose(fp);
			return 1;
		}

		for (i = 0; i < address_len; i++) {
			if (!parse_hex_byte(&line[4 + i * 2],&value)) {
				printf("FAIL line %lu: invalid address byte\n", line_no);
				fclose(fp);
				return 1;
			}
			address = (address << 8) | value;
			sum += value;
		}

		data_len = byte_count - address_len - 1;
		checksum_pos = 4 + (address_len + data_len) * 2;

		if ((type == '1') || (type == '2') || (type == '3')) {
			if (data_len == 0) {
				printf("FAIL line %lu: empty data record\n", line_no);
				fclose(fp);
				return 1;
			}
			if ((address < STM8_FLASH_START) || ((address + data_len - 1u) > STM8_FLASH_END)) {
				printf("FAIL line %lu: data outside STM8 flash at 0x%08lX length=%u\n",
					line_no,
					(unsigned long)address,
					data_len);
				fclose(fp);
				return 1;
			}
		}

		for (i = 0; i < data_len; i++) {
			if (!parse_hex_byte(&line[4 + (address_len + i) * 2],&value)) {
				printf("FAIL line %lu: invalid data byte\n", line_no);
				fclose(fp);
				return 1;
			}
			sum += value;

			if ((type == '1') || (type == '2') || (type == '3')) {
				uint32_t absolute = address + i;
				unsigned offset = (unsigned)(absolute - STM8_FLASH_START);
				if (seen[offset]) {
					if (data[offset] != value) {
						printf("FAIL line %lu: conflicting overlap at 0x%04lX old=0x%02X new=0x%02X\n",
							line_no,
							(unsigned long)absolute,
							data[offset],
							value);
						fclose(fp);
						return 1;
					}
					consistent_overlaps++;
				} else {
					seen[offset] = 1;
					data[offset] = value;
					total_bytes++;
					if (absolute < min_addr) {min_addr = absolute;}
					if (absolute > max_addr) {max_addr = absolute;}
				}
			}
		}

		if (!parse_hex_byte(&line[checksum_pos],&checksum)) {
			printf("FAIL line %lu: invalid checksum byte\n", line_no);
			fclose(fp);
			return 1;
		}
		sum += checksum;
		if ((sum & 0xFFu) != 0xFFu) {
			printf("FAIL line %lu: checksum mismatch\n", line_no);
			fclose(fp);
			return 1;
		}

		if ((type == '1') || (type == '2') || (type == '3')) {
			data_records++;
		}
	}

	if (ferror(fp)) {
		perror(argv[1]);
		fclose(fp);
		return 2;
	}
	fclose(fp);

	if (data_records == 0) {
		printf("FAIL no data records found\n");
		return 1;
	}

	printf("PASS records=%lu data_records=%lu min=0x%04lX max=0x%04lX bytes=%lu consistent_overlaps=%lu\n",
		line_no,
		data_records,
		(unsigned long)min_addr,
		(unsigned long)max_addr,
		total_bytes,
		consistent_overlaps);

	return 0;
}
