/*
 * Copyright © 2016 Southern Storm Software, Pty Ltd.
 * Copyright © 2017-2019 WireGuard LLC. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.wireguard.crypto;

import androidx.annotation.Nullable;

import java.util.Arrays;

/**
 * Implementation of Curve25519 ECDH.
 * <p>
 * This implementation was imported to WireGuard from noise-java:
 * https://github.com/rweather/noise-java
 * <p>
 * This implementation is based on that from arduinolibs:
 * https://github.com/rweather/arduinolibs
 * <p>
 * Differences in this version are due to using 26-bit limbs for the
 * representation instead of the 8/16/32-bit limbs in the original.
 * <p>
 * References: http://cr.yp.to/ecdh.html, RFC 7748
 */
@SuppressWarnings({"MagicNumber", "NonConstantFieldWithUpperCaseName", "SuspiciousNameCombination"})
public final class Curve25519 {
    // Numbers modulo 2^255 - 19 are broken up into ten 26-bit words.
    private static final int NUM_LIMBS_255BIT = 10;
    private static final int NUM_LIMBS_510BIT = 20;

    private final int[] A;
    private final int[] AA;
    private final int[] B;
    private final int[] BB;
    private final int[] C;
    private final int[] CB;
    private final int[] D;
    private final int[] DA;
    private final int[] E;
    private final long[] t1;
    private final int[] t2;
    private final int[] x_1;
    private final int[] x_2;
    private final int[] x_3;
    private final int[] z_2;
    private final int[] z_3;

    /**
     * Constructs the temporary state holder for Curve25519 evaluation.
     */
    private Curve25519() {
        // Allocate memory for all of the temporary variables we will need.
        x_1 = new int[NUM_LIMBS_255BIT];
        x_2 = new int[NUM_LIMBS_255BIT];
        x_3 = new int[NUM_LIMBS_255BIT];
        z_2 = new int[NUM_LIMBS_255BIT];
        z_3 = new int[NUM_LIMBS_255BIT];
        A = new int[NUM_LIMBS_255BIT];
        B = new int[NUM_LIMBS_255BIT];
        C = new int[NUM_LIMBS_255BIT];
        D = new int[NUM_LIMBS_255BIT];
        E = new int[NUM_LIMBS_255BIT];
        AA = new int[NUM_LIMBS_255BIT];
        BB = new int[NUM_LIMBS_255BIT];
        DA = new int[NUM_LIMBS_255BIT];
        CB = new int[NUM_LIMBS_255BIT];
        t1 = new long[NUM_LIMBS_510BIT];
        t2 = new int[NUM_LIMBS_510BIT];
    }

    /**
     * Conditional swap of two values.
     *
     * @param select Set to 1 to swap, 0 to leave as-is.
     * @param x      The first value.
     * @param y      The second value.
     */
    private static void cswap(int select, final int[] x, final int[] y) {
        select = -select;
        for (int index = 0; index < NUM_LIMBS_255BIT; ++index) {
            final int dummy = select & (x[index] ^ y[index]);
            x[index] ^= dummy;
            y[index] ^= dummy;
        }
    }

    /**
     * Evaluates the Curve25519 curve.
     *
     * @param result     Buffer to place the result of the evaluation into.
     * @param offset     Offset into the result buffer.
     * @param privateKey The private key to use in the evaluation.
     * @param publicKey  The public key to use in the evaluation, or null
     *                   if the base point of the curve should be used.
     */
    public static void eval(final byte[] result, final int offset,
                            final byte[] privateKey, @Nullable final byte[] publicKey) {
        final Curve25519 state = new Curve25519();
        try {
            // Unpack the public key value.  If null, use 9 as the base point.
            Arrays.fill(state.x_1, 0);
            if (publicKey != null) {
                // Convert the input value from little-endian into 26-bit limbs.
                for (int index = 0; index < 32; ++index) {
                    final int bit = (index * 8) % 26;
                    final int word = (index * 8) / 26;
                    final int value = publicKey[index] & 0xFF;
                    if (bit <= (26 - 8)) {
                        state.x_1[word] |= value << bit;
                    } else {
                        state.x_1[word] |= value << bit;
                        state.x_1[word] &= 0x03FFFFFF;
                        state.x_1[word + 1] |= value >> (26 - bit);
                    }
                }

                // Just in case, we reduce the number modulo 2^255 - 19 to
                // make sure that it is in range of the field before we start.
                // This eliminates values between 2^255 - 19 and 2^256 - 1.
                state.reduceQuick(state.x_1);
                state.reduceQuick(state.x_1);
            } else {
                state.x_1[0] = 9;
            }

            // Initialize the other temporary variables.
            Arrays.fill(state.x_2, 0);            // x_2 = 1
            state.x_2[0] = 1;
            Arrays.fill(state.z_2, 0);            // z_2 = 0
            System.arraycopy(state.x_1, 0, state.x_3, 0, state.x_1.length);  // x_3 = x_1
            Arrays.fill(state.z_3, 0);            // z_3 = 1
            state.z_3[0] = 1;

            // Evaluate the curve for every bit of the private key.
            state.evalCurve(privateKey);

            // Compute x_2 * (z_2 ^ (p - 2)) where p = 2^255 - 19.
            state.recip(state.z_3, state.z_2);
            state.mul(state.x_2, state.x_2, state.z_3);

            // Convert x_2 into little-endian in the result buffer.
            for (int index = 0; index < 32; ++index) {
                final int bit = (index * 8) % 26;
                final int word = (index * 8) / 26;
                if (bit <= (26 - 8))
                    result[offset + index] = (byte) (state.x_2[word] >> bit);
                else
                    result[offset + index] = (byte) ((state.x_2[word] >> bit) | (state.x_2[word + 1] << (26 - bit)));
            }
        } finally {
            // Clean up all temporary state before we exit.
            state.destroy();
        }
    }

