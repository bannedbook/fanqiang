/*
 * Copyright © 2017-2019 WireGuard LLC. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.wireguard.crypto;

/**
 * Represents a Curve25519 key pair as used by WireGuard.
 * <p>
 * Instances of this class are immutable.
 */
public class KeyPair {
    private final Key privateKey;
    private final Key publicKey;

    /**
     * Creates a key pair using a newly-generated private key.
     */
    public KeyPair() {
        this(Key.generatePrivateKey());
    }

    /**
     * Creates a key pair using an existing private key.
     *
     * @param privateKey a private key, used to derive the public key
     */
    public KeyPair(final Key privateKey) {
        this.privateKey = privateKey;
        publicKey = Key.generatePublicKey(privateKey);
    }

    /**
     * Returns the private key from the key pair.
     *
     * @return the private key
     */
    public Key getPrivateKey() {
        return privateKey;
    }

    /**
     * Returns the public key from the key pair.
     *
     * @return the public key
     */
    public Key getPublicKey() {
        return publicKey;
    }
}
