#define SAVL_PARAM_NAME FPAFramesTree
#define SAVL_PARAM_FEATURE_COUNTS 0
#define SAVL_PARAM_FEATURE_NOKEYS 0
#define SAVL_PARAM_TYPE_ENTRY struct FragmentProtoAssembler_frame
#define SAVL_PARAM_TYPE_KEY fragmentproto_frameid
#define SAVL_PARAM_TYPE_ARG int
#define SAVL_PARAM_FUN_COMPARE_ENTRIES(arg, entry1, entry2) B_COMPARE((entry1)->id, (entry2)->id)
#define SAVL_PARAM_FUN_COMPARE_KEY_ENTRY(arg, key1, entry2) B_COMPARE((key1), (entry2)->id)
#define SAVL_PARAM_MEMBER_NODE tree_node
