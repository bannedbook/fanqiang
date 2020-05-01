#define SAVL_PARAM_NAME FDGroupsTree
#define SAVL_PARAM_FEATURE_COUNTS 0
#define SAVL_PARAM_FEATURE_NOKEYS 0
#define SAVL_PARAM_TYPE_ENTRY struct _FrameDecider_group_entry
#define SAVL_PARAM_TYPE_KEY uint32_t
#define SAVL_PARAM_TYPE_ARG int
#define SAVL_PARAM_FUN_COMPARE_ENTRIES(arg, entry1, entry2) B_COMPARE((entry1)->group, (entry2)->group)
#define SAVL_PARAM_FUN_COMPARE_KEY_ENTRY(arg, key1, entry2) B_COMPARE((key1), (entry2)->group)
#define SAVL_PARAM_MEMBER_NODE tree_node
