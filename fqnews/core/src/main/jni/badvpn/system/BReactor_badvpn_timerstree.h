#define CAVL_PARAM_NAME BReactor__TimersTree
#define CAVL_PARAM_FEATURE_COUNTS 0
#define CAVL_PARAM_FEATURE_KEYS_ARE_INDICES 0
#define CAVL_PARAM_FEATURE_NOKEYS 1
#define CAVL_PARAM_TYPE_ENTRY struct BSmallTimer_t
#define CAVL_PARAM_TYPE_LINK BReactor_timerstree_link
#define CAVL_PARAM_TYPE_ARG int
#define CAVL_PARAM_VALUE_NULL ((BReactor_timerstree_link)NULL)
#define CAVL_PARAM_FUN_DEREF(arg, link) (link)
#define CAVL_PARAM_FUN_COMPARE_ENTRIES(arg, entry1, entry2) compare_timers((entry1).link, (entry2).link)
#define CAVL_PARAM_MEMBER_CHILD u.tree_child
#define CAVL_PARAM_MEMBER_BALANCE tree_balance
#define CAVL_PARAM_MEMBER_PARENT tree_parent
