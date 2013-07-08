#include "libavutil/avassert.h"
#include "libavutil/pixdesc.h"
#include "libavcodec/get_bits.h"
#include "libavcodec/hevcdata.h"
#include "libavcodec/hevc.h"

#include <emmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>

#define BIT_DEPTH 8

void ff_hevc_put_unweighted_pred_sse(uint8_t *_dst, ptrdiff_t _dststride,
                                      int16_t *src, ptrdiff_t srcstride,
                                      int width, int height)
{
    int x, y;
    uint8_t *dst = (uint8_t*)_dst;
    ptrdiff_t dststride = _dststride/sizeof(uint8_t);
    __m128i r0,r1,f0;
    int shift = 14 - BIT_DEPTH;
#if BIT_DEPTH < 14
    int16_t offset = 1 << (shift - 1);
#else
    int16_t offset = 0;

#endif
    f0= _mm_set1_epi16(offset);

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x+=16) {
            r0= _mm_load_si128((__m128i*)&src[x]);

            r1= _mm_load_si128((__m128i*)&src[x+8]);
            r0= _mm_adds_epi16(r0,f0);

            r1= _mm_adds_epi16(r1,f0);
            r0= _mm_srai_epi16(r0,shift);
            r1= _mm_srai_epi16(r1,shift);
            r0= _mm_packus_epi16(r0,r1);

            _mm_storeu_si128((__m128i*)&dst[x],r0);
        }
        dst += dststride;
        src += srcstride;
    }
}

void ff_hevc_put_weighted_pred_avg_sse(uint8_t *_dst, ptrdiff_t _dststride,
                                        int16_t *src1, int16_t *src2, ptrdiff_t srcstride,
                                        int width, int height)
{
    int x, y;
    uint8_t *dst = (uint8_t*)_dst;
    ptrdiff_t dststride = _dststride/sizeof(uint8_t);
    __m128i r0,r1,f0,r2,r3;
    int shift = 14 + 1 - BIT_DEPTH;
#if BIT_DEPTH < 14
    int offset = 1 << (shift - 1);
#else
    int offset = 0;
#endif
            f0= _mm_set1_epi16(offset);
    for (y = 0; y < height; y++) {

            for (x = 0; x < width; x+=16) {
            r0= _mm_load_si128((__m128i*)&src1[x]);
            r1= _mm_load_si128((__m128i*)&src1[x+8]);
            r2=_mm_load_si128((__m128i*)&src2[x]);
            r3= _mm_load_si128((__m128i*)&src2[x+8]);

            r0= _mm_adds_epi16(r0,f0);
            r1= _mm_adds_epi16(r1,f0);
            r0= _mm_adds_epi16(r0,r2);
            r1= _mm_adds_epi16(r1,r3);
            r0= _mm_srai_epi16(r0,shift);
            r1= _mm_srai_epi16(r1,shift);
            r0= _mm_packus_epi16(r0,r1);

            _mm_storeu_si128((__m128i*)(dst + x),r0);
        }
        dst  += dststride;
        src1 += srcstride;
        src2 += srcstride;
    }
}


void ff_hevc_weighted_pred_sse(uint8_t denom, int16_t wlxFlag, int16_t olxFlag,
                                     uint8_t *_dst, ptrdiff_t _dststride,
                                     int16_t *src, ptrdiff_t srcstride,
                                     int width, int height)
{

    int log2Wd;
    int x , y;

    uint8_t *dst = (uint8_t*)_dst;
    ptrdiff_t dststride = _dststride/sizeof(uint8_t);
    __m128i x0,x1,x2,x3,c0,add,add2;

    log2Wd = denom + 14 - BIT_DEPTH;

    add= _mm_set1_epi32(olxFlag * ( 1 << ( BIT_DEPTH - 8 ) ));
    add2= _mm_set1_epi32(1 << (log2Wd - 1));
    c0= _mm_set1_epi16(wlxFlag);
    if (log2Wd >= 1)
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x+=16) {
            x0= _mm_load_si128((__m128i*)&src[x]);
            x2= _mm_load_si128((__m128i*)&src[x+8]);
            x1= _mm_unpackhi_epi16(_mm_mullo_epi16(x0,c0),_mm_mulhi_epi16(x0,c0));
            x3= _mm_unpackhi_epi16(_mm_mullo_epi16(x2,c0),_mm_mulhi_epi16(x2,c0));
            x0= _mm_unpacklo_epi16(_mm_mullo_epi16(x0,c0),_mm_mulhi_epi16(x0,c0));
            x2= _mm_unpacklo_epi16(_mm_mullo_epi16(x2,c0),_mm_mulhi_epi16(x2,c0));
            x0= _mm_add_epi32(x0,add2);
            x1= _mm_add_epi32(x1,add2);
            x2= _mm_add_epi32(x2,add2);
            x3= _mm_add_epi32(x3,add2);
            x0= _mm_srai_epi32(x0,log2Wd);
            x1= _mm_srai_epi32(x1,log2Wd);
            x2= _mm_srai_epi32(x2,log2Wd);
            x3= _mm_srai_epi32(x3,log2Wd);
            x0= _mm_add_epi32(x0,add);
            x1= _mm_add_epi32(x1,add);
            x2= _mm_add_epi32(x2,add);
            x3= _mm_add_epi32(x3,add);
            x0= _mm_packus_epi32(x0,x1);
            x2= _mm_packus_epi32(x2,x3);
            x0= _mm_packus_epi16(x0,x2);

            _mm_storeu_si128((__m128i*)(dst+x),x0);

        }
        dst  += dststride;
        src  += srcstride;
    }else
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x+=16) {

            x0= _mm_load_si128((__m128i*)&src[x]);
            x2= _mm_load_si128((__m128i*)&src[x+8]);
            x1= _mm_unpackhi_epi16(_mm_mullo_epi16(x0,c0),_mm_mulhi_epi16(x0,c0));
            x3= _mm_unpackhi_epi16(_mm_mullo_epi16(x2,c0),_mm_mulhi_epi16(x2,c0));
            x0= _mm_unpacklo_epi16(_mm_mullo_epi16(x0,c0),_mm_mulhi_epi16(x0,c0));
            x2= _mm_unpacklo_epi16(_mm_mullo_epi16(x2,c0),_mm_mulhi_epi16(x2,c0));

            x0= _mm_add_epi32(x0,add2);
            x1= _mm_add_epi32(x1,add2);
            x2= _mm_add_epi32(x2,add2);
            x3= _mm_add_epi32(x3,add2);

            x0= _mm_packus_epi32(x0,x1);
            x2= _mm_packus_epi32(x2,x3);
            x0= _mm_packus_epi16(x0,x2);

            _mm_storeu_si128((__m128i*)(dst +x),x0);

        }
        dst  += dststride;
        src  += srcstride;
    }
}

void ff_hevc_weighted_pred_avg_sse(uint8_t denom, int16_t wl0Flag, int16_t wl1Flag,
                                         int16_t ol0Flag, int16_t ol1Flag, uint8_t *_dst, ptrdiff_t _dststride,
                                         int16_t *src1, int16_t *src2, ptrdiff_t srcstride,
                                         int width, int height)
{
    int shift, shift2;
    int log2Wd;
    int w0;
    int w1;
    int o0;
    int o1;
    int x , y;
    int add;
    uint8_t *dst = (uint8_t*)_dst;
    ptrdiff_t dststride = _dststride/sizeof(uint8_t);
    __m128i x0,x1,x2,x3,r0,r1,r2,r3,c0,c1,c2;
    shift = 14 - BIT_DEPTH;
    log2Wd = denom + shift;

    o0 = (ol0Flag) * ( 1 << ( BIT_DEPTH - 8 ) );
    o1 = (ol1Flag) * ( 1 << ( BIT_DEPTH - 8 ) );
    shift2= (log2Wd + 1);
    w0 = wl0Flag;
    w1 = wl1Flag;
    c0= _mm_set1_epi16(wl0Flag);
    c1= _mm_set1_epi16(wl1Flag);
    c2= _mm_set1_epi32((o0 + o1 + 1) << log2Wd);

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x+=16) {
            x0= _mm_load_si128((__m128i*)&src1[x]);
            x1= _mm_load_si128((__m128i*)&src1[x+8]);
            x2= _mm_load_si128((__m128i*)&src2[x]);
            x3= _mm_load_si128((__m128i*)&src2[x+8]);

            r0= _mm_unpacklo_epi16(_mm_mullo_epi16(x0,c0),_mm_mulhi_epi16(x0,c0));
            r1= _mm_unpacklo_epi16(_mm_mullo_epi16(x1,c0),_mm_mulhi_epi16(x1,c0));
            r2= _mm_unpacklo_epi16(_mm_mullo_epi16(x2,c1),_mm_mulhi_epi16(x2,c1));
            r3= _mm_unpacklo_epi16(_mm_mullo_epi16(x3,c1),_mm_mulhi_epi16(x3,c1));
            x0= _mm_unpackhi_epi16(_mm_mullo_epi16(x0,c0),_mm_mulhi_epi16(x0,c0));
            x1= _mm_unpackhi_epi16(_mm_mullo_epi16(x1,c0),_mm_mulhi_epi16(x1,c0));
            x2= _mm_unpackhi_epi16(_mm_mullo_epi16(x2,c1),_mm_mulhi_epi16(x2,c1));
            x3= _mm_unpackhi_epi16(_mm_mullo_epi16(x3,c1),_mm_mulhi_epi16(x3,c1));
            r0= _mm_add_epi32(r0,r2);
            r1= _mm_add_epi32(r1,r3);
            r2= _mm_add_epi32(x0,x2);
            r3= _mm_add_epi32(x1,x3);

            r0= _mm_add_epi32(r0,c2);
            r1= _mm_add_epi32(r1,c2);
            r2= _mm_add_epi32(r2,c2);
            r3= _mm_add_epi32(r3,c2);

            r0= _mm_srai_epi32(r0,shift2);
            r1= _mm_srai_epi32(r1,shift2);
            r2= _mm_srai_epi32(r2,shift2);
            r3= _mm_srai_epi32(r3,shift2);

            r0= _mm_packus_epi32(r0,r2);
            r1= _mm_packus_epi32(r1,r3);
            r0= _mm_packus_epi16(r0,r1);

            _mm_storeu_si128((__m128i*)(dst+x),r0);


        }
        dst  += dststride;
        src1 += srcstride;
        src2 += srcstride;
    }
}


void ff_hevc_put_hevc_epel_pixels_sse(int16_t *dst, ptrdiff_t dststride,
                                       uint8_t *_src, ptrdiff_t _srcstride,
                                       int width, int height, int mx, int my, int16_t* mcbuffer)
{
    int x, y;
    __m128i x1, x2;
    uint8_t *src = (uint8_t*)_src;
    ptrdiff_t srcstride = _srcstride/sizeof(uint8_t);
    if(width == 4){
        for (y = 0; y < height; y++) {
            x1= _mm_loadu_si128((__m128i*)&src[0]);
            x2 = _mm_unpacklo_epi8(x1,_mm_set1_epi8(0));
            x2= _mm_slli_epi16(x2,14 - BIT_DEPTH);

            _mm_storel_epi64((__m128i*)&dst[0],x2);
            src += srcstride;
            dst += dststride;
        }
    }
    else
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x+=16) {

                    x1= _mm_loadu_si128((__m128i*)&src[x]);
                    x2 = _mm_unpacklo_epi8(x1,_mm_set1_epi8(0));

                    x1 = _mm_unpackhi_epi8(x1,_mm_set1_epi8(0));

                x2= _mm_slli_epi16(x2,14 - BIT_DEPTH);
                x1= _mm_slli_epi16(x1,14 - BIT_DEPTH);
                _mm_store_si128((__m128i*)&dst[x], x2);
                _mm_store_si128((__m128i*)&dst[x+8], x1);

            }
            src += srcstride;
            dst += dststride;
        }

}


void ff_hevc_put_hevc_epel_h_sse(int16_t *dst, ptrdiff_t dststride,
                                  uint8_t *_src, ptrdiff_t _srcstride,
                                  int width, int height, int mx, int my, int16_t* mcbuffer)
{
    int x, y;
    uint8_t *src = (uint8_t*)_src;
    ptrdiff_t srcstride = _srcstride/sizeof(uint8_t);
    const int8_t *filter = epel_filters[mx-1];
    __m128i r0, bshuffle1, bshuffle2, x1, x2,x3;
    int8_t filter_0 = filter[0];
    int8_t filter_1 = filter[1];
    int8_t filter_2 = filter[2];
    int8_t filter_3 = filter[3];
    r0= _mm_set_epi8(filter_3,filter_2,filter_1, filter_0,filter_3,filter_2,filter_1, filter_0,filter_3,filter_2,filter_1, filter_0,filter_3,filter_2,filter_1, filter_0);
    bshuffle1=_mm_set_epi8(6,5,4,3,5,4,3,2,4,3,2,1,3,2,1,0);

    if(width == 4){

        for (y = 0; y < height; y++) {
            /* load data in register     */
            x1= _mm_loadu_si128((__m128i*)&src[-1]);
            x2= _mm_shuffle_epi8(x1,bshuffle1);

            /*  PMADDUBSW then PMADDW     */
            x2= _mm_maddubs_epi16(x2,r0);
            x2= _mm_hadd_epi16(x2,_mm_set1_epi16(0));
            x2= _mm_srli_epi16(x2, BIT_DEPTH - 8);
            /* give results back            */
            _mm_storel_epi64((__m128i*)&dst[0],x2);

            src += srcstride;
            dst += dststride;
        }
    }else{
        bshuffle2=_mm_set_epi8(10,9,8,7,9,8,7,6,8,7,6,5,7,6,5,4);
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x+=8) {

                x1= _mm_loadu_si128((__m128i*)&src[x-1]);
                x2= _mm_shuffle_epi8(x1,bshuffle1);
                x3= _mm_shuffle_epi8(x1,bshuffle2);

                /*  PMADDUBSW then PMADDW     */
                x2= _mm_maddubs_epi16(x2,r0);
                x3= _mm_maddubs_epi16(x3,r0);
                x2= _mm_hadd_epi16(x2,x3);
                x2= _mm_srli_epi16(x2,BIT_DEPTH - 8);
                _mm_store_si128((__m128i*)&dst[x],x2);
            }
            src += srcstride;
            dst += dststride;
        }
    }
}

void ff_hevc_put_hevc_epel_v_sse(int16_t *dst, ptrdiff_t dststride,
                                  uint8_t *_src, ptrdiff_t _srcstride,
                                  int width, int height, int mx, int my, int16_t* mcbuffer)
{
    int x, y;
    __m128i x0,x1,x2,x3,t0,t1,t2,t3,r0,f0,f1,f2,f3,r1;
    uint8_t *src = (uint8_t*)_src;
    ptrdiff_t srcstride = _srcstride/sizeof(uint8_t);
    const int8_t *filter = epel_filters[my-1];
    int8_t filter_0 = filter[0];
    int8_t filter_1 = filter[1];
    int8_t filter_2 = filter[2];
    int8_t filter_3 = filter[3];
    f0= _mm_set1_epi16(filter_0);
    f1= _mm_set1_epi16(filter_1);
    f2= _mm_set1_epi16(filter_2);
    f3= _mm_set1_epi16(filter_3);
    if(width == 4)
        for (y = 0; y < height; y++)  {

            /* check if memory needs to be reloaded */

                x0= _mm_loadu_si128((__m128i*)&src[-srcstride]);
                x1= _mm_loadu_si128((__m128i*)&src[0]);
                x2= _mm_loadu_si128((__m128i*)&src[srcstride]);
                x3= _mm_loadu_si128((__m128i*)&src[2*srcstride]);

                t0 = _mm_unpacklo_epi8(x0,_mm_set1_epi8(0));
                t1 = _mm_unpacklo_epi8(x1,_mm_set1_epi8(0));
                t2 = _mm_unpacklo_epi8(x2,_mm_set1_epi8(0));
                t3 = _mm_unpacklo_epi8(x3,_mm_set1_epi8(0));

            r0= _mm_set1_epi16(0);
            /* multiply by correct value : */
            r0= _mm_mullo_epi16(t0,f0) ;
            r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t1,f1)) ;

            r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t2,f2)) ;

            r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t3,f3)) ;
            r0= _mm_srli_epi16(r0,BIT_DEPTH - 8);
            /* give results back            */
            _mm_storel_epi64((__m128i*)&dst[0], r0);\

            src += srcstride;
            dst += dststride;
        }

    else
        for (y = 0; y < height; y++)  {
            for (x = 0; x < width; x+=16)  {
                /* check if memory needs to be reloaded */

                    x0= _mm_loadu_si128((__m128i*)&src[x-srcstride]);
                    x1= _mm_loadu_si128((__m128i*)&src[x]);
                    x2= _mm_loadu_si128((__m128i*)&src[x+srcstride]);
                    x3= _mm_loadu_si128((__m128i*)&src[x+2*srcstride]);

                    t0 = _mm_unpacklo_epi8(x0,_mm_set1_epi8(0));
                    t1 = _mm_unpacklo_epi8(x1,_mm_set1_epi8(0));
                    t2 = _mm_unpacklo_epi8(x2,_mm_set1_epi8(0));
                    t3 = _mm_unpacklo_epi8(x3,_mm_set1_epi8(0));

                    x0 = _mm_unpackhi_epi8(x0,_mm_set1_epi8(0));
                    x1 = _mm_unpackhi_epi8(x1,_mm_set1_epi8(0));
                    x2 = _mm_unpackhi_epi8(x2,_mm_set1_epi8(0));
                    x3 = _mm_unpackhi_epi8(x3,_mm_set1_epi8(0));


                /* multiply by correct value : */
                r0= _mm_mullo_epi16(t0,f0) ;
                r1= _mm_mullo_epi16(x0,f0) ;
                r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t1,f1)) ;
                r1= _mm_adds_epi16(r1, _mm_mullo_epi16(x1,f1)) ;
                r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t2,f2)) ;
                r1= _mm_adds_epi16(r1, _mm_mullo_epi16(x2,f2)) ;
                r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t3,f3)) ;
                r1= _mm_adds_epi16(r1, _mm_mullo_epi16(x3,f3)) ;
                r0= _mm_srli_epi16(r0,BIT_DEPTH - 8);
                r1= _mm_srli_epi16(r1,BIT_DEPTH - 8);
                /* give results back            */
                _mm_store_si128((__m128i*)&dst[x], r0);
                _mm_storeu_si128((__m128i*)&dst[x+8], r1);
            }
            src += srcstride;
            dst += dststride;
        }

}

