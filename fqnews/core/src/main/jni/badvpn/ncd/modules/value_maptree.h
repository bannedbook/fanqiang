#define SAVL_PARAM_NAME MapTree
#define SAVL_PARAM_FEATURE_COUNTS 1
#define SAVL_PARAM_FEATURE_NOKEYS 0
#define SAVL_PARAM_TYPE_ENTRY struct value
#define SAVL_PARAM_TYPE_KEY NCDValRef
#define SAVL_PARAM_TYPE_ARG int
#define SAVL_PARAM_TYPE_COUNT size_t
#define SAVL_PARAM_VALUE_COUNT_MAX SIZE_MAX
#define SAVL_PARAM_FUN_COMPARE_ENTRIES(arg, entry1, entry2) NCDVal_Compare((entry1)->map_parent.key, (entry2)->map_parent.key)
#define SAVL_PARAM_FUN_COMPARE_KEY_ENTRY(arg, key1, entry2) NCDVal_Compare((key1), (entry2)->map_parent.key)
#define SAVL_PARAM_MEMBER_NODE map_parent.maptree_node
