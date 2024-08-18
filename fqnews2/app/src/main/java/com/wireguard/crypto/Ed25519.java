/*
 * Copyright © 2020 WireGuard LLC. All Rights Reserved.
 * Copyright 2017 Google Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

package com.wireguard.crypto;

import java.math.BigInteger;
import java.security.GeneralSecurityException;
import java.security.MessageDigest;
import java.util.Arrays;

/**
 * Implementation of Ed25519 signature verification.
 *
 * <p>This implementation is based on the ed25519/ref10 implementation in NaCl.</p>
 *
 * <p>It implements this twisted Edwards curve:
 *
 * <pre>
 * -x^2 + y^2 = 1 + (-121665 / 121666 mod 2^255-19)*x^2*y^2
 * </pre>
 *
 * @see <a href="https://eprint.iacr.org/2008/013.pdf">Bernstein D.J., Birkner P., Joye M., Lange
 * T., Peters C. (2008) Twisted Edwards Curves</a>
 * @see <a href="https://eprint.iacr.org/2008/522.pdf">Hisil H., Wong K.KH., Carter G., Dawson E.
 * (2008) Twisted Edwards Curves Revisited</a>
 */
public final class Ed25519 {

    // d = -121665 / 121666 mod 2^255-19
    private static final long[] D;
    // 2d
    private static final long[] D2;
    // 2^((p-1)/4) mod p where p = 2^255-19
    private static final long[] SQRTM1;

    /**
     * Base point for the Edwards twisted curve = (x, 4/5) and its exponentiations. B_TABLE[i][j] =
     * (j+1)*256^i*B for i in [0, 32) and j in [0, 8). Base point B = B_TABLE[0][0]
     */
    private static final CachedXYT[][] B_TABLE;
    private static final CachedXYT[] B2;

    private static final BigInteger P_BI =
            BigInteger.valueOf(2).pow(255).subtract(BigInteger.valueOf(19));
    private static final BigInteger D_BI =
            BigInteger.valueOf(-121665).multiply(BigInteger.valueOf(121666).modInverse(P_BI)).mod(P_BI);
    private static final BigInteger D2_BI = BigInteger.valueOf(2).multiply(D_BI).mod(P_BI);
    private static final BigInteger SQRTM1_BI =
            BigInteger.valueOf(2).modPow(P_BI.subtract(BigInteger.ONE).divide(BigInteger.valueOf(4)), P_BI);

    private Ed25519() {
    }

    private static class Point {
        private BigInteger x;
        private BigInteger y;
    }

    private static BigInteger recoverX(BigInteger y) {
        // x^2 = (y^2 - 1) / (d * y^2 + 1) mod 2^255-19
        BigInteger xx =
                y.pow(2)
                        .subtract(BigInteger.ONE)
                        .multiply(D_BI.multiply(y.pow(2)).add(BigInteger.ONE).modInverse(P_BI));
        BigInteger x = xx.modPow(P_BI.add(BigInteger.valueOf(3)).divide(BigInteger.valueOf(8)), P_BI);
        if (!x.pow(2).subtract(xx).mod(P_BI).equals(BigInteger.ZERO)) {
            x = x.multiply(SQRTM1_BI).mod(P_BI);
        }
        if (x.testBit(0)) {
            x = P_BI.subtract(x);
        }
        return x;
    }

    private static Point edwards(Point a, Point b) {
        Point o = new Point();
        BigInteger xxyy = D_BI.multiply(a.x.multiply(b.x).multiply(a.y).multiply(b.y)).mod(P_BI);
        o.x =
                (a.x.multiply(b.y).add(b.x.multiply(a.y)))
                        .multiply(BigInteger.ONE.add(xxyy).modInverse(P_BI))
                        .mod(P_BI);
        o.y =
                (a.y.multiply(b.y).add(a.x.multiply(b.x)))
                        .multiply(BigInteger.ONE.subtract(xxyy).modInverse(P_BI))
                        .mod(P_BI);
        return o;
    }

    private static byte[] toLittleEndian(BigInteger n) {
        byte[] b = new byte[32];
        byte[] nBytes = n.toByteArray();
        System.arraycopy(nBytes, 0, b, 32 - nBytes.length, nBytes.length);
        for (int i = 0; i < b.length / 2; i++) {
            byte t = b[i];
            b[i] = b[b.length - i - 1];
            b[b.length - i - 1] = t;
        }
        return b;
    }

    private static CachedXYT getCachedXYT(Point p) {
        return new CachedXYT(
                Field25519.expand(toLittleEndian(p.y.add(p.x).mod(P_BI))),
                Field25519.expand(toLittleEndian(p.y.subtract(p.x).mod(P_BI))),
                Field25519.expand(toLittleEndian(D2_BI.multiply(p.x).multiply(p.y).mod(P_BI))));
    }

    static {
        Point b = new Point();
        b.y = BigInteger.valueOf(4).multiply(BigInteger.valueOf(5).modInverse(P_BI)).mod(P_BI);
        b.x = recoverX(b.y);

        D = Field25519.expand(toLittleEndian(D_BI));
        D2 = Field25519.expand(toLittleEndian(D2_BI));
        SQRTM1 = Field25519.expand(toLittleEndian(SQRTM1_BI));

        Point bi = b;
        B_TABLE = new CachedXYT[32][8];
        for (int i = 0; i < 32; i++) {
            Point bij = bi;
            for (int j = 0; j < 8; j++) {
                B_TABLE[i][j] = getCachedXYT(bij);
                bij = edwards(bij, bi);
            }
            for (int j = 0; j < 8; j++) {
                bi = edwards(bi, bi);
            }
        }
        bi = b;
        Point b2 = edwards(b, b);
        B2 = new CachedXYT[8];
        for (int i = 0; i < 8; i++) {
            B2[i] = getCachedXYT(bi);
            bi = edwards(bi, b2);
        }
    }

    private static final int PUBLIC_KEY_LEN = Field25519.FIELD_LEN;
    private static final int SIGNATURE_LEN = Field25519.FIELD_LEN * 2;

    /**
     * Defines field 25519 function based on <a
     * href="https://github.com/agl/curve25519-donna/blob/master/curve25519-donna.c">curve25519-donna C
     * implementation</a> (mostly identical).
     *
     * <p>Field elements are written as an array of signed, 64-bit limbs (an array of longs), least
     * significant first. The value of the field element is:
     *
     * <pre>
     * x[0] + 2^26·x[1] + 2^51·x[2] + 2^77·x[3] + 2^102·x[4] + 2^128·x[5] + 2^153·x[6] + 2^179·x[7] +
     * 2^204·x[8] + 2^230·x[9],
     * </pre>
     *
     * <p>i.e. the limbs are 26, 25, 26, 25, ... bits wide.
     */
    private static final class Field25519 {
        /**
         * During Field25519 computation, the mixed radix representation may be in different forms:
         * <ul>
         *  <li> Reduced-size form: the array has size at most 10.
         *  <li> Non-reduced-size form: the array is not reduced modulo 2^255 - 19 and has size at most
         *  19.
         * </ul>
         * <p>
         * TODO(quannguyen):
         * <ul>
         *  <li> Clarify ill-defined terminologies.
         *  <li> The reduction procedure is different from DJB's paper
         *  (http://cr.yp.to/ecdh/curve25519-20060209.pdf). The coefficients after reducing degree and
         *  reducing coefficients aren't guaranteed to be in range {-2^25, ..., 2^25}. We should check to
         *  see what's going on.
         *  <li> Consider using method mult() everywhere and making product() private.
         * </ul>
         */

        static final int FIELD_LEN = 32;
        static final int LIMB_CNT = 10;
        private static final long TWO_TO_25 = 1 << 25;
        private static final long TWO_TO_26 = TWO_TO_25 << 1;

        private static final int[] EXPAND_START = {0, 3, 6, 9, 12, 16, 19, 22, 25, 28};
        private static final int[] EXPAND_SHIFT = {0, 2, 3, 5, 6, 0, 1, 3, 4, 6};
        private static final int[] MASK = {0x3ffffff, 0x1ffffff};
        private static final int[] SHIFT = {26, 25};

        /**
         * Sums two numbers: output = in1 + in2
         * <p>
         * On entry: in1, in2 are in reduced-size form.
         */
        static void sum(long[] output, long[] in1, long[] in2) {
            for (int i = 0; i < LIMB_CNT; i++) {
                output[i] = in1[i] + in2[i];
            }
        }

        /**
         * Sums two numbers: output += in
         * <p>
         * On entry: in is in reduced-size form.
         */
        static void sum(long[] output, long[] in) {
            sum(output, output, in);
        }

        /**
         * Find the difference of two numbers: output = in1 - in2
         * (note the order of the arguments!).
         * <p>
         * On entry: in1, in2 are in reduced-size form.
         */
        static void sub(long[] output, long[] in1, long[] in2) {
            for (int i = 0; i < LIMB_CNT; i++) {
                output[i] = in1[i] - in2[i];
            }
        }

        /**
         * Find the difference of two numbers: output = in - output
         * (note the order of the arguments!).
         * <p>
         * On entry: in, output are in reduced-size form.
         */
        static void sub(long[] output, long[] in) {
            sub(output, in, output);
        }

        /**
         * Multiply a number by a scalar: output = in * scalar
         */
        static void scalarProduct(long[] output, long[] in, long scalar) {
            for (int i = 0; i < LIMB_CNT; i++) {
                output[i] = in[i] * scalar;
            }
        }

        /**
         * Multiply two numbers: out = in2 * in
         * <p>
         * output must be distinct to both inputs. The inputs are reduced coefficient form,
         * the output is not.
         * <p>
         * out[x] <= 14 * the largest product of the input limbs.
         */
        static void product(long[] out, long[] in2, long[] in) {
            out[0] = in2[0] * in[0];
            out[1] = in2[0] * in[1]
                    + in2[1] * in[0];
            out[2] = 2 * in2[1] * in[1]
                    + in2[0] * in[2]
                    + in2[2] * in[0];
            out[3] = in2[1] * in[2]
                    + in2[2] * in[1]
                    + in2[0] * in[3]
                    + in2[3] * in[0];
            out[4] = in2[2] * in[2]
                    + 2 * (in2[1] * in[3] + in2[3] * in[1])
                    + in2[0] * in[4]
                    + in2[4] * in[0];
            out[5] = in2[2] * in[3]
                    + in2[3] * in[2]
                    + in2[1] * in[4]
                    + in2[4] * in[1]
                    + in2[0] * in[5]
                    + in2[5] * in[0];
            out[6] = 2 * (in2[3] * in[3] + in2[1] * in[5] + in2[5] * in[1])
                    + in2[2] * in[4]
                    + in2[4] * in[2]
                    + in2[0] * in[6]
                    + in2[6] * in[0];
            out[7] = in2[3] * in[4]
                    + in2[4] * in[3]
                    + in2[2] * in[5]
                    + in2[5] * in[2]
                    + in2[1] * in[6]
                    + in2[6] * in[1]
                    + in2[0] * in[7]
                    + in2[7] * in[0];
            out[8] = in2[4] * in[4]
                    + 2 * (in2[3] * in[5] + in2[5] * in[3] + in2[1] * in[7] + in2[7] * in[1])
                    + in2[2] * in[6]
                    + in2[6] * in[2]
                    + in2[0] * in[8]
                    + in2[8] * in[0];
            out[9] = in2[4] * in[5]
                    + in2[5] * in[4]
                    + in2[3] * in[6]
                    + in2[6] * in[3]
                    + in2[2] * in[7]
                    + in2[7] * in[2]
                    + in2[1] * in[8]
                    + in2[8] * in[1]
                    + in2[0] * in[9]
                    + in2[9] * in[0];
            out[10] =
                    2 * (in2[5] * in[5] + in2[3] * in[7] + in2[7] * in[3] + in2[1] * in[9] + in2[9] * in[1])
                            + in2[4] * in[6]
                            + in2[6] * in[4]
                            + in2[2] * in[8]
                            + in2[8] * in[2];
            out[11] = in2[5] * in[6]
                    + in2[6] * in[5]
                    + in2[4] * in[7]
                    + in2[7] * in[4]
                    + in2[3] * in[8]
                    + in2[8] * in[3]
                    + in2[2] * in[9]
                    + in2[9] * in[2];
            out[12] = in2[6] * in[6]
                    + 2 * (in2[5] * in[7] + in2[7] * in[5] + in2[3] * in[9] + in2[9] * in[3])
                    + in2[4] * in[8]
                    + in2[8] * in[4];
            out[13] = in2[6] * in[7]
                    + in2[7] * in[6]
                    + in2[5] * in[8]
                    + in2[8] * in[5]
                    + in2[4] * in[9]
                    + in2[9] * in[4];
            out[14] = 2 * (in2[7] * in[7] + in2[5] * in[9] + in2[9] * in[5])
                    + in2[6] * in[8]
                    + in2[8] * in[6];
            out[15] = in2[7] * in[8]
                    + in2[8] * in[7]
                    + in2[6] * in[9]
                    + in2[9] * in[6];
            out[16] = in2[8] * in[8]
                    + 2 * (in2[7] * in[9] + in2[9] * in[7]);
            out[17] = in2[8] * in[9]
                    + in2[9] * in[8];
            out[18] = 2 * in2[9] * in[9];
        }