void ff_hevc_put_hevc_epel_hv_sse(int16_t *dst, ptrdiff_t dststride,
                                   uint8_t *_src, ptrdiff_t _srcstride,
                                   int width, int height, int mx, int my, int16_t* mcbuffer)
{
    int x, y;
    uint8_t *src = (uint8_t*)_src;
    ptrdiff_t srcstride = _srcstride/sizeof(uint8_t);
    const int8_t *filter_h = epel_filters[mx-1];
    const int8_t *filter_v = epel_filters[my-1];
    __m128i r0, bshuffle1, bshuffle2, x0, x1, x2,x3, t0, t1, t2, t3, f0, f1, f2, f3,r1,r2;
    int8_t filter_0 = filter_h[0];
    int8_t filter_1 = filter_h[1];
    int8_t filter_2 = filter_h[2];
    int8_t filter_3 = filter_h[3];
    r0= _mm_set_epi8(filter_3,filter_2,filter_1, filter_0,filter_3,filter_2,filter_1, filter_0,filter_3,filter_2,filter_1, filter_0,filter_3,filter_2,filter_1, filter_0);
    bshuffle1=_mm_set_epi8(6,5,4,3,5,4,3,2,4,3,2,1,3,2,1,0);
    //DECLARE_ALIGNED( 16, int16_t, tmp_array[(MAX_PB_SIZE+3)*MAX_PB_SIZE] );
    //#ifndef OPTIMIZATION_ENABLE
    // int16_t tmp_array[(MAX_PB_SIZE+3)*MAX_PB_SIZE];
    //#endif
     int16_t *tmp = mcbuffer;

    src -= epel_extra_before * srcstride;
    /* horizontal treatment */
    if(width == 4){

        for (y = 0; y < height + epel_extra; y+=2) {
            /* load data in register     */
            x1= _mm_loadu_si128((__m128i*)&src[-1]);
            src += srcstride;
            x2= _mm_loadu_si128((__m128i*)&src[-1]);
            x1= _mm_shuffle_epi8(x1,bshuffle1);
            x2= _mm_shuffle_epi8(x2,bshuffle1);

            /*  PMADDUBSW then PMADDW     */
            x1= _mm_maddubs_epi16(x1,r0);
            x1= _mm_hadd_epi16(x1,_mm_set1_epi16(0));
            x1= _mm_srli_epi16(x1, BIT_DEPTH - 8);
            x2= _mm_maddubs_epi16(x2,r0);
            x2= _mm_hadd_epi16(x2,_mm_set1_epi16(0));
            x2= _mm_srli_epi16(x2, BIT_DEPTH - 8);
            /* give results back            */
            _mm_storel_epi64((__m128i*)&tmp[0], x1);

            tmp += MAX_PB_SIZE;
            _mm_storel_epi64((__m128i*)&tmp[0], x2);

            src += srcstride;
            tmp += MAX_PB_SIZE;
        }
    }else{
        bshuffle2=_mm_set_epi8(10,9,8,7,9,8,7,6,8,7,6,5,7,6,5,4);
        for (y = 0; y < height + epel_extra; y++) {
            for (x = 0; x < width; x+=8) {

                x1= _mm_loadu_si128((__m128i*)&src[x-1]);
                x2= _mm_shuffle_epi8(x1,bshuffle1);
                x3= _mm_shuffle_epi8(x1,bshuffle2);

                /*  PMADDUBSW then PMADDW     */
                x2= _mm_maddubs_epi16(x2,r0);
                x3= _mm_maddubs_epi16(x3,r0);
                x2= _mm_hadd_epi16(x2,x3);
                x2= _mm_srli_epi16(x2,BIT_DEPTH - 8);
                _mm_store_si128((__m128i*)&tmp[x],x2);
            }
            src += srcstride;
            tmp += MAX_PB_SIZE;
        }
    }

    tmp = mcbuffer + epel_extra_before * MAX_PB_SIZE;

    /* vertical treatment */
    //f0= _mm_loadu_si128((__m128i *)&filter_v);
    f3= _mm_set1_epi16(filter_v[3]);
    f1= _mm_set1_epi16(filter_v[1]);
    f2= _mm_set1_epi16(filter_v[2]);
    f0= _mm_set1_epi16(filter_v[0]);
    for (y = 0; y < height; y++)  {
        for (x = 0; x < width; x+=8)  {
            /* check if memory needs to be reloaded */
                x0= _mm_load_si128((__m128i*)&tmp[x-MAX_PB_SIZE]);
                x1= _mm_load_si128((__m128i*)&tmp[x]);
                x2= _mm_load_si128((__m128i*)&tmp[x+MAX_PB_SIZE]);
                x3= _mm_load_si128((__m128i*)&tmp[x+2*MAX_PB_SIZE]);

                r0= _mm_mullo_epi16(x0,f0);
                r1= _mm_mulhi_epi16(x0,f0);
                r2= _mm_mullo_epi16(x1,f1);
                t0 = _mm_unpacklo_epi16(r0,r1);
                x0 = _mm_unpackhi_epi16(r0,r1);
                r0= _mm_mulhi_epi16(x1,f1);
                r1= _mm_mullo_epi16(x2,f2);
                t1 = _mm_unpacklo_epi16(r2,r0);
                x1 = _mm_unpackhi_epi16(r2,r0);
                r2= _mm_mulhi_epi16(x2,f2);
                r0= _mm_mullo_epi16(x3,f3);
                t2 = _mm_unpacklo_epi16(r1,r2);
                x2 = _mm_unpackhi_epi16(r1,r2);
                r1= _mm_mulhi_epi16(x3,f3);
                t3 = _mm_unpacklo_epi16(r0,r1);
                x3 = _mm_unpackhi_epi16(r0,r1);

            /* multiply by correct value : */
            r0= _mm_add_epi32(t0,t1);
            r1= _mm_add_epi32(x0,x1);
            r0= _mm_add_epi32(r0,t2) ;
            r1= _mm_add_epi32(r1,x2) ;
            r0= _mm_add_epi32(r0,t3) ;
            r1= _mm_add_epi32(r1,x3) ;
            r0= _mm_srli_epi32(r0,6);
            r1= _mm_srli_epi32(r1,6);

            /* give results back            */
            r0=_mm_packs_epi32(r0,r1);
            _mm_store_si128((__m128i*)&dst[x],r0);
        }
        tmp += MAX_PB_SIZE;
        dst += dststride;
    }

}

void ff_hevc_put_hevc_qpel_pixels_sse(int16_t *dst, ptrdiff_t dststride,
                                       uint8_t *_src, ptrdiff_t _srcstride,
                                       int width, int height, int16_t* mcbuffer)
{
    int x, y;
    __m128i x1, x2, x3;
    uint8_t *src = (uint8_t*)_src;
    ptrdiff_t srcstride = _srcstride/sizeof(uint8_t);
    if(width == 4){
        for (y = 0; y < height; y++) {
            x1= _mm_loadu_si128((__m128i*)&src[0]);
            x2 = _mm_unpacklo_epi8(x1,_mm_set1_epi8(0));
            x2= _mm_slli_epi16(x2,14 - BIT_DEPTH);

            _mm_storel_epi64((__m128i*)&dst[0], x2);


            src += srcstride;
            dst += dststride;
        }
    }
    else
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x+=16) {

                    x1= _mm_loadu_si128((__m128i*)&src[x]);
                    x2 = _mm_unpacklo_epi8(x1,_mm_set1_epi8(0));

                    x3 = _mm_unpackhi_epi8(x1,_mm_set1_epi8(0));

                x2= _mm_slli_epi16(x2,14 - BIT_DEPTH);
                x3= _mm_slli_epi16(x3,14 - BIT_DEPTH);
                _mm_storeu_si128((__m128i*)&dst[x], x2);
                _mm_storeu_si128((__m128i*)&dst[x+8], x3);

            }
            src += srcstride;
            dst += dststride;
        }

}



#define QPEL2_FILTER_3                                              \
_mm_set_epi16(-1 ,4 ,-10 ,58 ,17 ,-5 ,1 ,0)
#define QPEL2_FILTER_2                                             \
_mm_set_epi16(-1 ,4 ,-11 ,40 ,40 ,-11 ,4 ,-1)
#define QPEL2_FILTER_1                                              \
_mm_set_epi16(0 ,1 ,-5 ,17 ,58 ,-10 ,4 ,-1)
#define QPEL2_H_FILTER_3                                              \
_mm_set_epi8(-1 ,4 ,-10 ,58 ,17 ,-5 ,1 ,0 ,-1 ,4 ,-10 ,58 ,17 ,-5 ,1 ,0)
#define QPEL2_H_FILTER_2                                             \
_mm_set_epi8(-1 ,4 ,-11 ,40 ,40 ,-11 ,4 ,-1 ,-1 ,4 ,-11 ,40 ,40 ,-11 ,4 ,-1)
#define QPEL2_H_FILTER_1                                              \
_mm_set_epi8(0 ,1 ,-5 ,17 ,58 ,-10 ,4 ,-1 ,0 ,1 ,-5 ,17 ,58 ,-10 ,4 ,-1)
#define QPEL2_X3V_FILTER_3                                              \
_mm_setzero_si128()
#define QPEL2_X3V_FILTER_2                                              \
_mm_load_si128((__m128i*)&tmp[x-3*srcstride])
#define QPEL2_X3V_FILTER_1                                              \
_mm_load_si128((__m128i*)&tmp[x-3*srcstride])
#define QPEL2_X4V_FILTER_3                                              \
_mm_load_si128((__m128i*)&tmp[x+4*srcstride])
#define QPEL2_X4V_FILTER_2                                             \
_mm_load_si128((__m128i*)&tmp[x+4*srcstride])
#define QPEL2_X4V_FILTER_1                                              \
_mm_setzero_si128()
#define QPEL2_X3VV_FILTER_3                                              \
_mm_setzero_si128()
#define QPEL2_X3VV_FILTER_2                                              \
_mm_loadu_si128((__m128i*)&src[x-3*srcstride])
#define QPEL2_X3VV_FILTER_1                                              \
_mm_loadu_si128((__m128i*)&src[x-3*srcstride])
#define QPEL2_X4VV_FILTER_3                                              \
_mm_loadu_si128((__m128i*)&src[x+4*srcstride])
#define QPEL2_X4VV_FILTER_2                                             \
_mm_loadu_si128((__m128i*)&src[x+4*srcstride])
#define QPEL2_X4VV_FILTER_1                                              \
_mm_setzero_si128()



void ff_hevc_put_hevc_qpel_h_1_sse(int16_t *dst, ptrdiff_t dststride,
uint8_t *_src, ptrdiff_t _srcstride,
int width, int height, int16_t* mcbuffer)
{
int x, y, i;
uint8_t *src = _src;
ptrdiff_t srcstride = _srcstride/sizeof(uint8_t);
__m128i x1, rBuffer, rTemp, r0, r1, r2, x2, x3, x4,x5, y1,y2,y3;
const __m128i rk0 = _mm_set1_epi8(0);

r0= _mm_set_epi8(0 ,1 ,-5 ,17 ,58 ,-10 ,4 ,-1 ,0 ,1 ,-5 ,17 ,58 ,-10 ,4 ,-1);

/* LOAD src from memory to registers to limit memory bandwidth */
if(width == 4){

for (y = 0; y < height; y+=2) {
/* load data in register     */
x1= _mm_loadu_si128((__m128i*)&src[-3]);
    src+= srcstride;
y1= _mm_loadu_si128((__m128i*)&src[-3]);
x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
y2= _mm_unpacklo_epi64(y1,_mm_srli_si128(y1,1));
y3= _mm_unpacklo_epi64(_mm_srli_si128(y1,2),_mm_srli_si128(y1,3));

/*  PMADDUBSW then PMADDW     */
x2= _mm_maddubs_epi16(x2,r0);
    y2= _mm_maddubs_epi16(y2,r0);
x3= _mm_maddubs_epi16(x3,r0);
    y3= _mm_maddubs_epi16(y3,r0);
x2= _mm_hadd_epi16(x2,x3);
    y2= _mm_hadd_epi16(y2,y3);
x2= _mm_hadd_epi16(x2,_mm_set1_epi16(0));
    y2= _mm_hadd_epi16(y2,_mm_set1_epi16(0));
x2= _mm_srli_epi16(x2, BIT_DEPTH - 8);
    y2= _mm_srli_epi16(y2, BIT_DEPTH - 8);
/* give results back            */
    _mm_storel_epi64((__m128i*)&dst[0],x2);

    dst+=dststride;
    _mm_storel_epi64((__m128i*)&dst[0],y2);

src += srcstride;
dst += dststride;
}
}else
for (y = 0; y < height; y++) {
for (x = 0; x < width; x+=8)  {
/* load data in register     */
x1= _mm_loadu_si128((__m128i*)&src[x-3]);
x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
x4= _mm_unpacklo_epi64(_mm_srli_si128(x1,4),_mm_srli_si128(x1,5));
x5= _mm_unpacklo_epi64(_mm_srli_si128(x1,6),_mm_srli_si128(x1,7));

/*  PMADDUBSW then PMADDW     */
x2= _mm_maddubs_epi16(x2,r0);
x3= _mm_maddubs_epi16(x3,r0);
x4= _mm_maddubs_epi16(x4,r0);
x5= _mm_maddubs_epi16(x5,r0);
x2= _mm_hadd_epi16(x2,x3);
x4= _mm_hadd_epi16(x4,x5);
x2= _mm_hadd_epi16(x2,x4);
/* give results back            */
_mm_store_si128((__m128i*)&dst[x], _mm_srli_si128(x2,BIT_DEPTH -8));
}
src += srcstride;
dst += dststride;
}

}
void ff_hevc_put_hevc_qpel_h_2_sse(int16_t *dst, ptrdiff_t dststride,
uint8_t *_src, ptrdiff_t _srcstride,
int width, int height, int16_t* mcbuffer)
{
int x, y, i;
uint8_t *src = _src;
ptrdiff_t srcstride = _srcstride/sizeof(uint8_t);
__m128i x1, rBuffer, rTemp, r0, r1, r2, x2, x3, x4,x5, y1,y2,y3;
const __m128i rk0 = _mm_set1_epi8(0);

r0=_mm_set_epi8(-1 ,4 ,-11 ,40 ,40 ,-11 ,4 ,-1 ,-1 ,4 ,-11 ,40 ,40 ,-11 ,4 ,-1);

/* LOAD src from memory to registers to limit memory bandwidth */
if(width == 4){

for (y = 0; y < height; y+=2) {
/* load data in register     */
x1= _mm_loadu_si128((__m128i*)&src[-3]);
    src+= srcstride;
y1= _mm_loadu_si128((__m128i*)&src[-3]);
x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
y2= _mm_unpacklo_epi64(y1,_mm_srli_si128(y1,1));
y3= _mm_unpacklo_epi64(_mm_srli_si128(y1,2),_mm_srli_si128(y1,3));

/*  PMADDUBSW then PMADDW     */
x2= _mm_maddubs_epi16(x2,r0);
    y2= _mm_maddubs_epi16(y2,r0);
x3= _mm_maddubs_epi16(x3,r0);
    y3= _mm_maddubs_epi16(y3,r0);
x2= _mm_hadd_epi16(x2,x3);
    y2= _mm_hadd_epi16(y2,y3);
x2= _mm_hadd_epi16(x2,_mm_set1_epi16(0));
    y2= _mm_hadd_epi16(y2,_mm_set1_epi16(0));
x2= _mm_srli_epi16(x2, BIT_DEPTH - 8);
    y2= _mm_srli_epi16(y2, BIT_DEPTH - 8);
/* give results back            */
    _mm_storel_epi64((__m128i*)&dst[0],x2);

    dst+=dststride;
    _mm_storel_epi64((__m128i*)&dst[0],y2);

src += srcstride;
dst += dststride;
}
}else
for (y = 0; y < height; y++) {
for (x = 0; x < width; x+=8)  {
/* load data in register     */
x1= _mm_loadu_si128((__m128i*)&src[x-3]);
x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
x4= _mm_unpacklo_epi64(_mm_srli_si128(x1,4),_mm_srli_si128(x1,5));
x5= _mm_unpacklo_epi64(_mm_srli_si128(x1,6),_mm_srli_si128(x1,7));

/*  PMADDUBSW then PMADDW     */
x2= _mm_maddubs_epi16(x2,r0);
x3= _mm_maddubs_epi16(x3,r0);
x4= _mm_maddubs_epi16(x4,r0);
x5= _mm_maddubs_epi16(x5,r0);
x2= _mm_hadd_epi16(x2,x3);
x4= _mm_hadd_epi16(x4,x5);
x2= _mm_hadd_epi16(x2,x4);
/* give results back            */
_mm_store_si128((__m128i*)&dst[x], _mm_srli_si128(x2,BIT_DEPTH -8));
}
src += srcstride;
dst += dststride;
}

}
void ff_hevc_put_hevc_qpel_h_3_sse(int16_t *dst, ptrdiff_t dststride,
uint8_t *_src, ptrdiff_t _srcstride,
int width, int height, int16_t* mcbuffer)
{
int x, y, i;
uint8_t *src = _src;
ptrdiff_t srcstride = _srcstride/sizeof(uint8_t);
__m128i x1, rBuffer, rTemp, r0, r1, r2, x2, x3, x4,x5, y1,y2,y3;
const __m128i rk0 = _mm_set1_epi8(0);

r0= _mm_set_epi8(-1 ,4 ,-10 ,58 ,17 ,-5 ,1 ,0 ,-1 ,4 ,-10 ,58 ,17 ,-5 ,1 ,0);

/* LOAD src from memory to registers to limit memory bandwidth */
if(width == 4){

for (y = 0; y < height; y+=2) {
/* load data in register     */
x1= _mm_loadu_si128((__m128i*)&src[-2]);
    src+= srcstride;
    x1= _mm_slli_si128(x1,1);
y1= _mm_loadu_si128((__m128i*)&src[-2]);
x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
y1= _mm_slli_si128(y1,1);
x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
y2= _mm_unpacklo_epi64(y1,_mm_srli_si128(y1,1));
y3= _mm_unpacklo_epi64(_mm_srli_si128(y1,2),_mm_srli_si128(y1,3));

/*  PMADDUBSW then PMADDW     */
x2= _mm_maddubs_epi16(x2,r0);
    y2= _mm_maddubs_epi16(y2,r0);
x3= _mm_maddubs_epi16(x3,r0);
    y3= _mm_maddubs_epi16(y3,r0);
x2= _mm_hadd_epi16(x2,x3);
    y2= _mm_hadd_epi16(y2,y3);
x2= _mm_hadd_epi16(x2,_mm_set1_epi16(0));
    y2= _mm_hadd_epi16(y2,_mm_set1_epi16(0));
x2= _mm_srli_epi16(x2, BIT_DEPTH - 8);
    y2= _mm_srli_epi16(y2, BIT_DEPTH - 8);
/* give results back            */
    _mm_storel_epi64((__m128i*)&dst[0],x2);

    dst+=dststride;
    _mm_storel_epi64((__m128i*)&dst[0],y2);

src += srcstride;
dst += dststride;
}
}else
for (y = 0; y < height; y++) {
for (x = 0; x < width; x+=8)  {
/* load data in register     */
x1= _mm_loadu_si128((__m128i*)&src[x-2]);
x1= _mm_slli_si128(x1,1);
x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
x4= _mm_unpacklo_epi64(_mm_srli_si128(x1,4),_mm_srli_si128(x1,5));
x5= _mm_unpacklo_epi64(_mm_srli_si128(x1,6),_mm_srli_si128(x1,7));

/*  PMADDUBSW then PMADDW     */
x2= _mm_maddubs_epi16(x2,r0);
x3= _mm_maddubs_epi16(x3,r0);
x4= _mm_maddubs_epi16(x4,r0);
x5= _mm_maddubs_epi16(x5,r0);
x2= _mm_hadd_epi16(x2,x3);
x4= _mm_hadd_epi16(x4,x5);
x2= _mm_hadd_epi16(x2,x4);
/* give results back            */
_mm_store_si128((__m128i*)&dst[x], _mm_srli_si128(x2,BIT_DEPTH -8));
}
src += srcstride;
dst += dststride;
}

}
/**
 for column MC treatment, we will calculate 8 pixels at the same time by multiplying the values
 of each row.

 */
