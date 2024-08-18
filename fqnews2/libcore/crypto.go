package libcore

import (
	"crypto/sha1"
	"crypto/sha256"
	"encoding/hex"
)

func Sha1(data []byte) []byte {
	sum := sha1.Sum(data)
	return sum[:]
}

func Sha256Hex(data []byte) string {
	sum := sha256.Sum256(data)
	return hex.EncodeToString(sum[:])
}
