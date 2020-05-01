/**
 * @file FrameDecider.h
 * @author Ambroz Bizjak <ambrop7@gmail.com>
 * 
 * @section LICENSE
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * @section DESCRIPTION
 * 
 * Mudule which decides to which peers frames from the device are to be
 * forwarded.
 */

#ifndef BADVPN_CLIENT_FRAMEDECIDER_H
#define BADVPN_CLIENT_FRAMEDECIDER_H

#include <stdint.h>

#include <structure/LinkedList1.h>
#include <structure/LinkedList3.h>
#include <structure/SAvl.h>
#include <base/DebugObject.h>
#include <base/BLog.h>
#include <system/BReactor.h>

struct _FrameDeciderPeer;
struct _FrameDecider_mac_entry;
struct _FrameDecider_group_entry;

typedef const uint8_t *FDMacsTree_key;

#include "FrameDecider_macs_tree.h"
#include <structure/SAvl_decl.h>

#include "FrameDecider_groups_tree.h"
#include <structure/SAvl_decl.h>

#include "FrameDecider_multicast_tree.h"
#include <structure/SAvl_decl.h>

struct _FrameDecider_mac_entry {
    struct _FrameDeciderPeer *peer;
    LinkedList1Node list_node; // node in FrameDeciderPeer.mac_entries_free or FrameDeciderPeer.mac_entries_used
    // defined when used:
    uint8_t mac[6];
    FDMacsTreeNode tree_node; // node in FrameDecider.macs_tree, indexed by mac
};

struct _FrameDecider_group_entry {
    struct _FrameDeciderPeer *peer;
    LinkedList1Node list_node; // node in FrameDeciderPeer.group_entries_free or FrameDeciderPeer.group_entries_used
    BTimer timer; // timer for removing the group entry, running when used
    // defined when used:
    // basic group data
    uint32_t group; // group address
    FDGroupsTreeNode tree_node; // node in FrameDeciderPeer.groups_tree, indexed by group
    // all that folows is managed by add_to_multicast() and remove_from_multicast()
    LinkedList3Node sig_list_node; // node in list of group entries with the same sig
    btime_t timer_endtime;
    int is_master;
    // defined when used and we are master:
    struct {
        uint32_t sig; // last 23 bits of group address
        FDMulticastTreeNode tree_node; // node in FrameDecider.multicast_tree, indexed by sig
    } master;
};

/**
 * Object that represents a local device.
 */
typedef struct {
    int max_peer_macs;
    int max_peer_groups;
    btime_t igmp_group_membership_interval;
    btime_t igmp_last_member_query_time;
    BReactor *reactor;
    LinkedList1 peers_list;
    FDMacsTree macs_tree;
    FDMulticastTree multicast_tree;
    int decide_state;
    LinkedList1Node *decide_flood_current;
    struct _FrameDeciderPeer *decide_unicast_peer;
    LinkedList3Iterator decide_multicast_it;
    DebugObject d_obj;
} FrameDecider;

/**
 * Object that represents a peer that a local device can send frames to.
 */
typedef struct _FrameDeciderPeer {
    FrameDecider *d;
    void *user;
    BLog_logfunc logfunc;
    struct _FrameDecider_mac_entry *mac_entries;
    struct _FrameDecider_group_entry *group_entries;
    LinkedList1Node list_node; // node in FrameDecider.peers_list
    LinkedList1 mac_entries_free;
    LinkedList1 mac_entries_used;
    LinkedList1 group_entries_free;
    LinkedList1 group_entries_used;
    FDGroupsTree groups_tree;
    DebugObject d_obj;
} FrameDeciderPeer;

/**
 * Initializes the object.
 * 
 * @param o the object
 * @param max_peer_macs maximum number of MAC addresses a peer may posess. Must be >0.
 * @param max_peer_groups maximum number of multicast groups a peer may belong to. Must be >0.
 * @param igmp_group_membership_interval IGMP Group Membership Interval value. When a join
 *        is detected for a peer in {@link FrameDeciderPeer_Analyze}, this is how long we wait
 *        for another join before we remove the group from the peer. Note that the group may
 *        be removed sooner if the peer fails to respond to a Group-Specific Query (see below).
 * @param igmp_last_member_query_time IGMP Last Member Query Time value. When a Group-Specific
 *        Query is detected in {@link FrameDecider_AnalyzeAndDecide}, this is how long we wait for a peer
 *        belonging to the group to send a join before we remove the group from it.
 */
void FrameDecider_Init (FrameDecider *o, int max_peer_macs, int max_peer_groups, btime_t igmp_group_membership_interval, btime_t igmp_last_member_query_time, BReactor *reactor);

/**
 * Frees the object.
 * There must be no {@link FrameDeciderPeer} objects using this decider.
 * 
 * @param o the object
 */
void FrameDecider_Free (FrameDecider *o);

/**
 * Analyzes a frame read from the local device and starts deciding which peers
 * the frame should be forwarded to.
 * 
 * @param o the object
 * @param frame frame data
 * @param frame_len frame length. Must be >=0.
 */
void FrameDecider_AnalyzeAndDecide (FrameDecider *o, const uint8_t *frame, int frame_len);

/**
 * Returns the next peer that the frame submitted to {@link FrameDecider_AnalyzeAndDecide} should be
 * forwarded to.
 * 
 * @param o the object
 * @return peer to forward the frame to, or NULL if no more
 */
FrameDeciderPeer * FrameDecider_NextDestination (FrameDecider *o);

/**
 * Initializes the object.
 * 
 * @param o the object
 * @param d decider this peer will belong to
 * @param user argument to log function
 * @param logfunc function which prepends the log prefix using {@link BLog_Append}
 * @return 1 on success, 0 on failure
 */
int FrameDeciderPeer_Init (FrameDeciderPeer *o, FrameDecider *d, void *user, BLog_logfunc logfunc) WARN_UNUSED;

/**
 * Frees the object.
 * 
 * @param o the object
 */
void FrameDeciderPeer_Free (FrameDeciderPeer *o);

/**
 * Analyzes a frame received from the peer.
 * 
 * @param o the object
 * @param frame frame data
 * @param frame_len frame length. Must be >=0.
 */
void FrameDeciderPeer_Analyze (FrameDeciderPeer *o, const uint8_t *frame, int frame_len);

#endif
