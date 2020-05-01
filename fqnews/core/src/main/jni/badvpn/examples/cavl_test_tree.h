#define CAVL_PARAM_NAME MyTree
#define CAVL_PARAM_FEATURE_COUNTS USE_COUNTS
#define CAVL_PARAM_FEATURE_KEYS_ARE_INDICES 0
#define CAVL_PARAM_FEATURE_ASSOC USE_ASSOC
#define CAVL_PARAM_TYPE_ENTRY struct entry
#define CAVL_PARAM_TYPE_LINK entry_index
#define CAVL_PARAM_TYPE_KEY entry_key
#define CAVL_PARAM_TYPE_ARG entry_ptr
#define CAVL_PARAM_TYPE_COUNT size_t
#define CAVL_PARAM_TYPE_ASSOC assoc_sum
#define CAVL_PARAM_VALUE_COUNT_MAX SIZE_MAX
#define CAVL_PARAM_VALUE_NULL ((entry_index)-1)
#define CAVL_PARAM_VALUE_ASSOC_ZERO 0
#define CAVL_PARAM_FUN_DEREF(arg, link) (&(arg)[(link)])
#define CAVL_PARAM_FUN_COMPARE_ENTRIES(arg, entry1, entry2) B_COMPARE((entry1).ptr->key, (entry2).ptr->key)
#define CAVL_PARAM_FUN_COMPARE_KEY_ENTRY(arg, key1, entry2) B_COMPARE((key1), (entry2).ptr->key)
#define CAVL_PARAM_FUN_ASSOC_VALUE(arg, entry) ((entry).ptr->assoc_value)
#define CAVL_PARAM_FUN_ASSOC_OPER(arg, value1, value2) ((value1) + (value2))
#define CAVL_PARAM_MEMBER_CHILD tree_child
#define CAVL_PARAM_MEMBER_BALANCE tree_balance
#define CAVL_PARAM_MEMBER_PARENT tree_parent
#define CAVL_PARAM_MEMBER_COUNT tree_count
#define CAVL_PARAM_MEMBER_ASSOC assoc_sum
