set(PREDICATE_SOURCES
    BPredicate.c
    ${PROJECT_SOURCE_DIR}/generated/flex_BPredicate.c
    ${PROJECT_SOURCE_DIR}/generated/bison_BPredicate.c
)
badvpn_add_library(predicate "system" "" "${PREDICATE_SOURCES}")
