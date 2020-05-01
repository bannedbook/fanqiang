#define CHASH_PARAM_NAME NCDStringIndex__Hash
#define CHASH_PARAM_ENTRY struct NCDStringIndex__entry
#define CHASH_PARAM_LINK NCD_string_id_t
#define CHASH_PARAM_KEY NCDStringIndex_hash_key
#define CHASH_PARAM_ARG NCDStringIndex_hash_arg
#define CHASH_PARAM_NULL ((NCD_string_id_t)-1)
#define CHASH_PARAM_DEREF(arg, link) (&(arg)[(link)])
#define CHASH_PARAM_ENTRYHASH(arg, entry) badvpn_djb2_hash_bin((const uint8_t *)(entry).ptr->str, (entry).ptr->str_len)
#define CHASH_PARAM_KEYHASH(arg, key) badvpn_djb2_hash_bin((const uint8_t *)(key).str, (key).len)
#define CHASH_PARAM_ENTRYHASH_IS_CHEAP 0
#define CHASH_PARAM_COMPARE_ENTRIES(arg, entry1, entry2) ((entry1).ptr->str_len == (entry2).ptr->str_len && !memcmp((entry1).ptr->str, (entry2).ptr->str, (entry1).ptr->str_len))
#define CHASH_PARAM_COMPARE_KEY_ENTRY(arg, key1, entry2) ((key1).len == (entry2).ptr->str_len && !memcmp((key1).str, (entry2).ptr->str, (key1).len))
#define CHASH_PARAM_ENTRY_NEXT hash_next