void ff_hevc_put_hevc_qpel_v_1_sse(int16_t *dst, ptrdiff_t dststride,
		uint8_t *_src, ptrdiff_t _srcstride,
		int width, int height, int16_t* mcbuffer)
{
	int x, y;
	uint8_t *src = (uint8_t*)_src;
	ptrdiff_t srcstride = _srcstride/sizeof(uint8_t);
	__m128i x1,x2,x3,x4,x5,x6,x7,x8, r0, r1, r2;
	__m128i t1,t2,t3,t4,t5,t6,t7,t8;
	r1= _mm_set_epi16(0 ,1 ,-5 ,17 ,58 ,-10 ,4 ,-1);
	/* case width = 4 */
	if(width == 4){        x=0;
	for (y = 0; y < height; y+=2)  {
		r0= _mm_set1_epi16(0);
		/* load data in register  */
		x1= _mm_loadu_si128((__m128i*)&src[x-3*srcstride]);
		x2= _mm_loadu_si128((__m128i*)&src[-2*srcstride]);
		x3= _mm_loadu_si128((__m128i*)&src[-srcstride]);
		x4= _mm_loadu_si128((__m128i*)&src[0]);
		x5= _mm_loadu_si128((__m128i*)&src[srcstride]);
		x6= _mm_loadu_si128((__m128i*)&src[2*srcstride]);
		x7= _mm_loadu_si128((__m128i*)&src[3*srcstride]);
		x8= _mm_setzero_si128();
		src += srcstride;
		t1= _mm_loadu_si128((__m128i*)&src[x-3*srcstride]);
		t2= _mm_loadu_si128((__m128i*)&src[-2*srcstride]);
		t3= _mm_loadu_si128((__m128i*)&src[-srcstride]);
		t4= _mm_loadu_si128((__m128i*)&src[0]);
		t5= _mm_loadu_si128((__m128i*)&src[srcstride]);
		t6= _mm_loadu_si128((__m128i*)&src[2*srcstride]);
		t7= _mm_loadu_si128((__m128i*)&src[3*srcstride]);
		t8= _mm_setzero_si128();
		x1 = _mm_unpacklo_epi8(x1,_mm_set1_epi8(0));
		x2 = _mm_unpacklo_epi8(x2,_mm_set1_epi8(0));
		x3 = _mm_unpacklo_epi8(x3,_mm_set1_epi8(0));
		x4 = _mm_unpacklo_epi8(x4,_mm_set1_epi8(0));
		x5 = _mm_unpacklo_epi8(x5,_mm_set1_epi8(0));
		x6 = _mm_unpacklo_epi8(x6,_mm_set1_epi8(0));
		x7 = _mm_unpacklo_epi8(x7,_mm_set1_epi8(0));
		x8 = _mm_unpacklo_epi8(x8,_mm_set1_epi8(0));
		t1 = _mm_unpacklo_epi8(t1,_mm_set1_epi8(0));
		t2 = _mm_unpacklo_epi8(t2,_mm_set1_epi8(0));
		t3 = _mm_unpacklo_epi8(t3,_mm_set1_epi8(0));
		t4 = _mm_unpacklo_epi8(t4,_mm_set1_epi8(0));
		t5 = _mm_unpacklo_epi8(t5,_mm_set1_epi8(0));
		t6 = _mm_unpacklo_epi8(t6,_mm_set1_epi8(0));
		t7 = _mm_unpacklo_epi8(t7,_mm_set1_epi8(0));
		t8 = _mm_unpacklo_epi8(t8,_mm_set1_epi8(0));


		r0= _mm_mullo_epi16(x1,_mm_set1_epi16(_mm_extract_epi16(r1,0))) ;
		r2= _mm_mullo_epi16(t1,_mm_set1_epi16(_mm_extract_epi16(r1,0))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x2,_mm_set1_epi16(_mm_extract_epi16(r1,1)))) ;
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t2,_mm_set1_epi16(_mm_extract_epi16(r1,1)))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x3,_mm_set1_epi16(_mm_extract_epi16(r1,2)))) ;
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t3,_mm_set1_epi16(_mm_extract_epi16(r1,2)))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x4,_mm_set1_epi16(_mm_extract_epi16(r1,3)))) ;
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t4,_mm_set1_epi16(_mm_extract_epi16(r1,3)))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x5,_mm_set1_epi16(_mm_extract_epi16(r1,4)))) ;
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t5,_mm_set1_epi16(_mm_extract_epi16(r1,4)))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x6,_mm_set1_epi16(_mm_extract_epi16(r1,5)))) ;
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t6,_mm_set1_epi16(_mm_extract_epi16(r1,5)))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x7,_mm_set1_epi16(_mm_extract_epi16(r1,6)))) ;
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t7,_mm_set1_epi16(_mm_extract_epi16(r1,6)))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(r1,7))));
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t8,_mm_set1_epi16(_mm_extract_epi16(r1,7))));

		r0= _mm_srli_epi16(r0, BIT_DEPTH - 8);
		r2= _mm_srli_epi16(r2, BIT_DEPTH - 8);
		/* give results back            */
		_mm_storel_epi64((__m128i*)&dst[0],r0);
		dst += dststride;
		_mm_storel_epi64((__m128i*)&dst[0],r2);
		src += srcstride;
		dst += dststride;
	}

	}else      /*case width >=8 */
		for (y = 0; y < height; y++)  {
			for (x = 0; x < width; x+=16)  {
				/* check if memory needs to be reloaded */
				x1= _mm_loadu_si128((__m128i*)&src[x-3*srcstride]);
				x2= _mm_loadu_si128((__m128i*)&src[x-2*srcstride]);
				x3= _mm_loadu_si128((__m128i*)&src[x-srcstride]);
				x4= _mm_loadu_si128((__m128i*)&src[x]);
				x5= _mm_loadu_si128((__m128i*)&src[x+srcstride]);
				x6= _mm_loadu_si128((__m128i*)&src[x+2*srcstride]);
				x7= _mm_loadu_si128((__m128i*)&src[x+3*srcstride]);
				x8= _mm_setzero_si128();

				t1 = _mm_unpacklo_epi8(x1,_mm_setzero_si128());
				t2 = _mm_unpacklo_epi8(x2,_mm_setzero_si128());
				t3 = _mm_unpacklo_epi8(x3,_mm_setzero_si128());
				t4 = _mm_unpacklo_epi8(x4,_mm_setzero_si128());
				t5 = _mm_unpacklo_epi8(x5,_mm_setzero_si128());
				t6 = _mm_unpacklo_epi8(x6,_mm_setzero_si128());
				t7 = _mm_unpacklo_epi8(x7,_mm_setzero_si128());
				t8 = _mm_setzero_si128();

				x1 = _mm_unpackhi_epi8(x1,_mm_setzero_si128());
				x2 = _mm_unpackhi_epi8(x2,_mm_setzero_si128());
				x3 = _mm_unpackhi_epi8(x3,_mm_setzero_si128());
				x4 = _mm_unpackhi_epi8(x4,_mm_setzero_si128());
				x5 = _mm_unpackhi_epi8(x5,_mm_setzero_si128());
				x6 = _mm_unpackhi_epi8(x6,_mm_setzero_si128());
				x7 = _mm_unpackhi_epi8(x7,_mm_setzero_si128());
				x8 = _mm_setzero_si128();

				/* multiply by correct value : */
				r0= _mm_mullo_epi16(t1,_mm_set1_epi16(_mm_extract_epi16(r1,0))) ;
				r2= _mm_mullo_epi16(x1,_mm_set1_epi16(_mm_extract_epi16(r1,0))) ;
				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t2,_mm_set1_epi16(_mm_extract_epi16(r1,1)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x2,_mm_set1_epi16(_mm_extract_epi16(r1,1)))) ;
				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t3,_mm_set1_epi16(_mm_extract_epi16(r1,2)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x3,_mm_set1_epi16(_mm_extract_epi16(r1,2)))) ;

				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t4,_mm_set1_epi16(_mm_extract_epi16(r1,3)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x4,_mm_set1_epi16(_mm_extract_epi16(r1,3)))) ;

				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t5,_mm_set1_epi16(_mm_extract_epi16(r1,4)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x5,_mm_set1_epi16(_mm_extract_epi16(r1,4)))) ;

				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t6,_mm_set1_epi16(_mm_extract_epi16(r1,5)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x6,_mm_set1_epi16(_mm_extract_epi16(r1,5)))) ;

				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t7,_mm_set1_epi16(_mm_extract_epi16(r1,6)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x7,_mm_set1_epi16(_mm_extract_epi16(r1,6)))) ;

				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t8,_mm_set1_epi16(_mm_extract_epi16(r1,7)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(r1,7)))) ;



				/* give results back            */
				_mm_store_si128((__m128i*)&dst[x], _mm_srli_epi16(r0,BIT_DEPTH -8));
				_mm_store_si128((__m128i*)&dst[x+8], _mm_srli_epi16(r2,BIT_DEPTH -8));
			}
			src += srcstride;
			dst += dststride;
		}
}
void ff_hevc_put_hevc_qpel_v_2_sse(int16_t *dst, ptrdiff_t dststride,
		uint8_t *_src, ptrdiff_t _srcstride,
		int width, int height, int16_t* mcbuffer)
{
	int x, y;
	uint8_t *src = (uint8_t*)_src;
	ptrdiff_t srcstride = _srcstride/sizeof(uint8_t);
	__m128i x1,x2,x3,x4,x5,x6,x7,x8, r0, r1, r2;
	__m128i t1,t2,t3,t4,t5,t6,t7,t8;
	r1= _mm_set_epi16(-1 ,4 ,-11 ,40 ,40 ,-11 ,4 ,-1);
	/* case width = 4 */
	if(width == 4){        x=0;
	for (y = 0; y < height; y+=2)  {
		r0= _mm_set1_epi16(0);
		/* load data in register  */
		x1= _mm_loadu_si128((__m128i*)&src[x-3*srcstride]);
		x2= _mm_loadu_si128((__m128i*)&src[-2*srcstride]);
		x3= _mm_loadu_si128((__m128i*)&src[-srcstride]);
		x4= _mm_loadu_si128((__m128i*)&src[0]);
		x5= _mm_loadu_si128((__m128i*)&src[srcstride]);
		x6= _mm_loadu_si128((__m128i*)&src[2*srcstride]);
		x7= _mm_loadu_si128((__m128i*)&src[3*srcstride]);
		x8= _mm_loadu_si128((__m128i*)&src[x+4*srcstride]);
		src += srcstride;
		t1= _mm_loadu_si128((__m128i*)&src[x-3*srcstride]);
		t2= _mm_loadu_si128((__m128i*)&src[-2*srcstride]);
		t3= _mm_loadu_si128((__m128i*)&src[-srcstride]);
		t4= _mm_loadu_si128((__m128i*)&src[0]);
		t5= _mm_loadu_si128((__m128i*)&src[srcstride]);
		t6= _mm_loadu_si128((__m128i*)&src[2*srcstride]);
		t7= _mm_loadu_si128((__m128i*)&src[3*srcstride]);
		t8= _mm_loadu_si128((__m128i*)&src[x+4*srcstride]);
		x1 = _mm_unpacklo_epi8(x1,_mm_set1_epi8(0));
		x2 = _mm_unpacklo_epi8(x2,_mm_set1_epi8(0));
		x3 = _mm_unpacklo_epi8(x3,_mm_set1_epi8(0));
		x4 = _mm_unpacklo_epi8(x4,_mm_set1_epi8(0));
		x5 = _mm_unpacklo_epi8(x5,_mm_set1_epi8(0));
		x6 = _mm_unpacklo_epi8(x6,_mm_set1_epi8(0));
		x7 = _mm_unpacklo_epi8(x7,_mm_set1_epi8(0));
		x8 = _mm_unpacklo_epi8(x8,_mm_set1_epi8(0));
		t1 = _mm_unpacklo_epi8(t1,_mm_set1_epi8(0));
		t2 = _mm_unpacklo_epi8(t2,_mm_set1_epi8(0));
		t3 = _mm_unpacklo_epi8(t3,_mm_set1_epi8(0));
		t4 = _mm_unpacklo_epi8(t4,_mm_set1_epi8(0));
		t5 = _mm_unpacklo_epi8(t5,_mm_set1_epi8(0));
		t6 = _mm_unpacklo_epi8(t6,_mm_set1_epi8(0));
		t7 = _mm_unpacklo_epi8(t7,_mm_set1_epi8(0));
		t8 = _mm_unpacklo_epi8(t8,_mm_set1_epi8(0));


		r0= _mm_mullo_epi16(x1,_mm_set1_epi16(_mm_extract_epi16(r1,0))) ;
		r2= _mm_mullo_epi16(t1,_mm_set1_epi16(_mm_extract_epi16(r1,0))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x2,_mm_set1_epi16(_mm_extract_epi16(r1,1)))) ;
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t2,_mm_set1_epi16(_mm_extract_epi16(r1,1)))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x3,_mm_set1_epi16(_mm_extract_epi16(r1,2)))) ;
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t3,_mm_set1_epi16(_mm_extract_epi16(r1,2)))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x4,_mm_set1_epi16(_mm_extract_epi16(r1,3)))) ;
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t4,_mm_set1_epi16(_mm_extract_epi16(r1,3)))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x5,_mm_set1_epi16(_mm_extract_epi16(r1,4)))) ;
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t5,_mm_set1_epi16(_mm_extract_epi16(r1,4)))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x6,_mm_set1_epi16(_mm_extract_epi16(r1,5)))) ;
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t6,_mm_set1_epi16(_mm_extract_epi16(r1,5)))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x7,_mm_set1_epi16(_mm_extract_epi16(r1,6)))) ;
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t7,_mm_set1_epi16(_mm_extract_epi16(r1,6)))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(r1,7))));
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t8,_mm_set1_epi16(_mm_extract_epi16(r1,7))));

		r0= _mm_srli_epi16(r0, BIT_DEPTH - 8);
		r2= _mm_srli_epi16(r2, BIT_DEPTH - 8);
		/* give results back            */
		_mm_storel_epi64((__m128i*)&dst[0],r0);
		dst += dststride;
		_mm_storel_epi64((__m128i*)&dst[0],r2);
		src += srcstride;
		dst += dststride;
	}

	}else      /*case width >=8 */
		for (y = 0; y < height; y++)  {
			for (x = 0; x < width; x+=16)  {
				/* check if memory needs to be reloaded */
				x1= _mm_loadu_si128((__m128i*)&src[x-3*srcstride]);
				x2= _mm_loadu_si128((__m128i*)&src[x-2*srcstride]);
				x3= _mm_loadu_si128((__m128i*)&src[x-srcstride]);
				x4= _mm_loadu_si128((__m128i*)&src[x]);
				x5= _mm_loadu_si128((__m128i*)&src[x+srcstride]);
				x6= _mm_loadu_si128((__m128i*)&src[x+2*srcstride]);
				x7= _mm_loadu_si128((__m128i*)&src[x+3*srcstride]);
				x8= _mm_loadu_si128((__m128i*)&src[x+4*srcstride]);

				t1 = _mm_unpacklo_epi8(x1,_mm_set1_epi8(0));
				t2 = _mm_unpacklo_epi8(x2,_mm_set1_epi8(0));
				t3 = _mm_unpacklo_epi8(x3,_mm_set1_epi8(0));
				t4 = _mm_unpacklo_epi8(x4,_mm_set1_epi8(0));
				t5 = _mm_unpacklo_epi8(x5,_mm_set1_epi8(0));
				t6 = _mm_unpacklo_epi8(x6,_mm_set1_epi8(0));
				t7 = _mm_unpacklo_epi8(x7,_mm_set1_epi8(0));
				t8 = _mm_unpacklo_epi8(x8,_mm_set1_epi8(0));

				x1 = _mm_unpackhi_epi8(x1,_mm_set1_epi8(0));
				x2 = _mm_unpackhi_epi8(x2,_mm_set1_epi8(0));
				x3 = _mm_unpackhi_epi8(x3,_mm_set1_epi8(0));
				x4 = _mm_unpackhi_epi8(x4,_mm_set1_epi8(0));
				x5 = _mm_unpackhi_epi8(x5,_mm_set1_epi8(0));
				x6 = _mm_unpackhi_epi8(x6,_mm_set1_epi8(0));
				x7 = _mm_unpackhi_epi8(x7,_mm_set1_epi8(0));
				x8 = _mm_unpackhi_epi8(x8,_mm_set1_epi8(0));

				/* multiply by correct value : */
				r0= _mm_mullo_epi16(t1,_mm_set1_epi16(_mm_extract_epi16(r1,0))) ;
				r2= _mm_mullo_epi16(x1,_mm_set1_epi16(_mm_extract_epi16(r1,0))) ;
				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t2,_mm_set1_epi16(_mm_extract_epi16(r1,1)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x2,_mm_set1_epi16(_mm_extract_epi16(r1,1)))) ;
				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t3,_mm_set1_epi16(_mm_extract_epi16(r1,2)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x3,_mm_set1_epi16(_mm_extract_epi16(r1,2)))) ;

				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t4,_mm_set1_epi16(_mm_extract_epi16(r1,3)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x4,_mm_set1_epi16(_mm_extract_epi16(r1,3)))) ;

				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t5,_mm_set1_epi16(_mm_extract_epi16(r1,4)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x5,_mm_set1_epi16(_mm_extract_epi16(r1,4)))) ;

				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t6,_mm_set1_epi16(_mm_extract_epi16(r1,5)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x6,_mm_set1_epi16(_mm_extract_epi16(r1,5)))) ;

				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t7,_mm_set1_epi16(_mm_extract_epi16(r1,6)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x7,_mm_set1_epi16(_mm_extract_epi16(r1,6)))) ;

				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t8,_mm_set1_epi16(_mm_extract_epi16(r1,7)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(r1,7)))) ;



				/* give results back            */
				_mm_store_si128((__m128i*)&dst[x], _mm_srli_epi16(r0,BIT_DEPTH -8));
				_mm_store_si128((__m128i*)&dst[x+8], _mm_srli_epi16(r2,BIT_DEPTH -8));
			}
			src += srcstride;
			dst += dststride;
		}
}
void ff_hevc_put_hevc_qpel_v_3_sse(int16_t *dst, ptrdiff_t dststride,
		uint8_t *_src, ptrdiff_t _srcstride,
		int width, int height, int16_t* mcbuffer)
{
	int x, y;
	uint8_t *src = (uint8_t*)_src;
	ptrdiff_t srcstride = _srcstride/sizeof(uint8_t);
	__m128i x1,x2,x3,x4,x5,x6,x7,x8, r0, r1, r2;
	__m128i t1,t2,t3,t4,t5,t6,t7,t8;
	r1= _mm_set_epi16(-1 ,4 ,-10 ,58 ,17 ,-5 ,1 ,0);
	/* case width = 4 */
	if(width == 4){        x=0;
	for (y = 0; y < height; y+=2)  {
		r0= _mm_set1_epi16(0);
		/* load data in register  */
		x1= _mm_setzero_si128();
		x2= _mm_loadu_si128((__m128i*)&src[-2*srcstride]);
		x3= _mm_loadu_si128((__m128i*)&src[-srcstride]);
		x4= _mm_loadu_si128((__m128i*)&src[0]);
		x5= _mm_loadu_si128((__m128i*)&src[srcstride]);
		x6= _mm_loadu_si128((__m128i*)&src[2*srcstride]);
		x7= _mm_loadu_si128((__m128i*)&src[3*srcstride]);
		x8= _mm_loadu_si128((__m128i*)&src[x+4*srcstride]);
		src += srcstride;
		t1= _mm_setzero_si128();
		t2= _mm_loadu_si128((__m128i*)&src[-2*srcstride]);
		t3= _mm_loadu_si128((__m128i*)&src[-srcstride]);
		t4= _mm_loadu_si128((__m128i*)&src[0]);
		t5= _mm_loadu_si128((__m128i*)&src[srcstride]);
		t6= _mm_loadu_si128((__m128i*)&src[2*srcstride]);
		t7= _mm_loadu_si128((__m128i*)&src[3*srcstride]);
		t8= _mm_loadu_si128((__m128i*)&src[x+4*srcstride]);
		x1 = _mm_unpacklo_epi8(x1,_mm_set1_epi8(0));
		x2 = _mm_unpacklo_epi8(x2,_mm_set1_epi8(0));
		x3 = _mm_unpacklo_epi8(x3,_mm_set1_epi8(0));
		x4 = _mm_unpacklo_epi8(x4,_mm_set1_epi8(0));
		x5 = _mm_unpacklo_epi8(x5,_mm_set1_epi8(0));
		x6 = _mm_unpacklo_epi8(x6,_mm_set1_epi8(0));
		x7 = _mm_unpacklo_epi8(x7,_mm_set1_epi8(0));
		x8 = _mm_unpacklo_epi8(x8,_mm_set1_epi8(0));
		t1 = _mm_unpacklo_epi8(t1,_mm_set1_epi8(0));
		t2 = _mm_unpacklo_epi8(t2,_mm_set1_epi8(0));
		t3 = _mm_unpacklo_epi8(t3,_mm_set1_epi8(0));
		t4 = _mm_unpacklo_epi8(t4,_mm_set1_epi8(0));
		t5 = _mm_unpacklo_epi8(t5,_mm_set1_epi8(0));
		t6 = _mm_unpacklo_epi8(t6,_mm_set1_epi8(0));
		t7 = _mm_unpacklo_epi8(t7,_mm_set1_epi8(0));
		t8 = _mm_unpacklo_epi8(t8,_mm_set1_epi8(0));


		r0= _mm_mullo_epi16(x1,_mm_set1_epi16(_mm_extract_epi16(r1,0))) ;
		r2= _mm_mullo_epi16(t1,_mm_set1_epi16(_mm_extract_epi16(r1,0))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x2,_mm_set1_epi16(_mm_extract_epi16(r1,1)))) ;
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t2,_mm_set1_epi16(_mm_extract_epi16(r1,1)))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x3,_mm_set1_epi16(_mm_extract_epi16(r1,2)))) ;
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t3,_mm_set1_epi16(_mm_extract_epi16(r1,2)))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x4,_mm_set1_epi16(_mm_extract_epi16(r1,3)))) ;
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t4,_mm_set1_epi16(_mm_extract_epi16(r1,3)))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x5,_mm_set1_epi16(_mm_extract_epi16(r1,4)))) ;
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t5,_mm_set1_epi16(_mm_extract_epi16(r1,4)))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x6,_mm_set1_epi16(_mm_extract_epi16(r1,5)))) ;
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t6,_mm_set1_epi16(_mm_extract_epi16(r1,5)))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x7,_mm_set1_epi16(_mm_extract_epi16(r1,6)))) ;
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t7,_mm_set1_epi16(_mm_extract_epi16(r1,6)))) ;

		r0= _mm_adds_epi16(r0, _mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(r1,7))));
		r2= _mm_adds_epi16(r2, _mm_mullo_epi16(t8,_mm_set1_epi16(_mm_extract_epi16(r1,7))));

		r0= _mm_srli_epi16(r0, BIT_DEPTH - 8);
		r2= _mm_srli_epi16(r2, BIT_DEPTH - 8);
		/* give results back            */
		_mm_storel_epi64((__m128i*)&dst[0],r0);
		dst += dststride;
		_mm_storel_epi64((__m128i*)&dst[0],r2);
		src += srcstride;
		dst += dststride;
	}

	}else      /*case width >=8 */
		for (y = 0; y < height; y++)  {
			for (x = 0; x < width; x+=16)  {
				/* check if memory needs to be reloaded */
				x1= _mm_setzero_si128();
				x2= _mm_loadu_si128((__m128i*)&src[x-2*srcstride]);
				x3= _mm_loadu_si128((__m128i*)&src[x-srcstride]);
				x4= _mm_loadu_si128((__m128i*)&src[x]);
				x5= _mm_loadu_si128((__m128i*)&src[x+srcstride]);
				x6= _mm_loadu_si128((__m128i*)&src[x+2*srcstride]);
				x7= _mm_loadu_si128((__m128i*)&src[x+3*srcstride]);
				x8= _mm_loadu_si128((__m128i*)&src[x+4*srcstride]);

				t1 = _mm_unpacklo_epi8(x1,_mm_set1_epi8(0));
				t2 = _mm_unpacklo_epi8(x2,_mm_set1_epi8(0));
				t3 = _mm_unpacklo_epi8(x3,_mm_set1_epi8(0));
				t4 = _mm_unpacklo_epi8(x4,_mm_set1_epi8(0));
				t5 = _mm_unpacklo_epi8(x5,_mm_set1_epi8(0));
				t6 = _mm_unpacklo_epi8(x6,_mm_set1_epi8(0));
				t7 = _mm_unpacklo_epi8(x7,_mm_set1_epi8(0));
				t8 = _mm_unpacklo_epi8(x8,_mm_set1_epi8(0));

				x1 = _mm_unpackhi_epi8(x1,_mm_set1_epi8(0));
				x2 = _mm_unpackhi_epi8(x2,_mm_set1_epi8(0));
				x3 = _mm_unpackhi_epi8(x3,_mm_set1_epi8(0));
				x4 = _mm_unpackhi_epi8(x4,_mm_set1_epi8(0));
				x5 = _mm_unpackhi_epi8(x5,_mm_set1_epi8(0));
				x6 = _mm_unpackhi_epi8(x6,_mm_set1_epi8(0));
				x7 = _mm_unpackhi_epi8(x7,_mm_set1_epi8(0));
				x8 = _mm_unpackhi_epi8(x8,_mm_set1_epi8(0));

				/* multiply by correct value : */
				r0= _mm_mullo_epi16(t1,_mm_set1_epi16(_mm_extract_epi16(r1,0))) ;
				r2= _mm_mullo_epi16(x1,_mm_set1_epi16(_mm_extract_epi16(r1,0))) ;
				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t2,_mm_set1_epi16(_mm_extract_epi16(r1,1)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x2,_mm_set1_epi16(_mm_extract_epi16(r1,1)))) ;
				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t3,_mm_set1_epi16(_mm_extract_epi16(r1,2)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x3,_mm_set1_epi16(_mm_extract_epi16(r1,2)))) ;

				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t4,_mm_set1_epi16(_mm_extract_epi16(r1,3)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x4,_mm_set1_epi16(_mm_extract_epi16(r1,3)))) ;

				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t5,_mm_set1_epi16(_mm_extract_epi16(r1,4)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x5,_mm_set1_epi16(_mm_extract_epi16(r1,4)))) ;

				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t6,_mm_set1_epi16(_mm_extract_epi16(r1,5)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x6,_mm_set1_epi16(_mm_extract_epi16(r1,5)))) ;

				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t7,_mm_set1_epi16(_mm_extract_epi16(r1,6)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x7,_mm_set1_epi16(_mm_extract_epi16(r1,6)))) ;

				r0= _mm_adds_epi16(r0, _mm_mullo_epi16(t8,_mm_set1_epi16(_mm_extract_epi16(r1,7)))) ;
				r2= _mm_adds_epi16(r2, _mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(r1,7)))) ;



				/* give results back            */
				_mm_store_si128((__m128i*)&dst[x], _mm_srli_epi16(r0,BIT_DEPTH -8));
				_mm_store_si128((__m128i*)&dst[x+8], _mm_srli_epi16(r2,BIT_DEPTH -8));
			}
			src += srcstride;
			dst += dststride;
		}
}

