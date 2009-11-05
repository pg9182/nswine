/*
 * Unit test suite for cryptui.dll
 *
 * Copyright 2008 Juan Lang
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include <stdarg.h>
#include <stdio.h>
#define NONAMELESSUNION
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <winuser.h>
#include <wincrypt.h>
#include <cryptuiapi.h>

#include "wine/test.h"

static BYTE v1CertWithValidPubKey[] = {
0x30,0x81,0xcf,0x02,0x01,0x01,0x30,0x02,0x06,0x00,0x30,0x15,0x31,0x13,0x30,
0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,
0x6e,0x67,0x00,0x30,0x22,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,
0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,
0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x15,0x31,0x13,0x30,0x11,
0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,
0x67,0x00,0x30,0x5c,0x30,0x0d,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,
0x01,0x01,0x05,0x00,0x03,0x4b,0x00,0x30,0x48,0x02,0x41,0x00,0xe2,0x54,0x3a,
0xa7,0x83,0xb1,0x27,0x14,0x3e,0x59,0xbb,0xb4,0x53,0xe6,0x1f,0xe7,0x5d,0xf1,
0x21,0x68,0xad,0x85,0x53,0xdb,0x6b,0x1e,0xeb,0x65,0x97,0x03,0x86,0x60,0xde,
0xf3,0x6c,0x38,0x75,0xe0,0x4c,0x61,0xbb,0xbc,0x62,0x17,0xa9,0xcd,0x79,0x3f,
0x21,0x4e,0x96,0xcb,0x0e,0xdc,0x61,0x94,0x30,0x18,0x10,0x6b,0xd0,0x1c,0x10,
0x79,0x02,0x03,0x01,0x00,0x01,0xa3,0x16,0x30,0x14,0x30,0x12,0x06,0x03,0x55,
0x1d,0x13,0x01,0x01,0xff,0x04,0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01 };
static const BYTE iTunesCert1[] = {
0x30,0x82,0x03,0xff,0x30,0x82,0x02,0xe7,0xa0,0x03,0x02,0x01,0x02,0x02,0x10,
0x0d,0xe9,0x2b,0xf0,0xd4,0xd8,0x29,0x88,0x18,0x32,0x05,0x09,0x5e,0x9a,0x76,
0x88,0x30,0x0d,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x05,0x05,
0x00,0x30,0x53,0x31,0x0b,0x30,0x09,0x06,0x03,0x55,0x04,0x06,0x13,0x02,0x55,
0x53,0x31,0x17,0x30,0x15,0x06,0x03,0x55,0x04,0x0a,0x13,0x0e,0x56,0x65,0x72,
0x69,0x53,0x69,0x67,0x6e,0x2c,0x20,0x49,0x6e,0x63,0x2e,0x31,0x2b,0x30,0x29,
0x06,0x03,0x55,0x04,0x03,0x13,0x22,0x56,0x65,0x72,0x69,0x53,0x69,0x67,0x6e,
0x20,0x54,0x69,0x6d,0x65,0x20,0x53,0x74,0x61,0x6d,0x70,0x69,0x6e,0x67,0x20,
0x53,0x65,0x72,0x76,0x69,0x63,0x65,0x73,0x20,0x43,0x41,0x30,0x1e,0x17,0x0d,
0x30,0x33,0x31,0x32,0x30,0x34,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x17,0x0d,
0x30,0x38,0x31,0x32,0x30,0x33,0x32,0x33,0x35,0x39,0x35,0x39,0x5a,0x30,0x57,
0x31,0x0b,0x30,0x09,0x06,0x03,0x55,0x04,0x06,0x13,0x02,0x55,0x53,0x31,0x17,
0x30,0x15,0x06,0x03,0x55,0x04,0x0a,0x13,0x0e,0x56,0x65,0x72,0x69,0x53,0x69,
0x67,0x6e,0x2c,0x20,0x49,0x6e,0x63,0x2e,0x31,0x2f,0x30,0x2d,0x06,0x03,0x55,
0x04,0x03,0x13,0x26,0x56,0x65,0x72,0x69,0x53,0x69,0x67,0x6e,0x20,0x54,0x69,
0x6d,0x65,0x20,0x53,0x74,0x61,0x6d,0x70,0x69,0x6e,0x67,0x20,0x53,0x65,0x72,
0x76,0x69,0x63,0x65,0x73,0x20,0x53,0x69,0x67,0x6e,0x65,0x72,0x30,0x82,0x01,
0x22,0x30,0x0d,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x01,0x05,
0x00,0x03,0x82,0x01,0x0f,0x00,0x30,0x82,0x01,0x0a,0x02,0x82,0x01,0x01,0x00,
0xb2,0x50,0x28,0x48,0xdd,0xd3,0x68,0x7a,0x84,0x18,0x44,0x66,0x75,0x5d,0x7e,
0xc4,0xb8,0x9f,0x63,0x26,0xff,0x3d,0x43,0x9c,0x7c,0x11,0x38,0x10,0x25,0x55,
0x73,0xd9,0x75,0x27,0x69,0xfd,0x4e,0xb9,0x20,0x5c,0xd3,0x0a,0xf9,0xa0,0x1b,
0x2a,0xed,0x55,0x56,0x21,0x61,0xd8,0x1e,0xdb,0xe4,0xbc,0x33,0x6b,0xc7,0xef,
0xdd,0xa3,0x37,0x65,0x8e,0x1b,0x93,0x0c,0xb6,0x53,0x1e,0x5c,0x7c,0x66,0x35,
0x5f,0x05,0x8a,0x45,0xfe,0x76,0x4e,0xdf,0x53,0x80,0xa2,0x81,0x20,0x9d,0xae,
0x88,0x5c,0xa2,0x08,0xf7,0xe5,0x30,0xf9,0xee,0x22,0x37,0x4c,0x42,0x0a,0xce,
0xdf,0xc6,0x1f,0xc4,0xd6,0x55,0xe9,0x81,0x3f,0xb5,0x52,0xa3,0x2c,0xaa,0x01,
0x7a,0xf2,0xa2,0xaa,0x8d,0x35,0xfe,0x9f,0xe6,0x5d,0x6a,0x05,0x9f,0x3d,0x6b,
0xe3,0xbf,0x96,0xc0,0xfe,0xcc,0x60,0xf9,0x40,0xe7,0x07,0xa0,0x44,0xeb,0x81,
0x51,0x6e,0xa5,0x2a,0xf2,0xb6,0x8a,0x10,0x28,0xed,0x8f,0xdc,0x06,0xa0,0x86,
0x50,0x9a,0x7b,0x4a,0x08,0x0d,0x30,0x1d,0xca,0x10,0x9e,0x6b,0xf7,0xe9,0x58,
0xae,0x04,0xa9,0x40,0x99,0xb2,0x28,0xe8,0x8f,0x16,0xac,0x3c,0xe3,0x53,0x6f,
0x4b,0xd3,0x35,0x9d,0xb5,0x6f,0x64,0x1d,0xb3,0x96,0x2c,0xbb,0x3d,0xe7,0x79,
0xeb,0x6d,0x7a,0xf9,0x16,0xe6,0x26,0xad,0xaf,0xef,0x99,0x53,0xb7,0x40,0x2c,
0x95,0xb8,0x79,0xaa,0xfe,0xd4,0x52,0xab,0x29,0x74,0x7e,0x42,0xec,0x39,0x1e,
0xa2,0x6a,0x16,0xe6,0x59,0xbb,0x24,0x68,0xd8,0x00,0x80,0x43,0x10,0x87,0x80,
0x6b,0x02,0x03,0x01,0x00,0x01,0xa3,0x81,0xca,0x30,0x81,0xc7,0x30,0x34,0x06,
0x08,0x2b,0x06,0x01,0x05,0x05,0x07,0x01,0x01,0x04,0x28,0x30,0x26,0x30,0x24,
0x06,0x08,0x2b,0x06,0x01,0x05,0x05,0x07,0x30,0x01,0x86,0x18,0x68,0x74,0x74,
0x70,0x3a,0x2f,0x2f,0x6f,0x63,0x73,0x70,0x2e,0x76,0x65,0x72,0x69,0x73,0x69,
0x67,0x6e,0x2e,0x63,0x6f,0x6d,0x30,0x0c,0x06,0x03,0x55,0x1d,0x13,0x01,0x01,
0xff,0x04,0x02,0x30,0x00,0x30,0x33,0x06,0x03,0x55,0x1d,0x1f,0x04,0x2c,0x30,
0x2a,0x30,0x28,0xa0,0x26,0xa0,0x24,0x86,0x22,0x68,0x74,0x74,0x70,0x3a,0x2f,
0x2f,0x63,0x72,0x6c,0x2e,0x76,0x65,0x72,0x69,0x73,0x69,0x67,0x6e,0x2e,0x63,
0x6f,0x6d,0x2f,0x74,0x73,0x73,0x2d,0x63,0x61,0x2e,0x63,0x72,0x6c,0x30,0x16,
0x06,0x03,0x55,0x1d,0x25,0x01,0x01,0xff,0x04,0x0c,0x30,0x0a,0x06,0x08,0x2b,
0x06,0x01,0x05,0x05,0x07,0x03,0x08,0x30,0x0e,0x06,0x03,0x55,0x1d,0x0f,0x01,
0x01,0xff,0x04,0x04,0x03,0x02,0x06,0xc0,0x30,0x24,0x06,0x03,0x55,0x1d,0x11,
0x04,0x1d,0x30,0x1b,0xa4,0x19,0x30,0x17,0x31,0x15,0x30,0x13,0x06,0x03,0x55,
0x04,0x03,0x13,0x0c,0x54,0x53,0x41,0x32,0x30,0x34,0x38,0x2d,0x31,0x2d,0x35,
0x34,0x30,0x0d,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x05,0x05,
0x00,0x03,0x82,0x01,0x01,0x00,0x87,0x78,0x70,0xda,0x4e,0x52,0x01,0x20,0x5b,
0xe0,0x79,0xc9,0x82,0x30,0xc4,0xfd,0xb9,0x19,0x96,0xbd,0x91,0x00,0xc3,0xbd,
0xcd,0xcd,0xc6,0xf4,0x0e,0xd8,0xff,0xf9,0x4d,0xc0,0x33,0x62,0x30,0x11,0xc5,
0xf5,0x74,0x1b,0xd4,0x92,0xde,0x5f,0x9c,0x20,0x13,0xb1,0x7c,0x45,0xbe,0x50,
0xcd,0x83,0xe7,0x80,0x17,0x83,0xa7,0x27,0x93,0x67,0x13,0x46,0xfb,0xca,0xb8,
0x98,0x41,0x03,0xcc,0x9b,0x51,0x5b,0x05,0x8b,0x7f,0xa8,0x6f,0xf3,0x1b,0x50,
0x1b,0x24,0x2e,0xf2,0x69,0x8d,0x6c,0x22,0xf7,0xbb,0xca,0x16,0x95,0xed,0x0c,
0x74,0xc0,0x68,0x77,0xd9,0xeb,0x99,0x62,0x87,0xc1,0x73,0x90,0xf8,0x89,0x74,
0x7a,0x23,0xab,0xa3,0x98,0x7b,0x97,0xb1,0xf7,0x8f,0x29,0x71,0x4d,0x2e,0x75,
0x1b,0x48,0x41,0xda,0xf0,0xb5,0x0d,0x20,0x54,0xd6,0x77,0xa0,0x97,0x82,0x63,
0x69,0xfd,0x09,0xcf,0x8a,0xf0,0x75,0xbb,0x09,0x9b,0xd9,0xf9,0x11,0x55,0x26,
0x9a,0x61,0x32,0xbe,0x7a,0x02,0xb0,0x7b,0x86,0xbe,0xa2,0xc3,0x8b,0x22,0x2c,
0x78,0xd1,0x35,0x76,0xbc,0x92,0x73,0x5c,0xf9,0xb9,0xe6,0x4c,0x15,0x0a,0x23,
0xcc,0xe4,0xd2,0xd4,0x34,0x2e,0x49,0x40,0x15,0x3c,0x0f,0x60,0x7a,0x24,0xc6,
0xa5,0x66,0xef,0x96,0xcf,0x70,0xeb,0x3e,0xe7,0xf4,0x0d,0x7e,0xdc,0xd1,0x7c,
0xa3,0x76,0x71,0x69,0xc1,0x9c,0x4f,0x47,0x30,0x35,0x21,0xb1,0xa2,0xaf,0x1a,
0x62,0x3c,0x2b,0xd9,0x8e,0xaa,0x2a,0x07,0x7b,0xd8,0x18,0xb3,0x5c,0x7b,0xe2,
0x9d,0xa5,0x6f,0xfe,0x3c,0x89,0xad };
static const BYTE iTunesCert2[] = {
0x30,0x82,0x04,0xbf,0x30,0x82,0x04,0x28,0xa0,0x03,0x02,0x01,0x02,0x02,0x10,
0x41,0x91,0xa1,0x5a,0x39,0x78,0xdf,0xcf,0x49,0x65,0x66,0x38,0x1d,0x4c,0x75,
0xc2,0x30,0x0d,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x05,0x05,
0x00,0x30,0x5f,0x31,0x0b,0x30,0x09,0x06,0x03,0x55,0x04,0x06,0x13,0x02,0x55,
0x53,0x31,0x17,0x30,0x15,0x06,0x03,0x55,0x04,0x0a,0x13,0x0e,0x56,0x65,0x72,
0x69,0x53,0x69,0x67,0x6e,0x2c,0x20,0x49,0x6e,0x63,0x2e,0x31,0x37,0x30,0x35,
0x06,0x03,0x55,0x04,0x0b,0x13,0x2e,0x43,0x6c,0x61,0x73,0x73,0x20,0x33,0x20,
0x50,0x75,0x62,0x6c,0x69,0x63,0x20,0x50,0x72,0x69,0x6d,0x61,0x72,0x79,0x20,
0x43,0x65,0x72,0x74,0x69,0x66,0x69,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x41,
0x75,0x74,0x68,0x6f,0x72,0x69,0x74,0x79,0x30,0x1e,0x17,0x0d,0x30,0x34,0x30,
0x37,0x31,0x36,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x17,0x0d,0x31,0x34,0x30,
0x37,0x31,0x35,0x32,0x33,0x35,0x39,0x35,0x39,0x5a,0x30,0x81,0xb4,0x31,0x0b,
0x30,0x09,0x06,0x03,0x55,0x04,0x06,0x13,0x02,0x55,0x53,0x31,0x17,0x30,0x15,
0x06,0x03,0x55,0x04,0x0a,0x13,0x0e,0x56,0x65,0x72,0x69,0x53,0x69,0x67,0x6e,
0x2c,0x20,0x49,0x6e,0x63,0x2e,0x31,0x1f,0x30,0x1d,0x06,0x03,0x55,0x04,0x0b,
0x13,0x16,0x56,0x65,0x72,0x69,0x53,0x69,0x67,0x6e,0x20,0x54,0x72,0x75,0x73,
0x74,0x20,0x4e,0x65,0x74,0x77,0x6f,0x72,0x6b,0x31,0x3b,0x30,0x39,0x06,0x03,
0x55,0x04,0x0b,0x13,0x32,0x54,0x65,0x72,0x6d,0x73,0x20,0x6f,0x66,0x20,0x75,
0x73,0x65,0x20,0x61,0x74,0x20,0x68,0x74,0x74,0x70,0x73,0x3a,0x2f,0x2f,0x77,
0x77,0x77,0x2e,0x76,0x65,0x72,0x69,0x73,0x69,0x67,0x6e,0x2e,0x63,0x6f,0x6d,
0x2f,0x72,0x70,0x61,0x20,0x28,0x63,0x29,0x30,0x34,0x31,0x2e,0x30,0x2c,0x06,
0x03,0x55,0x04,0x03,0x13,0x25,0x56,0x65,0x72,0x69,0x53,0x69,0x67,0x6e,0x20,
0x43,0x6c,0x61,0x73,0x73,0x20,0x33,0x20,0x43,0x6f,0x64,0x65,0x20,0x53,0x69,
0x67,0x6e,0x69,0x6e,0x67,0x20,0x32,0x30,0x30,0x34,0x20,0x43,0x41,0x30,0x82,
0x01,0x22,0x30,0x0d,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x01,
0x05,0x00,0x03,0x82,0x01,0x0f,0x00,0x30,0x82,0x01,0x0a,0x02,0x82,0x01,0x01,
0x00,0xbe,0xbc,0xee,0xbc,0x7e,0xef,0x83,0xeb,0xe0,0x37,0x4f,0xfb,0x03,0x10,
0x38,0xbe,0x08,0xd2,0x8c,0x7d,0x9d,0xfa,0x92,0x7f,0x19,0x0c,0xc2,0x6b,0xee,
0x42,0x52,0x8c,0xde,0xd3,0x1c,0x48,0x13,0x25,0xea,0xc1,0x63,0x7a,0xf9,0x51,
0x65,0xee,0xd3,0xaa,0x3b,0xf5,0xf0,0x94,0x9c,0x2b,0xfb,0xf2,0x66,0xd4,0x24,
0xda,0xf7,0xf5,0x9f,0x6e,0x19,0x39,0x36,0xbc,0xd0,0xa3,0x76,0x08,0x1e,0x22,
0x27,0x24,0x6c,0x38,0x91,0x27,0xe2,0x84,0x49,0xae,0x1b,0x8a,0xa1,0xfd,0x25,
0x82,0x2c,0x10,0x30,0xe8,0x71,0xab,0x28,0xe8,0x77,0x4a,0x51,0xf1,0xec,0xcd,
0xf8,0xf0,0x54,0xd4,0x6f,0xc0,0xe3,0x6d,0x0a,0x8f,0xd9,0xd8,0x64,0x8d,0x63,
0xb2,0x2d,0x4e,0x27,0xf6,0x85,0x0e,0xfe,0x6d,0xe3,0x29,0x99,0xe2,0x85,0x47,
0x7c,0x2d,0x86,0x7f,0xe8,0x57,0x8f,0xad,0x67,0xc2,0x33,0x32,0x91,0x13,0x20,
0xfc,0xa9,0x23,0x14,0x9a,0x6d,0xc2,0x84,0x4b,0x76,0x68,0x04,0xd5,0x71,0x2c,
0x5d,0x21,0xfa,0x88,0x0d,0x26,0xfd,0x1f,0x2d,0x91,0x2b,0xe7,0x01,0x55,0x4d,
0xf2,0x6d,0x35,0x28,0x82,0xdf,0xd9,0x6b,0x5c,0xb6,0xd6,0xd9,0xaa,0x81,0xfd,
0x5f,0xcd,0x83,0xba,0x63,0x9d,0xd0,0x22,0xfc,0xa9,0x3b,0x42,0x69,0xb2,0x8e,
0x3a,0xb5,0xbc,0xb4,0x9e,0x0f,0x5e,0xc4,0xea,0x2c,0x82,0x8b,0x28,0xfd,0x53,
0x08,0x96,0xdd,0xb5,0x01,0x20,0xd1,0xf9,0xa5,0x18,0xe7,0xc0,0xee,0x51,0x70,
0x37,0xe1,0xb6,0x05,0x48,0x52,0x48,0x6f,0x38,0xea,0xc3,0xe8,0x6c,0x7b,0x44,
0x84,0xbb,0x02,0x03,0x01,0x00,0x01,0xa3,0x82,0x01,0xa0,0x30,0x82,0x01,0x9c,
0x30,0x12,0x06,0x03,0x55,0x1d,0x13,0x01,0x01,0xff,0x04,0x08,0x30,0x06,0x01,
0x01,0xff,0x02,0x01,0x00,0x30,0x44,0x06,0x03,0x55,0x1d,0x20,0x04,0x3d,0x30,
0x3b,0x30,0x39,0x06,0x0b,0x60,0x86,0x48,0x01,0x86,0xf8,0x45,0x01,0x07,0x17,
0x03,0x30,0x2a,0x30,0x28,0x06,0x08,0x2b,0x06,0x01,0x05,0x05,0x07,0x02,0x01,
0x16,0x1c,0x68,0x74,0x74,0x70,0x73,0x3a,0x2f,0x2f,0x77,0x77,0x77,0x2e,0x76,
0x65,0x72,0x69,0x73,0x69,0x67,0x6e,0x2e,0x63,0x6f,0x6d,0x2f,0x72,0x70,0x61,
0x30,0x31,0x06,0x03,0x55,0x1d,0x1f,0x04,0x2a,0x30,0x28,0x30,0x26,0xa0,0x24,
0xa0,0x22,0x86,0x20,0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f,0x63,0x72,0x6c,0x2e,
0x76,0x65,0x72,0x69,0x73,0x69,0x67,0x6e,0x2e,0x63,0x6f,0x6d,0x2f,0x70,0x63,
0x61,0x33,0x2e,0x63,0x72,0x6c,0x30,0x1d,0x06,0x03,0x55,0x1d,0x25,0x04,0x16,
0x30,0x14,0x06,0x08,0x2b,0x06,0x01,0x05,0x05,0x07,0x03,0x02,0x06,0x08,0x2b,
0x06,0x01,0x05,0x05,0x07,0x03,0x03,0x30,0x0e,0x06,0x03,0x55,0x1d,0x0f,0x01,
0x01,0xff,0x04,0x04,0x03,0x02,0x01,0x06,0x30,0x11,0x06,0x09,0x60,0x86,0x48,
0x01,0x86,0xf8,0x42,0x01,0x01,0x04,0x04,0x03,0x02,0x00,0x01,0x30,0x29,0x06,
0x03,0x55,0x1d,0x11,0x04,0x22,0x30,0x20,0xa4,0x1e,0x30,0x1c,0x31,0x1a,0x30,
0x18,0x06,0x03,0x55,0x04,0x03,0x13,0x11,0x43,0x6c,0x61,0x73,0x73,0x33,0x43,
0x41,0x32,0x30,0x34,0x38,0x2d,0x31,0x2d,0x34,0x33,0x30,0x1d,0x06,0x03,0x55,
0x1d,0x0e,0x04,0x16,0x04,0x14,0x08,0xf5,0x51,0xe8,0xfb,0xfe,0x3d,0x3d,0x64,
0x36,0x7c,0x68,0xcf,0x5b,0x78,0xa8,0xdf,0xb9,0xc5,0x37,0x30,0x81,0x80,0x06,
0x03,0x55,0x1d,0x23,0x04,0x79,0x30,0x77,0xa1,0x63,0xa4,0x61,0x30,0x5f,0x31,
0x0b,0x30,0x09,0x06,0x03,0x55,0x04,0x06,0x13,0x02,0x55,0x53,0x31,0x17,0x30,
0x15,0x06,0x03,0x55,0x04,0x0a,0x13,0x0e,0x56,0x65,0x72,0x69,0x53,0x69,0x67,
0x6e,0x2c,0x20,0x49,0x6e,0x63,0x2e,0x31,0x37,0x30,0x35,0x06,0x03,0x55,0x04,
0x0b,0x13,0x2e,0x43,0x6c,0x61,0x73,0x73,0x20,0x33,0x20,0x50,0x75,0x62,0x6c,
0x69,0x63,0x20,0x50,0x72,0x69,0x6d,0x61,0x72,0x79,0x20,0x43,0x65,0x72,0x74,
0x69,0x66,0x69,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x41,0x75,0x74,0x68,0x6f,
0x72,0x69,0x74,0x79,0x82,0x10,0x70,0xba,0xe4,0x1d,0x10,0xd9,0x29,0x34,0xb6,
0x38,0xca,0x7b,0x03,0xcc,0xba,0xbf,0x30,0x0d,0x06,0x09,0x2a,0x86,0x48,0x86,
0xf7,0x0d,0x01,0x01,0x05,0x05,0x00,0x03,0x81,0x81,0x00,0xae,0x3a,0x17,0xb8,
0x4a,0x7b,0x55,0xfa,0x64,0x55,0xec,0x40,0xa4,0xed,0x49,0x41,0x90,0x99,0x9c,
0x89,0xbc,0xaf,0x2e,0x1d,0xca,0x78,0x23,0xf9,0x1c,0x19,0x0f,0x7f,0xeb,0x68,
0xbc,0x32,0xd9,0x88,0x38,0xde,0xdc,0x3f,0xd3,0x89,0xb4,0x3f,0xb1,0x82,0x96,
0xf1,0xa4,0x5a,0xba,0xed,0x2e,0x26,0xd3,0xde,0x7c,0x01,0x6e,0x00,0x0a,0x00,
0xa4,0x06,0x92,0x11,0x48,0x09,0x40,0xf9,0x1c,0x18,0x79,0x67,0x23,0x24,0xe0,
0xbb,0xd5,0xe1,0x50,0xae,0x1b,0xf5,0x0e,0xdd,0xe0,0x2e,0x81,0xcd,0x80,0xa3,
0x6c,0x52,0x4f,0x91,0x75,0x55,0x8a,0xba,0x22,0xf2,0xd2,0xea,0x41,0x75,0x88,
0x2f,0x63,0x55,0x7d,0x1e,0x54,0x5a,0x95,0x59,0xca,0xd9,0x34,0x81,0xc0,0x5f,
0x5e,0xf6,0x7a,0xb5 };
static const BYTE iTunesCert3[] = {
0x30,0x82,0x04,0xf1,0x30,0x82,0x03,0xd9,0xa0,0x03,0x02,0x01,0x02,0x02,0x10,
0x0f,0x1a,0xa0,0xe0,0x9b,0x9b,0x61,0xa6,0xb6,0xfe,0x40,0xd2,0xdf,0x6a,0xf6,
0x8d,0x30,0x0d,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x05,0x05,
0x00,0x30,0x81,0xb4,0x31,0x0b,0x30,0x09,0x06,0x03,0x55,0x04,0x06,0x13,0x02,
0x55,0x53,0x31,0x17,0x30,0x15,0x06,0x03,0x55,0x04,0x0a,0x13,0x0e,0x56,0x65,
0x72,0x69,0x53,0x69,0x67,0x6e,0x2c,0x20,0x49,0x6e,0x63,0x2e,0x31,0x1f,0x30,
0x1d,0x06,0x03,0x55,0x04,0x0b,0x13,0x16,0x56,0x65,0x72,0x69,0x53,0x69,0x67,
0x6e,0x20,0x54,0x72,0x75,0x73,0x74,0x20,0x4e,0x65,0x74,0x77,0x6f,0x72,0x6b,
0x31,0x3b,0x30,0x39,0x06,0x03,0x55,0x04,0x0b,0x13,0x32,0x54,0x65,0x72,0x6d,
0x73,0x20,0x6f,0x66,0x20,0x75,0x73,0x65,0x20,0x61,0x74,0x20,0x68,0x74,0x74,
0x70,0x73,0x3a,0x2f,0x2f,0x77,0x77,0x77,0x2e,0x76,0x65,0x72,0x69,0x73,0x69,
0x67,0x6e,0x2e,0x63,0x6f,0x6d,0x2f,0x72,0x70,0x61,0x20,0x28,0x63,0x29,0x30,
0x34,0x31,0x2e,0x30,0x2c,0x06,0x03,0x55,0x04,0x03,0x13,0x25,0x56,0x65,0x72,
0x69,0x53,0x69,0x67,0x6e,0x20,0x43,0x6c,0x61,0x73,0x73,0x20,0x33,0x20,0x43,
0x6f,0x64,0x65,0x20,0x53,0x69,0x67,0x6e,0x69,0x6e,0x67,0x20,0x32,0x30,0x30,
0x34,0x20,0x43,0x41,0x30,0x1e,0x17,0x0d,0x30,0x36,0x30,0x31,0x31,0x37,0x30,
0x30,0x30,0x30,0x30,0x30,0x5a,0x17,0x0d,0x30,0x38,0x30,0x31,0x32,0x32,0x32,
0x33,0x35,0x39,0x35,0x39,0x5a,0x30,0x81,0xb4,0x31,0x0b,0x30,0x09,0x06,0x03,
0x55,0x04,0x06,0x13,0x02,0x55,0x53,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,
0x08,0x13,0x0a,0x43,0x61,0x6c,0x69,0x66,0x6f,0x72,0x6e,0x69,0x61,0x31,0x12,
0x30,0x10,0x06,0x03,0x55,0x04,0x07,0x13,0x09,0x43,0x75,0x70,0x65,0x72,0x74,
0x69,0x6e,0x6f,0x31,0x1d,0x30,0x1b,0x06,0x03,0x55,0x04,0x0a,0x14,0x14,0x41,
0x70,0x70,0x6c,0x65,0x20,0x43,0x6f,0x6d,0x70,0x75,0x74,0x65,0x72,0x2c,0x20,
0x49,0x6e,0x63,0x2e,0x31,0x3e,0x30,0x3c,0x06,0x03,0x55,0x04,0x0b,0x13,0x35,
0x44,0x69,0x67,0x69,0x74,0x61,0x6c,0x20,0x49,0x44,0x20,0x43,0x6c,0x61,0x73,
0x73,0x20,0x33,0x20,0x2d,0x20,0x4d,0x69,0x63,0x72,0x6f,0x73,0x6f,0x66,0x74,
0x20,0x53,0x6f,0x66,0x74,0x77,0x61,0x72,0x65,0x20,0x56,0x61,0x6c,0x69,0x64,
0x61,0x74,0x69,0x6f,0x6e,0x20,0x76,0x32,0x31,0x1d,0x30,0x1b,0x06,0x03,0x55,
0x04,0x03,0x14,0x14,0x41,0x70,0x70,0x6c,0x65,0x20,0x43,0x6f,0x6d,0x70,0x75,
0x74,0x65,0x72,0x2c,0x20,0x49,0x6e,0x63,0x2e,0x30,0x81,0x9f,0x30,0x0d,0x06,
0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x01,0x05,0x00,0x03,0x81,0x8d,
0x00,0x30,0x81,0x89,0x02,0x81,0x81,0x00,0xd3,0xab,0x3b,0x7f,0xec,0x48,0x84,
0xce,0xa8,0x1a,0x12,0xf3,0x3c,0x87,0xcb,0x24,0x58,0x96,0x02,0x87,0x66,0x49,
0xeb,0x89,0xee,0x79,0x44,0x70,0x8d,0xe7,0xd4,0x1f,0x30,0x92,0xc0,0x9c,0x35,
0x78,0xc0,0xaf,0x1c,0xb6,0x28,0xd3,0xe0,0xe0,0x9d,0xd3,0x49,0x76,0x73,0x57,
0x19,0x4d,0x8d,0x70,0x85,0x64,0x4d,0x1d,0xc6,0x02,0x3e,0xe5,0x2c,0x66,0x07,
0xd2,0x27,0x4b,0xd6,0xc8,0x3c,0x93,0xb6,0x15,0x0c,0xde,0x5b,0xd7,0x93,0xdd,
0xbe,0x85,0x62,0x34,0x17,0x8a,0x05,0x60,0xf0,0x8a,0x1c,0x5a,0x40,0x21,0x8d,
0x51,0x6c,0xb0,0x62,0xd8,0xb5,0xd4,0xf9,0xb1,0xd0,0x58,0x7a,0x7a,0x82,0x55,
0xb3,0xf9,0x53,0x71,0xde,0xd2,0xc9,0x37,0x8c,0xf6,0x5a,0x1f,0x2d,0xcd,0x7c,
0x67,0x02,0x03,0x01,0x00,0x01,0xa3,0x82,0x01,0x7f,0x30,0x82,0x01,0x7b,0x30,
0x09,0x06,0x03,0x55,0x1d,0x13,0x04,0x02,0x30,0x00,0x30,0x0e,0x06,0x03,0x55,
0x1d,0x0f,0x01,0x01,0xff,0x04,0x04,0x03,0x02,0x07,0x80,0x30,0x40,0x06,0x03,
0x55,0x1d,0x1f,0x04,0x39,0x30,0x37,0x30,0x35,0xa0,0x33,0xa0,0x31,0x86,0x2f,
0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f,0x43,0x53,0x43,0x33,0x2d,0x32,0x30,0x30,
0x34,0x2d,0x63,0x72,0x6c,0x2e,0x76,0x65,0x72,0x69,0x73,0x69,0x67,0x6e,0x2e,
0x63,0x6f,0x6d,0x2f,0x43,0x53,0x43,0x33,0x2d,0x32,0x30,0x30,0x34,0x2e,0x63,
0x72,0x6c,0x30,0x44,0x06,0x03,0x55,0x1d,0x20,0x04,0x3d,0x30,0x3b,0x30,0x39,
0x06,0x0b,0x60,0x86,0x48,0x01,0x86,0xf8,0x45,0x01,0x07,0x17,0x03,0x30,0x2a,
0x30,0x28,0x06,0x08,0x2b,0x06,0x01,0x05,0x05,0x07,0x02,0x01,0x16,0x1c,0x68,
0x74,0x74,0x70,0x73,0x3a,0x2f,0x2f,0x77,0x77,0x77,0x2e,0x76,0x65,0x72,0x69,
0x73,0x69,0x67,0x6e,0x2e,0x63,0x6f,0x6d,0x2f,0x72,0x70,0x61,0x30,0x13,0x06,
0x03,0x55,0x1d,0x25,0x04,0x0c,0x30,0x0a,0x06,0x08,0x2b,0x06,0x01,0x05,0x05,
0x07,0x03,0x03,0x30,0x75,0x06,0x08,0x2b,0x06,0x01,0x05,0x05,0x07,0x01,0x01,
0x04,0x69,0x30,0x67,0x30,0x24,0x06,0x08,0x2b,0x06,0x01,0x05,0x05,0x07,0x30,
0x01,0x86,0x18,0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f,0x6f,0x63,0x73,0x70,0x2e,
0x76,0x65,0x72,0x69,0x73,0x69,0x67,0x6e,0x2e,0x63,0x6f,0x6d,0x30,0x3f,0x06,
0x08,0x2b,0x06,0x01,0x05,0x05,0x07,0x30,0x02,0x86,0x33,0x68,0x74,0x74,0x70,
0x3a,0x2f,0x2f,0x43,0x53,0x43,0x33,0x2d,0x32,0x30,0x30,0x34,0x2d,0x61,0x69,
0x61,0x2e,0x76,0x65,0x72,0x69,0x73,0x69,0x67,0x6e,0x2e,0x63,0x6f,0x6d,0x2f,
0x43,0x53,0x43,0x33,0x2d,0x32,0x30,0x30,0x34,0x2d,0x61,0x69,0x61,0x2e,0x63,
0x65,0x72,0x30,0x1f,0x06,0x03,0x55,0x1d,0x23,0x04,0x18,0x30,0x16,0x80,0x14,
0x08,0xf5,0x51,0xe8,0xfb,0xfe,0x3d,0x3d,0x64,0x36,0x7c,0x68,0xcf,0x5b,0x78,
0xa8,0xdf,0xb9,0xc5,0x37,0x30,0x11,0x06,0x09,0x60,0x86,0x48,0x01,0x86,0xf8,
0x42,0x01,0x01,0x04,0x04,0x03,0x02,0x04,0x10,0x30,0x16,0x06,0x0a,0x2b,0x06,
0x01,0x04,0x01,0x82,0x37,0x02,0x01,0x1b,0x04,0x08,0x30,0x06,0x01,0x01,0x00,
0x01,0x01,0xff,0x30,0x0d,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,
0x05,0x05,0x00,0x03,0x82,0x01,0x01,0x00,0x6a,0xa6,0x06,0xd0,0x33,0x18,0x64,
0xe2,0x69,0x82,0xee,0x6e,0x36,0x9e,0x9d,0x9a,0x0e,0x18,0xa8,0xac,0x9d,0x10,
0xed,0x01,0x3c,0xb9,0x61,0x04,0x62,0xf3,0x85,0x8f,0xcc,0x4f,0x2c,0x66,0x35,
0x54,0x25,0x45,0x8d,0x95,0x1c,0xd2,0x33,0xbe,0x2e,0xdd,0x7f,0x74,0xaf,0x03,
0x7b,0x86,0x63,0xb0,0xc9,0xe6,0xbd,0xc7,0x8e,0xde,0x03,0x18,0x98,0x82,0xc3,
0xbb,0xf8,0x15,0x99,0x1a,0xa9,0xdd,0xb9,0x5d,0xb9,0xbd,0x53,0x95,0x25,0x76,
0xfb,0x5c,0x53,0x90,0xea,0x01,0x0a,0xa0,0xb1,0xbf,0x09,0x1b,0x97,0x8f,0x40,
0xfa,0x85,0x12,0x74,0x01,0xdb,0xf6,0xdb,0x09,0xd6,0x5f,0x4f,0xd7,0x17,0xb4,
0xbf,0x9e,0x2f,0x86,0x52,0x5d,0x70,0x24,0x52,0x32,0x1e,0xa5,0x1d,0x39,0x8b,
0x66,0xf6,0xba,0x9b,0x69,0x8e,0x12,0x60,0xdb,0xb6,0xcf,0xe6,0x0d,0xd6,0x1c,
0x8f,0xd4,0x5b,0x4b,0x00,0xde,0x21,0x93,0xfb,0x6e,0xc7,0x3d,0xb4,0x66,0x0d,
0x29,0x0c,0x4e,0xe9,0x3f,0x94,0xd6,0xd6,0xdc,0xec,0xf8,0x53,0x3b,0x62,0xd5,
0x97,0x50,0x53,0x84,0x17,0xfe,0xe2,0xed,0x4c,0x23,0x0a,0x49,0xce,0x5b,0xe9,
0x70,0x31,0xc1,0x04,0x02,0x02,0x6c,0xb8,0x52,0xcd,0xc7,0x4e,0x70,0xb4,0x13,
0xd7,0xe0,0x92,0xba,0x44,0x1a,0x10,0x4c,0x6e,0x45,0xc6,0x86,0x04,0xc6,0x64,
0xd3,0x9c,0x6e,0xc1,0x9c,0xac,0x74,0x3d,0x77,0x06,0x5e,0x28,0x28,0x5c,0xf5,
0xe0,0x9c,0x19,0xd8,0xba,0x74,0x81,0x2d,0x67,0x77,0x93,0x8d,0xbf,0xd2,0x52,
0x00,0xe6,0xa5,0x38,0x4e,0x2e,0x73,0x66,0x7a };
static const BYTE signedCRL[] = { 0x30, 0x45, 0x30, 0x2c, 0x30, 0x02, 0x06,
 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13,
 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x18, 0x0f,
 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x5a, 0x30, 0x02, 0x06, 0x00, 0x03, 0x11, 0x00, 0x0f, 0x0e, 0x0d, 0x0c,
 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 };

/* CBT hook to ensure a window (e.g., MessageBox) cannot be created */
static HHOOK hook;
static LRESULT CALLBACK cbt_hook_proc(int code, WPARAM wp, LPARAM lp)
{
    return code == HCBT_CREATEWND ? 1: CallNextHookEx(hook, code, wp, lp);
}

