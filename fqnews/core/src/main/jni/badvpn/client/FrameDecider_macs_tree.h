#define SAVL_PARAM_NAME FDMacsTree
#define SAVL_PARAM_FEATURE_COUNTS 0
#define SAVL_PARAM_FEATURE_NOKEYS 0
#define SAVL_PARAM_TYPE_ENTRY struct _FrameDecider_mac_entry
#define SAVL_PARAM_TYPE_KEY FDMacsTree_key
#define SAVL_PARAM_TYPE_ARG int
#define SAVL_PARAM_FUN_COMPARE_ENTRIES(arg, entry1, entry2) compare_macs((entry1)->mac, (entry2)->mac)
#define SAVL_PARAM_FUN_COMPARE_KEY_ENTRY(arg, key1, entry2) compare_macs((key1), (entry2)->mac)
#define SAVL_PARAM_MEMBER_NODE tree_node