void ff_hevc_put_hevc_qpel_h_1_v_1_sse(int16_t *dst, ptrdiff_t dststride,
uint8_t *_src, ptrdiff_t _srcstride,
int width, int height, int16_t* mcbuffer)
{
int x, y, temp1;
uint8_t* src = (uint8_t*)_src;
ptrdiff_t srcstride = _srcstride/sizeof(uint8_t);
/*DECLARE_ALIGNED( 16, int16_t, tmp_array[(MAX_PB_SIZE+7)*MAX_PB_SIZE] );*/
/* int16_t tmp_array[(MAX_PB_SIZE+7)*MAX_PB_SIZE];     */
int16_t *tmp = mcbuffer;
__m128i x1,x2,x3,x4,x5,x6,x7,x8, rBuffer, rTemp, r0, r1, r2;
__m128i t1,t2,t3,t4,t5,t6,t7,t8;

src -= qpel_extra_before[1] * srcstride;
r0= _mm_set_epi8(0 ,1 ,-5 ,17 ,58 ,-10 ,4 ,-1 ,0 ,1 ,-5 ,17 ,58 ,-10 ,4 ,-1);

/* LOAD src from memory to registers to limit memory bandwidth */
if(width == 4){

for (y = 0; y < height + qpel_extra[1]; y+=2) {
/* load data in register     */
    x1= _mm_loadu_si128((__m128i*)&src[-3]);
    src += srcstride;
    t1= _mm_loadu_si128((__m128i*)&src[-3]);
    x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
    t2= _mm_unpacklo_epi64(t1,_mm_srli_si128(t1,1));
    x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
    t3= _mm_unpacklo_epi64(_mm_srli_si128(t1,2),_mm_srli_si128(t1,3));

/*  PMADDUBSW then PMADDW     */
    x2= _mm_maddubs_epi16(x2,r0);
        t2= _mm_maddubs_epi16(t2,r0);
    x3= _mm_maddubs_epi16(x3,r0);
        t3= _mm_maddubs_epi16(t3,r0);
    x2= _mm_hadd_epi16(x2,x3);
        t2= _mm_hadd_epi16(t2,t3);
    x2= _mm_hadd_epi16(x2,_mm_set1_epi16(0));
        t2= _mm_hadd_epi16(t2,_mm_set1_epi16(0));
    x2= _mm_srli_epi16(x2, BIT_DEPTH - 8);
        t2= _mm_srli_epi16(t2, BIT_DEPTH - 8);
/* give results back            */
    _mm_storel_epi64((__m128i*)&tmp[0], x2);

    tmp += MAX_PB_SIZE;
    _mm_storel_epi64((__m128i*)&tmp[0], t2);

src += srcstride;
tmp += MAX_PB_SIZE;
}
}else
for (y = 0; y < height + qpel_extra[1]; y++) {
for (x = 0; x < width; x+=8)  {
/* load data in register     */
x1= _mm_loadu_si128((__m128i*)&src[x-3]);
x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
x4= _mm_unpacklo_epi64(_mm_srli_si128(x1,4),_mm_srli_si128(x1,5));
x5= _mm_unpacklo_epi64(_mm_srli_si128(x1,6),_mm_srli_si128(x1,7));

/*  PMADDUBSW then PMADDW     */
x2= _mm_maddubs_epi16(x2,r0);
x3= _mm_maddubs_epi16(x3,r0);
x4= _mm_maddubs_epi16(x4,r0);
x5= _mm_maddubs_epi16(x5,r0);
x2= _mm_hadd_epi16(x2,x3);
x4= _mm_hadd_epi16(x4,x5);
x2= _mm_hadd_epi16(x2,x4);
    x2= _mm_srli_si128(x2,BIT_DEPTH -8);

/* give results back            */
_mm_store_si128((__m128i*)&tmp[x], x2);

}
src += srcstride;
tmp += MAX_PB_SIZE;
}


tmp = mcbuffer + qpel_extra_before[1] * MAX_PB_SIZE;
    srcstride= MAX_PB_SIZE;

/* vertical treatment on temp table : tmp contains 16 bit values, so need to use 32 bit  integers
for register calculations */
rTemp= _mm_set_epi16(0 ,1 ,-5 ,17 ,58 ,-10 ,4 ,-1);
for (y = 0; y < height; y++)  {
for (x = 0; x < width; x+=8)  {

x1= _mm_load_si128((__m128i*)&tmp[x-3*srcstride]);
x2= _mm_load_si128((__m128i*)&tmp[x-2*srcstride]);
x3= _mm_load_si128((__m128i*)&tmp[x-srcstride]);
x4= _mm_load_si128((__m128i*)&tmp[x]);
x5= _mm_load_si128((__m128i*)&tmp[x+srcstride]);
x6= _mm_load_si128((__m128i*)&tmp[x+2*srcstride]);
x7= _mm_load_si128((__m128i*)&tmp[x+3*srcstride]);
x8= _mm_setzero_si128();

    r0= _mm_set1_epi16(_mm_extract_epi16(rTemp,0));
    r1= _mm_set1_epi16(_mm_extract_epi16(rTemp,1));
    t8= _mm_mullo_epi16(x1,r0);
    rBuffer= _mm_mulhi_epi16(x1,r0);
        t7= _mm_mullo_epi16(x2,r1);
    t1 = _mm_unpacklo_epi16(t8,rBuffer);
    x1 = _mm_unpackhi_epi16(t8,rBuffer);


    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,2));
    rBuffer= _mm_mulhi_epi16(x2,r1);
    t8= _mm_mullo_epi16(x3,r0);
    t2 = _mm_unpacklo_epi16(t7,rBuffer);
    x2 = _mm_unpackhi_epi16(t7,rBuffer);

    r1=_mm_set1_epi16(_mm_extract_epi16(rTemp,3));
    rBuffer= _mm_mulhi_epi16(x3,r0);
    t7= _mm_mullo_epi16(x4,r1);
t3 = _mm_unpacklo_epi16(t8,rBuffer);
x3 = _mm_unpackhi_epi16(t8,rBuffer);

    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,4));
