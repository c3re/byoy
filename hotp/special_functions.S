/* special_functions.S */
/*
    This file is part of the Labotage2011_Badge.
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

#include <avr/io.h>
#include "usbconfig.h"

.global soft_reset
soft_reset:
	cli
	andi r24, 0x0F
	cpi r24, 0x0F
	breq 1f
	bst r24, 3
	bld r24, 5
	ori r24, 0x08
	ldi r25, 0x18
	or r25, r24
	sbi _SFR_IO_ADDR(PORTB), USB_CFG_DMINUS_BIT
	out _SFR_IO_ADDR(WDTCR), r25
	out _SFR_IO_ADDR(WDTCR), r24
1:
	rjmp 1b
