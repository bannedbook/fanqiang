set(SECURITY_SOURCES
    BSecurity.c
    BEncryption.c
    BHash.c
    BRandom.c
    OTPCalculator.c
    OTPChecker.c
    OTPGenerator.c
)
badvpn_add_library(security "system;threadwork" "${LIBCRYPTO_LIBRARIES}" "${SECURITY_SOURCES}")
