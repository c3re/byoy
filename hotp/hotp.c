/* htop.c */
/*
    This file is part of the AVR-Crypto-Lib.
    Copyright (C) 2006-2013 Daniel Otte (daniel.otte@rub.de)

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


#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <hmac-sha1.h>

static
uint32_t dtrunc(uint8_t* buffer) {
    uint8_t idx;
    union {
        uint8_t w8[4];
        uint32_t w32;
    } r;

    idx = buffer[19] & 0x0f;
    r.w8[3] = buffer[idx++] & 0x7f;
    r.w8[2] = buffer[idx++];
    r.w8[1] = buffer[idx++];
    r.w8[0] = buffer[idx];
    return r.w32;
}

static
void reverse_string(char *str) {
    char *end;
    end = str + strlen(str) - 1;
    while (end > str) {
        *str ^= *end;
        *end ^= *str;
        *str ^= *end;
        ++str;
        --end;
    }
}

static
void to_digits(char *buffer, uint32_t value, uint8_t digits) {
    ldiv_t t;
    if (value == 0) {
        *buffer++ = '0';
    }
    while (value && digits--) {
        t = ldiv(value, 10);
        value = t.quot;
        *buffer++ = t.rem + '0';
    }
    *buffer = '\0';
}

void hotp(char *buffer, const void* secret, uint16_t secret_length_b, uint32_t counter, uint8_t digits) {
    union {
        uint8_t mac[20];
        uint8_t ctr_buffer[8];
    } d;
    uint32_t s;
    d.ctr_buffer[7] = counter & 0xff;
    counter >>= 8;
    d.ctr_buffer[6] = counter & 0xff;
    counter >>= 8;
    d.ctr_buffer[5] = counter & 0xff;
    counter >>= 8;
    d.ctr_buffer[4] = counter & 0xff;
    d.ctr_buffer[3] = 0;
    d.ctr_buffer[2] = 0;
    d.ctr_buffer[1] = 0;
    d.ctr_buffer[0] = 0;
    if (digits > 9) {
        digits = 9;
    }
    hmac_sha1(d.mac, secret, secret_length_b, d.ctr_buffer, 64);
    s = dtrunc(d.mac);
    to_digits(buffer, s, digits);
    reverse_string(buffer);
}
