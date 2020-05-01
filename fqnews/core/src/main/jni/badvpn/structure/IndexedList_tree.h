#define CAVL_PARAM_NAME IndexedList__Tree
#define CAVL_PARAM_FEATURE_COUNTS 1
#define CAVL_PARAM_FEATURE_KEYS_ARE_INDICES 1
#define CAVL_PARAM_FEATURE_NOKEYS 0
#define CAVL_PARAM_TYPE_ENTRY IndexedListNode
#define CAVL_PARAM_TYPE_LINK IndexedList__tree_link
#define CAVL_PARAM_TYPE_ARG int
#define CAVL_PARAM_TYPE_COUNT uint64_t
#define CAVL_PARAM_VALUE_COUNT_MAX UINT64_MAX
#define CAVL_PARAM_VALUE_NULL ((IndexedList__tree_link)NULL)
#define CAVL_PARAM_FUN_DEREF(arg, link) (link)
#define CAVL_PARAM_MEMBER_CHILD tree_child
#define CAVL_PARAM_MEMBER_BALANCE tree_balance
#define CAVL_PARAM_MEMBER_PARENT tree_parent
#define CAVL_PARAM_MEMBER_COUNT tree_count
