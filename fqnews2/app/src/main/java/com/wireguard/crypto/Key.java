/*
 * Copyright © 2017-2019 WireGuard LLC. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.wireguard.crypto;

import com.wireguard.crypto.KeyFormatException.Type;

import java.security.MessageDigest;
import java.security.SecureRandom;
import java.util.Arrays;

/**
 * Represents a WireGuard public or private key. This class uses specialized constant-time base64
 * and hexadecimal codec implementations that resist side-channel attacks.
 * <p>
 * Instances of this class are immutable.
 */
@SuppressWarnings("MagicNumber")
public final class Key {
    private final byte[] key;

    /**
     * Constructs an object encapsulating the supplied key.
     *
     * @param key an array of bytes containing a binary key. Callers of this constructor are
     *            responsible for ensuring that the array is of the correct length.
     */
    private Key(final byte[] key) {
        // Defensively copy to ensure immutability.
        this.key = Arrays.copyOf(key, key.length);
    }

    /**
     * Decodes a single 4-character base64 chunk to an integer in constant time.
     *
     * @param src       an array of at least 4 characters in base64 format
     * @param srcOffset the offset of the beginning of the chunk in {@code src}
     * @return the decoded 3-byte integer, or some arbitrary integer value if the input was not
     * valid base64
     */
    private static int decodeBase64(final char[] src, final int srcOffset) {
        int val = 0;
        for (int i = 0; i < 4; ++i) {
            final char c = src[i + srcOffset];
            val |= (-1
                    + ((((('A' - 1) - c) & (c - ('Z' + 1))) >>> 8) & (c - 64))
                    + ((((('a' - 1) - c) & (c - ('z' + 1))) >>> 8) & (c - 70))
                    + ((((('0' - 1) - c) & (c - ('9' + 1))) >>> 8) & (c + 5))
                    + ((((('+' - 1) - c) & (c - ('+' + 1))) >>> 8) & 63)
                    + ((((('/' - 1) - c) & (c - ('/' + 1))) >>> 8) & 64)
            ) << (18 - 6 * i);
        }
        return val;
    }

    /**
     * Encodes a single 4-character base64 chunk from 3 consecutive bytes in constant time.
     *
     * @param src        an array of at least 3 bytes
     * @param srcOffset  the offset of the beginning of the chunk in {@code src}
     * @param dest       an array of at least 4 characters
     * @param destOffset the offset of the beginning of the chunk in {@code dest}
     */
    private static void encodeBase64(final byte[] src, final int srcOffset,
                                     final char[] dest, final int destOffset) {
        final byte[] input = {
                (byte) ((src[srcOffset] >>> 2) & 63),
                (byte) ((src[srcOffset] << 4 | ((src[1 + srcOffset] & 0xff) >>> 4)) & 63),
                (byte) ((src[1 + srcOffset] << 2 | ((src[2 + srcOffset] & 0xff) >>> 6)) & 63),
                (byte) ((src[2 + srcOffset]) & 63),
        };
        for (int i = 0; i < 4; ++i) {
            dest[i + destOffset] = (char) (input[i] + 'A'
                    + (((25 - input[i]) >>> 8) & 6)
                    - (((51 - input[i]) >>> 8) & 75)
                    - (((61 - input[i]) >>> 8) & 15)
                    + (((62 - input[i]) >>> 8) & 3));
        }
    }

    /**
     * Decodes a WireGuard public or private key from its base64 string representation. This
     * function throws a {@link KeyFormatException} if the source string is not well-formed.
     *
     * @param str the base64 string representation of a WireGuard key
     * @return the decoded key encapsulated in an immutable container
     */
    public static Key fromBase64(final String str) throws KeyFormatException {
        final char[] input = str.toCharArray();
        if (input.length != Format.BASE64.length || input[Format.BASE64.length - 1] != '=')
            throw new KeyFormatException(Format.BASE64, Type.LENGTH);
        final byte[] key = new byte[Format.BINARY.length];
        int i;
        int ret = 0;
        for (i = 0; i < key.length / 3; ++i) {
            final int val = decodeBase64(input, i * 4);
            ret |= val >>> 31;
            key[i * 3] = (byte) ((val >>> 16) & 0xff);
            key[i * 3 + 1] = (byte) ((val >>> 8) & 0xff);
            key[i * 3 + 2] = (byte) (val & 0xff);
        }
        final char[] endSegment = {
                input[i * 4],
                input[i * 4 + 1],
                input[i * 4 + 2],
                'A',
        };
        final int val = decodeBase64(endSegment, 0);
        ret |= (val >>> 31) | (val & 0xff);
        key[i * 3] = (byte) ((val >>> 16) & 0xff);
        key[i * 3 + 1] = (byte) ((val >>> 8) & 0xff);

        if (ret != 0)
            throw new KeyFormatException(Format.BASE64, Type.CONTENTS);
        return new Key(key);
    }

    /**
     * Wraps a WireGuard public or private key in an immutable container. This function throws a
     * {@link KeyFormatException} if the source data is not the correct length.
     *
     * @param bytes an array of bytes containing a WireGuard key in binary format
     * @return the key encapsulated in an immutable container
     */
    public static Key fromBytes(final byte[] bytes) throws KeyFormatException {
        if (bytes.length != Format.BINARY.length)
            throw new KeyFormatException(Format.BINARY, Type.LENGTH);
        return new Key(bytes);
    }

