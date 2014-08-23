/*
 * DVOpus
 * Prototype for sending Opus voice frames over an amateur radio
 * KISS TNC
 *
 * $Id$
 * $URL$
 */

#include <stdio.h>
#include <string.h>
#include <pulse/simple.h>
#include <opus/opus.h>
#include <zlib.h>       // used for crc32 function
#include <netinet/in.h> // htons, we send in "network" order

#include "kiss_chars.h" // Byte defs common to tx and rx

const sample_size = 640;        // 8000 sample/sec * 16 bits (2 bytes) * 40ms (0.04)

pa_simple *ain;                 // PulseAudio input handle
pa_sample_spec ain_spec;        // PulseAudio sample specification
opus_int16 sample_buf[1024];    // PulseAudio sample buffer
OpusEncoder *oe;                // Opus encoder state
int oerr;                       // Error code from the encoder
opus_int32 opus_len;            // Return length from the encoder
uint8_t out_buf[256];           // Radio frame buffer
uint32_t csum;                  // Checksum of output frame
uint16_t i;                     // generic iterator
uint16_t frame_seq;             // Frame sequence number

const char magic[] = "DVOp";

int outfd;                      // output file descriptor

const char kiss_port = 0x00;    // If using a multiport TNC (eg: KPC9612)
                                // this will probably need to be 0x10

void main(void) {
    outfd = 1;  // This will be a tty fd some day

    // Set up our audio input
    ain_spec.format = PA_SAMPLE_S16NE;
    ain_spec.channels = 1;
    ain_spec.rate = 8000;

    ain = pa_simple_new(NULL, // Use the default server.
                        "DVOpus", // Our application's name.
                        PA_STREAM_RECORD,
                        NULL, // Use the default device.
                        "Amateur Digital Voice prototype", // Description of our stream.
                        &ain_spec, // Our sample format.
                        NULL, // Use default channel map
                        NULL, // Use default buffering attributes.
                        NULL  // Ignore error code.
                       );

    // Set up the Opus encoder
    oe = opus_encoder_create(8000,1,OPUS_APPLICATION_VOIP,&oerr);
    opus_encoder_ctl(oe,OPUS_SET_BITRATE(7200));

    for(frame_seq=1;;frame_seq++) {
        // For debugging purposes, blank out the output buffer
        memset(out_buf,0x00,sizeof(out_buf));

        if ( pa_simple_read(ain,sample_buf,sample_size,NULL) < 0 ) {
            break;
        } else {
            // Load output payload at offset to allow for sequence number
            opus_len = opus_encode(oe,sample_buf,sample_size/2,out_buf+2,240);
            if (opus_len < 0) {
                break;
            } else {
                // Load frame sequence to the beginning of our buffer
                // so that it can be checksummed with the payload
                memcpy(out_buf,&frame_seq,2);

                csum = crc32(0,out_buf,opus_len+2);
                // Load checksum at end of buffer
                memcpy(out_buf+(opus_len+2),&csum,4);

                fprintf(stderr,"DEBUG: seq=%5d, opus_len=%d, crc=%08x\n",frame_seq,opus_len,csum);

                // Write header
                write(outfd,&FEND,1);
                write(outfd,&kiss_port,1);
                write(outfd,&magic,4);

                // Payload is 2 byte seq, Opus frame (opus_len), and 4 byte checksum
                for(i=0;i<opus_len+6;i++) {
                    if (out_buf[i] == FEND) {
                        fprintf(stderr,"DEBUG: escaping FEND\n");
                        write(outfd,&FESC,1);
                        write(outfd,&TFEND,1);
                    } else if (out_buf[i] == FESC) {
                        fprintf(stderr,"DEBUG: escaping FESC\n");
                        write(outfd,&FESC,1);
                        write(outfd,&TFESC,1);
                    } else {
                        write(outfd,out_buf+i,1);
                    };
                }; // end KISS escaper write loop

                // Close out the frame
                write(outfd,&FEND,1);
            }; // end encoder block
        }; // end sampler block
    }; // end read/write loop
}; // end main

