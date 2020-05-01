#define CHASH_PARAM_NAME NCDModuleIndex__FuncHash
#define CHASH_PARAM_ENTRY struct NCDModuleIndex__Func
#define CHASH_PARAM_LINK int
#define CHASH_PARAM_KEY NCD_string_id_t
#define CHASH_PARAM_ARG NCDModuleIndex *
#define CHASH_PARAM_NULL ((int)-1)
#define CHASH_PARAM_DEREF(arg, link) (NCDModuleIndex__FuncVec_Get(&(arg)->func_vec, (link)))
#define CHASH_PARAM_ENTRYHASH(arg, entry) ((size_t)(entry).ptr->ifunc.func_name_id)
#define CHASH_PARAM_KEYHASH(arg, key) ((size_t)(key))
#define CHASH_PARAM_COMPARE_ENTRIES(arg, entry1, entry2) ((entry1).ptr->ifunc.func_name_id == (entry2).ptr->ifunc.func_name_id)
#define CHASH_PARAM_COMPARE_KEY_ENTRY(arg, key1, entry2) ((key1) == (entry2).ptr->ifunc.func_name_id)
#define CHASH_PARAM_ENTRY_NEXT hash_next