static BOOL (WINAPI *pCryptUIWizImport)(DWORD dwFlags, HWND hwndParent,
 LPCWSTR pwszWizardTitle, PCCRYPTUI_WIZ_IMPORT_SRC_INFO pImportSrc,
 HCERTSTORE hDestCertStore);

static BOOL find_and_delete_cert_in_store(HCERTSTORE store, PCCERT_CONTEXT cert)
{
    CERT_ID id;
    PCCERT_CONTEXT found;

    id.dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
    memcpy(&id.u.IssuerSerialNumber.Issuer,
     &cert->pCertInfo->Issuer, sizeof(CERT_NAME_BLOB));
    memcpy(&id.u.IssuerSerialNumber.SerialNumber,
     &cert->pCertInfo->SerialNumber, sizeof(CRYPT_INTEGER_BLOB));
    found = CertFindCertificateInStore(store, X509_ASN_ENCODING, 0,
     CERT_FIND_CERT_ID, &id, NULL);
    if (!found)
        return FALSE;

    CertDeleteCertificateFromStore(found);

    return TRUE;
}

static void test_crypt_ui_wiz_import(void)
{
    BOOL ret;
    CRYPTUI_WIZ_IMPORT_SRC_INFO info;
    HCERTSTORE store;
    PCCERT_CONTEXT cert;
    PCCRL_CONTEXT crl;
    DWORD count;

    if (!pCryptUIWizImport)
    {
        skip("No CryptUIWizImport\n");
        return;
    }

    /* Set CBT hook to disallow MessageBox and wizard creation in current
     * thread.
     */
    hook = SetWindowsHookExA(WH_CBT, cbt_hook_proc, 0, GetCurrentThreadId());

    /* Brings up UI.  Cancelling yields ret = 1. */
    if (0)
    {
        ret = pCryptUIWizImport(0, 0, NULL, NULL, NULL);
    }
    SetLastError(0xdeadbeef);
    ret = pCryptUIWizImport(CRYPTUI_WIZ_NO_UI, 0, NULL, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "expected E_INVALIDARG, got %08x\n", GetLastError());
    memset(&info, 0, sizeof(info));
    SetLastError(0xdeadbeef);
    ret = pCryptUIWizImport(CRYPTUI_WIZ_NO_UI, 0, NULL, &info, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "expected E_INVALIDARG, got %08x\n", GetLastError());
    info.dwSize = sizeof(info);
    SetLastError(0xdeadbeef);
    ret = pCryptUIWizImport(CRYPTUI_WIZ_NO_UI, 0, NULL, &info, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "expected E_INVALIDARG, got %08x\n", GetLastError());
    info.dwSubjectChoice = CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_CONTEXT;
    SetLastError(0xdeadbeef);
    ret = pCryptUIWizImport(CRYPTUI_WIZ_NO_UI, 0, NULL, &info, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "expected E_INVALIDARG, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pCryptUIWizImport(CRYPTUI_WIZ_NO_UI | CRYPTUI_WIZ_IMPORT_ALLOW_CERT,
     0, NULL, &info, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "expected E_INVALIDARG, got %08x\n", GetLastError());
    /* Check allowed vs. given type mismatches */
    info.u.pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING,
     v1CertWithValidPubKey, sizeof(v1CertWithValidPubKey));
    SetLastError(0xdeadbeef);
    ret = pCryptUIWizImport(CRYPTUI_WIZ_NO_UI | CRYPTUI_WIZ_IMPORT_ALLOW_CRL,
     0, NULL, &info, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "expected E_INVALIDARG, got %08x\n", GetLastError());
    CertFreeCertificateContext(info.u.pCertContext);
    info.dwSubjectChoice = CRYPTUI_WIZ_IMPORT_SUBJECT_CRL_CONTEXT;
    info.u.pCRLContext = CertCreateCRLContext(X509_ASN_ENCODING,
     signedCRL, sizeof(signedCRL));
    SetLastError(0xdeadbeef);
    ret = pCryptUIWizImport(CRYPTUI_WIZ_NO_UI | CRYPTUI_WIZ_IMPORT_ALLOW_CERT,
     0, NULL, &info, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "expected E_INVALIDARG, got %08x\n", GetLastError());
    CertFreeCRLContext(info.u.pCRLContext);
    /* Imports the following cert--self-signed, with no basic constraints set--
     * to the CA store.  Puts up a dialog at the end if it succeeds or fails.
     */
    info.dwSubjectChoice = CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_CONTEXT;
    info.u.pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING,
     v1CertWithValidPubKey, sizeof(v1CertWithValidPubKey));
    ret = pCryptUIWizImport(CRYPTUI_WIZ_NO_UI, 0, NULL, &info, NULL);
    ok(ret, "CryptUIWizImport failed: %08x\n", GetLastError());
    if (ret)
    {
        static const WCHAR CA[] = { 'C','A',0 };
        HCERTSTORE ca = CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0,
         CERT_SYSTEM_STORE_CURRENT_USER, CA);

        if (ca)
        {
            ret = find_and_delete_cert_in_store(ca, info.u.pCertContext);
            ok(ret ||
             broken(!ret) /* Win9x/NT4 */,
             "expected to find v1CertWithValidPubKey in CA store\n");
            CertCloseStore(ca, 0);
        }
    }
    CertFreeCertificateContext(info.u.pCertContext);
    /* Imports the following cert--not self-signed, with a basic constraints2
     * extensions--to the "AddressBook" store.  Puts up a dialog at the end if
     * it succeeds or fails.
     */
    info.u.pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING,
     iTunesCert3, sizeof(iTunesCert3));
    ret = pCryptUIWizImport(CRYPTUI_WIZ_NO_UI, 0, NULL, &info, NULL);
    ok(ret, "CryptUIWizImport failed: %08x\n", GetLastError());
    if (ret)
    {
        static const WCHAR AddressBook[] = { 'A','d','d','r','e','s','s',
         'B','o','o','k',0 };
        HCERTSTORE addressBook = CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0,
         CERT_SYSTEM_STORE_CURRENT_USER, AddressBook);

        if (addressBook)
        {
            ret = find_and_delete_cert_in_store(addressBook,
             info.u.pCertContext);
            ok(ret ||
             broken(!ret),  /* Windows 2000 and earlier */
             "expected to find iTunesCert3 in AddressBook store\n");
            CertCloseStore(addressBook, 0);
        }
    }
    /* Displays the wizard, but disables the "Certificate store" edit and
     * the Browse button.  Confusingly, the "Place all certificates in the
     * following store" radio button is not disabled.
     */
    if (0)
    {
        ret = pCryptUIWizImport(CRYPTUI_WIZ_IMPORT_NO_CHANGE_DEST_STORE, 0,
         NULL, &info, NULL);
        ok(ret, "CryptUIWizImport failed: %08x\n", GetLastError());
    }
    store = CertOpenStore(CERT_STORE_PROV_MEMORY, X509_ASN_ENCODING, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    /* Displays the wizard, but sets the "Certificate store" edit to the
     * string "Determined by the program", and disables it and the Browse
     * button, as well as the "Automatically select the certificate store
     * based on the type of certificate" radio button.
     */
    if (0)
    {
        ret = pCryptUIWizImport(CRYPTUI_WIZ_IMPORT_NO_CHANGE_DEST_STORE, 0,
         NULL, &info, store);
        ok(ret, "CryptUIWizImport failed: %08x\n", GetLastError());
    }
    ret = pCryptUIWizImport(CRYPTUI_WIZ_NO_UI |
     CRYPTUI_WIZ_IMPORT_NO_CHANGE_DEST_STORE, 0, NULL, &info, store);
    ok(ret, "CryptUIWizImport failed: %08x\n", GetLastError());
    ret = find_and_delete_cert_in_store(store, info.u.pCertContext);
    ok(ret ||
     broken(!ret) /* Win9x/NT4 */,
     "expected to find iTunesCert3 in memory store\n");
    CertFreeCertificateContext(info.u.pCertContext);
    CertCloseStore(store, 0);

    info.u.pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING,
     iTunesCert1, sizeof(iTunesCert1));
    ret = pCryptUIWizImport(CRYPTUI_WIZ_NO_UI, 0, NULL, &info, NULL);
    ok(ret, "CryptUIWizImport failed: %08x\n", GetLastError());
    if (ret)
    {
        static const WCHAR AddressBook[] = { 'A','d','d','r','e','s','s',
         'B','o','o','k',0 };
        HCERTSTORE addressBook = CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0,
         CERT_SYSTEM_STORE_CURRENT_USER, AddressBook);

        if (addressBook)
        {
            ret = find_and_delete_cert_in_store(addressBook,
             info.u.pCertContext);
            ok(ret ||
             broken(!ret),  /* Windows 2000 and earlier */
             "expected to find iTunesCert1 in AddressBook store\n");
            CertCloseStore(addressBook, 0);
        }
    }
    CertFreeCertificateContext(info.u.pCertContext);

    info.u.pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING,
     iTunesCert2, sizeof(iTunesCert2));
    ret = pCryptUIWizImport(CRYPTUI_WIZ_NO_UI, 0, NULL, &info, NULL);
    ok(ret, "CryptUIWizImport failed: %08x\n", GetLastError());
    if (ret)
    {
        static const WCHAR CA[] = { 'C','A',0 };
        HCERTSTORE ca = CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0,
         CERT_SYSTEM_STORE_CURRENT_USER, CA);

        if (ca)
        {
            ret = find_and_delete_cert_in_store(ca, info.u.pCertContext);
            ok(ret ||
             broken(!ret) /* Win9x/NT4 */,
             "expected to find iTunesCert2 in CA store\n");
            CertCloseStore(ca, 0);
        }
    }
    CertFreeCertificateContext(info.u.pCertContext);

    info.u.hCertStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    CertAddEncodedCertificateToStore(info.u.hCertStore, X509_ASN_ENCODING,
     v1CertWithValidPubKey, sizeof(v1CertWithValidPubKey),
     CERT_STORE_ADD_ALWAYS, NULL);
    CertAddEncodedCRLToStore(info.u.hCertStore, X509_ASN_ENCODING, signedCRL,
     sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, NULL);
    info.dwSubjectChoice = CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_STORE;
    /* The ALLOW flags aren't allowed with a store as the source if the source
     * contains types other than those allowed.
     */
    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    SetLastError(0xdeadbeef);
    ret = CryptUIWizImport(CRYPTUI_WIZ_NO_UI | CRYPTUI_WIZ_IMPORT_ALLOW_CERT,
     0, NULL, &info, store);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "expected E_INVALIDARG, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = CryptUIWizImport(CRYPTUI_WIZ_NO_UI | CRYPTUI_WIZ_IMPORT_ALLOW_CRL,
     0, NULL, &info, store);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "expected E_INVALIDARG, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = CryptUIWizImport(CRYPTUI_WIZ_NO_UI |
     CRYPTUI_WIZ_IMPORT_NO_CHANGE_DEST_STORE |
     CRYPTUI_WIZ_IMPORT_ALLOW_CERT | CRYPTUI_WIZ_IMPORT_ALLOW_CRL, 0, NULL,
     &info, store);
    ok(ret, "CryptUIWizImport failed: %08x\n", GetLastError());
    if (ret)
    {
        count = 0;
        cert = NULL;
        do {
            cert = CertEnumCertificatesInStore(store, cert);
            if (cert)
                count++;
        } while (cert);
        ok(count == 1, "expected 1 cert, got %d\n", count);
        count = 0;
        crl = NULL;
        do {
            crl = CertEnumCRLsInStore(store, crl);
            if (crl)
                count++;
        } while (crl);
        ok(count == 1, "expected 1 CRL, got %d\n", count);
    }
    CertCloseStore(store, 0);
    CertCloseStore(info.u.hCertStore, 0);

    /* If the ALLOW flags match the content of the store, the store can be
     * imported.
     */
    info.u.hCertStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    CertAddEncodedCertificateToStore(info.u.hCertStore, X509_ASN_ENCODING,
     v1CertWithValidPubKey, sizeof(v1CertWithValidPubKey),
     CERT_STORE_ADD_ALWAYS, NULL);
    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    SetLastError(0xdeadbeef);
    ret = CryptUIWizImport(CRYPTUI_WIZ_NO_UI | CRYPTUI_WIZ_IMPORT_ALLOW_CERT,
     0, NULL, &info, store);
    ok(ret, "CryptUIWizImport failed: %08x\n", GetLastError());
    if (ret)
    {
        count = 0;
        cert = NULL;
        do {
            cert = CertEnumCertificatesInStore(store, cert);
            if (cert)
                count++;
        } while (cert);
        ok(count == 1, "expected 1 cert, got %d\n", count);
        count = 0;
        crl = NULL;
        do {
            crl = CertEnumCRLsInStore(store, crl);
            if (crl)
                count++;
        } while (crl);
        ok(count == 0, "expected 0 CRLs, got %d\n", count);
    }
    SetLastError(0xdeadbeef);
    ret = CryptUIWizImport(CRYPTUI_WIZ_NO_UI | CRYPTUI_WIZ_IMPORT_ALLOW_CRL,
     0, NULL, &info, store);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "expected E_INVALIDARG, got %08x\n", GetLastError());
    CertCloseStore(store, 0);
    CertCloseStore(info.u.hCertStore, 0);

    /* Again, if the ALLOW flags match the content of the store, the store can
     * be imported.
     */
    info.u.hCertStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    CertAddEncodedCRLToStore(info.u.hCertStore, X509_ASN_ENCODING, signedCRL,
     sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, NULL);
    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    SetLastError(0xdeadbeef);
    ret = CryptUIWizImport(CRYPTUI_WIZ_NO_UI | CRYPTUI_WIZ_IMPORT_ALLOW_CRL,
     0, NULL, &info, store);
    ok(ret, "CryptUIWizImport failed: %08x\n", GetLastError());
    if (ret)
    {
        count = 0;
        cert = NULL;
        do {
            cert = CertEnumCertificatesInStore(store, cert);
            if (cert)
                count++;
        } while (cert);
        ok(count == 0, "expected 0 certs, got %d\n", count);
        count = 0;
        crl = NULL;
        do {
            crl = CertEnumCRLsInStore(store, crl);
            if (crl)
                count++;
        } while (crl);
        ok(count == 1, "expected 1 CRL, got %d\n", count);
    }
    SetLastError(0xdeadbeef);
    ret = CryptUIWizImport(CRYPTUI_WIZ_NO_UI | CRYPTUI_WIZ_IMPORT_ALLOW_CERT,
     0, NULL, &info, store);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "expected E_INVALIDARG, got %08x\n", GetLastError());
    CertCloseStore(store, 0);
    CertCloseStore(info.u.hCertStore, 0);

    UnhookWindowsHookEx(hook);
}

START_TEST(cryptui)
{
    HMODULE lib = LoadLibraryA("cryptui");

    if (lib)
    {
        pCryptUIWizImport = (void *)GetProcAddress(lib, "CryptUIWizImport");

        test_crypt_ui_wiz_import();
        FreeLibrary(lib);
    }
}