rBuffer= _mm_mulhi_epi16(x4,r1);
    t8= _mm_mullo_epi16(x5,r0);
t4 = _mm_unpacklo_epi16(t7,rBuffer);
x4 = _mm_unpackhi_epi16(t7,rBuffer);

    r1=_mm_set1_epi16(_mm_extract_epi16(rTemp,5));
rBuffer= _mm_mulhi_epi16(x5,r0);
    t7= _mm_mullo_epi16(x6,r1);
t5 = _mm_unpacklo_epi16(t8,rBuffer);
x5 = _mm_unpackhi_epi16(t8,rBuffer);

    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,6));
    rBuffer= _mm_mulhi_epi16(x6,r1);
    t8= _mm_mullo_epi16(x7,r0);
    t6 = _mm_unpacklo_epi16(t7,rBuffer);
    x6 = _mm_unpackhi_epi16(t7,rBuffer);

    rBuffer= _mm_mulhi_epi16(x7,r0);
    t7 = _mm_unpacklo_epi16(t8,rBuffer);
    x7 = _mm_unpackhi_epi16(t8,rBuffer);

    t8 = _mm_unpacklo_epi16(_mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))),_mm_mulhi_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))));
    x8 = _mm_unpackhi_epi16(_mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))),_mm_mulhi_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))));


/* add calculus by correct value : */

    r1= _mm_add_epi32(x1,x2);
    x3= _mm_add_epi32(x3,x4);
    x5= _mm_add_epi32(x5,x6);
    r1= _mm_add_epi32(r1,x3);
    x7= _mm_add_epi32(x7,x8);
    r1= _mm_add_epi32(r1,x5);

r0= _mm_add_epi32(t1,t2);
t3= _mm_add_epi32(t3,t4);
t5= _mm_add_epi32(t5,t6);
r0= _mm_add_epi32(r0,t3);
t7= _mm_add_epi32(t7,t8);
r0= _mm_add_epi32(r0,t5);
r1= _mm_add_epi32(r1,x7);
r0= _mm_add_epi32(r0,t7);
r1= _mm_srli_epi32(r1,6);
r0= _mm_srli_epi32(r0,6);

    r1= _mm_and_si128(r1,_mm_set_epi16(0,65535,0,65535,0,65535,0,65535));
    r0= _mm_and_si128(r0,_mm_set_epi16(0,65535,0,65535,0,65535,0,65535));
    r0= _mm_hadd_epi16(r0,r1);
    _mm_store_si128((__m128i*)&dst[x],r0);

}
tmp += MAX_PB_SIZE;
dst += dststride;
}
}
void ff_hevc_put_hevc_qpel_h_1_v_2_sse(int16_t *dst, ptrdiff_t dststride,
uint8_t *_src, ptrdiff_t _srcstride,
int width, int height, int16_t* mcbuffer)
{
int x, y, temp1;
uint8_t *src = (uint8_t*)_src;
ptrdiff_t srcstride = _srcstride/sizeof(uint8_t);
/*DECLARE_ALIGNED( 16, int16_t, tmp_array[(MAX_PB_SIZE+7)*MAX_PB_SIZE] );*/
/* int16_t tmp_array[(MAX_PB_SIZE+7)*MAX_PB_SIZE];     */
int16_t *tmp = mcbuffer;
__m128i x1,x2,x3,x4,x5,x6,x7,x8, rBuffer, rTemp, r0, r1, r2;
__m128i t1,t2,t3,t4,t5,t6,t7,t8;

src -= qpel_extra_before[2] * srcstride;
r0= _mm_set_epi8(0 ,1 ,-5 ,17 ,58 ,-10 ,4 ,-1 ,0 ,1 ,-5 ,17 ,58 ,-10 ,4 ,-1);

/* LOAD src from memory to registers to limit memory bandwidth */
if(width == 4){

for (y = 0; y < height + qpel_extra[2]; y+=2) {
/* load data in register     */
    x1= _mm_loadu_si128((__m128i*)&src[-3]);
    src += srcstride;
    t1= _mm_loadu_si128((__m128i*)&src[-3]);
    x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
    t2= _mm_unpacklo_epi64(t1,_mm_srli_si128(t1,1));
    x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
    t3= _mm_unpacklo_epi64(_mm_srli_si128(t1,2),_mm_srli_si128(t1,3));

/*  PMADDUBSW then PMADDW     */
    x2= _mm_maddubs_epi16(x2,r0);
        t2= _mm_maddubs_epi16(t2,r0);
    x3= _mm_maddubs_epi16(x3,r0);
        t3= _mm_maddubs_epi16(t3,r0);
    x2= _mm_hadd_epi16(x2,x3);
        t2= _mm_hadd_epi16(t2,t3);
    x2= _mm_hadd_epi16(x2,_mm_set1_epi16(0));
        t2= _mm_hadd_epi16(t2,_mm_set1_epi16(0));
    x2= _mm_srli_epi16(x2, BIT_DEPTH - 8);
        t2= _mm_srli_epi16(t2, BIT_DEPTH - 8);
/* give results back            */
    _mm_storel_epi64((__m128i*)&tmp[0], x2);

    tmp += MAX_PB_SIZE;
    _mm_storel_epi64((__m128i*)&tmp[0], t2);

src += srcstride;
tmp += MAX_PB_SIZE;
}
}else
for (y = 0; y < height + qpel_extra[2]; y++) {
for (x = 0; x < width; x+=8)  {
/* load data in register     */
x1= _mm_loadu_si128((__m128i*)&src[x-3]);
x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
x4= _mm_unpacklo_epi64(_mm_srli_si128(x1,4),_mm_srli_si128(x1,5));
x5= _mm_unpacklo_epi64(_mm_srli_si128(x1,6),_mm_srli_si128(x1,7));

/*  PMADDUBSW then PMADDW     */
x2= _mm_maddubs_epi16(x2,r0);
x3= _mm_maddubs_epi16(x3,r0);
x4= _mm_maddubs_epi16(x4,r0);
x5= _mm_maddubs_epi16(x5,r0);
x2= _mm_hadd_epi16(x2,x3);
x4= _mm_hadd_epi16(x4,x5);
x2= _mm_hadd_epi16(x2,x4);
    x2= _mm_srli_si128(x2,BIT_DEPTH -8);

/* give results back            */
_mm_store_si128((__m128i*)&tmp[x], x2);

}
src += srcstride;
tmp += MAX_PB_SIZE;
}


tmp = mcbuffer + qpel_extra_before[2] * MAX_PB_SIZE;
    srcstride= MAX_PB_SIZE;

/* vertical treatment on temp table : tmp contains 16 bit values, so need to use 32 bit  integers
for register calculations */
rTemp= _mm_set_epi16(-1 ,4 ,-11 ,40 ,40 ,-11 ,4 ,-1);
for (y = 0; y < height; y++)  {
for (x = 0; x < width; x+=8)  {

x1= _mm_load_si128((__m128i*)&tmp[x-3*srcstride]);
x2= _mm_load_si128((__m128i*)&tmp[x-2*srcstride]);
x3= _mm_load_si128((__m128i*)&tmp[x-srcstride]);
x4= _mm_load_si128((__m128i*)&tmp[x]);
x5= _mm_load_si128((__m128i*)&tmp[x+srcstride]);
x6= _mm_load_si128((__m128i*)&tmp[x+2*srcstride]);
x7= _mm_load_si128((__m128i*)&tmp[x+3*srcstride]);
x8= _mm_loadu_si128((__m128i*)&src[x+4*srcstride]);

    r0= _mm_set1_epi16(_mm_extract_epi16(rTemp,0));
    r1= _mm_set1_epi16(_mm_extract_epi16(rTemp,1));
    t8= _mm_mullo_epi16(x1,r0);
    rBuffer= _mm_mulhi_epi16(x1,r0);
        t7= _mm_mullo_epi16(x2,r1);
    t1 = _mm_unpacklo_epi16(t8,rBuffer);
    x1 = _mm_unpackhi_epi16(t8,rBuffer);


    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,2));
    rBuffer= _mm_mulhi_epi16(x2,r1);
    t8= _mm_mullo_epi16(x3,r0);
    t2 = _mm_unpacklo_epi16(t7,rBuffer);
    x2 = _mm_unpackhi_epi16(t7,rBuffer);

    r1=_mm_set1_epi16(_mm_extract_epi16(rTemp,3));
    rBuffer= _mm_mulhi_epi16(x3,r0);
    t7= _mm_mullo_epi16(x4,r1);
t3 = _mm_unpacklo_epi16(t8,rBuffer);
x3 = _mm_unpackhi_epi16(t8,rBuffer);

    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,4));
rBuffer= _mm_mulhi_epi16(x4,r1);
    t8= _mm_mullo_epi16(x5,r0);
t4 = _mm_unpacklo_epi16(t7,rBuffer);
x4 = _mm_unpackhi_epi16(t7,rBuffer);

    r1=_mm_set1_epi16(_mm_extract_epi16(rTemp,5));
rBuffer= _mm_mulhi_epi16(x5,r0);
    t7= _mm_mullo_epi16(x6,r1);
t5 = _mm_unpacklo_epi16(t8,rBuffer);
x5 = _mm_unpackhi_epi16(t8,rBuffer);

    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,6));
    rBuffer= _mm_mulhi_epi16(x6,r1);
    t8= _mm_mullo_epi16(x7,r0);
    t6 = _mm_unpacklo_epi16(t7,rBuffer);
    x6 = _mm_unpackhi_epi16(t7,rBuffer);

    rBuffer= _mm_mulhi_epi16(x7,r0);
    t7 = _mm_unpacklo_epi16(t8,rBuffer);
    x7 = _mm_unpackhi_epi16(t8,rBuffer);

    t8 = _mm_unpacklo_epi16(_mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))),_mm_mulhi_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))));
    x8 = _mm_unpackhi_epi16(_mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))),_mm_mulhi_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))));


/* add calculus by correct value : */

    r1= _mm_add_epi32(x1,x2);
    x3= _mm_add_epi32(x3,x4);
    x5= _mm_add_epi32(x5,x6);
    r1= _mm_add_epi32(r1,x3);
    x7= _mm_add_epi32(x7,x8);
    r1= _mm_add_epi32(r1,x5);

r0= _mm_add_epi32(t1,t2);
t3= _mm_add_epi32(t3,t4);
t5= _mm_add_epi32(t5,t6);
r0= _mm_add_epi32(r0,t3);
t7= _mm_add_epi32(t7,t8);
r0= _mm_add_epi32(r0,t5);
r1= _mm_add_epi32(r1,x7);
r0= _mm_add_epi32(r0,t7);
r1= _mm_srli_epi32(r1,6);
r0= _mm_srli_epi32(r0,6);

    r1= _mm_and_si128(r1,_mm_set_epi16(0,65535,0,65535,0,65535,0,65535));
    r0= _mm_and_si128(r0,_mm_set_epi16(0,65535,0,65535,0,65535,0,65535));
    r0= _mm_hadd_epi16(r0,r1);
    _mm_store_si128((__m128i*)&dst[x],r0);

}
tmp += MAX_PB_SIZE;
dst += dststride;
}
}
void ff_hevc_put_hevc_qpel_h_1_v_3_sse(int16_t *dst, ptrdiff_t dststride,
uint8_t *_src, ptrdiff_t _srcstride,
int width, int height, int16_t* mcbuffer)
{
int x, y, temp1;
uint8_t *src = (uint8_t*)_src;
ptrdiff_t srcstride = _srcstride/sizeof(uint8_t);
/*DECLARE_ALIGNED( 16, int16_t, tmp_array[(MAX_PB_SIZE+7)*MAX_PB_SIZE] );*/
/* int16_t tmp_array[(MAX_PB_SIZE+7)*MAX_PB_SIZE];     */
int16_t *tmp = mcbuffer;
__m128i x1,x2,x3,x4,x5,x6,x7,x8, rBuffer, rTemp, r0, r1, r2;
__m128i t1,t2,t3,t4,t5,t6,t7,t8;

src -= qpel_extra_before[3] * srcstride;
r0= _mm_set_epi8(0 ,1 ,-5 ,17 ,58 ,-10 ,4 ,-1 ,0 ,1 ,-5 ,17 ,58 ,-10 ,4 ,-1);

/* LOAD src from memory to registers to limit memory bandwidth */
if(width == 4){

for (y = 0; y < height + qpel_extra[3]; y+=2) {
/* load data in register     */
    x1= _mm_loadu_si128((__m128i*)&src[-3]);
    src += srcstride;
    t1= _mm_loadu_si128((__m128i*)&src[-3]);
    x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
    t2= _mm_unpacklo_epi64(t1,_mm_srli_si128(t1,1));
    x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
    t3= _mm_unpacklo_epi64(_mm_srli_si128(t1,2),_mm_srli_si128(t1,3));

/*  PMADDUBSW then PMADDW     */
    x2= _mm_maddubs_epi16(x2,r0);
        t2= _mm_maddubs_epi16(t2,r0);
    x3= _mm_maddubs_epi16(x3,r0);
        t3= _mm_maddubs_epi16(t3,r0);
    x2= _mm_hadd_epi16(x2,x3);
        t2= _mm_hadd_epi16(t2,t3);
    x2= _mm_hadd_epi16(x2,_mm_set1_epi16(0));
        t2= _mm_hadd_epi16(t2,_mm_set1_epi16(0));
    x2= _mm_srli_epi16(x2, BIT_DEPTH - 8);
        t2= _mm_srli_epi16(t2, BIT_DEPTH - 8);
/* give results back            */
    _mm_storel_epi64((__m128i*)&tmp[0], x2);

    tmp += MAX_PB_SIZE;
    _mm_storel_epi64((__m128i*)&tmp[0], t2);

src += srcstride;
tmp += MAX_PB_SIZE;
}
}else
for (y = 0; y < height + qpel_extra[3]; y++) {
for (x = 0; x < width; x+=8)  {
/* load data in register     */
x1= _mm_loadu_si128((__m128i*)&src[x-3]);
x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
x4= _mm_unpacklo_epi64(_mm_srli_si128(x1,4),_mm_srli_si128(x1,5));
x5= _mm_unpacklo_epi64(_mm_srli_si128(x1,6),_mm_srli_si128(x1,7));

/*  PMADDUBSW then PMADDW     */
x2= _mm_maddubs_epi16(x2,r0);
x3= _mm_maddubs_epi16(x3,r0);
x4= _mm_maddubs_epi16(x4,r0);
x5= _mm_maddubs_epi16(x5,r0);
x2= _mm_hadd_epi16(x2,x3);
x4= _mm_hadd_epi16(x4,x5);
x2= _mm_hadd_epi16(x2,x4);
    x2= _mm_srli_si128(x2,BIT_DEPTH -8);

/* give results back            */
_mm_store_si128((__m128i*)&tmp[x], x2);

}
src += srcstride;
tmp += MAX_PB_SIZE;
}


tmp = mcbuffer + qpel_extra_before[3] * MAX_PB_SIZE;
    srcstride= MAX_PB_SIZE;

/* vertical treatment on temp table : tmp contains 16 bit values, so need to use 32 bit  integers
for register calculations */
rTemp= _mm_set_epi16(-1 ,4 ,-10 ,58 ,17 ,-5 ,1 ,0);
for (y = 0; y < height; y++)  {
for (x = 0; x < width; x+=8)  {

x1= _mm_setzero_si128();
x2= _mm_load_si128((__m128i*)&tmp[x-2*srcstride]);
x3= _mm_load_si128((__m128i*)&tmp[x-srcstride]);
x4= _mm_load_si128((__m128i*)&tmp[x]);
x5= _mm_load_si128((__m128i*)&tmp[x+srcstride]);
x6= _mm_load_si128((__m128i*)&tmp[x+2*srcstride]);
x7= _mm_load_si128((__m128i*)&tmp[x+3*srcstride]);
x8= _mm_load_si128((__m128i*)&tmp[x+4*srcstride]);

    r0= _mm_set1_epi16(_mm_extract_epi16(rTemp,0));
    r1= _mm_set1_epi16(_mm_extract_epi16(rTemp,1));
    t8= _mm_mullo_epi16(x1,r0);
    rBuffer= _mm_mulhi_epi16(x1,r0);
        t7= _mm_mullo_epi16(x2,r1);
    t1 = _mm_unpacklo_epi16(t8,rBuffer);
    x1 = _mm_unpackhi_epi16(t8,rBuffer);


    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,2));
    rBuffer= _mm_mulhi_epi16(x2,r1);
    t8= _mm_mullo_epi16(x3,r0);
    t2 = _mm_unpacklo_epi16(t7,rBuffer);
    x2 = _mm_unpackhi_epi16(t7,rBuffer);

    r1=_mm_set1_epi16(_mm_extract_epi16(rTemp,3));
    rBuffer= _mm_mulhi_epi16(x3,r0);
    t7= _mm_mullo_epi16(x4,r1);
t3 = _mm_unpacklo_epi16(t8,rBuffer);
x3 = _mm_unpackhi_epi16(t8,rBuffer);

    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,4));
rBuffer= _mm_mulhi_epi16(x4,r1);
    t8= _mm_mullo_epi16(x5,r0);
