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
 * @file slip.cpp
 * @author Robin Lilja
 * @date 29 Sep 2015
 */

#include "slip.h"

#include <string.h>		// Using memcpy()
#include <stdint.h>

uint16_t SLIP::encode(unsigned char * frm, unsigned char * pkg, uint16_t pkglen) {

	uint16_t j = 0;

	frm[j++] = SLIP_END;	// Precede with END character

	// Iterate bytes of packet buffer
	for (uint16_t i = 0; i < pkglen; i++) {
	
		// Copy byte from source to destination
		frm[j] = pkg[i];			
	
		// Got hit on END character
		if (pkg[i] == SLIP_END) {
			
			frm[j] = SLIP_ESC;
			frm[++j] = SLIP_ESC_END;
		} 
		// Got hit on ESC character
		else if (pkg[i] == SLIP_ESC) {
		
			frm[++j] = SLIP_ESC_ESC;
		}
		
		j++; 
	}
	
	frm[j++] = SLIP_END;	// Finally add trailing END character
	
	return j;
}

uint16_t SLIP::decode(unsigned char * pkg, unsigned char * frm, uint16_t frmlen) {

	uint16_t j = 0;

	// Iterate bytes of frame buffer
	for (uint16_t i = 1; i < frmlen; i++) {
	
		// Hit on ordinary non-special byte, this is the most likely case, thus we check for that first
		if ( (frm[i-1] != SLIP_END) && (frm[i-1] != SLIP_ESC) ) {  
			
			// Copy byte from frame to packet
			pkg[j++] = frm[i-1];
		}
		// Hit on ESC and ECS_END i.e. the sequence for END
		else if ( (frm[i-1] == SLIP_ESC) && (frm[i] == SLIP_ESC_END) ) {
	
			pkg[j++] = SLIP_END;
			i++;					// Skip ahead
		}
		// Hit on ESC and ECS_ESC i.e. the sequence for ESC
		else if ( (frm[i-1] == SLIP_ESC) && (frm[i] == SLIP_ESC_ESC) ) {
	
			pkg[j++] = SLIP_ESC;
			i++;					// Skip ahead
		}
	
		// Hits on END is simply ignored
	}
	
	// This shall always be asserted as false
	// TODO: Handle this in a better way..
	if ( (frm[frmlen-1] != SLIP_END) && (frm[frmlen-1] != SLIP_ESC) ) { pkg[j++] = frm[frmlen-1]; }

	return j;
}

uint16_t SLIP::getFrame(unsigned char * frm, unsigned char * fifo, uint16_t fifonum) {

	// Frame search scenarios (perhaps not all..): 

	// Correct frame
	//
	//		|<------memcpy------>|
	//		|    |--search------>|
	//		[END][][][][][][][END][END][][]...
	
	// Incomplete frame (e.g. entire frame not yet received), nothing will be copied
	//		
	//		    |--search--------->
	//		[END][][][][][][][][][]
	
	// Relic END causes false frame, frame of length one will be returned (higher layer needs to detect and reject)
	//
	//		|<-----memcpy----->|
	//		    |--search----->|
	//		[END]          [END][][][][][][][][][]
	
	// Erroneous frame missing starting END e.g. caused by above example (higher layer needs to reject if frame is faulty)
	//
	//		|<------memcpy------->|
	//		| |-----search------->|
	//		[][][][][][][][][][END][END][][][]...
	
	// Spurious bytes in-between frames will be interpreted as a frame (higher layer needs to detect and reject)
	//
	//		|<---------memcpy------->|
	//		|    |-----search------->|
	//		[END]...[spurious]...[END][END][][]...
		
	uint16_t frmlen = 0;
	
	// Iterate FIFO byte buffer for trainling END
	for ( uint16_t i = 1; i < fifonum; i++ ) { if ( fifo[i] == SLIP_END ) { frmlen = i+1; break;} }
	
	// Copy frame from FIFO to given frame buffer
	if ( frmlen > 0 ) { memcpy(frm, fifo, frmlen); }
	
	return frmlen;
}

uint16_t SLIP::getFrameLength(unsigned char * pgk, uint16_t pkglen) {

	uint16_t count = 0;
	
	// Count the number of special characters in the package
	for (uint16_t i = 0; i < pkglen; i++) { if ( (pgk[i] == SLIP_END) || (pgk[i] == SLIP_ESC)) { count++; } }
	
	return pkglen+count+2;		// Add two for preceding and trailing END
}