    /**
     * Subtracts two numbers modulo 2^255 - 19.
     *
     * @param result The result.
     * @param x      The first number to subtract.
     * @param y      The second number to subtract.
     */
    private static void sub(final int[] result, final int[] x, final int[] y) {
        int index;
        int borrow;

        // Subtract y from x to generate the intermediate result.
        borrow = 0;
        for (index = 0; index < NUM_LIMBS_255BIT; ++index) {
            borrow = x[index] - y[index] - ((borrow >> 26) & 0x01);
            result[index] = borrow & 0x03FFFFFF;
        }

        // If we had a borrow, then the result has gone negative and we
        // have to add 2^255 - 19 to the result to make it positive again.
        // The top bits of "borrow" will be all 1's if there is a borrow
        // or it will be all 0's if there was no borrow.  Easiest is to
        // conditionally subtract 19 and then mask off the high bits.
        borrow = result[0] - ((-((borrow >> 26) & 0x01)) & 19);
        result[0] = borrow & 0x03FFFFFF;
        for (index = 1; index < NUM_LIMBS_255BIT; ++index) {
            borrow = result[index] - ((borrow >> 26) & 0x01);
            result[index] = borrow & 0x03FFFFFF;
        }
        result[NUM_LIMBS_255BIT - 1] &= 0x001FFFFF;
    }

    /**
     * Adds two numbers modulo 2^255 - 19.
     *
     * @param result The result.
     * @param x      The first number to add.
     * @param y      The second number to add.
     */
    private void add(final int[] result, final int[] x, final int[] y) {
        int carry = x[0] + y[0];
        result[0] = carry & 0x03FFFFFF;
        for (int index = 1; index < NUM_LIMBS_255BIT; ++index) {
            carry = (carry >> 26) + x[index] + y[index];
            result[index] = carry & 0x03FFFFFF;
        }
        reduceQuick(result);
    }

    /**
     * Destroy all sensitive data in this object.
     */
    private void destroy() {
        // Destroy all temporary variables.
        Arrays.fill(x_1, 0);
        Arrays.fill(x_2, 0);
        Arrays.fill(x_3, 0);
        Arrays.fill(z_2, 0);
        Arrays.fill(z_3, 0);
        Arrays.fill(A, 0);
        Arrays.fill(B, 0);
        Arrays.fill(C, 0);
        Arrays.fill(D, 0);
        Arrays.fill(E, 0);
        Arrays.fill(AA, 0);
        Arrays.fill(BB, 0);
        Arrays.fill(DA, 0);
        Arrays.fill(CB, 0);
        Arrays.fill(t1, 0L);
        Arrays.fill(t2, 0);
    }