        /**
         * Reduce a field element by calling reduceSizeByModularReduction and reduceCoefficients.
         *
         * @param input  An input array of any length. If the array has 19 elements, it will be used as
         *               temporary buffer and its contents changed.
         * @param output An output array of size LIMB_CNT. After the call |output[i]| < 2^26 will hold.
         */
        static void reduce(long[] input, long[] output) {
            long[] tmp;
            if (input.length == 19) {
                tmp = input;
            } else {
                tmp = new long[19];
                System.arraycopy(input, 0, tmp, 0, input.length);
            }
            reduceSizeByModularReduction(tmp);
            reduceCoefficients(tmp);
            System.arraycopy(tmp, 0, output, 0, LIMB_CNT);
        }

        /**
         * Reduce a long form to a reduced-size form by taking the input mod 2^255 - 19.
         * <p>
         * On entry: |output[i]| < 14*2^54
         * On exit: |output[0..8]| < 280*2^54
         */
        static void reduceSizeByModularReduction(long[] output) {
            // The coefficients x[10], x[11],..., x[18] are eliminated by reduction modulo 2^255 - 19.
            // For example, the coefficient x[18] is multiplied by 19 and added to the coefficient x[8].
            //
            // Each of these shifts and adds ends up multiplying the value by 19.
            //
            // For output[0..8], the absolute entry value is < 14*2^54 and we add, at most, 19*14*2^54 thus,
            // on exit, |output[0..8]| < 280*2^54.
            output[8] += output[18] << 4;
            output[8] += output[18] << 1;
            output[8] += output[18];
            output[7] += output[17] << 4;
            output[7] += output[17] << 1;
            output[7] += output[17];
            output[6] += output[16] << 4;
            output[6] += output[16] << 1;
            output[6] += output[16];
            output[5] += output[15] << 4;
            output[5] += output[15] << 1;
            output[5] += output[15];
            output[4] += output[14] << 4;
            output[4] += output[14] << 1;
            output[4] += output[14];
            output[3] += output[13] << 4;
            output[3] += output[13] << 1;
            output[3] += output[13];
            output[2] += output[12] << 4;
            output[2] += output[12] << 1;
            output[2] += output[12];
            output[1] += output[11] << 4;
            output[1] += output[11] << 1;
            output[1] += output[11];
            output[0] += output[10] << 4;
            output[0] += output[10] << 1;
            output[0] += output[10];
        }

        /**
         * Reduce all coefficients of the short form input so that |x| < 2^26.
         * <p>
         * On entry: |output[i]| < 280*2^54
         */
        static void reduceCoefficients(long[] output) {
            output[10] = 0;

            for (int i = 0; i < LIMB_CNT; i += 2) {
                long over = output[i] / TWO_TO_26;
                // The entry condition (that |output[i]| < 280*2^54) means that over is, at most, 280*2^28 in
                // the first iteration of this loop. This is added to the next limb and we can approximate the
                // resulting bound of that limb by 281*2^54.
                output[i] -= over << 26;
                output[i + 1] += over;

                // For the first iteration, |output[i+1]| < 281*2^54, thus |over| < 281*2^29. When this is
                // added to the next limb, the resulting bound can be approximated as 281*2^54.
                //
                // For subsequent iterations of the loop, 281*2^54 remains a conservative bound and no
                // overflow occurs.
                over = output[i + 1] / TWO_TO_25;
                output[i + 1] -= over << 25;
                output[i + 2] += over;
            }
            // Now |output[10]| < 281*2^29 and all other coefficients are reduced.
            output[0] += output[10] << 4;
            output[0] += output[10] << 1;
            output[0] += output[10];

            output[10] = 0;
            // Now output[1..9] are reduced, and |output[0]| < 2^26 + 19*281*2^29 so |over| will be no more
            // than 2^16.
            long over = output[0] / TWO_TO_26;
            output[0] -= over << 26;
            output[1] += over;
            // Now output[0,2..9] are reduced, and |output[1]| < 2^25 + 2^16 < 2^26. The bound on
            // |output[1]| is sufficient to meet our needs.
        }

        /**
         * A helpful wrapper around {@ref Field25519#product}: output = in * in2.
         * <p>
         * On entry: |in[i]| < 2^27 and |in2[i]| < 2^27.
         * <p>
         * The output is reduced degree (indeed, one need only provide storage for 10 limbs) and
         * |output[i]| < 2^26.
         */
        static void mult(long[] output, long[] in, long[] in2) {
            long[] t = new long[19];
            product(t, in, in2);
            // |t[i]| < 2^26
            reduce(t, output);
        }

        /**
         * Square a number: out = in**2
         * <p>
         * output must be distinct from the input. The inputs are reduced coefficient form, the output is
         * not.
         * <p>
         * out[x] <= 14 * the largest product of the input limbs.
         */
        private static void squareInner(long[] out, long[] in) {
            out[0] = in[0] * in[0];
            out[1] = 2 * in[0] * in[1];
            out[2] = 2 * (in[1] * in[1] + in[0] * in[2]);
            out[3] = 2 * (in[1] * in[2] + in[0] * in[3]);
            out[4] = in[2] * in[2]
                    + 4 * in[1] * in[3]
                    + 2 * in[0] * in[4];
            out[5] = 2 * (in[2] * in[3] + in[1] * in[4] + in[0] * in[5]);
            out[6] = 2 * (in[3] * in[3] + in[2] * in[4] + in[0] * in[6] + 2 * in[1] * in[5]);
            out[7] = 2 * (in[3] * in[4] + in[2] * in[5] + in[1] * in[6] + in[0] * in[7]);
            out[8] = in[4] * in[4]
                    + 2 * (in[2] * in[6] + in[0] * in[8] + 2 * (in[1] * in[7] + in[3] * in[5]));
            out[9] = 2 * (in[4] * in[5] + in[3] * in[6] + in[2] * in[7] + in[1] * in[8] + in[0] * in[9]);
            out[10] = 2 * (in[5] * in[5]
                    + in[4] * in[6]
                    + in[2] * in[8]
                    + 2 * (in[3] * in[7] + in[1] * in[9]));
            out[11] = 2 * (in[5] * in[6] + in[4] * in[7] + in[3] * in[8] + in[2] * in[9]);
            out[12] = in[6] * in[6]
                    + 2 * (in[4] * in[8] + 2 * (in[5] * in[7] + in[3] * in[9]));
            out[13] = 2 * (in[6] * in[7] + in[5] * in[8] + in[4] * in[9]);
            out[14] = 2 * (in[7] * in[7] + in[6] * in[8] + 2 * in[5] * in[9]);
            out[15] = 2 * (in[7] * in[8] + in[6] * in[9]);
            out[16] = in[8] * in[8] + 4 * in[7] * in[9];
            out[17] = 2 * in[8] * in[9];
            out[18] = 2 * in[9] * in[9];
        }

        /**
         * Returns in^2.
         * <p>
         * On entry: The |in| argument is in reduced coefficients form and |in[i]| < 2^27.
         * <p>
         * On exit: The |output| argument is in reduced coefficients form (indeed, one need only provide
         * storage for 10 limbs) and |out[i]| < 2^26.
         */
        static void square(long[] output, long[] in) {
            long[] t = new long[19];
            squareInner(t, in);
            // |t[i]| < 14*2^54 because the largest product of two limbs will be < 2^(27+27) and SquareInner
            // adds together, at most, 14 of those products.
            reduce(t, output);
        }

        /**
         * Takes a little-endian, 32-byte number and expands it into mixed radix form.
         */
        static long[] expand(byte[] input) {
            long[] output = new long[LIMB_CNT];
            for (int i = 0; i < LIMB_CNT; i++) {
                output[i] = ((((long) (input[EXPAND_START[i]] & 0xff))
                        | ((long) (input[EXPAND_START[i] + 1] & 0xff)) << 8
                        | ((long) (input[EXPAND_START[i] + 2] & 0xff)) << 16
                        | ((long) (input[EXPAND_START[i] + 3] & 0xff)) << 24) >> EXPAND_SHIFT[i]) & MASK[i & 1];
            }
            return output;
        }

        /**
         * Takes a fully reduced mixed radix form number and contract it into a little-endian, 32-byte
         * array.
         * <p>
         * On entry: |input_limbs[i]| < 2^26
         */
        @SuppressWarnings("NarrowingCompoundAssignment")
        static byte[] contract(long[] inputLimbs) {
            long[] input = Arrays.copyOf(inputLimbs, LIMB_CNT);
            for (int j = 0; j < 2; j++) {
                for (int i = 0; i < 9; i++) {
                    // This calculation is a time-invariant way to make input[i] non-negative by borrowing
                    // from the next-larger limb.
                    int carry = -(int) ((input[i] & (input[i] >> 31)) >> SHIFT[i & 1]);
                    input[i] = input[i] + (carry << SHIFT[i & 1]);
                    input[i + 1] -= carry;
                }

                // There's no greater limb for input[9] to borrow from, but we can multiply by 19 and borrow
                // from input[0], which is valid mod 2^255-19.
                {
                    int carry = -(int) ((input[9] & (input[9] >> 31)) >> 25);
                    input[9] += (carry << 25);
                    input[0] -= (carry * 19);
                }

                // After the first iteration, input[1..9] are non-negative and fit within 25 or 26 bits,
                // depending on position. However, input[0] may be negative.
            }

            // The first borrow-propagation pass above ended with every limb except (possibly) input[0]
            // non-negative.
            //
            // If input[0] was negative after the first pass, then it was because of a carry from input[9].
            // On entry, input[9] < 2^26 so the carry was, at most, one, since (2**26-1) >> 25 = 1. Thus
            // input[0] >= -19.
            //
            // In the second pass, each limb is decreased by at most one. Thus the second borrow-propagation
            // pass could only have wrapped around to decrease input[0] again if the first pass left
            // input[0] negative *and* input[1] through input[9] were all zero.  In that case, input[1] is
            // now 2^25 - 1, and this last borrow-propagation step will leave input[1] non-negative.
            {
                int carry = -(int) ((input[0] & (input[0] >> 31)) >> 26);
                input[0] += (carry << 26);
                input[1] -= carry;
            }

            // All input[i] are now non-negative. However, there might be values between 2^25 and 2^26 in a
            // limb which is, nominally, 25 bits wide.
            for (int j = 0; j < 2; j++) {
                for (int i = 0; i < 9; i++) {
                    int carry = (int) (input[i] >> SHIFT[i & 1]);
                    input[i] &= MASK[i & 1];
                    input[i + 1] += carry;
                }
            }

            {
                int carry = (int) (input[9] >> 25);
                input[9] &= 0x1ffffff;
                input[0] += 19 * carry;
            }

            // If the first carry-chain pass, just above, ended up with a carry from input[9], and that
            // caused input[0] to be out-of-bounds, then input[0] was < 2^26 + 2*19, because the carry was,
            // at most, two.
            //
            // If the second pass carried from input[9] again then input[0] is < 2*19 and the input[9] ->
            // input[0] carry didn't push input[0] out of bounds.

            // It still remains the case that input might be between 2^255-19 and 2^255. In this case,
            // input[1..9] must take their maximum value and input[0] must be >= (2^255-19) & 0x3ffffff,
            // which is 0x3ffffed.
            int mask = gte((int) input[0], 0x3ffffed);
            for (int i = 1; i < LIMB_CNT; i++) {
                mask &= eq((int) input[i], MASK[i & 1]);
            }

            // mask is either 0xffffffff (if input >= 2^255-19) and zero otherwise. Thus this conditionally
            // subtracts 2^255-19.
            input[0] -= mask & 0x3ffffed;
            input[1] -= mask & 0x1ffffff;
            for (int i = 2; i < LIMB_CNT; i += 2) {
                input[i] -= mask & 0x3ffffff;
                input[i + 1] -= mask & 0x1ffffff;
            }

            for (int i = 0; i < LIMB_CNT; i++) {
                input[i] <<= EXPAND_SHIFT[i];
            }
            byte[] output = new byte[FIELD_LEN];
            for (int i = 0; i < LIMB_CNT; i++) {
                output[EXPAND_START[i]] |= input[i] & 0xff;
                output[EXPAND_START[i] + 1] |= (input[i] >> 8) & 0xff;
                output[EXPAND_START[i] + 2] |= (input[i] >> 16) & 0xff;
                output[EXPAND_START[i] + 3] |= (input[i] >> 24) & 0xff;
            }
            return output;
        }

