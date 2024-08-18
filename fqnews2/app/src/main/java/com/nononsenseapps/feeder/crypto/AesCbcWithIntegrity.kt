/*
 * Copyright (c) 2014-2015 Tozny LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Created by Isaac Potoczny-Jones on 11/12/14.
 */
package com.nononsenseapps.feeder.crypto

import android.util.Base64
import java.io.UnsupportedEncodingException
import java.lang.Exception
import java.nio.charset.Charset
import java.security.GeneralSecurityException
import java.security.InvalidKeyException
import java.security.NoSuchAlgorithmException
import java.security.SecureRandom
import java.security.spec.KeySpec
import javax.crypto.Cipher
import javax.crypto.KeyGenerator
import javax.crypto.Mac
import javax.crypto.SecretKey
import javax.crypto.SecretKeyFactory
import javax.crypto.spec.IvParameterSpec
import javax.crypto.spec.PBEKeySpec
import javax.crypto.spec.SecretKeySpec
import kotlin.experimental.xor

/**
 * Simple library for the "right" defaults for AES key generation, encryption,
 * and decryption using 128-bit AES, CBC, PKCS5 padding, and a random 16-byte IV
 * with SHA1PRNG. Integrity with HmacSHA256.
 */
object AesCbcWithIntegrity {
    private const val CIPHER_TRANSFORMATION = "AES/CBC/PKCS5Padding"
    private const val CIPHER = "AES"
    private const val AES_KEY_LENGTH_BITS = 128
    private const val IV_LENGTH_BYTES = 16
    private const val PBE_ITERATION_COUNT = 10000
    private const val PBE_SALT_LENGTH_BITS = AES_KEY_LENGTH_BITS // same size as key output
    private const val PBE_ALGORITHM = "PBKDF2WithHmacSHA1"

    // Made BASE_64_FLAGS public as it's useful to know for compatibility.
    const val BASE64_FLAGS = Base64.NO_WRAP

    // default for testing
    private const val HMAC_ALGORITHM = "HmacSHA256"
    private const val HMAC_KEY_LENGTH_BITS = 256

    /**
     * Converts the given AES/HMAC keys into a base64 encoded string suitable for
     * storage. Sister function of keys.
     *
     * @param keys The combined aes and hmac keys
     * @return a base 64 encoded AES string and hmac key as base64(aesKey) : base64(hmacKey)
     */
    fun encodeKey(keys: SecretKeys): String {
        return keys.toString()
    }

    /**
     * An aes key derived from a base64 encoded key. This does not generate the
     * key. It's not random or a PBE key.
     *
     * @param keysStr a base64 encoded AES key / hmac key as base64(aesKey) : base64(hmacKey).
     * @return an AES and HMAC key set suitable for other functions.
     */
    @Throws(InvalidKeyException::class)
    fun decodeKey(keysStr: String): SecretKeys {
        val keysArr = keysStr.split(":")
        return if (keysArr.size != 2) {
            throw IllegalArgumentException("Cannot parse aesKey:hmacKey")
        } else {
            val confidentialityKey = Base64.decode(keysArr[0], BASE64_FLAGS)
            if (confidentialityKey.size != AES_KEY_LENGTH_BITS / 8) {
                throw InvalidKeyException("Base64 decoded key is not $AES_KEY_LENGTH_BITS bytes")
            }
            val integrityKey = Base64.decode(keysArr[1], BASE64_FLAGS)
            if (integrityKey.size != HMAC_KEY_LENGTH_BITS / 8) {
                throw InvalidKeyException("Base64 decoded key is not $HMAC_KEY_LENGTH_BITS bytes")
            }
            SecretKeys(
                SecretKeySpec(confidentialityKey, 0, confidentialityKey.size, CIPHER),
                SecretKeySpec(integrityKey, HMAC_ALGORITHM),
            )
        }
    }

    fun isKeyDecodable(keysStr: String): Boolean {
        return try {
            decodeKey(keysStr)
            true
        } catch (e: Exception) {
            false
        }
    }

