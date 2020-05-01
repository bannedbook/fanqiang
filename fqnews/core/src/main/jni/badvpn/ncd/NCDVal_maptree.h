#define CAVL_PARAM_NAME NCDVal__MapTree
#define CAVL_PARAM_FEATURE_COUNTS 0
#define CAVL_PARAM_FEATURE_KEYS_ARE_INDICES 0
#define CAVL_PARAM_FEATURE_NOKEYS 0
#define CAVL_PARAM_TYPE_ENTRY NCDVal__maptree_entry
#define CAVL_PARAM_TYPE_LINK NCDVal__idx
#define CAVL_PARAM_TYPE_KEY NCDValRef
#define CAVL_PARAM_TYPE_ARG NCDVal__maptree_arg
#define CAVL_PARAM_VALUE_NULL ((NCDVal__idx)-1)
#define CAVL_PARAM_FUN_DEREF(arg, link) ((struct NCDVal__mapelem *)buffer_at((arg), (link)))
#define CAVL_PARAM_FUN_COMPARE_ENTRIES(arg, node1, node2) NCDVal_Compare(make_ref((arg), (node1).ptr->key_idx), make_ref((arg), (node2).ptr->key_idx))
#define CAVL_PARAM_FUN_COMPARE_KEY_ENTRY(arg, key1, node2) NCDVal_Compare((key1), make_ref((arg), (node2).ptr->key_idx))
#define CAVL_PARAM_MEMBER_CHILD tree_child
#define CAVL_PARAM_MEMBER_BALANCE tree_balance
#define CAVL_PARAM_MEMBER_PARENT tree_parent
