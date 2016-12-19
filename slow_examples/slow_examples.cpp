
#include "stdafx.h"
#include <cstdio>
#include <stdint.h>
#include <tchar.h>
#include <cstring>
#include <iostream>


const char *byte_to_binary(uint8_t in)
{
	static char b[9];
	b[0] = '\0';

	int z;
	for (z = 128; z > 0; z >>= 1)
	{
		strcat(b, ((in & z) == z) ? "1" : "0");
	}

	return b;
}

const char *short_to_binary(uint16_t in)
{
	static char b[17];
	b[0] = '\0';

	uint16_t z;
	for (z = 32768; z > 0; z >>= 1)
	{
		strcat(b, ((in & z) == z) ? "1" : "0");
	}

	return b;
}


void crc16_print_reg_pad(uint8_t *data, int data_len, uint16_t reg, int reg_idx, uint16_t poly) {
	int paddedLen = data_len + 2; // include two 0 bytes

								  // create dynamic buffers
	char * data_buff = new char[paddedLen * 8 + 1];
	data_buff[0] = '\0';

	char * poly_buff = new char[paddedLen * 8 + 1];
	poly_buff[paddedLen * 8] = '\0';

	// copy in the data and 0 bytes as binary strings
	for (int i = 0; i < paddedLen; i++) {
		if (i < data_len)
			strcpy(data_buff + (i * 8), byte_to_binary(data[i]));
		else strcpy(data_buff + (i * 8), byte_to_binary(0));
	}

	// set shifted bits to 0 
	for (int i = 0; i < reg_idx; i++) {
		data_buff[i] = '0';
	}
	// copy in the working remainder
	memcpy(data_buff + reg_idx, short_to_binary(reg), 16);

	// initalise to space chars
	for (int i = 0; i < paddedLen * 8 - 1; i++) {
		strcpy(poly_buff + i, " ");
	}

	// copy in the truncated poly and add the omitted 1 or 0 
	memcpy(poly_buff + reg_idx + 1, short_to_binary(poly), 16);
	poly_buff[reg_idx] = poly == 0 ? '0' : '1';

	printf("\n%s\n%s", data_buff, poly_buff);

	// clean up
	delete[] data_buff;
	delete[] poly_buff;
}

/*
* follows the hand written long binary division process as closely as possible.
* uses truncated polys to keep registers at round byte numbers
*/
uint16_t crc16_sequential_bit_add(uint8_t* data, int data_len, uint16_t poly) {

	printf("\n\ncrc16_sequential_bit_add\ndata: ");
	for (int i = 0; i < data_len; i++)
		printf("%s ", byte_to_binary(data[i]));
	printf("\npoly: 1%s", short_to_binary(poly));

	uint16_t remainder; // remainder register. 
	remainder = (((uint16_t)data[0] << 8) ^ data[1]); // remainder inialised as first 2 bytes of data.

													  // for each byte
	for (int i = 0; i < data_len; i++) {
		uint8_t next_byte_idx = i + 2; // first 2 are already in the remainder register, start feeding from the 3rd.

		uint8_t byte = 0; // when in data range, load next byte, otherwise pad with 0's
		if (next_byte_idx < data_len)
			byte = data[next_byte_idx];

		// for each bit
		uint8_t bit = 8;
		while (bit--) {

			// shift and xor
			if (remainder & 0x8000) {
				// when top bit is 1

				// visual representation
				//crc16_print_reg_pad(data, 3, remainder, (i * 8) + (7 - bit), poly);

				// shift first, as we are using a truncated poly with the leading 1 removed.
				remainder = (remainder << 1) ^ poly;
			}
			else remainder = (remainder << 1); // just shift

											   // move in next bit
			remainder ^= (byte >> bit) & 1; // shift and mask
		}
	}

	printf("\nFinal remainder: %s", short_to_binary(remainder));

	return remainder;
}

/*
* XORS each new byte into the left hand side of the register rather than adding each bit separatly
* simplifies the code (I do no know why the xor works...)
* uses truncated polys to keep registers at round bytes
*/
uint16_t crc16_byte_xor_add(uint8_t* data, int data_len, uint16_t poly) {

	printf("\n\ncrc16_byte_xor_add\ndata: ");
	for (int i = 0; i < data_len; i++)
		printf("%s ", byte_to_binary(data[i]));
	printf("\npoly: 1%s", short_to_binary(poly));

	uint16_t remainder = 0; // remainder register

	for (int i = 0; i < data_len; i++) {
		//printf("\nbyte: %s", byte_to_binary(data[i]));
		// xor the next data byte to the left hand side of the register
		remainder ^= (data[i] << 8);
		//printf("\nremainder ^ byte: %s", short_to_binary(remainder));


		uint8_t bit = 8;
		while (bit--) {
			remainder = (remainder & 0x8000) ? (remainder << 1) ^ poly : (remainder << 1);
			//printf("\nbit %d remainder: %s", bit, short_to_binary(remainder));
		}
	}
	printf("\nFinal remainder: %s", short_to_binary(remainder));

	return remainder;
}




int main()
{
	uint8_t data[3] = { 0x46, 0xD0, 0xab };
	uint8_t data_remainder[5];
	memcpy(data_remainder, data, 3);
	uint16_t poly = 0xD5A9;
	uint16_t result;

	result = crc16_sequential_bit_add(data, 3, poly);
	data_remainder[3] = result >> 8; data_remainder[4] = result;
	crc16_sequential_bit_add(data_remainder, 5, poly);

	result = crc16_byte_xor_add(data, 3, poly);
	data_remainder[3] = result >> 8; data_remainder[4] = result;
	crc16_byte_xor_add(data_remainder, 5, poly);

	result = crc16_sequential_bit_add(data, 3, poly);
	data_remainder[3] = result >> 8; data_remainder[4] = result;
	crc16_byte_xor_add(data_remainder, 5, poly);

	result = crc16_byte_xor_add(data, 3, poly);
	data_remainder[3] = result >> 8; data_remainder[4] = result;
	crc16_sequential_bit_add(data_remainder, 5, poly);


	std::cin.ignore();

	return 0;

}