    /**
     * A function that generates random AES and HMAC keys and prints out exceptions but
     * doesn't throw them since none should be encountered. If they are
     * encountered, the return value is null.
     *
     * @return The AES and HMAC keys.
     * @throws GeneralSecurityException if AES is not implemented on this system,
     * or a suitable RNG is not available
     */
    @Throws(GeneralSecurityException::class)
    fun generateKey(): SecretKeys {
        val keyGen = KeyGenerator.getInstance(CIPHER)
        // No need to provide a SecureRandom or set a seed since that will
        // happen automatically.
        keyGen.init(AES_KEY_LENGTH_BITS)
        val confidentialityKey = keyGen.generateKey()

        // Now make the HMAC key
        val integrityKeyBytes = randomBytes(HMAC_KEY_LENGTH_BITS / 8) // to get bytes
        val integrityKey: SecretKey = SecretKeySpec(integrityKeyBytes, HMAC_ALGORITHM)
        return SecretKeys(confidentialityKey, integrityKey)
    }

    /**
     * A function that generates password-based AES and HMAC keys. It prints out exceptions but
     * doesn't throw them since none should be encountered. If they are
     * encountered, the return value is null.
     *
     * @param password The password to derive the keys from.
     * @return The AES and HMAC keys.
     * @throws GeneralSecurityException if AES is not implemented on this system,
     * or a suitable RNG is not available
     */
    @Throws(GeneralSecurityException::class)
    fun generateKeyFromPassword(
        password: String,
        salt: ByteArray,
    ): SecretKeys {
        // Get enough random bytes for both the AES key and the HMAC key:
        val keySpec: KeySpec =
            PBEKeySpec(
                password.toCharArray(),
                salt,
                PBE_ITERATION_COUNT,
                AES_KEY_LENGTH_BITS + HMAC_KEY_LENGTH_BITS,
            )
        val keyFactory =
            SecretKeyFactory
                .getInstance(PBE_ALGORITHM)
        val keyBytes = keyFactory.generateSecret(keySpec).encoded

        // Split the random bytes into two parts:
        val confidentialityKeyBytes = keyBytes.copyOfRange(0, AES_KEY_LENGTH_BITS / 8)
        val integrityKeyBytes =
            keyBytes.copyOfRange(
                AES_KEY_LENGTH_BITS / 8,
                AES_KEY_LENGTH_BITS / 8 + HMAC_KEY_LENGTH_BITS / 8,
            )

        // Generate the AES key
        val confidentialityKey: SecretKey = SecretKeySpec(confidentialityKeyBytes, CIPHER)

        // Generate the HMAC key
        val integrityKey: SecretKey = SecretKeySpec(integrityKeyBytes, HMAC_ALGORITHM)
        return SecretKeys(confidentialityKey, integrityKey)
    }

    /**
     * A function that generates password-based AES and HMAC keys. See generateKeyFromPassword.
     * @param password The password to derive the AES/HMAC keys from
     * @param salt A string version of the salt; base64 encoded.
     * @return The AES and HMAC keys.
     * @throws GeneralSecurityException
     */
    @Throws(GeneralSecurityException::class)
    fun generateKeyFromPassword(
        password: String,
        salt: String,
    ): SecretKeys {
        return generateKeyFromPassword(password, Base64.decode(salt, BASE64_FLAGS))
    }

    /**
     * Generates a random salt.
     * @return The random salt suitable for generateKeyFromPassword.
     */
    @Throws(GeneralSecurityException::class)
    fun generateSalt(): ByteArray {
        return randomBytes(PBE_SALT_LENGTH_BITS)
    }

    /**
     * Converts the given salt into a base64 encoded string suitable for
     * storage.
     *
     * @param salt
     * @return a base 64 encoded salt string suitable to pass into generateKeyFromPassword.
     */
    fun saltString(salt: ByteArray): String {
        return Base64.encodeToString(salt, BASE64_FLAGS)
    }

    /**
     * Creates a random Initialization Vector (IV) of IV_LENGTH_BYTES.
     *
     * @return The byte array of this IV
     * @throws GeneralSecurityException if a suitable RNG is not available
     */
    @Throws(GeneralSecurityException::class)
    fun generateIv(): ByteArray {
        return randomBytes(IV_LENGTH_BYTES)
    }

