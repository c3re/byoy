/* hexdump.h */
/*
    This file is part of the AVR-Crypto-Lib.
    Copyright (C) 2011 Daniel Otte (daniel.otte@rub.de)

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

#ifndef HEXDUMP_H_
#define HEXDUMP_H_

void hexdump_block(FILE* stream, void* block, void* print_addr, unsigned length, unsigned width);

#endif /* HEXDUMP_H_ */