    /**
     * Evaluates the curve for every bit in a secret key.
     *
     * @param s The 32-byte secret key.
     */
    private void evalCurve(final byte[] s) {
        int sposn = 31;
        int sbit = 6;
        int svalue = s[sposn] | 0x40;
        int swap = 0;

        // Iterate over all 255 bits of "s" from the highest to the lowest.
        // We ignore the high bit of the 256-bit representation of "s".
        while (true) {
            // Conditional swaps on entry to this bit but only if we
            // didn't swap on the previous bit.
            final int select = (svalue >> sbit) & 0x01;
            swap ^= select;
            cswap(swap, x_2, x_3);
            cswap(swap, z_2, z_3);
            swap = select;

            // Evaluate the curve.
            add(A, x_2, z_2);               // A = x_2 + z_2
            square(AA, A);                  // AA = A^2
            sub(B, x_2, z_2);               // B = x_2 - z_2
            square(BB, B);                  // BB = B^2
            sub(E, AA, BB);                 // E = AA - BB
            add(C, x_3, z_3);               // C = x_3 + z_3
            sub(D, x_3, z_3);               // D = x_3 - z_3
            mul(DA, D, A);                  // DA = D * A
            mul(CB, C, B);                  // CB = C * B
            add(x_3, DA, CB);               // x_3 = (DA + CB)^2
            square(x_3, x_3);
            sub(z_3, DA, CB);               // z_3 = x_1 * (DA - CB)^2
            square(z_3, z_3);
            mul(z_3, z_3, x_1);
            mul(x_2, AA, BB);               // x_2 = AA * BB
            mulA24(z_2, E);                 // z_2 = E * (AA + a24 * E)
            add(z_2, z_2, AA);
            mul(z_2, z_2, E);

            // Move onto the next lower bit of "s".
            if (sbit > 0) {
                --sbit;
            } else if (sposn == 0) {
                break;
            } else if (sposn == 1) {
                --sposn;
                svalue = s[sposn] & 0xF8;
                sbit = 7;
            } else {
                --sposn;
                svalue = s[sposn];
                sbit = 7;
            }
        }

        // Final conditional swaps.
        cswap(swap, x_2, x_3);
        cswap(swap, z_2, z_3);
    }

    /**
     * Multiplies two numbers modulo 2^255 - 19.
     *
     * @param result The result.
     * @param x      The first number to multiply.
     * @param y      The second number to multiply.
     */
    private void mul(final int[] result, final int[] x, final int[] y) {
        // Multiply the two numbers to create the intermediate result.
        long v = x[0];
        for (int i = 0; i < NUM_LIMBS_255BIT; ++i) {
            t1[i] = v * y[i];
        }
        for (int i = 1; i < NUM_LIMBS_255BIT; ++i) {
            v = x[i];
            for (int j = 0; j < (NUM_LIMBS_255BIT - 1); ++j) {
                t1[i + j] += v * y[j];
            }
            t1[i + NUM_LIMBS_255BIT - 1] = v * y[NUM_LIMBS_255BIT - 1];
        }

        // Propagate carries and convert back into 26-bit words.
        v = t1[0];
        t2[0] = ((int) v) & 0x03FFFFFF;
        for (int i = 1; i < NUM_LIMBS_510BIT; ++i) {
            v = (v >> 26) + t1[i];
            t2[i] = ((int) v) & 0x03FFFFFF;
        }

        // Reduce the result modulo 2^255 - 19.
        reduce(result, t2, NUM_LIMBS_255BIT);
    }

    /**
     * Multiplies a number by the a24 constant, modulo 2^255 - 19.
     *
     * @param result The result.
     * @param x      The number to multiply by a24.
     */
    private void mulA24(final int[] result, final int[] x) {
        final long a24 = 121665;
        long carry = 0;
        for (int index = 0; index < NUM_LIMBS_255BIT; ++index) {
            carry += a24 * x[index];
            t2[index] = ((int) carry) & 0x03FFFFFF;
            carry >>= 26;
        }
        t2[NUM_LIMBS_255BIT] = ((int) carry) & 0x03FFFFFF;
        reduce(result, t2, 1);
    }

    /**
     * Raise x to the power of (2^250 - 1).
     *
     * @param result The result.  Must not overlap with x.
     * @param x      The argument.
     */
    private void pow250(final int[] result, final int[] x) {
        // The big-endian hexadecimal expansion of (2^250 - 1) is:
        // 03FFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
        //
        // The naive implementation needs to do 2 multiplications per 1 bit and
        // 1 multiplication per 0 bit.  We can improve upon this by creating a
        // pattern 0000000001 ... 0000000001.  If we square and multiply the
        // pattern by itself we can turn the pattern into the partial results
        // 0000000011 ... 0000000011, 0000000111 ... 0000000111, etc.
        // This averages out to about 1.1 multiplications per 1 bit instead of 2.

        // Build a pattern of 250 bits in length of repeated copies of 0000000001.
        square(A, x);
        for (int j = 0; j < 9; ++j)
            square(A, A);
        mul(result, A, x);
        for (int i = 0; i < 23; ++i) {
            for (int j = 0; j < 10; ++j)
                square(A, A);
            mul(result, result, A);
        }

        // Multiply bit-shifted versions of the 0000000001 pattern into
        // the result to "fill in" the gaps in the pattern.
        square(A, result);
        mul(result, result, A);
        for (int j = 0; j < 8; ++j) {
            square(A, A);
            mul(result, result, A);
        }
    }