        /**
         * Computes inverse of z = z(2^255 - 21)
         * <p>
         * Shamelessly copied from agl's code which was shamelessly copied from djb's code. Only the
         * comment format and the variable namings are different from those.
         */
        static void inverse(long[] out, long[] z) {
            long[] z2 = new long[Field25519.LIMB_CNT];
            long[] z9 = new long[Field25519.LIMB_CNT];
            long[] z11 = new long[Field25519.LIMB_CNT];
            long[] z2To5Minus1 = new long[Field25519.LIMB_CNT];
            long[] z2To10Minus1 = new long[Field25519.LIMB_CNT];
            long[] z2To20Minus1 = new long[Field25519.LIMB_CNT];
            long[] z2To50Minus1 = new long[Field25519.LIMB_CNT];
            long[] z2To100Minus1 = new long[Field25519.LIMB_CNT];
            long[] t0 = new long[Field25519.LIMB_CNT];
            long[] t1 = new long[Field25519.LIMB_CNT];

            square(z2, z);                          // 2
            square(t1, z2);                         // 4
            square(t0, t1);                         // 8
            mult(z9, t0, z);                        // 9
            mult(z11, z9, z2);                      // 11
            square(t0, z11);                        // 22
            mult(z2To5Minus1, t0, z9);              // 2^5 - 2^0 = 31

            square(t0, z2To5Minus1);                // 2^6 - 2^1
            square(t1, t0);                         // 2^7 - 2^2
            square(t0, t1);                         // 2^8 - 2^3
            square(t1, t0);                         // 2^9 - 2^4
            square(t0, t1);                         // 2^10 - 2^5
            mult(z2To10Minus1, t0, z2To5Minus1);    // 2^10 - 2^0

            square(t0, z2To10Minus1);               // 2^11 - 2^1
            square(t1, t0);                         // 2^12 - 2^2
            for (int i = 2; i < 10; i += 2) {       // 2^20 - 2^10
                square(t0, t1);
                square(t1, t0);
            }
            mult(z2To20Minus1, t1, z2To10Minus1);   // 2^20 - 2^0

            square(t0, z2To20Minus1);               // 2^21 - 2^1
            square(t1, t0);                         // 2^22 - 2^2
            for (int i = 2; i < 20; i += 2) {       // 2^40 - 2^20
                square(t0, t1);
                square(t1, t0);
            }
            mult(t0, t1, z2To20Minus1);             // 2^40 - 2^0

            square(t1, t0);                         // 2^41 - 2^1
            square(t0, t1);                         // 2^42 - 2^2
            for (int i = 2; i < 10; i += 2) {       // 2^50 - 2^10
                square(t1, t0);
                square(t0, t1);
            }
            mult(z2To50Minus1, t0, z2To10Minus1);   // 2^50 - 2^0

            square(t0, z2To50Minus1);               // 2^51 - 2^1
            square(t1, t0);                         // 2^52 - 2^2
            for (int i = 2; i < 50; i += 2) {       // 2^100 - 2^50
                square(t0, t1);
                square(t1, t0);
            }
            mult(z2To100Minus1, t1, z2To50Minus1);  // 2^100 - 2^0

            square(t1, z2To100Minus1);              // 2^101 - 2^1
            square(t0, t1);                         // 2^102 - 2^2
            for (int i = 2; i < 100; i += 2) {      // 2^200 - 2^100
                square(t1, t0);
                square(t0, t1);
            }
            mult(t1, t0, z2To100Minus1);            // 2^200 - 2^0

            square(t0, t1);                         // 2^201 - 2^1
            square(t1, t0);                         // 2^202 - 2^2
            for (int i = 2; i < 50; i += 2) {       // 2^250 - 2^50
                square(t0, t1);
                square(t1, t0);
            }
            mult(t0, t1, z2To50Minus1);             // 2^250 - 2^0

            square(t1, t0);                         // 2^251 - 2^1
            square(t0, t1);                         // 2^252 - 2^2
            square(t1, t0);                         // 2^253 - 2^3
            square(t0, t1);                         // 2^254 - 2^4
            square(t1, t0);                         // 2^255 - 2^5
            mult(out, t1, z11);                     // 2^255 - 21
        }


        /**
         * Returns 0xffffffff iff a == b and zero otherwise.
         */
        private static int eq(int a, int b) {
            a = ~(a ^ b);
            a &= a << 16;
            a &= a << 8;
            a &= a << 4;
            a &= a << 2;
            a &= a << 1;
            return a >> 31;
        }

        /**
         * returns 0xffffffff if a >= b and zero otherwise, where a and b are both non-negative.
         */
        private static int gte(int a, int b) {
            a -= b;
            // a >= 0 iff a >= b.
            return ~(a >> 31);
        }
    }

