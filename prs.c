/* PRS compression and decompressions routines by fuzziqer software
 * found at: http://fuzziqer.10.forumer.com/viewtopic.php?t=110
 * many thanks to him!  =)
 * source code cleaned up a bit by lifning
 */

#include <string.h>
#include "prs.h"

void prs_put_control_bit(PRS_COMPRESSOR* pc,u8 bit)
{
    *pc->controlbyteptr = *pc->controlbyteptr >> 1;
    *pc->controlbyteptr |= ((!!bit) << 7);
    pc->bitpos++;
    if (pc->bitpos >= 8)
    {
        pc->bitpos = 0;
        pc->controlbyteptr = pc->dstptr;
        pc->dstptr++;
    }
}

void prs_put_control_bit_nosave(PRS_COMPRESSOR* pc,u8 bit)
{
    *pc->controlbyteptr = *pc->controlbyteptr >> 1;
    *pc->controlbyteptr |= ((!!bit) << 7);
    pc->bitpos++;
}

void prs_put_control_save(PRS_COMPRESSOR* pc)
{
    if (pc->bitpos >= 8)
    {
        pc->bitpos = 0;
        pc->controlbyteptr = pc->dstptr;
        pc->dstptr++;
    }
}

void prs_put_static_data(PRS_COMPRESSOR* pc,u8 data)
{
    *pc->dstptr = data;
    pc->dstptr++;
}

u8 prs_get_static_data(PRS_COMPRESSOR* pc)
{
    u8 data = *pc->srcptr;
    pc->srcptr++;
    return data;
}

/*////////////////////////////////////////////////////////////////////////////*/

void prs_init(PRS_COMPRESSOR* pc,void* src,void* dst)
{
    pc->bitpos = 0;
    /*pc->controlbyte = 0;*/
    pc->srcptr = (u8*)src;
    pc->srcptr_orig = (u8*)src;
    pc->dstptr = (u8*)dst;
    pc->dstptr_orig = (u8*)dst;
    pc->controlbyteptr = pc->dstptr;
    pc->dstptr++;
}

void prs_finish(PRS_COMPRESSOR* pc)
{
    prs_put_control_bit(pc,0);
    prs_put_control_bit(pc,1);
    if (pc->bitpos != 0)
    {
        *pc->controlbyteptr = ((*pc->controlbyteptr << pc->bitpos) >> 8);
    }
    prs_put_static_data(pc,0);
    prs_put_static_data(pc,0);
}

void prs_rawbyte(PRS_COMPRESSOR* pc)
{
    prs_put_control_bit_nosave(pc,1);
    prs_put_static_data(pc,prs_get_static_data(pc));
    prs_put_control_save(pc);
}

void prs_shortcopy(PRS_COMPRESSOR* pc,int offset,u8 size)
{
    size -= 2;
    prs_put_control_bit(pc,0);
    prs_put_control_bit(pc,0);
    prs_put_control_bit(pc,(size >> 1) & 1);
    prs_put_control_bit_nosave(pc,size & 1);
    prs_put_static_data(pc,offset & 0xFF);
    prs_put_control_save(pc);
}

void prs_longcopy(PRS_COMPRESSOR* pc,int offset,u8 size)
{
    if (size <= 9)
    {
        prs_put_control_bit(pc,0);
        prs_put_control_bit_nosave(pc,1);
        prs_put_static_data(pc,((offset << 3) & 0xF8) | ((size - 2) & 0x07));
        prs_put_static_data(pc,(offset >> 5) & 0xFF);
        prs_put_control_save(pc);
    } else {
        prs_put_control_bit(pc,0);
        prs_put_control_bit_nosave(pc,1);
        prs_put_static_data(pc,(offset << 3) & 0xF8);
        prs_put_static_data(pc,(offset >> 5) & 0xFF);
        prs_put_static_data(pc,size - 1);
        prs_put_control_save(pc);
    }
}

void prs_copy(PRS_COMPRESSOR* pc,int offset,u8 size)
{
    if ((offset > -0x100) && (size <= 5))
    {
        prs_shortcopy(pc,offset,size);
    } else {
        prs_longcopy(pc,offset,size);
    }
    pc->srcptr += size;
}

/*////////////////////////////////////////////////////////////////////////////*/

u32 prs_compress(void* source,void* dest,u32 size)
{
    PRS_COMPRESSOR pc;
    int x,y;
    u32 xsize;
    int lsoffset,lssize;
    prs_init(&pc,source,dest);
    for (x = 0; x < size; x++)
    {
        lsoffset = lssize = xsize = 0;
        for (y = x - 3; (y > 0) && (y > (x - 0x1FF0)) && (xsize < 255); y--)
        {
            xsize = 3;
            if (!memcmp((void*)((DWORD)source + y),(void*)((DWORD)source + x),xsize))
            {
                do xsize++;
                while (!memcmp((void*)((DWORD)source + y),
                               (void*)((DWORD)source + x),
                               xsize) &&
                       (xsize < 256) &&
                       ((y + xsize) < x) &&
                       ((x + xsize) <= size)
                );
                xsize--;
                if (xsize > lssize)
                {
                    lsoffset = -(x - y);
                    lssize = xsize;
                }
            }
        }
        if (lssize == 0)
        {
            prs_rawbyte(&pc);
        } else {
            prs_copy(&pc,lsoffset,lssize);
            x += (lssize - 1);
        }
    }
    prs_finish(&pc);
    return pc.dstptr - pc.dstptr_orig;
}

