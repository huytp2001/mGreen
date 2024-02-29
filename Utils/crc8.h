/*
 * crc8.h
 *
 *  Created on: Apr 7, 2020
 *      Author: ADMIN
 */

#ifndef CRC8_H_
#define CRC8_H_

unsigned crc8x_simple(unsigned crc, void const *mem, size_t len);
unsigned crc8x_fast(unsigned crc, void const *mem, size_t len);

#endif /* CRC8_H_ */