    // (x = 0, y = 1) point
    private static final CachedXYT CACHED_NEUTRAL = new CachedXYT(
            new long[]{1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            new long[]{1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            new long[]{0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    private static final PartialXYZT NEUTRAL = new PartialXYZT(
            new XYZ(new long[]{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                    new long[]{1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                    new long[]{1, 0, 0, 0, 0, 0, 0, 0, 0, 0}),
            new long[]{1, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    /**
     * Projective point representation (X:Y:Z) satisfying x = X/Z, y = Y/Z
     * <p>
     * Note that this is referred as ge_p2 in ref10 impl.
     * Also note that x = X, y = Y and z = Z below following Java coding style.
     * <p>
     * See
     * Koyama K., Tsuruoka Y. (1993) Speeding up Elliptic Cryptosystems by Using a Signed Binary
     * Window Method.
     * <p>
     * https://hyperelliptic.org/EFD/g1p/auto-twisted-projective.html
     */
    private static class XYZ {

        final long[] x;
        final long[] y;
        final long[] z;

        XYZ() {
            this(new long[Field25519.LIMB_CNT], new long[Field25519.LIMB_CNT], new long[Field25519.LIMB_CNT]);
        }

        XYZ(long[] x, long[] y, long[] z) {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        XYZ(XYZ xyz) {
            x = Arrays.copyOf(xyz.x, Field25519.LIMB_CNT);
            y = Arrays.copyOf(xyz.y, Field25519.LIMB_CNT);
            z = Arrays.copyOf(xyz.z, Field25519.LIMB_CNT);
        }

        XYZ(PartialXYZT partialXYZT) {
            this();
            fromPartialXYZT(this, partialXYZT);
        }

        /**
         * ge_p1p1_to_p2.c
         */
        static XYZ fromPartialXYZT(XYZ out, PartialXYZT in) {
            Field25519.mult(out.x, in.xyz.x, in.t);
            Field25519.mult(out.y, in.xyz.y, in.xyz.z);
            Field25519.mult(out.z, in.xyz.z, in.t);
            return out;
        }

        /**
         * Encodes this point to bytes.
         */
        byte[] toBytes() {
            long[] recip = new long[Field25519.LIMB_CNT];
            long[] x = new long[Field25519.LIMB_CNT];
            long[] y = new long[Field25519.LIMB_CNT];
            Field25519.inverse(recip, z);
            Field25519.mult(x, this.x, recip);
            Field25519.mult(y, this.y, recip);
            byte[] s = Field25519.contract(y);
            s[31] = (byte) (s[31] ^ (getLsb(x) << 7));
            return s;
        }


        /**
         * Best effort fix-timing array comparison.
         *
         * @return true if two arrays are equal.
         */
        private static boolean bytesEqual(final byte[] x, final byte[] y) {
            if (x == null || y == null) {
                return false;
            }
            if (x.length != y.length) {
                return false;
            }
            int res = 0;
            for (int i = 0; i < x.length; i++) {
                res |= x[i] ^ y[i];
            }
            return res == 0;
        }

        /**
         * Checks that the point is on curve
         */
        boolean isOnCurve() {
            long[] x2 = new long[Field25519.LIMB_CNT];
            Field25519.square(x2, x);
            long[] y2 = new long[Field25519.LIMB_CNT];
            Field25519.square(y2, y);
            long[] z2 = new long[Field25519.LIMB_CNT];
            Field25519.square(z2, z);
            long[] z4 = new long[Field25519.LIMB_CNT];
            Field25519.square(z4, z2);
            long[] lhs = new long[Field25519.LIMB_CNT];
            // lhs = y^2 - x^2
            Field25519.sub(lhs, y2, x2);
            // lhs = z^2 * (y2 - x2)
            Field25519.mult(lhs, lhs, z2);
            long[] rhs = new long[Field25519.LIMB_CNT];
            // rhs = x^2 * y^2
            Field25519.mult(rhs, x2, y2);
            // rhs = D * x^2 * y^2
            Field25519.mult(rhs, rhs, D);
            // rhs = z^4 + D * x^2 * y^2
            Field25519.sum(rhs, z4);
            // Field25519.mult reduces its output, but Field25519.sum does not, so we have to manually
            // reduce it here.
            Field25519.reduce(rhs, rhs);
            // z^2 (y^2 - x^2) == z^4 + D * x^2 * y^2
            return bytesEqual(Field25519.contract(lhs), Field25519.contract(rhs));
        }
    }

    /**
     * Represents extended projective point representation (X:Y:Z:T) satisfying x = X/Z, y = Y/Z,
     * XY = ZT
     * <p>
     * Note that this is referred as ge_p3 in ref10 impl.
     * Also note that t = T below following Java coding style.
     * <p>
     * See
     * Hisil H., Wong K.KH., Carter G., Dawson E. (2008) Twisted Edwards Curves Revisited.
     * <p>
     * https://hyperelliptic.org/EFD/g1p/auto-twisted-extended.html
     */
    private static class XYZT {

        final XYZ xyz;
        final long[] t;

        XYZT() {
            this(new XYZ(), new long[Field25519.LIMB_CNT]);
        }

        XYZT(XYZ xyz, long[] t) {
            this.xyz = xyz;
            this.t = t;
        }

        XYZT(PartialXYZT partialXYZT) {
            this();
            fromPartialXYZT(this, partialXYZT);
        }

        /**
         * ge_p1p1_to_p2.c
         */
        private static XYZT fromPartialXYZT(XYZT out, PartialXYZT in) {
            Field25519.mult(out.xyz.x, in.xyz.x, in.t);
            Field25519.mult(out.xyz.y, in.xyz.y, in.xyz.z);
            Field25519.mult(out.xyz.z, in.xyz.z, in.t);
            Field25519.mult(out.t, in.xyz.x, in.xyz.y);
            return out;
        }

        /**
         * Decodes {@code s} into an extented projective point.
         * See Section 5.1.3 Decoding in https://tools.ietf.org/html/rfc8032#section-5.1.3
         */
        private static XYZT fromBytesNegateVarTime(byte[] s) throws GeneralSecurityException {
            long[] x = new long[Field25519.LIMB_CNT];
            long[] y = Field25519.expand(s);
            long[] z = new long[Field25519.LIMB_CNT];
            z[0] = 1;
            long[] t = new long[Field25519.LIMB_CNT];
            long[] u = new long[Field25519.LIMB_CNT];
            long[] v = new long[Field25519.LIMB_CNT];
            long[] vxx = new long[Field25519.LIMB_CNT];
            long[] check = new long[Field25519.LIMB_CNT];
            Field25519.square(u, y);
            Field25519.mult(v, u, D);
            Field25519.sub(u, u, z); // u = y^2 - 1
            Field25519.sum(v, v, z); // v = dy^2 + 1

            long[] v3 = new long[Field25519.LIMB_CNT];
            Field25519.square(v3, v);
            Field25519.mult(v3, v3, v); // v3 = v^3
            Field25519.square(x, v3);
            Field25519.mult(x, x, v);
            Field25519.mult(x, x, u); // x = uv^7

            pow2252m3(x, x); // x = (uv^7)^((q-5)/8)
            Field25519.mult(x, x, v3);
            Field25519.mult(x, x, u); // x = uv^3(uv^7)^((q-5)/8)

            Field25519.square(vxx, x);
            Field25519.mult(vxx, vxx, v);
            Field25519.sub(check, vxx, u); // vx^2-u
            if (isNonZeroVarTime(check)) {
                Field25519.sum(check, vxx, u); // vx^2+u
                if (isNonZeroVarTime(check)) {
                    throw new GeneralSecurityException("Cannot convert given bytes to extended projective "
                            + "coordinates. No square root exists for modulo 2^255-19");
                }
                Field25519.mult(x, x, SQRTM1);
            }

            if (!isNonZeroVarTime(x) && (s[31] & 0xff) >> 7 != 0) {
                throw new GeneralSecurityException("Cannot convert given bytes to extended projective "
                        + "coordinates. Computed x is zero and encoded x's least significant bit is not zero");
            }
            if (getLsb(x) == ((s[31] & 0xff) >> 7)) {
                neg(x, x);
            }

            Field25519.mult(t, x, y);
            return new XYZT(new XYZ(x, y, z), t);
        }
    }

    /**
     * Partial projective point representation ((X:Z),(Y:T)) satisfying x=X/Z, y=Y/T
     * <p>
     * Note that this is referred as complete form in the original ref10 impl (ge_p1p1).
     * Also note that t = T below following Java coding style.
     * <p>
     * Although this has the same types as XYZT, it is redefined to have its own type so that it is
     * readable and 1:1 corresponds to ref10 impl.
     * <p>
     * Can be converted to XYZT as follows:
     * X1 = X * T = x * Z * T = x * Z1
     * Y1 = Y * Z = y * T * Z = y * Z1
     * Z1 = Z * T = Z * T
     * T1 = X * Y = x * Z * y * T = x * y * Z1 = X1Y1 / Z1
     */
    private static class PartialXYZT {

        final XYZ xyz;
        final long[] t;

        PartialXYZT() {
            this(new XYZ(), new long[Field25519.LIMB_CNT]);
        }

        PartialXYZT(XYZ xyz, long[] t) {
            this.xyz = xyz;
            this.t = t;
        }

        PartialXYZT(PartialXYZT other) {
            xyz = new XYZ(other.xyz);
            t = Arrays.copyOf(other.t, Field25519.LIMB_CNT);
        }
    }

    /**
     * Corresponds to the caching mentioned in the last paragraph of Section 3.1 of
     * Hisil H., Wong K.KH., Carter G., Dawson E. (2008) Twisted Edwards Curves Revisited.
     * with Z = 1.
     */
    private static class CachedXYT {

        final long[] yPlusX;
        final long[] yMinusX;
        final long[] t2d;

        /**
         * Creates a cached XYZT with Z = 1
         *
         * @param yPlusX  y + x
         * @param yMinusX y - x
         * @param t2d     2d * xy
         */
        CachedXYT(long[] yPlusX, long[] yMinusX, long[] t2d) {
            this.yPlusX = yPlusX;
            this.yMinusX = yMinusX;
            this.t2d = t2d;
        }

        CachedXYT(CachedXYT other) {
            yPlusX = Arrays.copyOf(other.yPlusX, Field25519.LIMB_CNT);
            yMinusX = Arrays.copyOf(other.yMinusX, Field25519.LIMB_CNT);
            t2d = Arrays.copyOf(other.t2d, Field25519.LIMB_CNT);
        }

        // z is one implicitly, so this just copies {@code in} to {@code output}.
        void multByZ(long[] output, long[] in) {
            System.arraycopy(in, 0, output, 0, Field25519.LIMB_CNT);
        }

        /**
         * If icopy is 1, copies {@code other} into this point. Time invariant wrt to icopy value.
         */
        void copyConditional(CachedXYT other, int icopy) {
            copyConditional(yPlusX, other.yPlusX, icopy);
            copyConditional(yMinusX, other.yMinusX, icopy);
            copyConditional(t2d, other.t2d, icopy);
        }

        /**
         * Conditionally copies a reduced-form limb arrays {@code b} into {@code a} if {@code icopy} is 1,
         * but leave {@code a} unchanged if 'iswap' is 0. Runs in data-invariant time to avoid
         * side-channel attacks.
         *
         * <p>NOTE that this function requires that {@code icopy} be 1 or 0; other values give wrong
         * results. Also, the two limb arrays must be in reduced-coefficient, reduced-degree form: the
         * values in a[10..19] or b[10..19] aren't swapped, and all all values in a[0..9],b[0..9] must
         * have magnitude less than Integer.MAX_VALUE.
         */
        static void copyConditional(long[] a, long[] b, int icopy) {
            int copy = -icopy;
            for (int i = 0; i < Field25519.LIMB_CNT; i++) {
                int x = copy & (((int) a[i]) ^ ((int) b[i]));
                a[i] = ((int) a[i]) ^ x;
            }
        }
    }

    private static class CachedXYZT extends CachedXYT {

        private final long[] z;

        CachedXYZT() {
            this(new long[Field25519.LIMB_CNT], new long[Field25519.LIMB_CNT], new long[Field25519.LIMB_CNT], new long[Field25519.LIMB_CNT]);
        }

        /**
         * ge_p3_to_cached.c
         */
        CachedXYZT(XYZT xyzt) {
            this();
            Field25519.sum(yPlusX, xyzt.xyz.y, xyzt.xyz.x);
            Field25519.sub(yMinusX, xyzt.xyz.y, xyzt.xyz.x);
            System.arraycopy(xyzt.xyz.z, 0, z, 0, Field25519.LIMB_CNT);
            Field25519.mult(t2d, xyzt.t, D2);
        }

        /**
         * Creates a cached XYZT
         *
         * @param yPlusX  Y + X
         * @param yMinusX Y - X
         * @param z       Z
         * @param t2d     2d * (XY/Z)
         */
        CachedXYZT(long[] yPlusX, long[] yMinusX, long[] z, long[] t2d) {
            super(yPlusX, yMinusX, t2d);
            this.z = z;
        }

        @Override
        public void multByZ(long[] output, long[] in) {
            Field25519.mult(output, in, z);
        }
    }

    /**
     * Addition defined in Section 3.1 of
     * Hisil H., Wong K.KH., Carter G., Dawson E. (2008) Twisted Edwards Curves Revisited.
     * <p>
     * Please note that this is a partial of the operation listed there leaving out the final
     * conversion from PartialXYZT to XYZT.
     *
     * @param extended extended projective point input
     * @param cached   cached projective point input
     */
    private static void add(PartialXYZT partialXYZT, XYZT extended, CachedXYT cached) {
        long[] t = new long[Field25519.LIMB_CNT];

        // Y1 + X1
        Field25519.sum(partialXYZT.xyz.x, extended.xyz.y, extended.xyz.x);

        // Y1 - X1
        Field25519.sub(partialXYZT.xyz.y, extended.xyz.y, extended.xyz.x);

        // A = (Y1 - X1) * (Y2 - X2)
        Field25519.mult(partialXYZT.xyz.y, partialXYZT.xyz.y, cached.yMinusX);

        // B = (Y1 + X1) * (Y2 + X2)
        Field25519.mult(partialXYZT.xyz.z, partialXYZT.xyz.x, cached.yPlusX);

        // C = T1 * 2d * T2 = 2d * T1 * T2 (2d is written as k in the paper)
        Field25519.mult(partialXYZT.t, extended.t, cached.t2d);

        // Z1 * Z2
        cached.multByZ(partialXYZT.xyz.x, extended.xyz.z);

        // D = 2 * Z1 * Z2
        Field25519.sum(t, partialXYZT.xyz.x, partialXYZT.xyz.x);

        // X3 = B - A
        Field25519.sub(partialXYZT.xyz.x, partialXYZT.xyz.z, partialXYZT.xyz.y);

        // Y3 = B + A
        Field25519.sum(partialXYZT.xyz.y, partialXYZT.xyz.z, partialXYZT.xyz.y);

        // Z3 = D + C
        Field25519.sum(partialXYZT.xyz.z, t, partialXYZT.t);

        // T3 = D - C
        Field25519.sub(partialXYZT.t, t, partialXYZT.t);
    }

    /**
     * Based on the addition defined in Section 3.1 of
     * Hisil H., Wong K.KH., Carter G., Dawson E. (2008) Twisted Edwards Curves Revisited.
     * <p>
     * Please note that this is a partial of the operation listed there leaving out the final
     * conversion from PartialXYZT to XYZT.
     *
     * @param extended extended projective point input
     * @param cached   cached projective point input
     */
    private static void sub(PartialXYZT partialXYZT, XYZT extended, CachedXYT cached) {
        long[] t = new long[Field25519.LIMB_CNT];

        // Y1 + X1
        Field25519.sum(partialXYZT.xyz.x, extended.xyz.y, extended.xyz.x);

        // Y1 - X1
        Field25519.sub(partialXYZT.xyz.y, extended.xyz.y, extended.xyz.x);

        // A = (Y1 - X1) * (Y2 + X2)
        Field25519.mult(partialXYZT.xyz.y, partialXYZT.xyz.y, cached.yPlusX);

        // B = (Y1 + X1) * (Y2 - X2)
        Field25519.mult(partialXYZT.xyz.z, partialXYZT.xyz.x, cached.yMinusX);

        // C = T1 * 2d * T2 = 2d * T1 * T2 (2d is written as k in the paper)
        Field25519.mult(partialXYZT.t, extended.t, cached.t2d);

        // Z1 * Z2
        cached.multByZ(partialXYZT.xyz.x, extended.xyz.z);

        // D = 2 * Z1 * Z2
        Field25519.sum(t, partialXYZT.xyz.x, partialXYZT.xyz.x);

        // X3 = B - A
        Field25519.sub(partialXYZT.xyz.x, partialXYZT.xyz.z, partialXYZT.xyz.y);

        // Y3 = B + A
        Field25519.sum(partialXYZT.xyz.y, partialXYZT.xyz.z, partialXYZT.xyz.y);

        // Z3 = D - C
        Field25519.sub(partialXYZT.xyz.z, t, partialXYZT.t);

        // T3 = D + C
        Field25519.sum(partialXYZT.t, t, partialXYZT.t);
    }

    /**
     * Doubles {@code p} and puts the result into this PartialXYZT.
     * <p>
     * This is based on the addition defined in formula 7 in Section 3.3 of
     * Hisil H., Wong K.KH., Carter G., Dawson E. (2008) Twisted Edwards Curves Revisited.
     * <p>
     * Please note that this is a partial of the operation listed there leaving out the final
     * conversion from PartialXYZT to XYZT and also this fixes a typo in calculation of Y3 and T3 in
     * the paper, H should be replaced with A+B.
     */
    private static void doubleXYZ(PartialXYZT partialXYZT, XYZ p) {
        long[] t0 = new long[Field25519.LIMB_CNT];

        // XX = X1^2
        Field25519.square(partialXYZT.xyz.x, p.x);

        // YY = Y1^2
        Field25519.square(partialXYZT.xyz.z, p.y);

        // B' = Z1^2
        Field25519.square(partialXYZT.t, p.z);

        // B = 2 * B'
        Field25519.sum(partialXYZT.t, partialXYZT.t, partialXYZT.t);

        // A = X1 + Y1
        Field25519.sum(partialXYZT.xyz.y, p.x, p.y);

        // AA = A^2
        Field25519.square(t0, partialXYZT.xyz.y);

        // Y3 = YY + XX
        Field25519.sum(partialXYZT.xyz.y, partialXYZT.xyz.z, partialXYZT.xyz.x);

        // Z3 = YY - XX
        Field25519.sub(partialXYZT.xyz.z, partialXYZT.xyz.z, partialXYZT.xyz.x);

        // X3 = AA - Y3
        Field25519.sub(partialXYZT.xyz.x, t0, partialXYZT.xyz.y);

        // T3 = B - Z3
        Field25519.sub(partialXYZT.t, partialXYZT.t, partialXYZT.xyz.z);
    }

    /**
     * Doubles {@code p} and puts the result into this PartialXYZT.
     */
    private static void doubleXYZT(PartialXYZT partialXYZT, XYZT p) {
        doubleXYZ(partialXYZT, p.xyz);
    }

    /**
     * Compares two byte values in constant time.
     */
    private static int eq(int a, int b) {
        int r = ~(a ^ b) & 0xff;
        r &= r << 4;
        r &= r << 2;
        r &= r << 1;
        return (r >> 7) & 1;
    }

    /**
     * This is a constant time operation where point b*B*256^pos is stored in {@code t}.
     * When b is 0, t remains the same (i.e., neutral point).
     * <p>
     * Although B_TABLE[32][8] (B_TABLE[i][j] = (j+1)*B*256^i) has j values in [0, 7], the select
     * method negates the corresponding point if b is negative (which is straight forward in elliptic
     * curves by just negating y coordinate). Therefore we can get multiples of B with the half of
     * memory requirements.
     *
     * @param t   neutral element (i.e., point 0), also serves as output.
     * @param pos in B[pos][j] = (j+1)*B*256^pos
     * @param b   value in [-8, 8] range.
     */
    private static void select(CachedXYT t, int pos, byte b) {
        int bnegative = (b & 0xff) >> 7;
        int babs = b - (((-bnegative) & b) << 1);

        t.copyConditional(B_TABLE[pos][0], eq(babs, 1));
        t.copyConditional(B_TABLE[pos][1], eq(babs, 2));
        t.copyConditional(B_TABLE[pos][2], eq(babs, 3));
        t.copyConditional(B_TABLE[pos][3], eq(babs, 4));
        t.copyConditional(B_TABLE[pos][4], eq(babs, 5));
        t.copyConditional(B_TABLE[pos][5], eq(babs, 6));
        t.copyConditional(B_TABLE[pos][6], eq(babs, 7));
        t.copyConditional(B_TABLE[pos][7], eq(babs, 8));

        long[] yPlusX = Arrays.copyOf(t.yMinusX, Field25519.LIMB_CNT);
        long[] yMinusX = Arrays.copyOf(t.yPlusX, Field25519.LIMB_CNT);
        long[] t2d = Arrays.copyOf(t.t2d, Field25519.LIMB_CNT);
        neg(t2d, t2d);
        CachedXYT minust = new CachedXYT(yPlusX, yMinusX, t2d);
        t.copyConditional(minust, bnegative);
    }

    /**
     * Computes {@code a}*B
     * where a = a[0]+256*a[1]+...+256^31 a[31] and
     * B is the Ed25519 base point (x,4/5) with x positive.
     * <p>
     * Preconditions:
     * a[31] <= 127
     *
     * @throws IllegalStateException iff there is arithmetic error.
     */
    @SuppressWarnings("NarrowingCompoundAssignment")
    private static XYZ scalarMultWithBase(byte[] a) {
        byte[] e = new byte[2 * Field25519.FIELD_LEN];
        for (int i = 0; i < Field25519.FIELD_LEN; i++) {
            e[2 * i + 0] = (byte) (((a[i] & 0xff) >> 0) & 0xf);
            e[2 * i + 1] = (byte) (((a[i] & 0xff) >> 4) & 0xf);
        }
        // each e[i] is between 0 and 15
        // e[63] is between 0 and 7

        // Rewrite e in a way that each e[i] is in [-8, 8].
        // This can be done since a[63] is in [0, 7], the carry-over onto the most significant byte
        // a[63] can be at most 1.
        int carry = 0;
        for (int i = 0; i < e.length - 1; i++) {
            e[i] += carry;
            carry = e[i] + 8;
            carry >>= 4;
            e[i] -= carry << 4;
        }
        e[e.length - 1] += carry;

        PartialXYZT ret = new PartialXYZT(NEUTRAL);
        XYZT xyzt = new XYZT();
        // Although B_TABLE's i can be at most 31 (stores only 32 4bit multiples of B) and we have 64
        // 4bit values in e array, the below for loop adds cached values by iterating e by two in odd
        // indices. After the result, we can double the result point 4 times to shift the multiplication
        // scalar by 4 bits.
        for (int i = 1; i < e.length; i += 2) {
            CachedXYT t = new CachedXYT(CACHED_NEUTRAL);
            select(t, i / 2, e[i]);
            add(ret, XYZT.fromPartialXYZT(xyzt, ret), t);
        }

        // Doubles the result 4 times to shift the multiplication scalar 4 bits to get the actual result
        // for the odd indices in e.
        XYZ xyz = new XYZ();
        doubleXYZ(ret, XYZ.fromPartialXYZT(xyz, ret));
        doubleXYZ(ret, XYZ.fromPartialXYZT(xyz, ret));
        doubleXYZ(ret, XYZ.fromPartialXYZT(xyz, ret));
        doubleXYZ(ret, XYZ.fromPartialXYZT(xyz, ret));

        // Add multiples of B for even indices of e.
        for (int i = 0; i < e.length; i += 2) {
            CachedXYT t = new CachedXYT(CACHED_NEUTRAL);
            select(t, i / 2, e[i]);
            add(ret, XYZT.fromPartialXYZT(xyzt, ret), t);
        }

        // This check is to protect against flaws, i.e. if there is a computation error through a
        // faulty CPU or if the implementation contains a bug.
        XYZ result = new XYZ(ret);
        if (!result.isOnCurve()) {
            throw new IllegalStateException("arithmetic error in scalar multiplication");
        }
        return result;
    }

    @SuppressWarnings("NarrowingCompoundAssignment")
    private static byte[] slide(byte[] a) {
        byte[] r = new byte[256];
        // Writes each bit in a[0..31] into r[0..255]:
        // a = a[0]+256*a[1]+...+256^31*a[31] is equal to
        // r = r[0]+2*r[1]+...+2^255*r[255]
        for (int i = 0; i < 256; i++) {
            r[i] = (byte) (1 & ((a[i >> 3] & 0xff) >> (i & 7)));
        }

        // Transforms r[i] as odd values in [-15, 15]
        for (int i = 0; i < 256; i++) {
            if (r[i] != 0) {
                for (int b = 1; b <= 6 && i + b < 256; b++) {
                    if (r[i + b] != 0) {
                        if (r[i] + (r[i + b] << b) <= 15) {
                            r[i] += r[i + b] << b;
                            r[i + b] = 0;
                        } else if (r[i] - (r[i + b] << b) >= -15) {
                            r[i] -= r[i + b] << b;
                            for (int k = i + b; k < 256; k++) {
                                if (r[k] == 0) {
                                    r[k] = 1;
                                    break;
                                }
                                r[k] = 0;
                            }
                        } else {
                            break;
                        }
                    }
                }
            }
        }
        return r;
    }

    /**
     * Computes {@code a}*{@code pointA}+{@code b}*B
     * where a = a[0]+256*a[1]+...+256^31*a[31].
     * and b = b[0]+256*b[1]+...+256^31*b[31].
     * B is the Ed25519 base point (x,4/5) with x positive.
     * <p>
     * Note that execution time varies based on the input since this will only be used in verification
     * of signatures.
     */
    private static XYZ doubleScalarMultVarTime(byte[] a, XYZT pointA, byte[] b) {
        // pointA, 3*pointA, 5*pointA, 7*pointA, 9*pointA, 11*pointA, 13*pointA, 15*pointA
        CachedXYZT[] pointAArray = new CachedXYZT[8];
        pointAArray[0] = new CachedXYZT(pointA);
        PartialXYZT t = new PartialXYZT();
        doubleXYZT(t, pointA);
        XYZT doubleA = new XYZT(t);
        for (int i = 1; i < pointAArray.length; i++) {
            add(t, doubleA, pointAArray[i - 1]);
            pointAArray[i] = new CachedXYZT(new XYZT(t));
        }

        byte[] aSlide = slide(a);
        byte[] bSlide = slide(b);
        t = new PartialXYZT(NEUTRAL);
        XYZT u = new XYZT();
        int i = 255;
        for (; i >= 0; i--) {
            if (aSlide[i] != 0 || bSlide[i] != 0) {
                break;
            }
        }
        for (; i >= 0; i--) {
            doubleXYZ(t, new XYZ(t));
            if (aSlide[i] > 0) {
                add(t, XYZT.fromPartialXYZT(u, t), pointAArray[aSlide[i] / 2]);
            } else if (aSlide[i] < 0) {
                sub(t, XYZT.fromPartialXYZT(u, t), pointAArray[-aSlide[i] / 2]);
            }
            if (bSlide[i] > 0) {
                add(t, XYZT.fromPartialXYZT(u, t), B2[bSlide[i] / 2]);
            } else if (bSlide[i] < 0) {
                sub(t, XYZT.fromPartialXYZT(u, t), B2[-bSlide[i] / 2]);
            }
        }

        return new XYZ(t);
    }

    /**
     * Returns true if {@code in} is nonzero.
     * <p>
     * Note that execution time might depend on the input {@code in}.
     */
    private static boolean isNonZeroVarTime(long[] in) {
        long[] inCopy = new long[in.length + 1];
        System.arraycopy(in, 0, inCopy, 0, in.length);
        Field25519.reduceCoefficients(inCopy);
        byte[] bytes = Field25519.contract(inCopy);
        for (byte b : bytes) {
            if (b != 0) {
                return true;
            }
        }
        return false;
    }

    /**
     * Returns the least significant bit of {@code in}.
     */
    private static int getLsb(long[] in) {
        return Field25519.contract(in)[0] & 1;
    }

    /**
     * Negates all values in {@code in} and store it in {@code out}.
     */
    private static void neg(long[] out, long[] in) {
        for (int i = 0; i < in.length; i++) {
            out[i] = -in[i];
        }
    }

    /**
     * Computes {@code in}^(2^252-3) mod 2^255-19 and puts the result in {@code out}.
     */
    private static void pow2252m3(long[] out, long[] in) {
        long[] t0 = new long[Field25519.LIMB_CNT];
        long[] t1 = new long[Field25519.LIMB_CNT];
        long[] t2 = new long[Field25519.LIMB_CNT];

        // z2 = z1^2^1
        Field25519.square(t0, in);

        // z8 = z2^2^2
        Field25519.square(t1, t0);
        for (int i = 1; i < 2; i++) {
            Field25519.square(t1, t1);
        }

        // z9 = z1*z8
        Field25519.mult(t1, in, t1);

        // z11 = z2*z9
        Field25519.mult(t0, t0, t1);

        // z22 = z11^2^1
        Field25519.square(t0, t0);

        // z_5_0 = z9*z22
        Field25519.mult(t0, t1, t0);

        // z_10_5 = z_5_0^2^5
        Field25519.square(t1, t0);
        for (int i = 1; i < 5; i++) {
            Field25519.square(t1, t1);
        }

        // z_10_0 = z_10_5*z_5_0
        Field25519.mult(t0, t1, t0);

        // z_20_10 = z_10_0^2^10
        Field25519.square(t1, t0);
        for (int i = 1; i < 10; i++) {
            Field25519.square(t1, t1);
        }

        // z_20_0 = z_20_10*z_10_0
        Field25519.mult(t1, t1, t0);

        // z_40_20 = z_20_0^2^20
        Field25519.square(t2, t1);
        for (int i = 1; i < 20; i++) {
            Field25519.square(t2, t2);
        }

        // z_40_0 = z_40_20*z_20_0
        Field25519.mult(t1, t2, t1);

        // z_50_10 = z_40_0^2^10
        Field25519.square(t1, t1);
        for (int i = 1; i < 10; i++) {
            Field25519.square(t1, t1);
        }

        // z_50_0 = z_50_10*z_10_0
        Field25519.mult(t0, t1, t0);

        // z_100_50 = z_50_0^2^50
        Field25519.square(t1, t0);
        for (int i = 1; i < 50; i++) {
            Field25519.square(t1, t1);
        }

        // z_100_0 = z_100_50*z_50_0
        Field25519.mult(t1, t1, t0);

        // z_200_100 = z_100_0^2^100
        Field25519.square(t2, t1);
        for (int i = 1; i < 100; i++) {
            Field25519.square(t2, t2);
        }

        // z_200_0 = z_200_100*z_100_0
        Field25519.mult(t1, t2, t1);

        // z_250_50 = z_200_0^2^50
        Field25519.square(t1, t1);
        for (int i = 1; i < 50; i++) {
            Field25519.square(t1, t1);
        }

        // z_250_0 = z_250_50*z_50_0
        Field25519.mult(t0, t1, t0);

        // z_252_2 = z_250_0^2^2
        Field25519.square(t0, t0);
        for (int i = 1; i < 2; i++) {
            Field25519.square(t0, t0);
        }

        // z_252_3 = z_252_2*z1
        Field25519.mult(out, t0, in);
    }

    /**
     * Returns 3 bytes of {@code in} starting from {@code idx} in Little-Endian format.
     */
    private static long load3(byte[] in, int idx) {
        long result;
        result = (long) in[idx] & 0xff;
        result |= (long) (in[idx + 1] & 0xff) << 8;
        result |= (long) (in[idx + 2] & 0xff) << 16;
        return result;
    }

    /**
     * Returns 4 bytes of {@code in} starting from {@code idx} in Little-Endian format.
     */
    private static long load4(byte[] in, int idx) {
        long result = load3(in, idx);
        result |= (long) (in[idx + 3] & 0xff) << 24;
        return result;
    }

    /**
     * Input:
     * s[0]+256*s[1]+...+256^63*s[63] = s
     * <p>
     * Output:
     * s[0]+256*s[1]+...+256^31*s[31] = s mod l
     * where l = 2^252 + 27742317777372353535851937790883648493.
     * Overwrites s in place.
     */
    private static void reduce(byte[] s) {
        // Observation:
        // 2^252 mod l is equivalent to -27742317777372353535851937790883648493 mod l
        // Let m = -27742317777372353535851937790883648493
        // Thus a*2^252+b mod l is equivalent to a*m+b mod l
        //
        // First s is divided into chunks of 21 bits as follows:
        // s0+2^21*s1+2^42*s3+...+2^462*s23 = s[0]+256*s[1]+...+256^63*s[63]
        long s0 = 2097151 & load3(s, 0);
        long s1 = 2097151 & (load4(s, 2) >> 5);
        long s2 = 2097151 & (load3(s, 5) >> 2);
        long s3 = 2097151 & (load4(s, 7) >> 7);
        long s4 = 2097151 & (load4(s, 10) >> 4);
        long s5 = 2097151 & (load3(s, 13) >> 1);
        long s6 = 2097151 & (load4(s, 15) >> 6);
        long s7 = 2097151 & (load3(s, 18) >> 3);
        long s8 = 2097151 & load3(s, 21);
        long s9 = 2097151 & (load4(s, 23) >> 5);
        long s10 = 2097151 & (load3(s, 26) >> 2);
        long s11 = 2097151 & (load4(s, 28) >> 7);
        long s12 = 2097151 & (load4(s, 31) >> 4);
        long s13 = 2097151 & (load3(s, 34) >> 1);
        long s14 = 2097151 & (load4(s, 36) >> 6);
        long s15 = 2097151 & (load3(s, 39) >> 3);
        long s16 = 2097151 & load3(s, 42);
        long s17 = 2097151 & (load4(s, 44) >> 5);
        long s18 = 2097151 & (load3(s, 47) >> 2);
        long s19 = 2097151 & (load4(s, 49) >> 7);
        long s20 = 2097151 & (load4(s, 52) >> 4);
        long s21 = 2097151 & (load3(s, 55) >> 1);
        long s22 = 2097151 & (load4(s, 57) >> 6);
        long s23 = (load4(s, 60) >> 3);
        long carry0;
        long carry1;
        long carry2;
        long carry3;
        long carry4;
        long carry5;
        long carry6;
        long carry7;
        long carry8;
        long carry9;
        long carry10;
        long carry11;
        long carry12;
        long carry13;
        long carry14;
        long carry15;
        long carry16;

        // s23*2^462 = s23*2^210*2^252 is equivalent to s23*2^210*m in mod l
        // As m is a 125 bit number, the result needs to scattered to 6 limbs (125/21 ceil is 6)
        // starting from s11 (s11*2^210)
        // m = [666643, 470296, 654183, -997805, 136657, -683901] in 21-bit limbs
        s11 += s23 * 666643;
        s12 += s23 * 470296;
        s13 += s23 * 654183;
        s14 -= s23 * 997805;
        s15 += s23 * 136657;
        s16 -= s23 * 683901;
        // s23 = 0;

        s10 += s22 * 666643;
        s11 += s22 * 470296;
        s12 += s22 * 654183;
        s13 -= s22 * 997805;
        s14 += s22 * 136657;
        s15 -= s22 * 683901;
        // s22 = 0;

        s9 += s21 * 666643;
        s10 += s21 * 470296;
        s11 += s21 * 654183;
        s12 -= s21 * 997805;
        s13 += s21 * 136657;
        s14 -= s21 * 683901;
        // s21 = 0;

        s8 += s20 * 666643;
        s9 += s20 * 470296;
        s10 += s20 * 654183;
        s11 -= s20 * 997805;
        s12 += s20 * 136657;
        s13 -= s20 * 683901;
        // s20 = 0;

        s7 += s19 * 666643;
        s8 += s19 * 470296;
        s9 += s19 * 654183;
        s10 -= s19 * 997805;
        s11 += s19 * 136657;
        s12 -= s19 * 683901;
        // s19 = 0;

        s6 += s18 * 666643;
        s7 += s18 * 470296;
        s8 += s18 * 654183;
        s9 -= s18 * 997805;
        s10 += s18 * 136657;
        s11 -= s18 * 683901;
        // s18 = 0;

        // Reduce the bit length of limbs from s6 to s15 to 21-bits.
        carry6 = (s6 + (1 << 20)) >> 21;
        s7 += carry6;
        s6 -= carry6 << 21;
        carry8 = (s8 + (1 << 20)) >> 21;
        s9 += carry8;
        s8 -= carry8 << 21;
        carry10 = (s10 + (1 << 20)) >> 21;
        s11 += carry10;
        s10 -= carry10 << 21;
        carry12 = (s12 + (1 << 20)) >> 21;
        s13 += carry12;
        s12 -= carry12 << 21;
        carry14 = (s14 + (1 << 20)) >> 21;
        s15 += carry14;
        s14 -= carry14 << 21;
        carry16 = (s16 + (1 << 20)) >> 21;
        s17 += carry16;
        s16 -= carry16 << 21;

        carry7 = (s7 + (1 << 20)) >> 21;
        s8 += carry7;
        s7 -= carry7 << 21;
        carry9 = (s9 + (1 << 20)) >> 21;
        s10 += carry9;
        s9 -= carry9 << 21;
        carry11 = (s11 + (1 << 20)) >> 21;
        s12 += carry11;
        s11 -= carry11 << 21;
        carry13 = (s13 + (1 << 20)) >> 21;
        s14 += carry13;
        s13 -= carry13 << 21;
        carry15 = (s15 + (1 << 20)) >> 21;
        s16 += carry15;
        s15 -= carry15 << 21;

        // Resume reduction where we left off.
        s5 += s17 * 666643;
        s6 += s17 * 470296;
        s7 += s17 * 654183;
        s8 -= s17 * 997805;
        s9 += s17 * 136657;
        s10 -= s17 * 683901;
        // s17 = 0;

        s4 += s16 * 666643;
        s5 += s16 * 470296;
        s6 += s16 * 654183;
        s7 -= s16 * 997805;
        s8 += s16 * 136657;
        s9 -= s16 * 683901;
        // s16 = 0;

        s3 += s15 * 666643;
        s4 += s15 * 470296;
        s5 += s15 * 654183;
        s6 -= s15 * 997805;
        s7 += s15 * 136657;
        s8 -= s15 * 683901;
        // s15 = 0;

        s2 += s14 * 666643;
        s3 += s14 * 470296;
        s4 += s14 * 654183;
        s5 -= s14 * 997805;
        s6 += s14 * 136657;
        s7 -= s14 * 683901;
        // s14 = 0;

        s1 += s13 * 666643;
        s2 += s13 * 470296;
        s3 += s13 * 654183;
        s4 -= s13 * 997805;
        s5 += s13 * 136657;
        s6 -= s13 * 683901;
        // s13 = 0;

        s0 += s12 * 666643;
        s1 += s12 * 470296;
        s2 += s12 * 654183;
        s3 -= s12 * 997805;
        s4 += s12 * 136657;
        s5 -= s12 * 683901;
        s12 = 0;

        // Reduce the range of limbs from s0 to s11 to 21-bits.
        carry0 = (s0 + (1 << 20)) >> 21;
        s1 += carry0;
        s0 -= carry0 << 21;
        carry2 = (s2 + (1 << 20)) >> 21;
        s3 += carry2;
        s2 -= carry2 << 21;
        carry4 = (s4 + (1 << 20)) >> 21;
        s5 += carry4;
        s4 -= carry4 << 21;
        carry6 = (s6 + (1 << 20)) >> 21;
        s7 += carry6;
        s6 -= carry6 << 21;
        carry8 = (s8 + (1 << 20)) >> 21;
        s9 += carry8;
        s8 -= carry8 << 21;
        carry10 = (s10 + (1 << 20)) >> 21;
        s11 += carry10;
        s10 -= carry10 << 21;

        carry1 = (s1 + (1 << 20)) >> 21;
        s2 += carry1;
        s1 -= carry1 << 21;
        carry3 = (s3 + (1 << 20)) >> 21;
        s4 += carry3;
        s3 -= carry3 << 21;
        carry5 = (s5 + (1 << 20)) >> 21;
        s6 += carry5;
        s5 -= carry5 << 21;
        carry7 = (s7 + (1 << 20)) >> 21;
        s8 += carry7;
        s7 -= carry7 << 21;
        carry9 = (s9 + (1 << 20)) >> 21;
        s10 += carry9;
        s9 -= carry9 << 21;
        carry11 = (s11 + (1 << 20)) >> 21;
        s12 += carry11;
        s11 -= carry11 << 21;

        s0 += s12 * 666643;
        s1 += s12 * 470296;
        s2 += s12 * 654183;
        s3 -= s12 * 997805;
        s4 += s12 * 136657;
        s5 -= s12 * 683901;
        s12 = 0;

        // Carry chain reduction to propagate excess bits from s0 to s5 to the most significant limbs.
        carry0 = s0 >> 21;
        s1 += carry0;
        s0 -= carry0 << 21;
        carry1 = s1 >> 21;
        s2 += carry1;
        s1 -= carry1 << 21;
        carry2 = s2 >> 21;
        s3 += carry2;
        s2 -= carry2 << 21;
        carry3 = s3 >> 21;
        s4 += carry3;
        s3 -= carry3 << 21;
        carry4 = s4 >> 21;
        s5 += carry4;
        s4 -= carry4 << 21;
        carry5 = s5 >> 21;
        s6 += carry5;
        s5 -= carry5 << 21;
        carry6 = s6 >> 21;
        s7 += carry6;
        s6 -= carry6 << 21;
        carry7 = s7 >> 21;
        s8 += carry7;
        s7 -= carry7 << 21;
        carry8 = s8 >> 21;
        s9 += carry8;
        s8 -= carry8 << 21;
        carry9 = s9 >> 21;
        s10 += carry9;
        s9 -= carry9 << 21;
        carry10 = s10 >> 21;
        s11 += carry10;
        s10 -= carry10 << 21;
        carry11 = s11 >> 21;
        s12 += carry11;
        s11 -= carry11 << 21;

        // Do one last reduction as s12 might be 1.
        s0 += s12 * 666643;
        s1 += s12 * 470296;
        s2 += s12 * 654183;
        s3 -= s12 * 997805;
        s4 += s12 * 136657;
        s5 -= s12 * 683901;
        // s12 = 0;

        carry0 = s0 >> 21;
        s1 += carry0;
        s0 -= carry0 << 21;
        carry1 = s1 >> 21;
        s2 += carry1;
        s1 -= carry1 << 21;
        carry2 = s2 >> 21;
        s3 += carry2;
        s2 -= carry2 << 21;
        carry3 = s3 >> 21;
        s4 += carry3;
        s3 -= carry3 << 21;
        carry4 = s4 >> 21;
        s5 += carry4;
        s4 -= carry4 << 21;
        carry5 = s5 >> 21;
        s6 += carry5;
        s5 -= carry5 << 21;
        carry6 = s6 >> 21;
        s7 += carry6;
        s6 -= carry6 << 21;
        carry7 = s7 >> 21;
        s8 += carry7;
        s7 -= carry7 << 21;
        carry8 = s8 >> 21;
        s9 += carry8;
        s8 -= carry8 << 21;
        carry9 = s9 >> 21;
        s10 += carry9;
        s9 -= carry9 << 21;
        carry10 = s10 >> 21;
        s11 += carry10;
        s10 -= carry10 << 21;

        // Serialize the result into the s.
        s[0] = (byte) s0;
        s[1] = (byte) (s0 >> 8);
        s[2] = (byte) ((s0 >> 16) | (s1 << 5));
        s[3] = (byte) (s1 >> 3);
        s[4] = (byte) (s1 >> 11);
        s[5] = (byte) ((s1 >> 19) | (s2 << 2));
        s[6] = (byte) (s2 >> 6);
        s[7] = (byte) ((s2 >> 14) | (s3 << 7));
        s[8] = (byte) (s3 >> 1);
        s[9] = (byte) (s3 >> 9);
        s[10] = (byte) ((s3 >> 17) | (s4 << 4));
        s[11] = (byte) (s4 >> 4);
        s[12] = (byte) (s4 >> 12);
        s[13] = (byte) ((s4 >> 20) | (s5 << 1));
        s[14] = (byte) (s5 >> 7);
        s[15] = (byte) ((s5 >> 15) | (s6 << 6));
        s[16] = (byte) (s6 >> 2);
        s[17] = (byte) (s6 >> 10);
        s[18] = (byte) ((s6 >> 18) | (s7 << 3));
        s[19] = (byte) (s7 >> 5);
        s[20] = (byte) (s7 >> 13);
        s[21] = (byte) s8;
        s[22] = (byte) (s8 >> 8);
        s[23] = (byte) ((s8 >> 16) | (s9 << 5));
        s[24] = (byte) (s9 >> 3);
        s[25] = (byte) (s9 >> 11);
        s[26] = (byte) ((s9 >> 19) | (s10 << 2));
        s[27] = (byte) (s10 >> 6);
        s[28] = (byte) ((s10 >> 14) | (s11 << 7));
        s[29] = (byte) (s11 >> 1);
        s[30] = (byte) (s11 >> 9);
        s[31] = (byte) (s11 >> 17);
    }

    /**
     * Input:
     * a[0]+256*a[1]+...+256^31*a[31] = a
     * b[0]+256*b[1]+...+256^31*b[31] = b
     * c[0]+256*c[1]+...+256^31*c[31] = c
     * <p>
     * Output:
     * s[0]+256*s[1]+...+256^31*s[31] = (ab+c) mod l
     * where l = 2^252 + 27742317777372353535851937790883648493.
     */
    private static void mulAdd(byte[] s, byte[] a, byte[] b, byte[] c) {
        // This is very similar to Ed25519.reduce, the difference in here is that it computes ab+c
        // See Ed25519.reduce for related comments.
        long a0 = 2097151 & load3(a, 0);
        long a1 = 2097151 & (load4(a, 2) >> 5);
        long a2 = 2097151 & (load3(a, 5) >> 2);
        long a3 = 2097151 & (load4(a, 7) >> 7);
        long a4 = 2097151 & (load4(a, 10) >> 4);
        long a5 = 2097151 & (load3(a, 13) >> 1);
        long a6 = 2097151 & (load4(a, 15) >> 6);
        long a7 = 2097151 & (load3(a, 18) >> 3);
        long a8 = 2097151 & load3(a, 21);
        long a9 = 2097151 & (load4(a, 23) >> 5);
        long a10 = 2097151 & (load3(a, 26) >> 2);
        long a11 = (load4(a, 28) >> 7);
        long b0 = 2097151 & load3(b, 0);
        long b1 = 2097151 & (load4(b, 2) >> 5);
        long b2 = 2097151 & (load3(b, 5) >> 2);
        long b3 = 2097151 & (load4(b, 7) >> 7);
        long b4 = 2097151 & (load4(b, 10) >> 4);
        long b5 = 2097151 & (load3(b, 13) >> 1);
        long b6 = 2097151 & (load4(b, 15) >> 6);
        long b7 = 2097151 & (load3(b, 18) >> 3);
        long b8 = 2097151 & load3(b, 21);
        long b9 = 2097151 & (load4(b, 23) >> 5);
        long b10 = 2097151 & (load3(b, 26) >> 2);
        long b11 = (load4(b, 28) >> 7);
        long c0 = 2097151 & load3(c, 0);
        long c1 = 2097151 & (load4(c, 2) >> 5);
        long c2 = 2097151 & (load3(c, 5) >> 2);
        long c3 = 2097151 & (load4(c, 7) >> 7);
        long c4 = 2097151 & (load4(c, 10) >> 4);
        long c5 = 2097151 & (load3(c, 13) >> 1);
        long c6 = 2097151 & (load4(c, 15) >> 6);
        long c7 = 2097151 & (load3(c, 18) >> 3);
        long c8 = 2097151 & load3(c, 21);
        long c9 = 2097151 & (load4(c, 23) >> 5);
        long c10 = 2097151 & (load3(c, 26) >> 2);
        long c11 = (load4(c, 28) >> 7);
        long s0;
        long s1;
        long s2;
        long s3;
        long s4;
        long s5;
        long s6;
        long s7;
        long s8;
        long s9;
        long s10;
        long s11;
        long s12;
        long s13;
        long s14;
        long s15;
        long s16;
        long s17;
        long s18;
        long s19;
        long s20;
        long s21;
        long s22;
        long s23;
        long carry0;
        long carry1;
        long carry2;
        long carry3;
        long carry4;
        long carry5;
        long carry6;
        long carry7;
        long carry8;
        long carry9;
        long carry10;
        long carry11;
        long carry12;
        long carry13;
        long carry14;
        long carry15;
        long carry16;
        long carry17;
        long carry18;
        long carry19;
        long carry20;
        long carry21;
        long carry22;

        s0 = c0 + a0 * b0;
        s1 = c1 + a0 * b1 + a1 * b0;
        s2 = c2 + a0 * b2 + a1 * b1 + a2 * b0;
        s3 = c3 + a0 * b3 + a1 * b2 + a2 * b1 + a3 * b0;
        s4 = c4 + a0 * b4 + a1 * b3 + a2 * b2 + a3 * b1 + a4 * b0;
        s5 = c5 + a0 * b5 + a1 * b4 + a2 * b3 + a3 * b2 + a4 * b1 + a5 * b0;
        s6 = c6 + a0 * b6 + a1 * b5 + a2 * b4 + a3 * b3 + a4 * b2 + a5 * b1 + a6 * b0;
        s7 = c7 + a0 * b7 + a1 * b6 + a2 * b5 + a3 * b4 + a4 * b3 + a5 * b2 + a6 * b1 + a7 * b0;
        s8 = c8 + a0 * b8 + a1 * b7 + a2 * b6 + a3 * b5 + a4 * b4 + a5 * b3 + a6 * b2 + a7 * b1
                + a8 * b0;
        s9 = c9 + a0 * b9 + a1 * b8 + a2 * b7 + a3 * b6 + a4 * b5 + a5 * b4 + a6 * b3 + a7 * b2
                + a8 * b1 + a9 * b0;
        s10 = c10 + a0 * b10 + a1 * b9 + a2 * b8 + a3 * b7 + a4 * b6 + a5 * b5 + a6 * b4 + a7 * b3
                + a8 * b2 + a9 * b1 + a10 * b0;
        s11 = c11 + a0 * b11 + a1 * b10 + a2 * b9 + a3 * b8 + a4 * b7 + a5 * b6 + a6 * b5 + a7 * b4
                + a8 * b3 + a9 * b2 + a10 * b1 + a11 * b0;
        s12 = a1 * b11 + a2 * b10 + a3 * b9 + a4 * b8 + a5 * b7 + a6 * b6 + a7 * b5 + a8 * b4 + a9 * b3
                + a10 * b2 + a11 * b1;
        s13 = a2 * b11 + a3 * b10 + a4 * b9 + a5 * b8 + a6 * b7 + a7 * b6 + a8 * b5 + a9 * b4 + a10 * b3
                + a11 * b2;
        s14 = a3 * b11 + a4 * b10 + a5 * b9 + a6 * b8 + a7 * b7 + a8 * b6 + a9 * b5 + a10 * b4
                + a11 * b3;
        s15 = a4 * b11 + a5 * b10 + a6 * b9 + a7 * b8 + a8 * b7 + a9 * b6 + a10 * b5 + a11 * b4;
        s16 = a5 * b11 + a6 * b10 + a7 * b9 + a8 * b8 + a9 * b7 + a10 * b6 + a11 * b5;
        s17 = a6 * b11 + a7 * b10 + a8 * b9 + a9 * b8 + a10 * b7 + a11 * b6;
        s18 = a7 * b11 + a8 * b10 + a9 * b9 + a10 * b8 + a11 * b7;
        s19 = a8 * b11 + a9 * b10 + a10 * b9 + a11 * b8;
        s20 = a9 * b11 + a10 * b10 + a11 * b9;
        s21 = a10 * b11 + a11 * b10;
        s22 = a11 * b11;
        s23 = 0;

        carry0 = (s0 + (1 << 20)) >> 21;
        s1 += carry0;
        s0 -= carry0 << 21;
        carry2 = (s2 + (1 << 20)) >> 21;
        s3 += carry2;
        s2 -= carry2 << 21;
        carry4 = (s4 + (1 << 20)) >> 21;
        s5 += carry4;
        s4 -= carry4 << 21;
        carry6 = (s6 + (1 << 20)) >> 21;
        s7 += carry6;
        s6 -= carry6 << 21;
        carry8 = (s8 + (1 << 20)) >> 21;
        s9 += carry8;
        s8 -= carry8 << 21;
        carry10 = (s10 + (1 << 20)) >> 21;
        s11 += carry10;
        s10 -= carry10 << 21;
        carry12 = (s12 + (1 << 20)) >> 21;
        s13 += carry12;
        s12 -= carry12 << 21;
        carry14 = (s14 + (1 << 20)) >> 21;
        s15 += carry14;
        s14 -= carry14 << 21;
        carry16 = (s16 + (1 << 20)) >> 21;
        s17 += carry16;
        s16 -= carry16 << 21;
        carry18 = (s18 + (1 << 20)) >> 21;
        s19 += carry18;
        s18 -= carry18 << 21;
        carry20 = (s20 + (1 << 20)) >> 21;
        s21 += carry20;
        s20 -= carry20 << 21;
        carry22 = (s22 + (1 << 20)) >> 21;
        s23 += carry22;
        s22 -= carry22 << 21;

        carry1 = (s1 + (1 << 20)) >> 21;
        s2 += carry1;
        s1 -= carry1 << 21;
        carry3 = (s3 + (1 << 20)) >> 21;
        s4 += carry3;
        s3 -= carry3 << 21;
        carry5 = (s5 + (1 << 20)) >> 21;
        s6 += carry5;
        s5 -= carry5 << 21;
        carry7 = (s7 + (1 << 20)) >> 21;
        s8 += carry7;
        s7 -= carry7 << 21;
        carry9 = (s9 + (1 << 20)) >> 21;
        s10 += carry9;
        s9 -= carry9 << 21;
        carry11 = (s11 + (1 << 20)) >> 21;
        s12 += carry11;
        s11 -= carry11 << 21;
        carry13 = (s13 + (1 << 20)) >> 21;
        s14 += carry13;
        s13 -= carry13 << 21;
        carry15 = (s15 + (1 << 20)) >> 21;
        s16 += carry15;
        s15 -= carry15 << 21;
        carry17 = (s17 + (1 << 20)) >> 21;
        s18 += carry17;
        s17 -= carry17 << 21;
        carry19 = (s19 + (1 << 20)) >> 21;
        s20 += carry19;
        s19 -= carry19 << 21;
        carry21 = (s21 + (1 << 20)) >> 21;
        s22 += carry21;
        s21 -= carry21 << 21;

        s11 += s23 * 666643;
        s12 += s23 * 470296;
        s13 += s23 * 654183;
        s14 -= s23 * 997805;
        s15 += s23 * 136657;
        s16 -= s23 * 683901;
        // s23 = 0;

        s10 += s22 * 666643;
        s11 += s22 * 470296;
        s12 += s22 * 654183;
        s13 -= s22 * 997805;
        s14 += s22 * 136657;
        s15 -= s22 * 683901;
        // s22 = 0;

        s9 += s21 * 666643;
        s10 += s21 * 470296;
        s11 += s21 * 654183;
        s12 -= s21 * 997805;
        s13 += s21 * 136657;
        s14 -= s21 * 683901;
        // s21 = 0;

        s8 += s20 * 666643;
        s9 += s20 * 470296;
        s10 += s20 * 654183;
        s11 -= s20 * 997805;
        s12 += s20 * 136657;
        s13 -= s20 * 683901;
        // s20 = 0;

        s7 += s19 * 666643;
        s8 += s19 * 470296;
        s9 += s19 * 654183;
        s10 -= s19 * 997805;
        s11 += s19 * 136657;
        s12 -= s19 * 683901;
        // s19 = 0;

        s6 += s18 * 666643;
        s7 += s18 * 470296;
        s8 += s18 * 654183;
        s9 -= s18 * 997805;
        s10 += s18 * 136657;
        s11 -= s18 * 683901;
        // s18 = 0;

        carry6 = (s6 + (1 << 20)) >> 21;
        s7 += carry6;
        s6 -= carry6 << 21;
        carry8 = (s8 + (1 << 20)) >> 21;
        s9 += carry8;
        s8 -= carry8 << 21;
        carry10 = (s10 + (1 << 20)) >> 21;
        s11 += carry10;
        s10 -= carry10 << 21;
        carry12 = (s12 + (1 << 20)) >> 21;
        s13 += carry12;
        s12 -= carry12 << 21;
        carry14 = (s14 + (1 << 20)) >> 21;
        s15 += carry14;
        s14 -= carry14 << 21;
        carry16 = (s16 + (1 << 20)) >> 21;
        s17 += carry16;
        s16 -= carry16 << 21;

        carry7 = (s7 + (1 << 20)) >> 21;
        s8 += carry7;
        s7 -= carry7 << 21;
        carry9 = (s9 + (1 << 20)) >> 21;
        s10 += carry9;
        s9 -= carry9 << 21;
        carry11 = (s11 + (1 << 20)) >> 21;
        s12 += carry11;
        s11 -= carry11 << 21;
        carry13 = (s13 + (1 << 20)) >> 21;
        s14 += carry13;
        s13 -= carry13 << 21;
        carry15 = (s15 + (1 << 20)) >> 21;
        s16 += carry15;
        s15 -= carry15 << 21;

        s5 += s17 * 666643;
        s6 += s17 * 470296;
        s7 += s17 * 654183;
        s8 -= s17 * 997805;
        s9 += s17 * 136657;
        s10 -= s17 * 683901;
        // s17 = 0;

        s4 += s16 * 666643;
        s5 += s16 * 470296;
        s6 += s16 * 654183;
        s7 -= s16 * 997805;
        s8 += s16 * 136657;
        s9 -= s16 * 683901;
        // s16 = 0;

        s3 += s15 * 666643;
        s4 += s15 * 470296;
        s5 += s15 * 654183;
        s6 -= s15 * 997805;
        s7 += s15 * 136657;
        s8 -= s15 * 683901;
        // s15 = 0;

        s2 += s14 * 666643;
        s3 += s14 * 470296;
        s4 += s14 * 654183;
        s5 -= s14 * 997805;
        s6 += s14 * 136657;
        s7 -= s14 * 683901;
        // s14 = 0;

        s1 += s13 * 666643;
        s2 += s13 * 470296;
        s3 += s13 * 654183;
        s4 -= s13 * 997805;
        s5 += s13 * 136657;
        s6 -= s13 * 683901;
        // s13 = 0;

        s0 += s12 * 666643;
        s1 += s12 * 470296;
        s2 += s12 * 654183;
        s3 -= s12 * 997805;
        s4 += s12 * 136657;
        s5 -= s12 * 683901;
        s12 = 0;

        carry0 = (s0 + (1 << 20)) >> 21;
        s1 += carry0;
        s0 -= carry0 << 21;
        carry2 = (s2 + (1 << 20)) >> 21;
        s3 += carry2;
        s2 -= carry2 << 21;
        carry4 = (s4 + (1 << 20)) >> 21;
        s5 += carry4;
        s4 -= carry4 << 21;
        carry6 = (s6 + (1 << 20)) >> 21;
        s7 += carry6;
        s6 -= carry6 << 21;
        carry8 = (s8 + (1 << 20)) >> 21;
        s9 += carry8;
        s8 -= carry8 << 21;
        carry10 = (s10 + (1 << 20)) >> 21;
        s11 += carry10;
        s10 -= carry10 << 21;

        carry1 = (s1 + (1 << 20)) >> 21;
        s2 += carry1;
        s1 -= carry1 << 21;
        carry3 = (s3 + (1 << 20)) >> 21;
        s4 += carry3;
        s3 -= carry3 << 21;
        carry5 = (s5 + (1 << 20)) >> 21;
        s6 += carry5;
        s5 -= carry5 << 21;
        carry7 = (s7 + (1 << 20)) >> 21;
        s8 += carry7;
        s7 -= carry7 << 21;
        carry9 = (s9 + (1 << 20)) >> 21;
        s10 += carry9;
        s9 -= carry9 << 21;
        carry11 = (s11 + (1 << 20)) >> 21;
        s12 += carry11;
        s11 -= carry11 << 21;

        s0 += s12 * 666643;
        s1 += s12 * 470296;
        s2 += s12 * 654183;
        s3 -= s12 * 997805;
        s4 += s12 * 136657;
        s5 -= s12 * 683901;
        s12 = 0;

        carry0 = s0 >> 21;
        s1 += carry0;
        s0 -= carry0 << 21;
        carry1 = s1 >> 21;
        s2 += carry1;
        s1 -= carry1 << 21;
        carry2 = s2 >> 21;
        s3 += carry2;
        s2 -= carry2 << 21;
        carry3 = s3 >> 21;
        s4 += carry3;
        s3 -= carry3 << 21;
        carry4 = s4 >> 21;
        s5 += carry4;
        s4 -= carry4 << 21;
        carry5 = s5 >> 21;
        s6 += carry5;
        s5 -= carry5 << 21;
        carry6 = s6 >> 21;
        s7 += carry6;
        s6 -= carry6 << 21;
        carry7 = s7 >> 21;
        s8 += carry7;
        s7 -= carry7 << 21;
        carry8 = s8 >> 21;
        s9 += carry8;
        s8 -= carry8 << 21;
        carry9 = s9 >> 21;
        s10 += carry9;
        s9 -= carry9 << 21;
        carry10 = s10 >> 21;
        s11 += carry10;
        s10 -= carry10 << 21;
        carry11 = s11 >> 21;
        s12 += carry11;
        s11 -= carry11 << 21;

        s0 += s12 * 666643;
        s1 += s12 * 470296;
        s2 += s12 * 654183;
        s3 -= s12 * 997805;
        s4 += s12 * 136657;
        s5 -= s12 * 683901;
        // s12 = 0;

        carry0 = s0 >> 21;
        s1 += carry0;
        s0 -= carry0 << 21;
        carry1 = s1 >> 21;
        s2 += carry1;
        s1 -= carry1 << 21;
        carry2 = s2 >> 21;
        s3 += carry2;
        s2 -= carry2 << 21;
        carry3 = s3 >> 21;
        s4 += carry3;
        s3 -= carry3 << 21;
        carry4 = s4 >> 21;
        s5 += carry4;
        s4 -= carry4 << 21;
        carry5 = s5 >> 21;
        s6 += carry5;
        s5 -= carry5 << 21;
        carry6 = s6 >> 21;
        s7 += carry6;
        s6 -= carry6 << 21;
        carry7 = s7 >> 21;
        s8 += carry7;
        s7 -= carry7 << 21;
        carry8 = s8 >> 21;
        s9 += carry8;
        s8 -= carry8 << 21;
        carry9 = s9 >> 21;
        s10 += carry9;
        s9 -= carry9 << 21;
        carry10 = s10 >> 21;
        s11 += carry10;
        s10 -= carry10 << 21;

        s[0] = (byte) s0;
        s[1] = (byte) (s0 >> 8);
        s[2] = (byte) ((s0 >> 16) | (s1 << 5));
        s[3] = (byte) (s1 >> 3);
        s[4] = (byte) (s1 >> 11);
        s[5] = (byte) ((s1 >> 19) | (s2 << 2));
        s[6] = (byte) (s2 >> 6);
        s[7] = (byte) ((s2 >> 14) | (s3 << 7));
        s[8] = (byte) (s3 >> 1);
        s[9] = (byte) (s3 >> 9);
        s[10] = (byte) ((s3 >> 17) | (s4 << 4));
        s[11] = (byte) (s4 >> 4);
        s[12] = (byte) (s4 >> 12);
        s[13] = (byte) ((s4 >> 20) | (s5 << 1));
        s[14] = (byte) (s5 >> 7);
        s[15] = (byte) ((s5 >> 15) | (s6 << 6));
        s[16] = (byte) (s6 >> 2);
        s[17] = (byte) (s6 >> 10);
        s[18] = (byte) ((s6 >> 18) | (s7 << 3));
        s[19] = (byte) (s7 >> 5);
        s[20] = (byte) (s7 >> 13);
        s[21] = (byte) s8;
        s[22] = (byte) (s8 >> 8);
        s[23] = (byte) ((s8 >> 16) | (s9 << 5));
        s[24] = (byte) (s9 >> 3);
        s[25] = (byte) (s9 >> 11);
        s[26] = (byte) ((s9 >> 19) | (s10 << 2));
        s[27] = (byte) (s10 >> 6);
        s[28] = (byte) ((s10 >> 14) | (s11 << 7));
        s[29] = (byte) (s11 >> 1);
        s[30] = (byte) (s11 >> 9);
        s[31] = (byte) (s11 >> 17);
    }

    // The order of the generator as unsigned bytes in little endian order.
    // (2^252 + 0x14def9dea2f79cd65812631a5cf5d3ed, cf. RFC 7748)
    private static final byte[] GROUP_ORDER = {
            (byte) 0xed, (byte) 0xd3, (byte) 0xf5, (byte) 0x5c,
            (byte) 0x1a, (byte) 0x63, (byte) 0x12, (byte) 0x58,
            (byte) 0xd6, (byte) 0x9c, (byte) 0xf7, (byte) 0xa2,
            (byte) 0xde, (byte) 0xf9, (byte) 0xde, (byte) 0x14,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x10};

    // Checks whether s represents an integer smaller than the order of the group.
    // This is needed to ensure that EdDSA signatures are non-malleable, as failing to check
    // the range of S allows to modify signatures (cf. RFC 8032, Section 5.2.7 and Section 8.4.)
    // @param s an integer in little-endian order.
    private static boolean isSmallerThanGroupOrder(byte[] s) {
        for (int j = Field25519.FIELD_LEN - 1; j >= 0; j--) {
            // compare unsigned bytes
            int a = s[j] & 0xff;
            int b = GROUP_ORDER[j] & 0xff;
            if (a != b) {
                return a < b;
            }
        }
        return false;
    }

    /**
     * Returns true if the EdDSA {@code signature} with {@code message}, can be verified with
     * {@code publicKey}.
     */
    public static boolean verify(final byte[] message, final byte[] signature,
                                 final byte[] publicKey) {
        try {
            if (signature.length != SIGNATURE_LEN) {
                return false;
            }
            if (publicKey.length != PUBLIC_KEY_LEN) {
                return false;
            }
            byte[] s = Arrays.copyOfRange(signature, Field25519.FIELD_LEN, SIGNATURE_LEN);
            if (!isSmallerThanGroupOrder(s)) {
                return false;
            }
            MessageDigest digest = MessageDigest.getInstance("SHA-512");
            digest.update(signature, 0, Field25519.FIELD_LEN);
            digest.update(publicKey);
            digest.update(message);
            byte[] h = digest.digest();
            reduce(h);

            XYZT negPublicKey = XYZT.fromBytesNegateVarTime(publicKey);
            XYZ xyz = doubleScalarMultVarTime(h, negPublicKey, s);
            byte[] expectedR = xyz.toBytes();
            for (int i = 0; i < Field25519.FIELD_LEN; i++) {
                if (expectedR[i] != signature[i]) {
                    return false;
                }
            }
            return true;
        } catch (final GeneralSecurityException ignored) {
            return false;
        }
    }
}
