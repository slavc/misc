#ifndef CHAR_BITMAPS_H
#define CHAR_BITMAPS_H

#define MASK_NIBBLE_0 (15)
#define MASK_NIBBLE_1 (15 << 4)
#define MASK_NIBBLE_2 (15 << 8)
#define MASK_NIBBLE_3 (15 << 12)

#define LINE_0(x) (x & MASK_NIBBLE_0)
#define LINE_1(x) ((x & MASK_NIBBLE_0) << 4)
#define LINE_2(x) ((x & MASK_NIBBLE_0) << 8)
#define LINE_3(x) ((x & MASK_NIBBLE_0) << 12)

/* x is unsigned short */
#define GET_LINE_0(x) (x & MASK_NIBBLE_0)
#define GET_LINE_1(x) ((x & MASK_NIBBLE_1) >> 4)
#define GET_LINE_2(x) ((x & MASK_NIBBLE_2) >> 8)
#define GET_LINE_3(x) ((x & MASK_NIBBLE_3) >> 12)

#endif