    /**
     * Computes the reciprocal of a number modulo 2^255 - 19.
     *
     * @param result The result.  Must not overlap with x.
     * @param x      The argument.
     */
    private void recip(final int[] result, final int[] x) {
        // The reciprocal is the same as x ^ (p - 2) where p = 2^255 - 19.
        // The big-endian hexadecimal expansion of (p - 2) is:
        // 7FFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFEB
        // Start with the 250 upper bits of the expansion of (p - 2).
        pow250(result, x);

        // Deal with the 5 lowest bits of (p - 2), 01011, from highest to lowest.
        square(result, result);
        square(result, result);
        mul(result, result, x);
        square(result, result);
        square(result, result);
        mul(result, result, x);
        square(result, result);
        mul(result, result, x);
    }

    /**
     * Reduce a number modulo 2^255 - 19.
     *
     * @param result The result.
     * @param x      The value to be reduced.  This array will be
     *               modified during the reduction.
     * @param size   The number of limbs in the high order half of x.
     */
    private void reduce(final int[] result, final int[] x, final int size) {
        // Calculate (x mod 2^255) + ((x / 2^255) * 19) which will
        // either produce the answer we want or it will produce a
        // value of the form "answer + j * (2^255 - 19)".  There are
        // 5 left-over bits in the top-most limb of the bottom half.
        int carry = 0;
        int limb = x[NUM_LIMBS_255BIT - 1] >> 21;
        x[NUM_LIMBS_255BIT - 1] &= 0x001FFFFF;
        for (int index = 0; index < size; ++index) {
            limb += x[NUM_LIMBS_255BIT + index] << 5;
            carry += (limb & 0x03FFFFFF) * 19 + x[index];
            x[index] = carry & 0x03FFFFFF;
            limb >>= 26;
            carry >>= 26;
        }
        if (size < NUM_LIMBS_255BIT) {
            // The high order half of the number is short; e.g. for mulA24().
            // Propagate the carry through the rest of the low order part.
            for (int index = size; index < NUM_LIMBS_255BIT; ++index) {
                carry += x[index];
                x[index] = carry & 0x03FFFFFF;
                carry >>= 26;
            }
        }

        // The "j" value may still be too large due to the final carry-out.
        // We must repeat the reduction.  If we already have the answer,
        // then this won't do any harm but we must still do the calculation
        // to preserve the overall timing.  The "j" value will be between
        // 0 and 19, which means that the carry we care about is in the
        // top 5 bits of the highest limb of the bottom half.
        carry = (x[NUM_LIMBS_255BIT - 1] >> 21) * 19;
        x[NUM_LIMBS_255BIT - 1] &= 0x001FFFFF;
        for (int index = 0; index < NUM_LIMBS_255BIT; ++index) {
            carry += x[index];
            result[index] = carry & 0x03FFFFFF;
            carry >>= 26;
        }

        // At this point "x" will either be the answer or it will be the
        // answer plus (2^255 - 19).  Perform a trial subtraction to
        // complete the reduction process.
        reduceQuick(result);
    }

    /**
     * Reduces a number modulo 2^255 - 19 where it is known that the
     * number can be reduced with only 1 trial subtraction.
     *
     * @param x The number to reduce, and the result.
     */
    private void reduceQuick(final int[] x) {
        // Perform a trial subtraction of (2^255 - 19) from "x" which is
        // equivalent to adding 19 and subtracting 2^255.  We add 19 here;
        // the subtraction of 2^255 occurs in the next step.
        int carry = 19;
        for (int index = 0; index < NUM_LIMBS_255BIT; ++index) {
            carry += x[index];
            t2[index] = carry & 0x03FFFFFF;
            carry >>= 26;
        }

        // If there was a borrow, then the original "x" is the correct answer.
        // If there was no borrow, then "t2" is the correct answer.  Select the
        // correct answer but do it in a way that instruction timing will not
        // reveal which value was selected.  Borrow will occur if bit 21 of
        // "t2" is zero.  Turn the bit into a selection mask.
        final int mask = -((t2[NUM_LIMBS_255BIT - 1] >> 21) & 0x01);
        final int nmask = ~mask;
        t2[NUM_LIMBS_255BIT - 1] &= 0x001FFFFF;
        for (int index = 0; index < NUM_LIMBS_255BIT; ++index)
            x[index] = (x[index] & nmask) | (t2[index] & mask);
    }

    /**
     * Squares a number modulo 2^255 - 19.
     *
     * @param result The result.
     * @param x      The number to square.
     */
    private void square(final int[] result, final int[] x) {
        mul(result, x, x);
    }
}
