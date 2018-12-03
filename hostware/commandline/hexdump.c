/* hexdump.c */
/*
    This file is part of the ARM-Crypto-Lib.
    Copyright (C) 2006-2011 Daniel Otte (daniel.otte@rub.de)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

static
void int_hexdump_line_buffer(FILE* stream, uint8_t* buffer, unsigned width, unsigned fill){
	unsigned i;
	for(i=0; i<width; ++i){
		if(i<fill){
			fprintf(stream, "%2.2"PRIx8" ", buffer[i]);
		}else{
			fputs("   ", stream);
		}
	}
	fputs("    |", stream);
	for(i=0; i<width; ++i){
		if(i<fill){
			if(isgraph(buffer[i])){
				fputc(buffer[i], stream);
			}else
				fputc('.', stream);
		}else{
			fputs(" ", stream);
		}
	}
	fputc('|', stream);

}


void hexdump_block(FILE* stream, void* block, void* print_addr, unsigned length, unsigned width){
	uint8_t buffer[width];
	unsigned index=0;
	while(length>width){
		memcpy(buffer, block, width);
		if(print_addr){
			fprintf(stream, "%p ", print_addr);
			print_addr = (uint8_t*)print_addr + width;
		}
		fprintf(stream, "<%4.4x>: ", index);
		int_hexdump_line_buffer(stream, buffer, width, width);
		fputc('\n', stream);
		block = (uint8_t*)block + width;
		length -= width;
		index += width;
	}
	memcpy(buffer, block, length);
	if(print_addr){
		fprintf(stream, "%p ", print_addr);
	}
	fprintf(stream, "<%4.4x>: ", index);
	int_hexdump_line_buffer(stream, buffer, width, length);
	fputc('\n', stream);
}
