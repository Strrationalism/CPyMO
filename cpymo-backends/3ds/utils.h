
#define MAKE_PTR_TEX(OUT, TEX, X, Y, CHANNELS, PW, PH) \
    u8 * OUT; \
    { \
        const u32 ix = PW - X - 1; \
        const u32 offset = ((((ix >> 3) * (PH >> 3) + (Y >> 3)) << 6) + ((Y & 1) | ((ix & 1) << 1) | ((Y & 2) << 1) | ((ix & 2) << 2) | ((Y & 4) << 2) | ((ix & 4) << 3))) * CHANNELS; \
        OUT = ((u8 *)(TEX).data) + offset; \
    }
    