    @Throws(GeneralSecurityException::class)
    private fun randomBytes(length: Int): ByteArray {
        val random = SecureRandom()
        val b = ByteArray(length)
        random.nextBytes(b)
        return b
    }

/*
 * -----------------------------------------------------------------
 * Encryption
 * -----------------------------------------------------------------
 */

    /**
     * Generates a random IV and encrypts this plain text with the given key. Then attaches
     * a hashed MAC, which is contained in the CipherTextIvMac class.
     *
     * @param plaintext The text that will be encrypted, which
     * will be serialized with UTF-8
     * @param secretKeys The AES and HMAC keys with which to encrypt
     * @param encoding The string encoding to use to encode the bytes before encryption
     * @return a tuple of the IV, ciphertext, mac
     * @throws GeneralSecurityException if AES is not implemented on this system
     * @throws UnsupportedEncodingException if UTF-8 is not supported in this system
     */
    @Throws(UnsupportedEncodingException::class, GeneralSecurityException::class)
    fun encryptString(
        plaintext: String,
        secretKeys: SecretKeys,
        encoding: Charset = Charsets.UTF_8,
    ): String =
        encrypt(
            plaintext = plaintext,
            secretKeys = secretKeys,
            encoding = encoding,
        ).toString()

    /**
     * Generates a random IV and encrypts this plain text with the given key. Then attaches
     * a hashed MAC, which is contained in the CipherTextIvMac class.
     *
     * @param plaintext The text that will be encrypted, which
     * will be serialized with UTF-8
     * @param secretKeys The AES and HMAC keys with which to encrypt
     * @param encoding The string encoding to use to encode the bytes before encryption
     * @return a tuple of the IV, ciphertext, mac
     * @throws GeneralSecurityException if AES is not implemented on this system
     * @throws UnsupportedEncodingException if UTF-8 is not supported in this system
     */
    @Throws(UnsupportedEncodingException::class, GeneralSecurityException::class)
    fun encrypt(
        plaintext: String,
        secretKeys: SecretKeys,
        encoding: Charset = Charsets.UTF_8,
    ): CipherTextIvMac {
        return encrypt(plaintext.toByteArray(encoding), secretKeys)
    }

    /**
     * Generates a random IV and encrypts this plain text with the given key. Then attaches
     * a hashed MAC, which is contained in the CipherTextIvMac class.
     *
     * @param plaintext The text that will be encrypted
     * @param secretKeys The combined AES and HMAC keys with which to encrypt
     * @return a tuple of the IV, ciphertext, mac
     * @throws GeneralSecurityException if AES is not implemented on this system
     */
    @Throws(GeneralSecurityException::class)
    fun encrypt(
        plaintext: ByteArray,
        secretKeys: SecretKeys,
    ): CipherTextIvMac {
        var iv = generateIv()
        val aesCipherForEncryption = Cipher.getInstance(CIPHER_TRANSFORMATION)
        aesCipherForEncryption.init(
            Cipher.ENCRYPT_MODE,
            secretKeys.confidentialityKey,
            IvParameterSpec(iv),
        )

        /*
         * Now we get back the IV that will actually be used. Some Android
         * versions do funny stuff w/ the IV, so this is to work around bugs:
         */
        iv = aesCipherForEncryption.iv
        val byteCipherText = aesCipherForEncryption.doFinal(plaintext)
        val ivCipherConcat = CipherTextIvMac.ivCipherConcat(iv, byteCipherText)
        val integrityMac = generateMac(ivCipherConcat, secretKeys.integrityKey)
        return CipherTextIvMac(byteCipherText, iv, integrityMac)
    }

/*
 * -----------------------------------------------------------------
 * Decryption
 * -----------------------------------------------------------------
 */

