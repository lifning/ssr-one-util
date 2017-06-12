/* PRS compression and decompressions routines by fuzziqer software
 * found at: http://fuzziqer.10.forumer.com/viewtopic.php?t=110
 * many thanks to him!  =)
 * source code cleaned up a bit by lifning
 */

#ifndef __PRS_H__
#define __PRS_H__

typedef unsigned long u32;
typedef unsigned char u8;
typedef u32 DWORD;

#ifndef NULL
#define NULL 0
#endif

#define bool int
#define true 1
#define false 0

typedef struct {
    u8 bitpos;
    /*u8 controlbyte;*/
    u8* controlbyteptr;
    u8* srcptr_orig;
    u8* dstptr_orig;
    u8* srcptr;
    u8* dstptr;
} PRS_COMPRESSOR;

/* functions used internally */
void prs_put_control_bit(PRS_COMPRESSOR* pc,u8 bit);
void prs_put_control_bit_nosave(PRS_COMPRESSOR* pc,u8 bit);
void prs_put_control_save(PRS_COMPRESSOR* pc);
void prs_put_static_data(PRS_COMPRESSOR* pc,u8 data);
u8 prs_get_static_data(PRS_COMPRESSOR* pc);
void prs_init(PRS_COMPRESSOR* pc,void* src,void* dst);
void prs_finish(PRS_COMPRESSOR* pc);
void prs_rawbyte(PRS_COMPRESSOR* pc);
void prs_shortcopy(PRS_COMPRESSOR* pc,int offset,u8 size);
void prs_longcopy(PRS_COMPRESSOR* pc,int offset,u8 size);
void prs_copy(PRS_COMPRESSOR* pc,int offset,u8 size);

/* main functions */
u32 prs_compress(void* source,void* dest,u32 size);
u32 prs_decompress(void* source,void* dest);
u32 prs_decompress_size(void* source);

#endif /* __PRS_H__ */
