/*
 * DVOpus
 * Prototype for sending Opus voice frames over an amateur radio
 * KISS TNC
 *
 * $Id: 
 * $URL: 
 */

#include <stdio.h>
#include <string.h>
#include <pulse/simple.h>
#include <opus/opus.h>
#include <zlib.h>       // used for crc32 function
#include <netinet/in.h> // htons, we send in "network" order

#include "kiss_chars.h" // Byte defs common to tx and rx

const sample_size = 640;        // 8000 sample/sec * 16 bits (2 bytes) * 40ms (0.04)

pa_simple *aout;                // PulseAudio input handle
pa_sample_spec aout_spec;       // PulseAudio sample specification
opus_int16 sample_buf[1024];    // PulseAudio sample buffer
OpusEncoder *od;                // Opus decoder state
int oerr;                       // Error code from the decoder
opus_int32 opus_len;            // Return length from the decoder
uint8_t in_buf[256];            // Radio frame buffer
uint8_t inchar;                 // Single char buffer from radio stream
uint32_t csum;                  // Checksum calculated locally
uint32_t csum_in;               // Checksum received in input frame
uint16_t i;                     // generic iterator
uint16_t frame_seq;             // Frame sequence number
uint16_t frame_seq_in;          // Frame sequence recieved

const char magic[] = "DVOp";

int infd;                       // output file descriptor

const char kiss_port = 0x00;    // If using a multiport TNC (eg: KPC9612)
                                // this will probably need to be 0x10

void main(void) {
    infd = 0;  // This will be a tty fd some day

    // Set up our audio input
    aout_spec.format = PA_SAMPLE_S16NE;
    aout_spec.channels = 1;
    aout_spec.rate = 8000;

    aout = pa_simple_new(NULL, // Use the default server.
                         "DVOpus", // Our application's name.
                         PA_STREAM_PLAYBACK,
                         NULL, // Use the default device.
                         "Amateur Digital Voice prototype", // Description of our stream.
                         &aout_spec, // Our sample format.
                         NULL, // Use default channel map
                         NULL, // Use default buffering attributes.
                         NULL  // Ignore error code.
                        );

    // Set up the Opus encoder
    od = opus_encoder_create(8000,1,OPUS_APPLICATION_VOIP,&oerr);

    // Set initial frame sequence counter
    frame_seq = 1;

    // Clear frame length counter
    i = 0;

    // For debugging purposes, blank out the input buffer
    memset(in_buf,0x00,sizeof(in_buf));

    while(1) {
        if (i>sizeof(in_buf)) {
            fprintf(stderr,"Buffer limit exceeded. Bailing.\n");
            break;
        };

        // Grab a byte
        if (read(infd,&inchar,1) != 1) {
            fprintf(stderr,"DEBUG: Couldn't read character. Bailing.\n");
            break;
        };
        
        if (inchar == FEND) {
            fprintf(stderr,"DEBUG: Got FEND\n");

            // A minimal frame should be magic, sequence and checksum, so 10 bytes
            // because KISS sends FEND at start and end, we should trip this every frame
            if (i<10) {
                fprintf(stderr,"DEBUG: Frame too small, ignoring.\n");
                i=0;
                continue;
            };

            // NOTE: All buffer positions will be offset by 1 because of the KISS port ID

            // Check for magic
            if (strncmp(in_buf+1,magic,4) != 0) {
                fprintf(stderr,"DEBUG: Invalid magic. Not one of ours or frame corrupt.\n");
                i=0;
                continue;
            };

            // extract received frame sequence
            memcpy(&frame_seq_in,in_buf+5,2);

            // extract received checksum
            memcpy(&csum_in,in_buf+(i-4),4);

            // Compute local checksum
            csum = crc32(0,in_buf+5,i-9);

            fprintf(stderr,"DEBUG: seq_mine=%5d, seq_theirs=%5d\n", frame_seq, frame_seq_in);
            fprintf(stderr,"DEBUG: crc_mine=%08x, crc_theirs=%08x, ",csum,csum_in);

            if (csum == csum_in) {
                fprintf(stderr,"valid\n");
                // TODO: Decode opus and play sample
            } else {
                fprintf(stderr,"INVALID\n");
            };

            i=0;
            frame_seq++;
            continue;
        }; // end frame decode handler

        if (inchar == FESC) {
            fprintf(stderr,"DEBUG: Handling FESC\n");

            // Read next char and process transposition
            read(infd,&inchar,1);

            if (inchar == TFESC) {
                fprintf(stderr,"DEBUG: TFESC->FESC\n");
                in_buf[i]=FESC;
            } else if (inchar == TFEND) {
                fprintf(stderr,"DEBUG: TFEND->FEND\n");
                in_buf[i]=FEND;
            } else {
                fprintf(stderr,"DEBUG: Invalid transpose. Appending char and hoping for the best.\n");
                in_buf[i]=inchar;
            };

            i++;
            continue;
        }; // end escaped character handler

        in_buf[i]=inchar;
        i++;
    }; // end character reader loop

}; // end main