t4 = _mm_unpacklo_epi16(t7,rBuffer);
x4 = _mm_unpackhi_epi16(t7,rBuffer);

    r1=_mm_set1_epi16(_mm_extract_epi16(rTemp,5));
rBuffer= _mm_mulhi_epi16(x5,r0);
    t7= _mm_mullo_epi16(x6,r1);
t5 = _mm_unpacklo_epi16(t8,rBuffer);
x5 = _mm_unpackhi_epi16(t8,rBuffer);

    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,6));
    rBuffer= _mm_mulhi_epi16(x6,r1);
    t8= _mm_mullo_epi16(x7,r0);
    t6 = _mm_unpacklo_epi16(t7,rBuffer);
    x6 = _mm_unpackhi_epi16(t7,rBuffer);

    rBuffer= _mm_mulhi_epi16(x7,r0);
    t7 = _mm_unpacklo_epi16(t8,rBuffer);
    x7 = _mm_unpackhi_epi16(t8,rBuffer);

    t8 = _mm_unpacklo_epi16(_mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))),_mm_mulhi_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))));
    x8 = _mm_unpackhi_epi16(_mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))),_mm_mulhi_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))));


/* add calculus by correct value : */

    r1= _mm_add_epi32(x1,x2);
    x3= _mm_add_epi32(x3,x4);
    x5= _mm_add_epi32(x5,x6);
    r1= _mm_add_epi32(r1,x3);
    x7= _mm_add_epi32(x7,x8);
    r1= _mm_add_epi32(r1,x5);

r0= _mm_add_epi32(t1,t2);
t3= _mm_add_epi32(t3,t4);
t5= _mm_add_epi32(t5,t6);
r0= _mm_add_epi32(r0,t3);
t7= _mm_add_epi32(t7,t8);
r0= _mm_add_epi32(r0,t5);
r1= _mm_add_epi32(r1,x7);
r0= _mm_add_epi32(r0,t7);
r1= _mm_srli_epi32(r1,6);
r0= _mm_srli_epi32(r0,6);

    r1= _mm_and_si128(r1,_mm_set_epi16(0,65535,0,65535,0,65535,0,65535));
    r0= _mm_and_si128(r0,_mm_set_epi16(0,65535,0,65535,0,65535,0,65535));
    r0= _mm_hadd_epi16(r0,r1);
    _mm_store_si128((__m128i*)&dst[x],r0);

}
tmp += MAX_PB_SIZE;
dst += dststride;
}
}
void ff_hevc_put_hevc_qpel_h_2_v_1_sse(int16_t *dst, ptrdiff_t dststride,
uint8_t *_src, ptrdiff_t _srcstride,
int width, int height, int16_t* mcbuffer)
{
int x, y, temp1;
uint8_t *src = (uint8_t*)_src;
ptrdiff_t srcstride = _srcstride/sizeof(uint8_t);
/*DECLARE_ALIGNED( 16, int16_t, tmp_array[(MAX_PB_SIZE+7)*MAX_PB_SIZE] );*/
/* int16_t tmp_array[(MAX_PB_SIZE+7)*MAX_PB_SIZE];     */
int16_t *tmp = mcbuffer;
__m128i x1,x2,x3,x4,x5,x6,x7,x8, rBuffer, rTemp, r0, r1, r2;
__m128i t1,t2,t3,t4,t5,t6,t7,t8;

src -= qpel_extra_before[1] * srcstride;
r0= _mm_set_epi8(-1 ,4 ,-11 ,40 ,40 ,-11 ,4 ,-1 ,-1 ,4 ,-11 ,40 ,40 ,-11 ,4 ,-1);

/* LOAD src from memory to registers to limit memory bandwidth */
if(width == 4){

for (y = 0; y < height + qpel_extra[1]; y+=2) {
/* load data in register     */
    x1= _mm_loadu_si128((__m128i*)&src[-3]);
    src += srcstride;
    t1= _mm_loadu_si128((__m128i*)&src[-3]);
    x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
    t2= _mm_unpacklo_epi64(t1,_mm_srli_si128(t1,1));
    x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
    t3= _mm_unpacklo_epi64(_mm_srli_si128(t1,2),_mm_srli_si128(t1,3));

/*  PMADDUBSW then PMADDW     */
    x2= _mm_maddubs_epi16(x2,r0);
        t2= _mm_maddubs_epi16(t2,r0);
    x3= _mm_maddubs_epi16(x3,r0);
        t3= _mm_maddubs_epi16(t3,r0);
    x2= _mm_hadd_epi16(x2,x3);
        t2= _mm_hadd_epi16(t2,t3);
    x2= _mm_hadd_epi16(x2,_mm_set1_epi16(0));
        t2= _mm_hadd_epi16(t2,_mm_set1_epi16(0));
    x2= _mm_srli_epi16(x2, BIT_DEPTH - 8);
        t2= _mm_srli_epi16(t2, BIT_DEPTH - 8);
/* give results back            */
    _mm_storel_epi64((__m128i*)&tmp[0], x2);

    tmp += MAX_PB_SIZE;
    _mm_storel_epi64((__m128i*)&tmp[0], t2);

src += srcstride;
tmp += MAX_PB_SIZE;
}
}else
for (y = 0; y < height + qpel_extra[1]; y++) {
for (x = 0; x < width; x+=8)  {
/* load data in register     */
x1= _mm_loadu_si128((__m128i*)&src[x-3]);
x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
x4= _mm_unpacklo_epi64(_mm_srli_si128(x1,4),_mm_srli_si128(x1,5));
x5= _mm_unpacklo_epi64(_mm_srli_si128(x1,6),_mm_srli_si128(x1,7));

/*  PMADDUBSW then PMADDW     */
x2= _mm_maddubs_epi16(x2,r0);
x3= _mm_maddubs_epi16(x3,r0);
x4= _mm_maddubs_epi16(x4,r0);
x5= _mm_maddubs_epi16(x5,r0);
x2= _mm_hadd_epi16(x2,x3);
x4= _mm_hadd_epi16(x4,x5);
x2= _mm_hadd_epi16(x2,x4);
    x2= _mm_srli_si128(x2,BIT_DEPTH -8);

/* give results back            */
_mm_store_si128((__m128i*)&tmp[x], x2);

}
src += srcstride;
tmp += MAX_PB_SIZE;
}


tmp = mcbuffer + qpel_extra_before[1] * MAX_PB_SIZE;
    srcstride= MAX_PB_SIZE;

/* vertical treatment on temp table : tmp contains 16 bit values, so need to use 32 bit  integers
for register calculations */
rTemp= _mm_set_epi16(0 ,1 ,-5 ,17 ,58 ,-10 ,4 ,-1);
for (y = 0; y < height; y++)  {
for (x = 0; x < width; x+=8)  {

x1= _mm_load_si128((__m128i*)&tmp[x-3*srcstride]);
x2= _mm_load_si128((__m128i*)&tmp[x-2*srcstride]);
x3= _mm_load_si128((__m128i*)&tmp[x-srcstride]);
x4= _mm_load_si128((__m128i*)&tmp[x]);
x5= _mm_load_si128((__m128i*)&tmp[x+srcstride]);
x6= _mm_load_si128((__m128i*)&tmp[x+2*srcstride]);
x7= _mm_load_si128((__m128i*)&tmp[x+3*srcstride]);
x8= _mm_setzero_si128();

    r0= _mm_set1_epi16(_mm_extract_epi16(rTemp,0));
    r1= _mm_set1_epi16(_mm_extract_epi16(rTemp,1));
    t8= _mm_mullo_epi16(x1,r0);
    rBuffer= _mm_mulhi_epi16(x1,r0);
        t7= _mm_mullo_epi16(x2,r1);
    t1 = _mm_unpacklo_epi16(t8,rBuffer);
    x1 = _mm_unpackhi_epi16(t8,rBuffer);


    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,2));
    rBuffer= _mm_mulhi_epi16(x2,r1);
    t8= _mm_mullo_epi16(x3,r0);
    t2 = _mm_unpacklo_epi16(t7,rBuffer);
    x2 = _mm_unpackhi_epi16(t7,rBuffer);

    r1=_mm_set1_epi16(_mm_extract_epi16(rTemp,3));
    rBuffer= _mm_mulhi_epi16(x3,r0);
    t7= _mm_mullo_epi16(x4,r1);
t3 = _mm_unpacklo_epi16(t8,rBuffer);
x3 = _mm_unpackhi_epi16(t8,rBuffer);

    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,4));
rBuffer= _mm_mulhi_epi16(x4,r1);
    t8= _mm_mullo_epi16(x5,r0);
t4 = _mm_unpacklo_epi16(t7,rBuffer);
x4 = _mm_unpackhi_epi16(t7,rBuffer);

    r1=_mm_set1_epi16(_mm_extract_epi16(rTemp,5));
rBuffer= _mm_mulhi_epi16(x5,r0);
    t7= _mm_mullo_epi16(x6,r1);
t5 = _mm_unpacklo_epi16(t8,rBuffer);
x5 = _mm_unpackhi_epi16(t8,rBuffer);

    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,6));
    rBuffer= _mm_mulhi_epi16(x6,r1);
    t8= _mm_mullo_epi16(x7,r0);
    t6 = _mm_unpacklo_epi16(t7,rBuffer);
    x6 = _mm_unpackhi_epi16(t7,rBuffer);

    rBuffer= _mm_mulhi_epi16(x7,r0);
    t7 = _mm_unpacklo_epi16(t8,rBuffer);
    x7 = _mm_unpackhi_epi16(t8,rBuffer);

    t8 = _mm_unpacklo_epi16(_mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))),_mm_mulhi_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))));
    x8 = _mm_unpackhi_epi16(_mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))),_mm_mulhi_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))));


/* add calculus by correct value : */

    r1= _mm_add_epi32(x1,x2);
    x3= _mm_add_epi32(x3,x4);
    x5= _mm_add_epi32(x5,x6);
    r1= _mm_add_epi32(r1,x3);
    x7= _mm_add_epi32(x7,x8);
    r1= _mm_add_epi32(r1,x5);

r0= _mm_add_epi32(t1,t2);
t3= _mm_add_epi32(t3,t4);
t5= _mm_add_epi32(t5,t6);
r0= _mm_add_epi32(r0,t3);
t7= _mm_add_epi32(t7,t8);
r0= _mm_add_epi32(r0,t5);
r1= _mm_add_epi32(r1,x7);
r0= _mm_add_epi32(r0,t7);
r1= _mm_srli_epi32(r1,6);
r0= _mm_srli_epi32(r0,6);

    r1= _mm_and_si128(r1,_mm_set_epi16(0,65535,0,65535,0,65535,0,65535));
    r0= _mm_and_si128(r0,_mm_set_epi16(0,65535,0,65535,0,65535,0,65535));
    r0= _mm_hadd_epi16(r0,r1);
    _mm_store_si128((__m128i*)&dst[x],r0);

}
tmp += MAX_PB_SIZE;
dst += dststride;
}
}
void ff_hevc_put_hevc_qpel_h_2_v_2_sse(int16_t *dst, ptrdiff_t dststride,
uint8_t *_src, ptrdiff_t _srcstride,
int width, int height, int16_t* mcbuffer)
{
int x, y, temp1;
uint8_t *src = (uint8_t*)_src;
ptrdiff_t srcstride = _srcstride/sizeof(uint8_t);
/*DECLARE_ALIGNED( 16, int16_t, tmp_array[(MAX_PB_SIZE+7)*MAX_PB_SIZE] );*/
/* int16_t tmp_array[(MAX_PB_SIZE+7)*MAX_PB_SIZE];     */
int16_t *tmp = mcbuffer;
__m128i x1,x2,x3,x4,x5,x6,x7,x8, rBuffer, rTemp, r0, r1, r2;
__m128i t1,t2,t3,t4,t5,t6,t7,t8;

src -= qpel_extra_before[2] * srcstride;
r0= _mm_set_epi8(-1 ,4 ,-11 ,40 ,40 ,-11 ,4 ,-1 ,-1 ,4 ,-11 ,40 ,40 ,-11 ,4 ,-1);

/* LOAD src from memory to registers to limit memory bandwidth */
if(width == 4){

for (y = 0; y < height + qpel_extra[2]; y+=2) {
/* load data in register     */
    x1= _mm_loadu_si128((__m128i*)&src[-3]);
    src += srcstride;
    t1= _mm_loadu_si128((__m128i*)&src[-3]);
    x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
    t2= _mm_unpacklo_epi64(t1,_mm_srli_si128(t1,1));
    x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
    t3= _mm_unpacklo_epi64(_mm_srli_si128(t1,2),_mm_srli_si128(t1,3));

/*  PMADDUBSW then PMADDW     */
    x2= _mm_maddubs_epi16(x2,r0);
        t2= _mm_maddubs_epi16(t2,r0);
    x3= _mm_maddubs_epi16(x3,r0);
        t3= _mm_maddubs_epi16(t3,r0);
    x2= _mm_hadd_epi16(x2,x3);
        t2= _mm_hadd_epi16(t2,t3);
    x2= _mm_hadd_epi16(x2,_mm_set1_epi16(0));
        t2= _mm_hadd_epi16(t2,_mm_set1_epi16(0));
    x2= _mm_srli_epi16(x2, BIT_DEPTH - 8);
        t2= _mm_srli_epi16(t2, BIT_DEPTH - 8);
/* give results back            */
    _mm_storel_epi64((__m128i*)&tmp[0], x2);

    tmp += MAX_PB_SIZE;
    _mm_storel_epi64((__m128i*)&tmp[0], t2);

src += srcstride;
tmp += MAX_PB_SIZE;
}
}else
for (y = 0; y < height + qpel_extra[2]; y++) {
for (x = 0; x < width; x+=8)  {
/* load data in register     */
x1= _mm_loadu_si128((__m128i*)&src[x-3]);
x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
x4= _mm_unpacklo_epi64(_mm_srli_si128(x1,4),_mm_srli_si128(x1,5));
x5= _mm_unpacklo_epi64(_mm_srli_si128(x1,6),_mm_srli_si128(x1,7));

/*  PMADDUBSW then PMADDW     */
x2= _mm_maddubs_epi16(x2,r0);
x3= _mm_maddubs_epi16(x3,r0);
x4= _mm_maddubs_epi16(x4,r0);
x5= _mm_maddubs_epi16(x5,r0);
x2= _mm_hadd_epi16(x2,x3);
x4= _mm_hadd_epi16(x4,x5);
x2= _mm_hadd_epi16(x2,x4);
    x2= _mm_srli_si128(x2,BIT_DEPTH -8);

/* give results back            */
_mm_store_si128((__m128i*)&tmp[x], x2);

}
src += srcstride;
tmp += MAX_PB_SIZE;
}


tmp = mcbuffer + qpel_extra_before[2] * MAX_PB_SIZE;
    srcstride= MAX_PB_SIZE;

/* vertical treatment on temp table : tmp contains 16 bit values, so need to use 32 bit  integers
for register calculations */
rTemp= _mm_set_epi16(-1 ,4 ,-11 ,40 ,40 ,-11 ,4 ,-1);
for (y = 0; y < height; y++)  {
for (x = 0; x < width; x+=8)  {

x1= _mm_load_si128((__m128i*)&tmp[x-3*srcstride]);
x2= _mm_load_si128((__m128i*)&tmp[x-2*srcstride]);
x3= _mm_load_si128((__m128i*)&tmp[x-srcstride]);
x4= _mm_load_si128((__m128i*)&tmp[x]);
x5= _mm_load_si128((__m128i*)&tmp[x+srcstride]);
x6= _mm_load_si128((__m128i*)&tmp[x+2*srcstride]);
x7= _mm_load_si128((__m128i*)&tmp[x+3*srcstride]);
x8= _mm_load_si128((__m128i*)&tmp[x+4*srcstride]);

    r0= _mm_set1_epi16(_mm_extract_epi16(rTemp,0));
    r1= _mm_set1_epi16(_mm_extract_epi16(rTemp,1));
    t8= _mm_mullo_epi16(x1,r0);
    rBuffer= _mm_mulhi_epi16(x1,r0);
        t7= _mm_mullo_epi16(x2,r1);
    t1 = _mm_unpacklo_epi16(t8,rBuffer);
    x1 = _mm_unpackhi_epi16(t8,rBuffer);


    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,2));
    rBuffer= _mm_mulhi_epi16(x2,r1);
    t8= _mm_mullo_epi16(x3,r0);
    t2 = _mm_unpacklo_epi16(t7,rBuffer);
    x2 = _mm_unpackhi_epi16(t7,rBuffer);

    r1=_mm_set1_epi16(_mm_extract_epi16(rTemp,3));
    rBuffer= _mm_mulhi_epi16(x3,r0);
    t7= _mm_mullo_epi16(x4,r1);
t3 = _mm_unpacklo_epi16(t8,rBuffer);
x3 = _mm_unpackhi_epi16(t8,rBuffer);

    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,4));
rBuffer= _mm_mulhi_epi16(x4,r1);
    t8= _mm_mullo_epi16(x5,r0);
t4 = _mm_unpacklo_epi16(t7,rBuffer);
x4 = _mm_unpackhi_epi16(t7,rBuffer);

    r1=_mm_set1_epi16(_mm_extract_epi16(rTemp,5));
rBuffer= _mm_mulhi_epi16(x5,r0);
    t7= _mm_mullo_epi16(x6,r1);
t5 = _mm_unpacklo_epi16(t8,rBuffer);
x5 = _mm_unpackhi_epi16(t8,rBuffer);

    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,6));
    rBuffer= _mm_mulhi_epi16(x6,r1);
    t8= _mm_mullo_epi16(x7,r0);
    t6 = _mm_unpacklo_epi16(t7,rBuffer);
    x6 = _mm_unpackhi_epi16(t7,rBuffer);

    rBuffer= _mm_mulhi_epi16(x7,r0);
    t7 = _mm_unpacklo_epi16(t8,rBuffer);
    x7 = _mm_unpackhi_epi16(t8,rBuffer);

    t8 = _mm_unpacklo_epi16(_mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))),_mm_mulhi_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))));
    x8 = _mm_unpackhi_epi16(_mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))),_mm_mulhi_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))));


