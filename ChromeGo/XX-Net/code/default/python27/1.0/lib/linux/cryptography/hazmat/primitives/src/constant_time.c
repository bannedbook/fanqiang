// This file is dual licensed under the terms of the Apache License, Version
// 2.0, and the BSD License. See the LICENSE file in the root of this
// repository for complete details.

uint8_t Cryptography_constant_time_bytes_eq(uint8_t *a, size_t len_a,
                                            uint8_t *b, size_t len_b) {
    size_t i = 0;
    uint8_t mismatch = 0;
    if (len_a != len_b) {
        return 0;
    }
    for (i = 0; i < len_a; i++) {
        mismatch |= a[i] ^ b[i];
    }

    /* Make sure any bits set are copied to the lowest bit */
    mismatch |= mismatch >> 4;
    mismatch |= mismatch >> 2;
    mismatch |= mismatch >> 1;
    /* Now check the low bit to see if it's set */
    return (mismatch & 1) == 0;
}