    /**
     * AES CBC decrypt.
     *
     * @param civ The cipher text, IV, and mac
     * @param secretKeys The AES and HMAC keys
     * @param encoding The string encoding to use to decode the bytes after decryption
     * @return A string derived from the decrypted bytes (not base64 encoded)
     * @throws GeneralSecurityException if AES is not implemented on this system
     * @throws UnsupportedEncodingException if the encoding is unsupported
     */
    @Throws(UnsupportedEncodingException::class, GeneralSecurityException::class)
    fun decryptString(
        civ: String,
        secretKeys: SecretKeys,
        encoding: Charset = Charsets.UTF_8,
    ): String {
        return String(decrypt(CipherTextIvMac(civ), secretKeys), encoding)
    }

    /**
     * AES CBC decrypt.
     *
     * @param civ The cipher text, IV, and mac
     * @param secretKeys The AES and HMAC keys
     * @param encoding The string encoding to use to decode the bytes after decryption
     * @return A string derived from the decrypted bytes (not base64 encoded)
     * @throws GeneralSecurityException if AES is not implemented on this system
     * @throws UnsupportedEncodingException if the encoding is unsupported
     */
    @Throws(UnsupportedEncodingException::class, GeneralSecurityException::class)
    fun decrypt(
        civ: CipherTextIvMac,
        secretKeys: SecretKeys,
        encoding: Charset = Charsets.UTF_8,
    ): String {
        return String(decrypt(civ, secretKeys), encoding)
    }

    /**
     * AES CBC decrypt.
     *
     * @param civ the cipher text, iv, and mac
     * @param secretKeys the AES and HMAC keys
     * @return The raw decrypted bytes
     * @throws GeneralSecurityException if MACs don't match or AES is not implemented
     */
    @Throws(GeneralSecurityException::class)
    fun decrypt(
        civ: CipherTextIvMac,
        secretKeys: SecretKeys,
    ): ByteArray {
        val ivCipherConcat = CipherTextIvMac.ivCipherConcat(civ.iv, civ.cipherText)
        val computedMac = generateMac(ivCipherConcat, secretKeys.integrityKey)
        return if (constantTimeEq(computedMac, civ.mac)) {
            val aesCipherForDecryption = Cipher.getInstance(CIPHER_TRANSFORMATION)
            aesCipherForDecryption.init(
                Cipher.DECRYPT_MODE,
                secretKeys.confidentialityKey,
                IvParameterSpec(civ.iv),
            )
            aesCipherForDecryption.doFinal(civ.cipherText)
        } else {
            throw GeneralSecurityException("MAC stored in civ does not match computed MAC.")
        }
    }

/*
 * -----------------------------------------------------------------
 * Helper Code
 * -----------------------------------------------------------------
 */

    /**
     * Generate the mac based on HMAC_ALGORITHM
     * @param integrityKey The key used for hmac
     * @param byteCipherText the cipher text
     * @return A byte array of the HMAC for the given key and ciphertext
     * @throws NoSuchAlgorithmException
     * @throws InvalidKeyException
     */
    @Throws(NoSuchAlgorithmException::class, InvalidKeyException::class)
    fun generateMac(
        byteCipherText: ByteArray,
        integrityKey: SecretKey,
    ): ByteArray {
        // Now compute the mac for later integrity checking
        val sha256HMAC = Mac.getInstance(HMAC_ALGORITHM)
        sha256HMAC.init(integrityKey)
        return sha256HMAC.doFinal(byteCipherText)
    }

    /**
     * Simple constant-time equality of two byte arrays. Used for security to avoid timing attacks.
     * @param a
     * @param b
     * @return true iff the arrays are exactly equal.
     */
    private fun constantTimeEq(
        a: ByteArray,
        b: ByteArray,
    ): Boolean {
        if (a.size != b.size) {
            return false
        }
        var result = 0
        for (i in a.indices) {
            result = result or (a[i] xor b[i]).toInt()
        }
        return result == 0
    }
}

/**
 * Holder class that has both the secret AES key for encryption (confidentiality)
 * and the secret HMAC key for integrity.
 */
