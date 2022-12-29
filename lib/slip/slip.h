/**
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <robin.lilja@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return - Robin Lilja
 *
 * SLIP - Serial Line Internet Protocol 
 *
 * @see RFC 1055 
 *
 * @file slip.h
 * @author Robin Lilja
 * @date 29 Sep 2015
 */

#ifndef _SLIP_H_
#define _SLIP_H_

#include <cstdint>

/**
 * SLIP special characters.
 */
typedef enum  {

	SLIP_END 	= 192,	///< 0xC0 Frame End
	SLIP_ESC 	= 219,	///< 0xDB Frame Escape
	SLIP_ESC_END 	= 220,	///< 0xDC Transposed Frame End
	SLIP_ESC_ESC 	= 221	///< 0xDD Transposed Frame Escape
} slip_char_t;

//#define SLIP_END	192	///< 0xC0 Frame End
//#define SLIP_ESC 	219	///< 0xDB Frame Escape
//#define SLIP_ESC_END	220 	///< 0xDC Transposed Frame End
//#define SLIP_ESC_ESC	221	///< 0xDD Transposed Frame Escape

/**
 * Serial Line Internet Protocol decoding/encoding class.
 */
class SLIP {

public:

	/**
	 * Encode given packet according to SLIP. 
	 * @param frm frame byte buffer (destination).
	 * @param pkg packet byte buffer (source).
	 * @param pkglen packet length.
	 * @return frame length.
	 */
	static uint16_t encode(unsigned char * frm, unsigned char * pkg, uint16_t pkglen);
	
	/**
	 * Decode given frame according to SLIP. 
	 * @param pkg packet byte buffer (destination).
	 * @param frm frame byte buffer (source).
	 * @param frmlen frame length.
	 * @return packet length.
	 */
	static uint16_t decode(unsigned char * pkg, unsigned char * frm, uint16_t frmlen);
	
	/**
	 * Get frame from FIFO buffer. 
	 * @param frm frame byte buffer (destination).
	 * @param fifo fifo byte buffer (source).
	 * @param fifonum number of bytes available in fifo.
	 * @return frame length, zero if no frame found.
	 */
	static uint16_t getFrame(unsigned char * frm, unsigned char * fifo, uint16_t fifonum);
	
	/**
	 * Get necessary frame (destination) buffer length.
	 * @param pgk packet byte buffer (source).
	 * @param pkglen packet length.
	 * @return frame length.
	 */
	static uint16_t getFrameLength(unsigned char * pgk, uint16_t pkglen);
};

#endif /* _SLIP_H_ */
