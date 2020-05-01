#define SAVL_PARAM_NAME MyTree
#define SAVL_PARAM_FEATURE_COUNTS 1
#define SAVL_PARAM_FEATURE_NOKEYS 1
#define SAVL_PARAM_TYPE_ENTRY struct mynode
#define SAVL_PARAM_TYPE_ARG int
#define SAVL_PARAM_TYPE_COUNT int
#define SAVL_PARAM_VALUE_COUNT_MAX INT_MAX
#define SAVL_PARAM_FUN_COMPARE_ENTRIES(arg, entry1, entry2) B_COMPARE((entry1)->num, (entry2)->num)
#define SAVL_PARAM_MEMBER_NODE tree_node
