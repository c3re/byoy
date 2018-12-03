/* hotp.h */
/*
    This file is part of the AVR-Crypto-Lib.
    Copyright (C) 2013 Daniel Otte (daniel.otte@rub.de)

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

#ifndef HOTP_H_
#define HOTP_H_

#include <stdint.h>

void hotp(char *buffer, const void* secret, uint16_t secret_length_b, uint32_t counter, uint8_t digits);

#endif /* HOTP_H_ */