/* add calculus by correct value : */

    r1= _mm_add_epi32(x1,x2);
    x3= _mm_add_epi32(x3,x4);
    x5= _mm_add_epi32(x5,x6);
    r1= _mm_add_epi32(r1,x3);
    x7= _mm_add_epi32(x7,x8);
    r1= _mm_add_epi32(r1,x5);

r0= _mm_add_epi32(t1,t2);
t3= _mm_add_epi32(t3,t4);
t5= _mm_add_epi32(t5,t6);
r0= _mm_add_epi32(r0,t3);
t7= _mm_add_epi32(t7,t8);
r0= _mm_add_epi32(r0,t5);
r1= _mm_add_epi32(r1,x7);
r0= _mm_add_epi32(r0,t7);
r1= _mm_srli_epi32(r1,6);
r0= _mm_srli_epi32(r0,6);

    r1= _mm_and_si128(r1,_mm_set_epi16(0,65535,0,65535,0,65535,0,65535));
    r0= _mm_and_si128(r0,_mm_set_epi16(0,65535,0,65535,0,65535,0,65535));
    r0= _mm_hadd_epi16(r0,r1);
    _mm_store_si128((__m128i*)&dst[x],r0);

}
tmp += MAX_PB_SIZE;
dst += dststride;
}
}
void ff_hevc_put_hevc_qpel_h_2_v_3_sse(int16_t *dst, ptrdiff_t dststride,
uint8_t *_src, ptrdiff_t _srcstride,
int width, int height, int16_t* mcbuffer)
{
int x, y, temp1;
uint8_t *src = (uint8_t*)_src;
ptrdiff_t srcstride = _srcstride/sizeof(uint8_t);
/*DECLARE_ALIGNED( 16, int16_t, tmp_array[(MAX_PB_SIZE+7)*MAX_PB_SIZE] );*/
/* int16_t tmp_array[(MAX_PB_SIZE+7)*MAX_PB_SIZE];     */
int16_t *tmp = mcbuffer;
__m128i x1,x2,x3,x4,x5,x6,x7,x8, rBuffer, rTemp, r0, r1, r2;
__m128i t1,t2,t3,t4,t5,t6,t7,t8;

src -= qpel_extra_before[3] * srcstride;
r0= _mm_set_epi8(-1 ,4 ,-11 ,40 ,40 ,-11 ,4 ,-1 ,-1 ,4 ,-11 ,40 ,40 ,-11 ,4 ,-1);

/* LOAD src from memory to registers to limit memory bandwidth */
if(width == 4){

for (y = 0; y < height + qpel_extra[3]; y+=2) {
/* load data in register     */
    x1= _mm_loadu_si128((__m128i*)&src[-3]);
    src += srcstride;
    t1= _mm_loadu_si128((__m128i*)&src[-3]);
    x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
    t2= _mm_unpacklo_epi64(t1,_mm_srli_si128(t1,1));
    x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
    t3= _mm_unpacklo_epi64(_mm_srli_si128(t1,2),_mm_srli_si128(t1,3));

/*  PMADDUBSW then PMADDW     */
    x2= _mm_maddubs_epi16(x2,r0);
        t2= _mm_maddubs_epi16(t2,r0);
    x3= _mm_maddubs_epi16(x3,r0);
        t3= _mm_maddubs_epi16(t3,r0);
    x2= _mm_hadd_epi16(x2,x3);
        t2= _mm_hadd_epi16(t2,t3);
    x2= _mm_hadd_epi16(x2,_mm_set1_epi16(0));
        t2= _mm_hadd_epi16(t2,_mm_set1_epi16(0));
    x2= _mm_srli_epi16(x2, BIT_DEPTH - 8);
        t2= _mm_srli_epi16(t2, BIT_DEPTH - 8);
/* give results back            */
    _mm_storel_epi64((__m128i*)&tmp[0], x2);

    tmp += MAX_PB_SIZE;
    _mm_storel_epi64((__m128i*)&tmp[0], t2);

src += srcstride;
tmp += MAX_PB_SIZE;
}
}else
for (y = 0; y < height + qpel_extra[3]; y++) {
for (x = 0; x < width; x+=8)  {
/* load data in register     */
x1= _mm_loadu_si128((__m128i*)&src[x-3]);
x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
x4= _mm_unpacklo_epi64(_mm_srli_si128(x1,4),_mm_srli_si128(x1,5));
x5= _mm_unpacklo_epi64(_mm_srli_si128(x1,6),_mm_srli_si128(x1,7));

/*  PMADDUBSW then PMADDW     */
x2= _mm_maddubs_epi16(x2,r0);
x3= _mm_maddubs_epi16(x3,r0);
x4= _mm_maddubs_epi16(x4,r0);
x5= _mm_maddubs_epi16(x5,r0);
x2= _mm_hadd_epi16(x2,x3);
x4= _mm_hadd_epi16(x4,x5);
x2= _mm_hadd_epi16(x2,x4);
    x2= _mm_srli_si128(x2,BIT_DEPTH -8);

/* give results back            */
_mm_store_si128((__m128i*)&tmp[x], x2);

}
src += srcstride;
tmp += MAX_PB_SIZE;
}


tmp = mcbuffer + qpel_extra_before[3] * MAX_PB_SIZE;
    srcstride= MAX_PB_SIZE;

/* vertical treatment on temp table : tmp contains 16 bit values, so need to use 32 bit  integers
for register calculations */
rTemp= _mm_set_epi16(-1 ,4 ,-10 ,58 ,17 ,-5 ,1 ,0);
for (y = 0; y < height; y++)  {
for (x = 0; x < width; x+=8)  {

x1= _mm_setzero_si128();
x2= _mm_load_si128((__m128i*)&tmp[x-2*srcstride]);
x3= _mm_load_si128((__m128i*)&tmp[x-srcstride]);
x4= _mm_load_si128((__m128i*)&tmp[x]);
x5= _mm_load_si128((__m128i*)&tmp[x+srcstride]);
x6= _mm_load_si128((__m128i*)&tmp[x+2*srcstride]);
x7= _mm_load_si128((__m128i*)&tmp[x+3*srcstride]);
x8= _mm_load_si128((__m128i*)&tmp[x+4*srcstride]);

    r0= _mm_set1_epi16(_mm_extract_epi16(rTemp,0));
    r1= _mm_set1_epi16(_mm_extract_epi16(rTemp,1));
    t8= _mm_mullo_epi16(x1,r0);
    rBuffer= _mm_mulhi_epi16(x1,r0);
        t7= _mm_mullo_epi16(x2,r1);
    t1 = _mm_unpacklo_epi16(t8,rBuffer);
    x1 = _mm_unpackhi_epi16(t8,rBuffer);


    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,2));
    rBuffer= _mm_mulhi_epi16(x2,r1);
    t8= _mm_mullo_epi16(x3,r0);
    t2 = _mm_unpacklo_epi16(t7,rBuffer);
    x2 = _mm_unpackhi_epi16(t7,rBuffer);

    r1=_mm_set1_epi16(_mm_extract_epi16(rTemp,3));
    rBuffer= _mm_mulhi_epi16(x3,r0);
    t7= _mm_mullo_epi16(x4,r1);
t3 = _mm_unpacklo_epi16(t8,rBuffer);
x3 = _mm_unpackhi_epi16(t8,rBuffer);

    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,4));
rBuffer= _mm_mulhi_epi16(x4,r1);
    t8= _mm_mullo_epi16(x5,r0);
t4 = _mm_unpacklo_epi16(t7,rBuffer);
x4 = _mm_unpackhi_epi16(t7,rBuffer);

    r1=_mm_set1_epi16(_mm_extract_epi16(rTemp,5));
rBuffer= _mm_mulhi_epi16(x5,r0);
    t7= _mm_mullo_epi16(x6,r1);
t5 = _mm_unpacklo_epi16(t8,rBuffer);
x5 = _mm_unpackhi_epi16(t8,rBuffer);

    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,6));
    rBuffer= _mm_mulhi_epi16(x6,r1);
    t8= _mm_mullo_epi16(x7,r0);
    t6 = _mm_unpacklo_epi16(t7,rBuffer);
    x6 = _mm_unpackhi_epi16(t7,rBuffer);

    rBuffer= _mm_mulhi_epi16(x7,r0);
    t7 = _mm_unpacklo_epi16(t8,rBuffer);
    x7 = _mm_unpackhi_epi16(t8,rBuffer);

    t8 = _mm_unpacklo_epi16(_mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))),_mm_mulhi_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))));
    x8 = _mm_unpackhi_epi16(_mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))),_mm_mulhi_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))));


/* add calculus by correct value : */

    r1= _mm_add_epi32(x1,x2);
    x3= _mm_add_epi32(x3,x4);
    x5= _mm_add_epi32(x5,x6);
    r1= _mm_add_epi32(r1,x3);
    x7= _mm_add_epi32(x7,x8);
    r1= _mm_add_epi32(r1,x5);

r0= _mm_add_epi32(t1,t2);
t3= _mm_add_epi32(t3,t4);
t5= _mm_add_epi32(t5,t6);
r0= _mm_add_epi32(r0,t3);
t7= _mm_add_epi32(t7,t8);
r0= _mm_add_epi32(r0,t5);
r1= _mm_add_epi32(r1,x7);
r0= _mm_add_epi32(r0,t7);
r1= _mm_srli_epi32(r1,6);
r0= _mm_srli_epi32(r0,6);

    r1= _mm_and_si128(r1,_mm_set_epi16(0,65535,0,65535,0,65535,0,65535));
    r0= _mm_and_si128(r0,_mm_set_epi16(0,65535,0,65535,0,65535,0,65535));
    r0= _mm_hadd_epi16(r0,r1);
    _mm_store_si128((__m128i*)&dst[x],r0);

}
tmp += MAX_PB_SIZE;
dst += dststride;
}
}
void ff_hevc_put_hevc_qpel_h_3_v_1_sse(int16_t *dst, ptrdiff_t dststride,
uint8_t *_src, ptrdiff_t _srcstride,
int width, int height, int16_t* mcbuffer)
{
int x, y, temp1;
uint8_t *src = (uint8_t*)_src;
ptrdiff_t srcstride = _srcstride/sizeof(uint8_t);
/*DECLARE_ALIGNED( 16, int16_t, tmp_array[(MAX_PB_SIZE+7)*MAX_PB_SIZE] );*/
/* int16_t tmp_array[(MAX_PB_SIZE+7)*MAX_PB_SIZE];     */
int16_t *tmp = mcbuffer;
__m128i x1,x2,x3,x4,x5,x6,x7,x8, rBuffer, rTemp, r0, r1, r2;
__m128i t1,t2,t3,t4,t5,t6,t7,t8;

src -= qpel_extra_before[1] * srcstride;
r0= _mm_set_epi8(-1 ,4 ,-10 ,58 ,17 ,-5 ,1 ,0 ,-1 ,4 ,-10 ,58 ,17 ,-5 ,1 ,0);

/* LOAD src from memory to registers to limit memory bandwidth */
if(width == 4){

for (y = 0; y < height + qpel_extra[1]; y+=2) {
/* load data in register     */
    x1= _mm_loadu_si128((__m128i*)&src[-3]);
    src += srcstride;
    t1= _mm_loadu_si128((__m128i*)&src[-3]);
    x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
    t2= _mm_unpacklo_epi64(t1,_mm_srli_si128(t1,1));
    x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
    t3= _mm_unpacklo_epi64(_mm_srli_si128(t1,2),_mm_srli_si128(t1,3));

/*  PMADDUBSW then PMADDW     */
    x2= _mm_maddubs_epi16(x2,r0);
        t2= _mm_maddubs_epi16(t2,r0);
    x3= _mm_maddubs_epi16(x3,r0);
        t3= _mm_maddubs_epi16(t3,r0);
    x2= _mm_hadd_epi16(x2,x3);
        t2= _mm_hadd_epi16(t2,t3);
    x2= _mm_hadd_epi16(x2,_mm_set1_epi16(0));
        t2= _mm_hadd_epi16(t2,_mm_set1_epi16(0));
    x2= _mm_srli_epi16(x2, BIT_DEPTH - 8);
        t2= _mm_srli_epi16(t2, BIT_DEPTH - 8);
/* give results back            */
    _mm_storel_epi64((__m128i*)&tmp[0], x2);

    tmp += MAX_PB_SIZE;
    _mm_storel_epi64((__m128i*)&tmp[0], t2);

src += srcstride;
tmp += MAX_PB_SIZE;
}
}else
for (y = 0; y < height + qpel_extra[1]; y++) {
for (x = 0; x < width; x+=8)  {
/* load data in register     */
x1= _mm_loadu_si128((__m128i*)&src[x-3]);
x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
x4= _mm_unpacklo_epi64(_mm_srli_si128(x1,4),_mm_srli_si128(x1,5));
x5= _mm_unpacklo_epi64(_mm_srli_si128(x1,6),_mm_srli_si128(x1,7));

/*  PMADDUBSW then PMADDW     */
x2= _mm_maddubs_epi16(x2,r0);
x3= _mm_maddubs_epi16(x3,r0);
x4= _mm_maddubs_epi16(x4,r0);
x5= _mm_maddubs_epi16(x5,r0);
x2= _mm_hadd_epi16(x2,x3);
x4= _mm_hadd_epi16(x4,x5);
x2= _mm_hadd_epi16(x2,x4);
    x2= _mm_srli_si128(x2,BIT_DEPTH -8);

/* give results back            */
_mm_store_si128((__m128i*)&tmp[x], x2);

}
src += srcstride;
tmp += MAX_PB_SIZE;
}


tmp = mcbuffer + qpel_extra_before[1] * MAX_PB_SIZE;
    srcstride= MAX_PB_SIZE;

/* vertical treatment on temp table : tmp contains 16 bit values, so need to use 32 bit  integers
for register calculations */
rTemp= _mm_set_epi16(0 ,1 ,-5 ,17 ,58 ,-10 ,4 ,-1);
for (y = 0; y < height; y++)  {
for (x = 0; x < width; x+=8)  {

x1= _mm_load_si128((__m128i*)&tmp[x-3*srcstride]);
x2= _mm_load_si128((__m128i*)&tmp[x-2*srcstride]);
x3= _mm_load_si128((__m128i*)&tmp[x-srcstride]);
x4= _mm_load_si128((__m128i*)&tmp[x]);
x5= _mm_load_si128((__m128i*)&tmp[x+srcstride]);
x6= _mm_load_si128((__m128i*)&tmp[x+2*srcstride]);
x7= _mm_load_si128((__m128i*)&tmp[x+3*srcstride]);
x8= _mm_setzero_si128();

    r0= _mm_set1_epi16(_mm_extract_epi16(rTemp,0));
    r1= _mm_set1_epi16(_mm_extract_epi16(rTemp,1));
    t8= _mm_mullo_epi16(x1,r0);
    rBuffer= _mm_mulhi_epi16(x1,r0);
        t7= _mm_mullo_epi16(x2,r1);
    t1 = _mm_unpacklo_epi16(t8,rBuffer);
    x1 = _mm_unpackhi_epi16(t8,rBuffer);


    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,2));
    rBuffer= _mm_mulhi_epi16(x2,r1);
    t8= _mm_mullo_epi16(x3,r0);
    t2 = _mm_unpacklo_epi16(t7,rBuffer);
    x2 = _mm_unpackhi_epi16(t7,rBuffer);

    r1=_mm_set1_epi16(_mm_extract_epi16(rTemp,3));
    rBuffer= _mm_mulhi_epi16(x3,r0);
    t7= _mm_mullo_epi16(x4,r1);
t3 = _mm_unpacklo_epi16(t8,rBuffer);
x3 = _mm_unpackhi_epi16(t8,rBuffer);

    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,4));
rBuffer= _mm_mulhi_epi16(x4,r1);
    t8= _mm_mullo_epi16(x5,r0);
t4 = _mm_unpacklo_epi16(t7,rBuffer);
x4 = _mm_unpackhi_epi16(t7,rBuffer);

    r1=_mm_set1_epi16(_mm_extract_epi16(rTemp,5));
rBuffer= _mm_mulhi_epi16(x5,r0);
    t7= _mm_mullo_epi16(x6,r1);
t5 = _mm_unpacklo_epi16(t8,rBuffer);
x5 = _mm_unpackhi_epi16(t8,rBuffer);

    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,6));
    rBuffer= _mm_mulhi_epi16(x6,r1);
    t8= _mm_mullo_epi16(x7,r0);
    t6 = _mm_unpacklo_epi16(t7,rBuffer);
    x6 = _mm_unpackhi_epi16(t7,rBuffer);

    rBuffer= _mm_mulhi_epi16(x7,r0);
    t7 = _mm_unpacklo_epi16(t8,rBuffer);
    x7 = _mm_unpackhi_epi16(t8,rBuffer);

    t8 = _mm_unpacklo_epi16(_mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))),_mm_mulhi_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))));
    x8 = _mm_unpackhi_epi16(_mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))),_mm_mulhi_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))));


/* add calculus by correct value : */

    r1= _mm_add_epi32(x1,x2);
    x3= _mm_add_epi32(x3,x4);
    x5= _mm_add_epi32(x5,x6);
    r1= _mm_add_epi32(r1,x3);
    x7= _mm_add_epi32(x7,x8);
    r1= _mm_add_epi32(r1,x5);

