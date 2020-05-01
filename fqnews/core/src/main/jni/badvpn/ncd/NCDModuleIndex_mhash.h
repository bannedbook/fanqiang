#define CHASH_PARAM_NAME NCDModuleIndex__MHash
#define CHASH_PARAM_ENTRY struct NCDModuleIndex_module
#define CHASH_PARAM_LINK NCDModuleIndex__mhash_link
#define CHASH_PARAM_KEY NCDModuleIndex__mhash_key
#define CHASH_PARAM_ARG int
#define CHASH_PARAM_NULL ((NCDModuleIndex__mhash_link)NULL)
#define CHASH_PARAM_DEREF(arg, link) (link)
#define CHASH_PARAM_ENTRYHASH(arg, entry) (badvpn_djb2_hash((const uint8_t *)(entry).ptr->imodule.module.type))
#define CHASH_PARAM_KEYHASH(arg, key) (badvpn_djb2_hash((const uint8_t *)(key)))
#define CHASH_PARAM_COMPARE_ENTRIES(arg, entry1, entry2) (!strcmp((entry1).ptr->imodule.module.type, (entry2).ptr->imodule.module.type))
#define CHASH_PARAM_COMPARE_KEY_ENTRY(arg, key1, entry2) (!strcmp((key1), (entry2).ptr->imodule.module.type))
#define CHASH_PARAM_ENTRY_NEXT hash_next
