#define SAVL_PARAM_NAME PacketPassFairQueue__Tree
#define SAVL_PARAM_FEATURE_COUNTS 0
#define SAVL_PARAM_FEATURE_NOKEYS 1
#define SAVL_PARAM_TYPE_ENTRY struct PacketPassFairQueueFlow_s
#define SAVL_PARAM_TYPE_ARG int
#define SAVL_PARAM_FUN_COMPARE_ENTRIES(arg, entry1, entry2) compare_flows((entry1), (entry2))
#define SAVL_PARAM_MEMBER_NODE queued.tree_node