r0= _mm_add_epi32(t1,t2);
t3= _mm_add_epi32(t3,t4);
t5= _mm_add_epi32(t5,t6);
r0= _mm_add_epi32(r0,t3);
t7= _mm_add_epi32(t7,t8);
r0= _mm_add_epi32(r0,t5);
r1= _mm_add_epi32(r1,x7);
r0= _mm_add_epi32(r0,t7);
r1= _mm_srli_epi32(r1,6);
r0= _mm_srli_epi32(r0,6);

    r1= _mm_and_si128(r1,_mm_set_epi16(0,65535,0,65535,0,65535,0,65535));
    r0= _mm_and_si128(r0,_mm_set_epi16(0,65535,0,65535,0,65535,0,65535));
    r0= _mm_hadd_epi16(r0,r1);
    _mm_store_si128((__m128i*)&dst[x],r0);

}
tmp += MAX_PB_SIZE;
dst += dststride;
}
}
void ff_hevc_put_hevc_qpel_h_3_v_2_sse(int16_t *dst, ptrdiff_t dststride,
uint8_t *_src, ptrdiff_t _srcstride,
int width, int height, int16_t* mcbuffer)
{
int x, y, temp1;
uint8_t *src = (uint8_t*)_src;
ptrdiff_t srcstride = _srcstride/sizeof(uint8_t);
/*DECLARE_ALIGNED( 16, int16_t, tmp_array[(MAX_PB_SIZE+7)*MAX_PB_SIZE] );*/
/* int16_t tmp_array[(MAX_PB_SIZE+7)*MAX_PB_SIZE];     */
int16_t *tmp = mcbuffer;
__m128i x1,x2,x3,x4,x5,x6,x7,x8, rBuffer, rTemp, r0, r1, r2;
__m128i t1,t2,t3,t4,t5,t6,t7,t8;

src -= qpel_extra_before[2] * srcstride;
r0= _mm_set_epi8(-1 ,4 ,-10 ,58 ,17 ,-5 ,1 ,0 ,-1 ,4 ,-10 ,58 ,17 ,-5 ,1 ,0);

/* LOAD src from memory to registers to limit memory bandwidth */
if(width == 4){

for (y = 0; y < height + qpel_extra[2]; y+=2) {
/* load data in register     */
    x1= _mm_loadu_si128((__m128i*)&src[-3]);
    src += srcstride;
    t1= _mm_loadu_si128((__m128i*)&src[-3]);
    x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
    t2= _mm_unpacklo_epi64(t1,_mm_srli_si128(t1,1));
    x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
    t3= _mm_unpacklo_epi64(_mm_srli_si128(t1,2),_mm_srli_si128(t1,3));

/*  PMADDUBSW then PMADDW     */
    x2= _mm_maddubs_epi16(x2,r0);
        t2= _mm_maddubs_epi16(t2,r0);
    x3= _mm_maddubs_epi16(x3,r0);
        t3= _mm_maddubs_epi16(t3,r0);
    x2= _mm_hadd_epi16(x2,x3);
        t2= _mm_hadd_epi16(t2,t3);
    x2= _mm_hadd_epi16(x2,_mm_set1_epi16(0));
        t2= _mm_hadd_epi16(t2,_mm_set1_epi16(0));
    x2= _mm_srli_epi16(x2, BIT_DEPTH - 8);
        t2= _mm_srli_epi16(t2, BIT_DEPTH - 8);
/* give results back            */
    _mm_storel_epi64((__m128i*)&tmp[0], x2);

    tmp += MAX_PB_SIZE;
    _mm_storel_epi64((__m128i*)&tmp[0], t2);

src += srcstride;
tmp += MAX_PB_SIZE;
}
}else
for (y = 0; y < height + qpel_extra[2]; y++) {
for (x = 0; x < width; x+=8)  {
/* load data in register     */
x1= _mm_loadu_si128((__m128i*)&src[x-3]);
x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
x4= _mm_unpacklo_epi64(_mm_srli_si128(x1,4),_mm_srli_si128(x1,5));
x5= _mm_unpacklo_epi64(_mm_srli_si128(x1,6),_mm_srli_si128(x1,7));

/*  PMADDUBSW then PMADDW     */
x2= _mm_maddubs_epi16(x2,r0);
x3= _mm_maddubs_epi16(x3,r0);
x4= _mm_maddubs_epi16(x4,r0);
x5= _mm_maddubs_epi16(x5,r0);
x2= _mm_hadd_epi16(x2,x3);
x4= _mm_hadd_epi16(x4,x5);
x2= _mm_hadd_epi16(x2,x4);
    x2= _mm_srli_si128(x2,BIT_DEPTH -8);

/* give results back            */
_mm_store_si128((__m128i*)&tmp[x], x2);

}
src += srcstride;
tmp += MAX_PB_SIZE;
}


tmp = mcbuffer + qpel_extra_before[2] * MAX_PB_SIZE;
    srcstride= MAX_PB_SIZE;

/* vertical treatment on temp table : tmp contains 16 bit values, so need to use 32 bit  integers
for register calculations */
rTemp= _mm_set_epi16(-1 ,4 ,-11 ,40 ,40 ,-11 ,4 ,-1);
for (y = 0; y < height; y++)  {
for (x = 0; x < width; x+=8)  {

x1= _mm_load_si128((__m128i*)&tmp[x-3*srcstride]);
x2= _mm_load_si128((__m128i*)&tmp[x-2*srcstride]);
x3= _mm_load_si128((__m128i*)&tmp[x-srcstride]);
x4= _mm_load_si128((__m128i*)&tmp[x]);
x5= _mm_load_si128((__m128i*)&tmp[x+srcstride]);
x6= _mm_load_si128((__m128i*)&tmp[x+2*srcstride]);
x7= _mm_load_si128((__m128i*)&tmp[x+3*srcstride]);
x8= _mm_load_si128((__m128i*)&tmp[x+4*srcstride]);

    r0= _mm_set1_epi16(_mm_extract_epi16(rTemp,0));
    r1= _mm_set1_epi16(_mm_extract_epi16(rTemp,1));
    t8= _mm_mullo_epi16(x1,r0);
    rBuffer= _mm_mulhi_epi16(x1,r0);
        t7= _mm_mullo_epi16(x2,r1);
    t1 = _mm_unpacklo_epi16(t8,rBuffer);
    x1 = _mm_unpackhi_epi16(t8,rBuffer);


    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,2));
    rBuffer= _mm_mulhi_epi16(x2,r1);
    t8= _mm_mullo_epi16(x3,r0);
    t2 = _mm_unpacklo_epi16(t7,rBuffer);
    x2 = _mm_unpackhi_epi16(t7,rBuffer);

    r1=_mm_set1_epi16(_mm_extract_epi16(rTemp,3));
    rBuffer= _mm_mulhi_epi16(x3,r0);
    t7= _mm_mullo_epi16(x4,r1);
t3 = _mm_unpacklo_epi16(t8,rBuffer);
x3 = _mm_unpackhi_epi16(t8,rBuffer);

    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,4));
rBuffer= _mm_mulhi_epi16(x4,r1);
    t8= _mm_mullo_epi16(x5,r0);
t4 = _mm_unpacklo_epi16(t7,rBuffer);
x4 = _mm_unpackhi_epi16(t7,rBuffer);

    r1=_mm_set1_epi16(_mm_extract_epi16(rTemp,5));
rBuffer= _mm_mulhi_epi16(x5,r0);
    t7= _mm_mullo_epi16(x6,r1);
t5 = _mm_unpacklo_epi16(t8,rBuffer);
x5 = _mm_unpackhi_epi16(t8,rBuffer);

    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,6));
    rBuffer= _mm_mulhi_epi16(x6,r1);
    t8= _mm_mullo_epi16(x7,r0);
    t6 = _mm_unpacklo_epi16(t7,rBuffer);
    x6 = _mm_unpackhi_epi16(t7,rBuffer);

    rBuffer= _mm_mulhi_epi16(x7,r0);
    t7 = _mm_unpacklo_epi16(t8,rBuffer);
    x7 = _mm_unpackhi_epi16(t8,rBuffer);

    t8 = _mm_unpacklo_epi16(_mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))),_mm_mulhi_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))));
    x8 = _mm_unpackhi_epi16(_mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))),_mm_mulhi_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))));


/* add calculus by correct value : */

    r1= _mm_add_epi32(x1,x2);
    x3= _mm_add_epi32(x3,x4);
    x5= _mm_add_epi32(x5,x6);
    r1= _mm_add_epi32(r1,x3);
    x7= _mm_add_epi32(x7,x8);
    r1= _mm_add_epi32(r1,x5);

r0= _mm_add_epi32(t1,t2);
t3= _mm_add_epi32(t3,t4);
t5= _mm_add_epi32(t5,t6);
r0= _mm_add_epi32(r0,t3);
t7= _mm_add_epi32(t7,t8);
r0= _mm_add_epi32(r0,t5);
r1= _mm_add_epi32(r1,x7);
r0= _mm_add_epi32(r0,t7);
r1= _mm_srli_epi32(r1,6);
r0= _mm_srli_epi32(r0,6);

    r1= _mm_and_si128(r1,_mm_set_epi16(0,65535,0,65535,0,65535,0,65535));
    r0= _mm_and_si128(r0,_mm_set_epi16(0,65535,0,65535,0,65535,0,65535));
    r0= _mm_hadd_epi16(r0,r1);
    _mm_store_si128((__m128i*)&dst[x],r0);

}
tmp += MAX_PB_SIZE;
dst += dststride;
}
}
void ff_hevc_put_hevc_qpel_h_3_v_3_sse(int16_t *dst, ptrdiff_t dststride,
uint8_t *_src, ptrdiff_t _srcstride,
int width, int height, int16_t* mcbuffer)
{
int x, y, temp1;
uint8_t *src = (uint8_t*)_src;
ptrdiff_t srcstride = _srcstride/sizeof(uint8_t);
/*DECLARE_ALIGNED( 16, int16_t, tmp_array[(MAX_PB_SIZE+7)*MAX_PB_SIZE] );*/
/* int16_t tmp_array[(MAX_PB_SIZE+7)*MAX_PB_SIZE];     */
int16_t *tmp = mcbuffer;
__m128i x1,x2,x3,x4,x5,x6,x7,x8, rBuffer, rTemp, r0, r1, r2;
__m128i t1,t2,t3,t4,t5,t6,t7,t8;

src -= qpel_extra_before[3] * srcstride;
r0= _mm_set_epi8(-1 ,4 ,-10 ,58 ,17 ,-5 ,1 ,0 ,-1 ,4 ,-10 ,58 ,17 ,-5 ,1 ,0);

/* LOAD src from memory to registers to limit memory bandwidth */
if(width == 4){

for (y = 0; y < height + qpel_extra[3]; y+=2) {
/* load data in register     */
    x1= _mm_loadu_si128((__m128i*)&src[-2]);
    x1= _mm_slli_si128(x1,1);
    src += srcstride;
    t1= _mm_loadu_si128((__m128i*)&src[-2]);
    t1= _mm_slli_si128(t1,1);
    x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
    t2= _mm_unpacklo_epi64(t1,_mm_srli_si128(t1,1));
    x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
    t3= _mm_unpacklo_epi64(_mm_srli_si128(t1,2),_mm_srli_si128(t1,3));

/*  PMADDUBSW then PMADDW     */
    x2= _mm_maddubs_epi16(x2,r0);
        t2= _mm_maddubs_epi16(t2,r0);
    x3= _mm_maddubs_epi16(x3,r0);
        t3= _mm_maddubs_epi16(t3,r0);
    x2= _mm_hadd_epi16(x2,x3);
        t2= _mm_hadd_epi16(t2,t3);
    x2= _mm_hadd_epi16(x2,_mm_set1_epi16(0));
        t2= _mm_hadd_epi16(t2,_mm_set1_epi16(0));
    x2= _mm_srli_epi16(x2, BIT_DEPTH - 8);
        t2= _mm_srli_epi16(t2, BIT_DEPTH - 8);
/* give results back            */
    _mm_storel_epi64((__m128i*)&tmp[0], x2);

    tmp += MAX_PB_SIZE;
    _mm_storel_epi64((__m128i*)&tmp[0], t2);

src += srcstride;
tmp += MAX_PB_SIZE;
}
}else
for (y = 0; y < height + qpel_extra[3]; y++) {
for (x = 0; x < width; x+=8)  {
/* load data in register     */
x1= _mm_loadu_si128((__m128i*)&src[x-3]);
x2= _mm_unpacklo_epi64(x1,_mm_srli_si128(x1,1));
x3= _mm_unpacklo_epi64(_mm_srli_si128(x1,2),_mm_srli_si128(x1,3));
x4= _mm_unpacklo_epi64(_mm_srli_si128(x1,4),_mm_srli_si128(x1,5));
x5= _mm_unpacklo_epi64(_mm_srli_si128(x1,6),_mm_srli_si128(x1,7));

/*  PMADDUBSW then PMADDW     */
x2= _mm_maddubs_epi16(x2,r0);
x3= _mm_maddubs_epi16(x3,r0);
x4= _mm_maddubs_epi16(x4,r0);
x5= _mm_maddubs_epi16(x5,r0);
x2= _mm_hadd_epi16(x2,x3);
x4= _mm_hadd_epi16(x4,x5);
x2= _mm_hadd_epi16(x2,x4);
    x2= _mm_srli_si128(x2,BIT_DEPTH -8);

/* give results back            */
_mm_store_si128((__m128i*)&tmp[x], x2);

}
src += srcstride;
tmp += MAX_PB_SIZE;
}


tmp = mcbuffer + qpel_extra_before[3] * MAX_PB_SIZE;
    srcstride= MAX_PB_SIZE;

/* vertical treatment on temp table : tmp contains 16 bit values, so need to use 32 bit  integers
for register calculations */
rTemp= _mm_set_epi16(-1 ,4 ,-10 ,58 ,17 ,-5 ,1 ,0);
for (y = 0; y < height; y++)  {
for (x = 0; x < width; x+=8)  {

x1= _mm_setzero_si128();
x2= _mm_load_si128((__m128i*)&tmp[x-2*srcstride]);
x3= _mm_load_si128((__m128i*)&tmp[x-srcstride]);
x4= _mm_load_si128((__m128i*)&tmp[x]);
x5= _mm_load_si128((__m128i*)&tmp[x+srcstride]);
x6= _mm_load_si128((__m128i*)&tmp[x+2*srcstride]);
x7= _mm_load_si128((__m128i*)&tmp[x+3*srcstride]);
x8= _mm_load_si128((__m128i*)&tmp[x+4*srcstride]);

    r0= _mm_set1_epi16(_mm_extract_epi16(rTemp,0));
    r1= _mm_set1_epi16(_mm_extract_epi16(rTemp,1));
    t8= _mm_mullo_epi16(x1,r0);
    rBuffer= _mm_mulhi_epi16(x1,r0);
        t7= _mm_mullo_epi16(x2,r1);
    t1 = _mm_unpacklo_epi16(t8,rBuffer);
    x1 = _mm_unpackhi_epi16(t8,rBuffer);


    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,2));
    rBuffer= _mm_mulhi_epi16(x2,r1);
    t8= _mm_mullo_epi16(x3,r0);
    t2 = _mm_unpacklo_epi16(t7,rBuffer);
    x2 = _mm_unpackhi_epi16(t7,rBuffer);

    r1=_mm_set1_epi16(_mm_extract_epi16(rTemp,3));
    rBuffer= _mm_mulhi_epi16(x3,r0);
    t7= _mm_mullo_epi16(x4,r1);
t3 = _mm_unpacklo_epi16(t8,rBuffer);
x3 = _mm_unpackhi_epi16(t8,rBuffer);

    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,4));
rBuffer= _mm_mulhi_epi16(x4,r1);
    t8= _mm_mullo_epi16(x5,r0);
t4 = _mm_unpacklo_epi16(t7,rBuffer);
x4 = _mm_unpackhi_epi16(t7,rBuffer);

    r1=_mm_set1_epi16(_mm_extract_epi16(rTemp,5));
rBuffer= _mm_mulhi_epi16(x5,r0);
    t7= _mm_mullo_epi16(x6,r1);
t5 = _mm_unpacklo_epi16(t8,rBuffer);
x5 = _mm_unpackhi_epi16(t8,rBuffer);

    r0=_mm_set1_epi16(_mm_extract_epi16(rTemp,6));
    rBuffer= _mm_mulhi_epi16(x6,r1);
    t8= _mm_mullo_epi16(x7,r0);
    t6 = _mm_unpacklo_epi16(t7,rBuffer);
    x6 = _mm_unpackhi_epi16(t7,rBuffer);

    rBuffer= _mm_mulhi_epi16(x7,r0);
    t7 = _mm_unpacklo_epi16(t8,rBuffer);
    x7 = _mm_unpackhi_epi16(t8,rBuffer);

    t8 = _mm_unpacklo_epi16(_mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))),_mm_mulhi_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))));
    x8 = _mm_unpackhi_epi16(_mm_mullo_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))),_mm_mulhi_epi16(x8,_mm_set1_epi16(_mm_extract_epi16(rTemp,7))));


/* add calculus by correct value : */

    r1= _mm_add_epi32(x1,x2);
    x3= _mm_add_epi32(x3,x4);
    x5= _mm_add_epi32(x5,x6);
    r1= _mm_add_epi32(r1,x3);
    x7= _mm_add_epi32(x7,x8);
    r1= _mm_add_epi32(r1,x5);

r0= _mm_add_epi32(t1,t2);
t3= _mm_add_epi32(t3,t4);
t5= _mm_add_epi32(t5,t6);
r0= _mm_add_epi32(r0,t3);
t7= _mm_add_epi32(t7,t8);
r0= _mm_add_epi32(r0,t5);
r1= _mm_add_epi32(r1,x7);
r0= _mm_add_epi32(r0,t7);
r1= _mm_srli_epi32(r1,6);
r0= _mm_srli_epi32(r0,6);

    r1= _mm_and_si128(r1,_mm_set_epi16(0,65535,0,65535,0,65535,0,65535));
    r0= _mm_and_si128(r0,_mm_set_epi16(0,65535,0,65535,0,65535,0,65535));
    r0= _mm_hadd_epi16(r0,r1);
    _mm_store_si128((__m128i*)&dst[x],r0);

}
tmp += MAX_PB_SIZE;
dst += dststride;
}
}


