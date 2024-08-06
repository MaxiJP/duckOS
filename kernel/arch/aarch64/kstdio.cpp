/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2016-2024 Byteduck */

#include "rpi/MiniUART.h"

void serial_putch(char c) {
	RPi::MiniUART::tx(c);
}