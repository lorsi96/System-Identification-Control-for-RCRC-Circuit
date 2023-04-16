#ifndef __UNIT_CONVERSIONS__
#define __UNIT_CONVERSIONS__

inline static q15_t dn10b_to_q15(uint16_t dn) {
    return ((q15_t)dn - 512) << 6;  // -1 to 1
}

inline static float q15_to_float(q15_t dn) {
    float ret;
    arm_q15_to_float(&dn, &ret, 1);
    return ret;
}

inline static float float_to_q15(float fp) {
    q15_t ret;
    arm_float_to_q15(&fp, &ret, 1);
    return ret;
}

static uint16_t float_to_dn10b(float val) {
    q15_t ret;
    arm_float_to_q15(&val, &ret, 1);
    volatile uint16_t res = (ret >> 6) + 512;
    return res;
}

#endif  // 