    /**
     * Decodes a WireGuard public or private key from its hexadecimal string representation. This
     * function throws a {@link KeyFormatException} if the source string is not well-formed.
     *
     * @param str the hexadecimal string representation of a WireGuard key
     * @return the decoded key encapsulated in an immutable container
     */
    public static Key fromHex(final String str) throws KeyFormatException {
        final char[] input = str.toCharArray();
        if (input.length != Format.HEX.length)
            throw new KeyFormatException(Format.HEX, Type.LENGTH);
        final byte[] key = new byte[Format.BINARY.length];
        int ret = 0;
        for (int i = 0; i < key.length; ++i) {
            int c;
            int cNum;
            int cNum0;
            int cAlpha;
            int cAlpha0;
            int cVal;
            final int cAcc;

            c = input[i * 2];
            cNum = c ^ 48;
            cNum0 = ((cNum - 10) >>> 8) & 0xff;
            cAlpha = (c & ~32) - 55;
            cAlpha0 = (((cAlpha - 10) ^ (cAlpha - 16)) >>> 8) & 0xff;
            ret |= ((cNum0 | cAlpha0) - 1) >>> 8;
            cVal = (cNum0 & cNum) | (cAlpha0 & cAlpha);
            cAcc = cVal * 16;

            c = input[i * 2 + 1];
            cNum = c ^ 48;
            cNum0 = ((cNum - 10) >>> 8) & 0xff;
            cAlpha = (c & ~32) - 55;
            cAlpha0 = (((cAlpha - 10) ^ (cAlpha - 16)) >>> 8) & 0xff;
            ret |= ((cNum0 | cAlpha0) - 1) >>> 8;
            cVal = (cNum0 & cNum) | (cAlpha0 & cAlpha);
            key[i] = (byte) (cAcc | cVal);
        }
        if (ret != 0)
            throw new KeyFormatException(Format.HEX, Type.CONTENTS);
        return new Key(key);
    }

    /**
     * Generates a private key using the system's {@link SecureRandom} number generator.
     *
     * @return a well-formed random private key
     */
    static Key generatePrivateKey() {
        final SecureRandom secureRandom = new SecureRandom();
        final byte[] privateKey = new byte[Format.BINARY.getLength()];
        secureRandom.nextBytes(privateKey);
        privateKey[0] &= 248;
        privateKey[31] &= 127;
        privateKey[31] |= 64;
        return new Key(privateKey);
    }

    /**
     * Generates a public key from an existing private key.
     *
     * @param privateKey a private key
     * @return a well-formed public key that corresponds to the supplied private key
     */
    static Key generatePublicKey(final Key privateKey) {
        final byte[] publicKey = new byte[Format.BINARY.getLength()];
        Curve25519.eval(publicKey, 0, privateKey.getBytes(), null);
        return new Key(publicKey);
    }

    @Override
    public boolean equals(final Object obj) {
        if (obj == this)
            return true;
        if (obj == null || obj.getClass() != getClass())
            return false;
        final Key other = (Key) obj;
        return MessageDigest.isEqual(key, other.key);
    }

    /**
     * Returns the key as an array of bytes.
     *
     * @return an array of bytes containing the raw binary key
     */
    public byte[] getBytes() {
        // Defensively copy to ensure immutability.
        return Arrays.copyOf(key, key.length);
    }

    @Override
    public int hashCode() {
        int ret = 0;
        for (int i = 0; i < key.length / 4; ++i)
            ret ^= (key[i * 4 + 0] >> 0) + (key[i * 4 + 1] >> 8) + (key[i * 4 + 2] >> 16) + (key[i * 4 + 3] >> 24);
        return ret;
    }

    /**
     * Encodes the key to base64.
     *
     * @return a string containing the encoded key
     */
    public String toBase64() {
        final char[] output = new char[Format.BASE64.length];
        int i;
        for (i = 0; i < key.length / 3; ++i)
            encodeBase64(key, i * 3, output, i * 4);
        final byte[] endSegment = {
                key[i * 3],
                key[i * 3 + 1],
                0,
        };
        encodeBase64(endSegment, 0, output, i * 4);
        output[Format.BASE64.length - 1] = '=';
        return new String(output);
    }

    /**
     * Encodes the key to hexadecimal ASCII characters.
     *
     * @return a string containing the encoded key
     */
    public String toHex() {
        final char[] output = new char[Format.HEX.length];
        for (int i = 0; i < key.length; ++i) {
            output[i * 2] = (char) (87 + (key[i] >> 4 & 0xf)
                    + ((((key[i] >> 4 & 0xf) - 10) >> 8) & ~38));
            output[i * 2 + 1] = (char) (87 + (key[i] & 0xf)
                    + ((((key[i] & 0xf) - 10) >> 8) & ~38));
        }
        return new String(output);
    }

    /**
     * The supported formats for encoding a WireGuard key.
     */
    public enum Format {
        BASE64(44),
        BINARY(32),
        HEX(64);

        private final int length;

        Format(final int length) {
            this.length = length;
        }

        public int getLength() {
            return length;
        }
    }

}