/*////////////////////////////////////////////////////////////////////////////*/

u32 prs_decompress(void* source,void* dest)
{
    u32 r3,r5; /* 6 unnamed registers */ 
    u32 bitpos = 9; /* 4 named registers */
    u8* sourceptr = (u8*)source;
    u8* destptr = (u8*)dest;
    u8* destptr_orig = (u8*)dest;
    u8 currentbyte;
    bool flag;
    int offset;
    u32 x,t; /* 2 placed variables */

    currentbyte = sourceptr[0];
    sourceptr++;
    for (;;)
    {
        bitpos--;
        if (bitpos == 0)
        {
            currentbyte = sourceptr[0];
            bitpos = 8;
            sourceptr++;
        }
        flag = currentbyte & 1;
        currentbyte = currentbyte >> 1;
        if (flag)
        {
            destptr[0] = sourceptr[0];
            sourceptr++;
            destptr++;
            continue;
        }
        bitpos--;
        if (bitpos == 0)
        {
            currentbyte = sourceptr[0];
            bitpos = 8;
            sourceptr++;
        }
        flag = currentbyte & 1;
        currentbyte = currentbyte >> 1;
        if (flag)
        {
            r3 = sourceptr[0] & 0xFF;
            offset = ((sourceptr[1] & 0xFF) << 8) | r3;
            sourceptr += 2;
            if (offset == 0) return (u32)(destptr - destptr_orig);
            r3 = r3 & 0x00000007;
            r5 = (offset >> 3) | 0xFFFFE000;
            if (r3 == 0)
            {
                flag = 0;
                r3 = sourceptr[0] & 0xFF;
                sourceptr++;
                r3++;
            } else r3 += 2;
            r5 += (u32)destptr;
        } else {
            r3 = 0;
            for (x = 0; x < 2; x++)
            {
                bitpos--;
                if (bitpos == 0)
                {
                    currentbyte = sourceptr[0];
                    bitpos = 8;
                    sourceptr++;
                }
                flag = currentbyte & 1;
                currentbyte = currentbyte >> 1;
                offset = r3 << 1;
                r3 = offset | flag;
            }
            offset = sourceptr[0] | 0xFFFFFF00;
            r3 += 2;
            sourceptr++;
            r5 = offset + (u32)destptr;
        }
        if (r3 == 0) continue;
        t = r3;
        for (x = 0; x < t; x++)
        {
            destptr[0] = *(u8*)r5;
            r5++;
            r3++;
            destptr++;
        }
    }
}

u32 prs_decompress_size(void* source)
{
    u32 r3,r5; /* 6 unnamed registers */ 
    u32 bitpos = 9; /* 4 named registers */
    u8* sourceptr = (u8*)source;
    u8* destptr = NULL;
    u8* destptr_orig = NULL;
    u8 currentbyte,lastbyte;
    bool flag;
    int offset;
    u32 x,t; /* 2 placed variables */ 

    currentbyte = sourceptr[0];
    sourceptr++;
    for (;;)
    {
        bitpos--;
        if (bitpos == 0)
        {
            lastbyte = currentbyte = sourceptr[0];
            bitpos = 8;
            sourceptr++;
        }
        flag = currentbyte & 1;
        currentbyte = currentbyte >> 1;
        if (flag)
        {
            sourceptr++;
            destptr++;
            continue;
        }
        bitpos--;
        if (bitpos == 0)
        {
            lastbyte = currentbyte = sourceptr[0];
            bitpos = 8;
            sourceptr++;
        }
        flag = currentbyte & 1;
        currentbyte = currentbyte >> 1;
        if (flag)
        {
            r3 = sourceptr[0];
            offset = (sourceptr[1] << 8) | r3;
            sourceptr += 2;
            if (offset == 0) return (u32)(destptr - destptr_orig);
            r3 = r3 & 0x00000007;
            r5 = (offset >> 3) | 0xFFFFE000;
            if (r3 == 0)
            {
                r3 = sourceptr[0];
                sourceptr++;
                r3++;
            } else r3 += 2;
            r5 += (u32)destptr;
        } else {
            r3 = 0;
            for (x = 0; x < 2; x++)
            {
                bitpos--;
                if (bitpos == 0)
                {
                    lastbyte = currentbyte = sourceptr[0];
                    bitpos = 8;
                    sourceptr++;
                }
                flag = currentbyte & 1;
                currentbyte = currentbyte >> 1;
                offset = r3 << 1;
                r3 = offset | flag;
            }
            offset = sourceptr[0] | 0xFFFFFF00;
            r3 += 2;
            sourceptr++;
            r5 = offset + (u32)destptr;
        }
        if (r3 == 0) continue;
        t = r3;
        for (x = 0; x < t; x++)
        {
            r5++;
            r3++;
            destptr++;
        }
   }
}