class SecretKeys(
    val confidentialityKey: SecretKey,
    val integrityKey: SecretKey,
) {
    /**
     * Encodes the two keys as a string
     * @return base64(confidentialityKey):base64(integrityKey)
     */
    override fun toString(): String {
        val a =
            Base64.encodeToString(
                confidentialityKey.encoded,
                AesCbcWithIntegrity.BASE64_FLAGS,
            )
        val b =
            Base64.encodeToString(
                integrityKey.encoded,
                AesCbcWithIntegrity.BASE64_FLAGS,
            )
        return "$a:$b"
    }

    override fun hashCode(): Int {
        val prime = 31
        var result = 1
        result = prime * result + confidentialityKey.hashCode()
        result = prime * result + integrityKey.hashCode()
        return result
    }

    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other == null) return false
        if (other !is SecretKeys) return false
        if (integrityKey != other.integrityKey) return false
        return confidentialityKey == other.confidentialityKey
    }
}

/**
 * Holder class that allows us to bundle ciphertext and IV together.
 */
class CipherTextIvMac {
    val cipherText: ByteArray
    val iv: ByteArray
    val mac: ByteArray

    /**
     * Construct a new bundle of ciphertext and IV.
     * @param c The ciphertext
     * @param i The IV
     * @param h The mac
     */
    constructor(c: ByteArray, i: ByteArray, h: ByteArray) {
        cipherText = ByteArray(c.size)
        System.arraycopy(c, 0, cipherText, 0, c.size)
        iv = ByteArray(i.size)
        System.arraycopy(i, 0, iv, 0, i.size)
        mac = ByteArray(h.size)
        System.arraycopy(h, 0, mac, 0, h.size)
    }

    /**
     * Constructs a new bundle of ciphertext and IV from a string of the
     * format `base64(iv):base64(ciphertext)`.
     *
     * @param base64IvAndCiphertext A string of the format
     * `iv:ciphertext` The IV and ciphertext must each
     * be base64-encoded.
     */
    constructor(base64IvAndCiphertext: String) {
        val civArray = base64IvAndCiphertext.split(":")
        require(civArray.size == 3) { "Cannot parse iv:mac:ciphertext" }
        iv = Base64.decode(civArray[0], AesCbcWithIntegrity.BASE64_FLAGS)
        mac = Base64.decode(civArray[1], AesCbcWithIntegrity.BASE64_FLAGS)
        cipherText = Base64.decode(civArray[2], AesCbcWithIntegrity.BASE64_FLAGS)
    }

    /**
     * Encodes this ciphertext, IV, mac as a string.
     *
     * @return base64(iv) : base64(mac) : base64(ciphertext).
     * The iv and mac go first because they're fixed length.
     */
    override fun toString(): String {
        val ivString = Base64.encodeToString(iv, AesCbcWithIntegrity.BASE64_FLAGS)
        val cipherTextString = Base64.encodeToString(cipherText, AesCbcWithIntegrity.BASE64_FLAGS)
        val macString = Base64.encodeToString(mac, AesCbcWithIntegrity.BASE64_FLAGS)
        return String.format("$ivString:$macString:$cipherTextString")
    }

    override fun hashCode(): Int {
        val prime = 31
        var result = 1
        result = prime * result + cipherText.contentHashCode()
        result = prime * result + iv.contentHashCode()
        result = prime * result + mac.contentHashCode()
        return result
    }

    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other == null) return false
        if (other !is CipherTextIvMac) return false
        if (!cipherText.contentEquals(other.cipherText)) return false
        if (!iv.contentEquals(other.iv)) return false
        return mac.contentEquals(other.mac)
    }

    companion object {
        /**
         * Concatenate the IV to the cipherText using array copy.
         * This is used e.g. before computing mac.
         * @param iv The IV to prepend
         * @param cipherText the cipherText to append
         * @return iv:cipherText, a new byte array.
         */
        fun ivCipherConcat(
            iv: ByteArray,
            cipherText: ByteArray,
        ): ByteArray {
            val combined = ByteArray(iv.size + cipherText.size)
            System.arraycopy(iv, 0, combined, 0, iv.size)
            System.arraycopy(cipherText, 0, combined, iv.size, cipherText.size)
            return combined
        }
    }
}
