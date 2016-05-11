/*
 * Copyright 2010-2016 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_DOSFONT_H
#define OPENXCOM_DOSFONT_H

#define DOSFONT_SIZE 1790


unsigned char dosFont[DOSFONT_SIZE]
{
	0x42,0x4D,0xFE,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x3E,0x00,0x00,0x00,0x28,0x00,
	0x00,0x00,0x20,0x01,0x00,0x00,0x30,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0xC3,0x0E,0x00,0x00,0xC3,0x0E,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x00,0x00,0x0F,0x00,0x00,
	0x00,0x00,0x00,0x00,0xF0,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7C,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x98,0x00,
	0x00,0x19,0x80,0x00,0x00,0x00,0x00,0x00,0x60,0x06,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x18,0x00,0x00,0x19,0x80,0x00,0x00,0x00,0x00,0x00,0x60,0x06,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3B,
	0x1F,0x0F,0x87,0x63,0xE3,0xC0,0xF8,0xE6,0x1E,0x01,0x9C,0xC3,0xC6,0xD9,0x98,0xF8,
	0x7C,0x3E,0x3C,0x0F,0x81,0xC3,0xB0,0x60,0xCC,0xC3,0x3F,0x3F,0x81,0xC1,0x83,0x80,
	0x00,0x00,0x00,0x66,0x19,0x98,0xCC,0xC6,0x31,0x81,0x98,0x66,0x0C,0x01,0x8C,0xC1,
	0x86,0xD9,0x99,0x8C,0x66,0x66,0x18,0x18,0xC3,0x66,0x60,0xF1,0xFE,0x66,0x63,0x31,
	0x83,0x01,0x80,0xC0,0x01,0xFC,0x00,0x66,0x19,0x98,0x0C,0xC6,0x01,0x81,0x98,0x66,
	0x0C,0x01,0x8D,0x81,0x86,0xD9,0x99,0x8C,0x66,0x66,0x18,0x01,0x83,0x06,0x61,0x99,
	0xB6,0x3C,0x63,0x18,0x03,0x01,0x80,0xC0,0x01,0x8C,0x00,0x66,0x19,0x98,0x0C,0xC6,
	0x01,0x81,0x98,0x66,0x0C,0x01,0x8F,0x01,0x86,0xD9,0x99,0x8C,0x66,0x66,0x18,0x07,
	0x03,0x06,0x63,0x0D,0xB6,0x18,0x63,0x0C,0x03,0x01,0x80,0xC0,0x01,0x8C,0x00,0x3E,
	0x19,0x98,0x0C,0xC7,0xF1,0x81,0x98,0x66,0x0C,0x01,0x8F,0x01,0x86,0xD9,0x99,0x8C,
	0x66,0x66,0x19,0x8C,0x03,0x06,0x63,0x0D,0x86,0x3C,0x63,0x06,0x03,0x01,0x80,0xC0,
	0x01,0x8C,0x00,0x06,0x1B,0x18,0xC6,0xC6,0x33,0xC1,0x98,0x76,0x0C,0x01,0x8D,0x81,
	0x87,0xF9,0x99,0x8C,0x66,0x66,0x1D,0x98,0xC3,0x06,0x63,0x0D,0x86,0x66,0x63,0x33,
	0x0E,0x00,0x00,0x70,0x00,0xD8,0x00,0x3C,0x1E,0x0F,0x83,0xC3,0xE1,0x80,0xEC,0x6C,
	0x1C,0x03,0x8C,0xC1,0x87,0x33,0x70,0xF8,0xDC,0x3B,0x37,0x0F,0x8F,0xC6,0x63,0x0D,
	0x86,0xC3,0x63,0x3F,0x83,0x01,0x80,0xC0,0x00,0x70,0x00,0x00,0x18,0x00,0x00,0xC0,
	0x01,0x90,0x00,0x60,0x00,0x00,0x0C,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x01,0x80,0xC0,0x00,0x20,0x00,0x00,
	0x18,0x00,0x00,0xC0,0x01,0xB0,0x00,0x60,0x0C,0x01,0x8C,0x01,0x80,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x01,0x80,0xC3,
	0x70,0x00,0x18,0x00,0x38,0x00,0x01,0xC0,0x00,0xE0,0x00,0xE0,0x0C,0x01,0x9C,0x03,
	0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x01,0xC1,0x83,0x81,0xD8,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xFE,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7C,0x63,
	0x3F,0x07,0x8F,0x87,0xF3,0xC0,0x74,0xC6,0x1E,0x1E,0x1C,0xCF,0xE6,0x1B,0x18,0xF8,
	0xF0,0x3E,0x39,0x8F,0x83,0xC3,0xE0,0x60,0xCC,0xC3,0x1E,0x3F,0xC7,0x80,0x21,0xE0,
	0x00,0x00,0xC0,0x63,0x19,0x8C,0xC6,0xC3,0x31,0x80,0xCC,0xC6,0x0C,0x33,0x0C,0xC6,
	0x66,0x1B,0x19,0x8C,0x60,0x6F,0x19,0x98,0xC1,0x86,0x30,0xF0,0xCC,0xC3,0x0C,0x30,
	0xC6,0x00,0x60,0x60,0x00,0x00,0xDC,0x63,0x19,0x98,0x46,0x63,0x11,0x81,0x8C,0xC6,
	0x0C,0x33,0x0C,0xC6,0x26,0x1B,0x19,0x8C,0x60,0x6B,0x19,0x98,0xC1,0x86,0x31,0x99,
	0xFE,0x66,0x0C,0x30,0x46,0x00,0xE0,0x60,0x00,0x00,0xDE,0x63,0x19,0x98,0x06,0x63,
	0x01,0x81,0x8C,0xC6,0x0C,0x33,0x0D,0x86,0x06,0x1B,0x19,0x8C,0x60,0x63,0x19,0x80,
	0xC1,0x86,0x33,0x0D,0xB6,0x3C,0x0C,0x18,0x06,0x01,0xC0,0x60,0x00,0x00,0xDE,0x7F,
	0x19,0x98,0x06,0x63,0x41,0xA1,0xBC,0xC6,0x0C,0x03,0x0F,0x06,0x06,0x1B,0x39,0x8C,
	0x60,0x63,0x1B,0x01,0x81,0x86,0x33,0x0D,0xB6,0x18,0x0C,0x0C,0x06,0x03,0x80,0x60,
	0x00,0x00,0xDE,0x63,0x1F,0x18,0x06,0x63,0xC1,0xE1,0x80,0xFE,0x0C,0x03,0x0F,0x06,
	0x06,0xDB,0x79,0x8C,0x7C,0x63,0x1F,0x07,0x01,0x86,0x33,0x0D,0x86,0x18,0x1E,0x06,
	0x06,0x07,0x00,0x60,0x00,0x00,0xC6,0x63,0x19,0x98,0x06,0x63,0x41,0xA1,0x80,0xC6,
	0x0C,0x03,0x0D,0x86,0x07,0xFB,0xF9,0x8C,0x66,0x63,0x19,0x8C,0x01,0x86,0x33,0x0D,
	0x86,0x3C,0x33,0x03,0x06,0x0E,0x00,0x60,0x00,0x00,0xC6,0x36,0x19,0x98,0x46,0x63,
	0x11,0x89,0x84,0xC6,0x0C,0x03,0x0C,0xC6,0x07,0xFB,0xD9,0x8C,0x66,0x63,0x19,0x98,
	0xC9,0x96,0x33,0x0D,0x86,0x66,0x61,0xA1,0x86,0x0C,0x00,0x60,0x00,0x00,0x7C,0x1C,
	0x19,0x8C,0xC6,0xC3,0x31,0x98,0xCC,0xC6,0x0C,0x03,0x0C,0xC6,0x07,0x3B,0x99,0x8C,
	0x66,0x63,0x19,0x98,0xCD,0xB6,0x33,0x0D,0x86,0xC3,0x61,0xB0,0xC6,0x08,0x00,0x63,
	0x18,0x00,0x00,0x08,0x3F,0x07,0x8F,0x87,0xF3,0xF8,0x78,0xC6,0x1E,0x07,0x9C,0xCF,
	0x06,0x1B,0x18,0xF8,0xFC,0x3E,0x3F,0x0F,0x8F,0xF6,0x33,0x0D,0x86,0xC3,0x61,0xBF,
	0xC7,0x80,0x01,0xE1,0xB0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x80,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0C,
	0x00,0x0D,0x87,0xC4,0x31,0xD8,0x00,0x0C,0x18,0x00,0x00,0x01,0x80,0x00,0x61,0x00,
	0x3C,0x3F,0x3F,0x8F,0x81,0xE3,0xE1,0xF0,0x60,0x7C,0x3C,0x00,0x06,0x00,0x60,0x01,
	0x80,0x30,0x00,0x0C,0x00,0x0D,0x8C,0x66,0x33,0x30,0x00,0x18,0x0C,0x00,0x00,0x01,
	0x80,0x00,0x61,0x80,0x66,0x0C,0x31,0x98,0xC0,0xC6,0x33,0x18,0x60,0xC6,0x06,0x06,
	0x03,0x00,0xC0,0x00,0xC0,0x30,0x00,0x00,0x00,0x1F,0xC8,0x63,0x03,0x30,0x00,0x30,
	0x06,0x19,0x83,0x01,0x80,0x00,0x00,0xC0,0xC3,0x0C,0x30,0x00,0xC0,0xC0,0x33,0x18,
	0x60,0xC6,0x03,0x06,0x03,0x01,0x80,0x00,0x60,0x00,0x00,0x0C,0x00,0x0D,0x80,0x61,
	0x83,0x30,0x00,0x30,0x06,0x0F,0x03,0x00,0x00,0x00,0x00,0x60,0xC3,0x0C,0x18,0x00,
	0xC0,0xC0,0x33,0x18,0x60,0xC6,0x03,0x00,0x00,0x03,0x03,0xF0,0x30,0x30,0x00,0x0C,
	0x00,0x0D,0x80,0x60,0xC3,0x70,0x00,0x30,0x06,0x3F,0xCF,0xC0,0x07,0xF0,0x00,0x30,
	0xDB,0x0C,0x0C,0x00,0xCF,0xE0,0x33,0x18,0x30,0xC6,0x03,0x00,0x00,0x06,0x00,0x00,
	0x18,0x30,0x00,0x0C,0x00,0x0D,0x87,0xC0,0x61,0xD8,0x00,0x30,0x06,0x0F,0x03,0x00,
	0x00,0x00,0x00,0x18,0xDB,0x0C,0x06,0x07,0x8C,0xC7,0xE3,0xF0,0x18,0x7C,0x3F,0x00,
	0x00,0x03,0x00,0x00,0x30,0x30,0x00,0x1E,0x00,0x1F,0xCC,0x06,0x30,0xE0,0x00,0x30,
	0x06,0x19,0x83,0x00,0x00,0x00,0x00,0x0C,0xC3,0x0C,0x03,0x00,0xC6,0xC6,0x03,0x00,
	0x0C,0xC6,0x63,0x06,0x03,0x01,0x83,0xF0,0x60,0x18,0x00,0x1E,0x09,0x0D,0x8C,0x26,
	0x11,0xB0,0xC0,0x30,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0xC3,0x3C,0x01,0x80,
	0xC3,0xC6,0x03,0x00,0x0C,0xC6,0x63,0x06,0x03,0x00,0xC0,0x00,0xC1,0x8C,0x00,0x1E,
	0x19,0x8D,0x8C,0x60,0x01,0xB0,0x60,0x18,0x0C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x66,0x1C,0x31,0x98,0xC1,0xC6,0x01,0x81,0x8C,0xC6,0x63,0x00,0x00,0x00,0x60,0x01,
	0x81,0x8C,0x00,0x0C,0x19,0x80,0x07,0xC0,0x00,0xE0,0x60,0x0C,0x18,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x3C,0x0C,0x1F,0x0F,0x80,0xC7,0xF0,0xE1,0xFC,0x7C,0x3E,0x00,
	0x00,0x00,0x00,0x00,0x00,0xF8,0x00,0x00,0x19,0x80,0x01,0x80,0x00,0x00,0x60,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x80,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

#endif
