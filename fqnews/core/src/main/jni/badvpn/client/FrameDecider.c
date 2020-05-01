/**
 * @file FrameDecider.c
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
 */

#include <string.h>
#include <stddef.h>

#include <misc/debug.h>
#include <misc/offset.h>
#include <misc/balloc.h>
#include <misc/ethernet_proto.h>
#include <misc/ipv4_proto.h>
#include <misc/igmp_proto.h>
#include <misc/byteorder.h>
#include <misc/compare.h>
#include <misc/print_macros.h>

#include <client/FrameDecider.h>

#include <generated/blog_channel_FrameDecider.h>

#define DECIDE_STATE_NONE 1
#define DECIDE_STATE_UNICAST 2
#define DECIDE_STATE_FLOOD 3
#define DECIDE_STATE_MULTICAST 4

#define PeerLog(_o, ...) BLog_LogViaFunc((_o)->logfunc, (_o)->user, BLOG_CURRENT_CHANNEL, __VA_ARGS__)

static int compare_macs (const uint8_t *mac1, const uint8_t *mac2)
{
    int c = memcmp(mac1, mac2, 6);
    return B_COMPARE(c, 0);
}

#include "FrameDecider_macs_tree.h"
#include <structure/SAvl_impl.h>

#include "FrameDecider_groups_tree.h"
#include <structure/SAvl_impl.h>

#include "FrameDecider_multicast_tree.h"
#include <structure/SAvl_impl.h>

static void add_mac_to_peer (FrameDeciderPeer *o, uint8_t *mac)
{
    FrameDecider *d = o->d;
    
    // locate entry in tree
    struct _FrameDecider_mac_entry *e_entry = FDMacsTree_LookupExact(&d->macs_tree, 0, mac);
    if (e_entry) {
        if (e_entry->peer == o) {
            // this is our MAC; only move it to the end of the used list
            LinkedList1_Remove(&o->mac_entries_used, &e_entry->list_node);
            LinkedList1_Append(&o->mac_entries_used, &e_entry->list_node);
            return;
        }
        
        // some other peer has that MAC; disassociate it
        FDMacsTree_Remove(&d->macs_tree, 0, e_entry);
        LinkedList1_Remove(&e_entry->peer->mac_entries_used, &e_entry->list_node);
        LinkedList1_Append(&e_entry->peer->mac_entries_free, &e_entry->list_node);
    }
    
    // aquire MAC address entry, if there are no free ones reuse the oldest used one
    LinkedList1Node *list_node;
    struct _FrameDecider_mac_entry *entry;
    if (list_node = LinkedList1_GetFirst(&o->mac_entries_free)) {
        entry = UPPER_OBJECT(list_node, struct _FrameDecider_mac_entry, list_node);
        ASSERT(entry->peer == o)
        
        // remove from free
        LinkedList1_Remove(&o->mac_entries_free, &entry->list_node);
    } else {
        list_node = LinkedList1_GetFirst(&o->mac_entries_used);
        ASSERT(list_node)
        entry = UPPER_OBJECT(list_node, struct _FrameDecider_mac_entry, list_node);
        ASSERT(entry->peer == o)
        
        // remove from used
        FDMacsTree_Remove(&d->macs_tree, 0, entry);
        LinkedList1_Remove(&o->mac_entries_used, &entry->list_node);
    }
    
    PeerLog(o, BLOG_INFO, "adding MAC %02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8"", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // set MAC in entry
    memcpy(entry->mac, mac, sizeof(entry->mac));
    
    // add to used
    LinkedList1_Append(&o->mac_entries_used, &entry->list_node);
    int res = FDMacsTree_Insert(&d->macs_tree, 0, entry, NULL);
    ASSERT_EXECUTE(res)
}

static uint32_t compute_sig_for_group (uint32_t group)
{
    return hton32(ntoh32(group)&0x7FFFFF);
}

static uint32_t compute_sig_for_mac (uint8_t *mac)
{
    uint32_t sig;
    memcpy(&sig, mac + 2, 4);
    sig = hton32(ntoh32(sig)&0x7FFFFF);
    return sig;
}

static void add_to_multicast (FrameDecider *d, struct _FrameDecider_group_entry *group_entry)
{
    // compute sig
    uint32_t sig = compute_sig_for_group(group_entry->group);
    
    struct _FrameDecider_group_entry *master = FDMulticastTree_LookupExact(&d->multicast_tree, 0, sig);
    if (master) {
        // use existing master
        ASSERT(master->is_master)
        
        // set not master
        group_entry->is_master = 0;
        
        // insert to list
        LinkedList3Node_InitAfter(&group_entry->sig_list_node, &master->sig_list_node);
    } else {
        // make this entry master
        
        // set master
        group_entry->is_master = 1;
        
        // set sig
        group_entry->master.sig = sig;
        
        // insert to multicast tree
        int res = FDMulticastTree_Insert(&d->multicast_tree, 0, group_entry, NULL);
        ASSERT_EXECUTE(res)
        
        // init list node
        LinkedList3Node_InitLonely(&group_entry->sig_list_node);
    }
}

static void remove_from_multicast (FrameDecider *d, struct _FrameDecider_group_entry *group_entry)
{
    // compute sig
    uint32_t sig = compute_sig_for_group(group_entry->group);
    
    if (group_entry->is_master) {
        // remove master from multicast tree
        FDMulticastTree_Remove(&d->multicast_tree, 0, group_entry);
        
        if (!LinkedList3Node_IsLonely(&group_entry->sig_list_node)) {
            // at least one more group entry for this sig; make another entry the master
            
            // get an entry
            LinkedList3Node *list_node = LinkedList3Node_NextOrPrev(&group_entry->sig_list_node);
            struct _FrameDecider_group_entry *newmaster = UPPER_OBJECT(list_node, struct _FrameDecider_group_entry, sig_list_node);
            ASSERT(!newmaster->is_master)
            
            // set master
            newmaster->is_master = 1;
            
            // set sig
            newmaster->master.sig = sig;
            
            // insert to multicast tree
            int res = FDMulticastTree_Insert(&d->multicast_tree, 0, newmaster, NULL);
            ASSERT_EXECUTE(res)
        }
    }
    
    // free linked list node
    LinkedList3Node_Free(&group_entry->sig_list_node);
}

static void add_group_to_peer (FrameDeciderPeer *o, uint32_t group)
{
    FrameDecider *d = o->d;
    
    struct _FrameDecider_group_entry *group_entry = FDGroupsTree_LookupExact(&o->groups_tree, 0, group);
    if (group_entry) {
        // move to end of used list
        LinkedList1_Remove(&o->group_entries_used, &group_entry->list_node);
        LinkedList1_Append(&o->group_entries_used, &group_entry->list_node);
    } else {
        PeerLog(o, BLOG_INFO, "joined group %"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8"",
            ((uint8_t *)&group)[0], ((uint8_t *)&group)[1], ((uint8_t *)&group)[2], ((uint8_t *)&group)[3]
        );
        
        // aquire group entry, if there are no free ones reuse the earliest used one
        LinkedList1Node *node;
        if (node = LinkedList1_GetFirst(&o->group_entries_free)) {
            group_entry = UPPER_OBJECT(node, struct _FrameDecider_group_entry, list_node);
            
            // remove from free list
            LinkedList1_Remove(&o->group_entries_free, &group_entry->list_node);
        } else {
            node = LinkedList1_GetFirst(&o->group_entries_used);
            ASSERT(node)
            group_entry = UPPER_OBJECT(node, struct _FrameDecider_group_entry, list_node);
            
            // remove from multicast
            remove_from_multicast(d, group_entry);
            
            // remove from peer's groups tree
            FDGroupsTree_Remove(&o->groups_tree, 0, group_entry);
            
            // remove from used list
            LinkedList1_Remove(&o->group_entries_used, &group_entry->list_node);
        }
        
        // add entry to used list
        LinkedList1_Append(&o->group_entries_used, &group_entry->list_node);
        
        // set group address
        group_entry->group = group;
        
        // insert to peer's groups tree
        int res = FDGroupsTree_Insert(&o->groups_tree, 0, group_entry, NULL);
        ASSERT_EXECUTE(res)
        
        // add to multicast
        add_to_multicast(d, group_entry);
    }
    
    // set timer
    group_entry->timer_endtime = btime_gettime() + d->igmp_group_membership_interval;
    BReactor_SetTimerAbsolute(d->reactor, &group_entry->timer, group_entry->timer_endtime);
}

static void remove_group_entry (struct _FrameDecider_group_entry *group_entry)
{
    FrameDeciderPeer *peer = group_entry->peer;
    FrameDecider *d = peer->d;
    
    uint32_t group = group_entry->group;
    
    PeerLog(peer, BLOG_INFO, "left group %"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8"",
        ((uint8_t *)&group)[0], ((uint8_t *)&group)[1], ((uint8_t *)&group)[2], ((uint8_t *)&group)[3]
    );
    
    // remove from multicast
    remove_from_multicast(d, group_entry);
    
    // remove from peer's groups tree
    FDGroupsTree_Remove(&peer->groups_tree, 0, group_entry);
    
    // remove from used list
    LinkedList1_Remove(&peer->group_entries_used, &group_entry->list_node);
    
    // add to free list
    LinkedList1_Append(&peer->group_entries_free, &group_entry->list_node);
    
    // stop timer
    BReactor_RemoveTimer(d->reactor, &group_entry->timer);
}

static void lower_group_timers_to_lmqt (FrameDecider *d, uint32_t group)
{
    // have to lower all the group timers of this group down to LMQT
    
    // compute sig
    uint32_t sig = compute_sig_for_group(group);
    
    // look up the sig in multicast tree
    struct _FrameDecider_group_entry *master = FDMulticastTree_LookupExact(&d->multicast_tree, 0, sig);
    if (!master) {
        return;
    }
    ASSERT(master->is_master)
    
    // iterate all group entries with this sig
    LinkedList3Iterator it;
    LinkedList3Iterator_Init(&it, LinkedList3Node_First(&master->sig_list_node), 1);
    LinkedList3Node *sig_list_node;
    while (sig_list_node = LinkedList3Iterator_Next(&it)) {
        struct _FrameDecider_group_entry *group_entry = UPPER_OBJECT(sig_list_node, struct _FrameDecider_group_entry, sig_list_node);
        
        // skip wrong groups
        if (group_entry->group != group) {
            continue;
        }
        
        // lower timer down to LMQT
        btime_t now = btime_gettime();
        if (group_entry->timer_endtime > now + d->igmp_last_member_query_time) {
            group_entry->timer_endtime = now + d->igmp_last_member_query_time;
            BReactor_SetTimerAbsolute(d->reactor, &group_entry->timer, group_entry->timer_endtime);
        }
    }
}

static void group_entry_timer_handler (struct _FrameDecider_group_entry *group_entry)
{
    DebugObject_Access(&group_entry->peer->d_obj);
    
    remove_group_entry(group_entry);
}

void FrameDecider_Init (FrameDecider *o, int max_peer_macs, int max_peer_groups, btime_t igmp_group_membership_interval, btime_t igmp_last_member_query_time, BReactor *reactor)
{
    ASSERT(max_peer_macs > 0)
    ASSERT(max_peer_groups > 0)
    
    // init arguments
    o->max_peer_macs = max_peer_macs;
    o->max_peer_groups = max_peer_groups;
    o->igmp_group_membership_interval = igmp_group_membership_interval;
    o->igmp_last_member_query_time = igmp_last_member_query_time;
    o->reactor = reactor;
    
    // init peers list
    LinkedList1_Init(&o->peers_list);
    
    // init MAC tree
    FDMacsTree_Init(&o->macs_tree);
    
    // init multicast tree
    FDMulticastTree_Init(&o->multicast_tree);
    
    // init decide state
    o->decide_state = DECIDE_STATE_NONE;
    
    // set no current flood peer
    o->decide_flood_current = NULL;
    
    DebugObject_Init(&o->d_obj);
}

void FrameDecider_Free (FrameDecider *o)
{
    ASSERT(FDMulticastTree_IsEmpty(&o->multicast_tree))
    ASSERT(FDMacsTree_IsEmpty(&o->macs_tree))
    ASSERT(LinkedList1_IsEmpty(&o->peers_list))
    DebugObject_Free(&o->d_obj);
}

void FrameDecider_AnalyzeAndDecide (FrameDecider *o, const uint8_t *frame, int frame_len)
{
    ASSERT(frame_len >= 0)
    DebugObject_Access(&o->d_obj);
    
    // reset decide state
    switch (o->decide_state) {
        case DECIDE_STATE_NONE:
            break;
        case DECIDE_STATE_UNICAST:
            break;
        case DECIDE_STATE_FLOOD:
            break;
        case DECIDE_STATE_MULTICAST:
            LinkedList3Iterator_Free(&o->decide_multicast_it);
            return;
        default:
            ASSERT(0);
    }
    o->decide_state = DECIDE_STATE_NONE;
    o->decide_flood_current = NULL;
    
    // analyze frame
    
    const uint8_t *pos = frame;
    int len = frame_len;
    
    if (len < sizeof(struct ethernet_header)) {
        return;
    }
    struct ethernet_header eh;
    memcpy(&eh, pos, sizeof(eh));
    pos += sizeof(struct ethernet_header);
    len -= sizeof(struct ethernet_header);
    
    int is_igmp = 0;
    
    switch (ntoh16(eh.type)) {
        case ETHERTYPE_IPV4: {
            // check IPv4 header
            struct ipv4_header ipv4_header;
            if (!ipv4_check((uint8_t *)pos, len, &ipv4_header, (uint8_t **)&pos, &len)) {
                BLog(BLOG_INFO, "decide: wrong IP packet");
                goto out;
            }
            
            // check if it's IGMP
            if (ntoh8(ipv4_header.protocol) != IPV4_PROTOCOL_IGMP) {
                goto out;
            }
            
            // remember that it's IGMP; we have to flood IGMP frames
            is_igmp = 1;
            
            // check IGMP header
            if (len < sizeof(struct igmp_base)) {
                BLog(BLOG_INFO, "decide: IGMP: short packet");
                goto out;
            }
            struct igmp_base igmp_base;
            memcpy(&igmp_base, pos, sizeof(igmp_base));
            pos += sizeof(struct igmp_base);
            len -= sizeof(struct igmp_base);
            
            switch (ntoh8(igmp_base.type)) {
                case IGMP_TYPE_MEMBERSHIP_QUERY: {
                    if (len == sizeof(struct igmp_v2_extra) && ntoh8(igmp_base.max_resp_code) != 0) {
                        // V2 query
                        struct igmp_v2_extra query;
                        memcpy(&query, pos, sizeof(query));
                        pos += sizeof(struct igmp_v2_extra);
                        len -= sizeof(struct igmp_v2_extra);
                        
                        if (ntoh32(query.group) != 0) {
                            // got a Group-Specific Query, lower group timers to LMQT
                            lower_group_timers_to_lmqt(o, query.group);
                        }
                    }
                    else if (len >= sizeof(struct igmp_v3_query_extra)) {
                        // V3 query
                        struct igmp_v3_query_extra query;
                        memcpy(&query, pos, sizeof(query));
                        pos += sizeof(struct igmp_v3_query_extra);
                        len -= sizeof(struct igmp_v3_query_extra);
                        
                        // iterate sources
                        uint16_t num_sources = ntoh16(query.number_of_sources);
                        int i;
                        for (i = 0; i < num_sources; i++) {
                            // check source
                            if (len < sizeof(struct igmp_source)) {
                                BLog(BLOG_NOTICE, "decide: IGMP: short source");
                                goto out;
                            }
                            pos += sizeof(struct igmp_source);
                            len -= sizeof(struct igmp_source);
                        }
                        
                        if (ntoh32(query.group) != 0 && num_sources == 0) {
                            // got a Group-Specific Query, lower group timers to LMQT
                            lower_group_timers_to_lmqt(o, query.group);
                        }
                    }
                } break;
            }
        } break;
    }
    
out:;
    
    const uint8_t broadcast_mac[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    const uint8_t multicast_mac_header[] = {0x01, 0x00, 0x5e};
    
    // if it's broadcast or IGMP, flood it
    if (is_igmp || !memcmp(eh.dest, broadcast_mac, sizeof(broadcast_mac))) {
        o->decide_state = DECIDE_STATE_FLOOD;
        o->decide_flood_current = LinkedList1_GetFirst(&o->peers_list);
        return;
    }
    
    // if it's multicast, forward to all peers with the given sig
    if (!memcmp(eh.dest, multicast_mac_header, sizeof(multicast_mac_header))) {
        // extract group's sig from destination MAC
        uint32_t sig = compute_sig_for_mac(eh.dest);
        
        // look up the sig in multicast tree
        struct _FrameDecider_group_entry *master = FDMulticastTree_LookupExact(&o->multicast_tree, 0, sig);
        if (master) {
            ASSERT(master->is_master)
            
            o->decide_state = DECIDE_STATE_MULTICAST;
            LinkedList3Iterator_Init(&o->decide_multicast_it, LinkedList3Node_First(&master->sig_list_node), 1);
        }
        
        return;
    }
    
    // look for MAC entry
    struct _FrameDecider_mac_entry *entry = FDMacsTree_LookupExact(&o->macs_tree, 0, eh.dest);
    if (entry) {
        o->decide_state = DECIDE_STATE_UNICAST;
        o->decide_unicast_peer = entry->peer;
        return;
    }
    
    // unknown destination MAC, flood
    o->decide_state = DECIDE_STATE_FLOOD;
    o->decide_flood_current = LinkedList1_GetFirst(&o->peers_list);
    return;
}

FrameDeciderPeer * FrameDecider_NextDestination (FrameDecider *o)
{
    DebugObject_Access(&o->d_obj);
    
    switch (o->decide_state) {
        case DECIDE_STATE_NONE: {
            return NULL;
        } break;
            
        case DECIDE_STATE_UNICAST: {
            o->decide_state = DECIDE_STATE_NONE;
            
            return o->decide_unicast_peer;
        } break;
        
        case DECIDE_STATE_FLOOD: {
            if (!o->decide_flood_current) {
                o->decide_state = DECIDE_STATE_NONE;
                return NULL;
            }
            
            LinkedList1Node *list_node = o->decide_flood_current;
            o->decide_flood_current = LinkedList1Node_Next(o->decide_flood_current);
            
            FrameDeciderPeer *peer = UPPER_OBJECT(list_node, FrameDeciderPeer, list_node);
            
            return peer;
        } break;
        
        case DECIDE_STATE_MULTICAST: {
            LinkedList3Node *list_node = LinkedList3Iterator_Next(&o->decide_multicast_it);
            if (!list_node) {
                o->decide_state = DECIDE_STATE_NONE;
                return NULL;
            }
            struct _FrameDecider_group_entry *group_entry = UPPER_OBJECT(list_node, struct _FrameDecider_group_entry, sig_list_node);
            
            return group_entry->peer;
        } break;
        
        default:
            ASSERT(0);
            return NULL;
    }
}

int FrameDeciderPeer_Init (FrameDeciderPeer *o, FrameDecider *d, void *user, BLog_logfunc logfunc)
{
    // init arguments
    o->d = d;
    o->user = user;
    o->logfunc = logfunc;
    
    // allocate MAC entries
    if (!(o->mac_entries = (struct _FrameDecider_mac_entry *)BAllocArray(d->max_peer_macs, sizeof(struct _FrameDecider_mac_entry)))) {
        PeerLog(o, BLOG_ERROR, "failed to allocate MAC entries");
        goto fail0;
    }
    
    // allocate group entries
    if (!(o->group_entries = (struct _FrameDecider_group_entry *)BAllocArray(d->max_peer_groups, sizeof(struct _FrameDecider_group_entry)))) {
        PeerLog(o, BLOG_ERROR, "failed to allocate group entries");
        goto fail1;
    }
    
    // insert to peers list
    LinkedList1_Append(&d->peers_list, &o->list_node);
    
    // init MAC entry lists
    LinkedList1_Init(&o->mac_entries_free);
    LinkedList1_Init(&o->mac_entries_used);
    
    // initialize MAC entries
    for (int i = 0; i < d->max_peer_macs; i++) {
        struct _FrameDecider_mac_entry *entry = &o->mac_entries[i];
        
        // set peer
        entry->peer = o;
        
        // insert to free list
        LinkedList1_Append(&o->mac_entries_free, &entry->list_node);
    }
    
    // init group entry lists
    LinkedList1_Init(&o->group_entries_free);
    LinkedList1_Init(&o->group_entries_used);
    
    // initialize group entries
    for (int i = 0; i < d->max_peer_groups; i++) {
        struct _FrameDecider_group_entry *entry = &o->group_entries[i];
        
        // set peer
        entry->peer = o;
        
        // insert to free list
        LinkedList1_Append(&o->group_entries_free, &entry->list_node);
        
        // init timer
        BTimer_Init(&entry->timer, 0, (BTimer_handler)group_entry_timer_handler, entry);
    }
    
    // initialize groups tree
    FDGroupsTree_Init(&o->groups_tree);
    
    DebugObject_Init(&o->d_obj);
    
    return 1;
    
fail1:
    BFree(o->mac_entries);
fail0:
    return 0;
}

void FrameDeciderPeer_Free (FrameDeciderPeer *o)
{
    DebugObject_Free(&o->d_obj);
    
    FrameDecider *d = o->d;
    
    // remove decide unicast reference
    if (d->decide_state == DECIDE_STATE_UNICAST && d->decide_unicast_peer == o) {
        d->decide_state = DECIDE_STATE_NONE;
    }
    
    LinkedList1Node *node;
    
    // free group entries
    for (node = LinkedList1_GetFirst(&o->group_entries_used); node; node = LinkedList1Node_Next(node)) {
        struct _FrameDecider_group_entry *entry = UPPER_OBJECT(node, struct _FrameDecider_group_entry, list_node);
        
        // remove from multicast
        remove_from_multicast(d, entry);
        
        // stop timer
        BReactor_RemoveTimer(d->reactor, &entry->timer);
    }
    
    // remove used MAC entries from tree
    for (node = LinkedList1_GetFirst(&o->mac_entries_used); node; node = LinkedList1Node_Next(node)) {
        struct _FrameDecider_mac_entry *entry = UPPER_OBJECT(node, struct _FrameDecider_mac_entry, list_node);
        
        // remove from tree
        FDMacsTree_Remove(&d->macs_tree, 0, entry);
    }
    
    // remove from peers list
    if (d->decide_flood_current == &o->list_node) {
        d->decide_flood_current = LinkedList1Node_Next(d->decide_flood_current);
    }
    LinkedList1_Remove(&d->peers_list, &o->list_node);
    
    // free group entries
    BFree(o->group_entries);
    
    // free MAC entries
    BFree(o->mac_entries);
}

void FrameDeciderPeer_Analyze (FrameDeciderPeer *o, const uint8_t *frame, int frame_len)
{
    ASSERT(frame_len >= 0)
    DebugObject_Access(&o->d_obj);
    
    const uint8_t *pos = frame;
    int len = frame_len;
    
    if (len < sizeof(struct ethernet_header)) {
        goto out;
    }
    struct ethernet_header eh;
    memcpy(&eh, pos, sizeof(eh));
    pos += sizeof(struct ethernet_header);
    len -= sizeof(struct ethernet_header);
    
    // register source MAC address with this peer
    add_mac_to_peer(o, eh.source);
    
    switch (ntoh16(eh.type)) {
        case ETHERTYPE_IPV4: {
            // check IPv4 header
            struct ipv4_header ipv4_header;
            if (!ipv4_check((uint8_t *)pos, len, &ipv4_header, (uint8_t **)&pos, &len)) {
                PeerLog(o, BLOG_INFO, "analyze: wrong IP packet");
                goto out;
            }
            
            // check if it's IGMP
            if (ntoh8(ipv4_header.protocol) != IPV4_PROTOCOL_IGMP) {
                goto out;
            }
            
            // check IGMP header
            if (len < sizeof(struct igmp_base)) {
                PeerLog(o, BLOG_INFO, "analyze: IGMP: short packet");
                goto out;
            }
            struct igmp_base igmp_base;
            memcpy(&igmp_base, pos, sizeof(igmp_base));
            pos += sizeof(struct igmp_base);
            len -= sizeof(struct igmp_base);
            
            switch (ntoh8(igmp_base.type)) {
                case IGMP_TYPE_V2_MEMBERSHIP_REPORT: {
                    // check extra
                    if (len < sizeof(struct igmp_v2_extra)) {
                        PeerLog(o, BLOG_INFO, "analyze: IGMP: short v2 report");
                        goto out;
                    }
                    struct igmp_v2_extra report;
                    memcpy(&report, pos, sizeof(report));
                    pos += sizeof(struct igmp_v2_extra);
                    len -= sizeof(struct igmp_v2_extra);
                    
                    // add to group
                    add_group_to_peer(o, report.group);
                } break;
                
                case IGMP_TYPE_V3_MEMBERSHIP_REPORT: {
                    // check extra
                    if (len < sizeof(struct igmp_v3_report_extra)) {
                        PeerLog(o, BLOG_INFO, "analyze: IGMP: short v3 report");
                        goto out;
                    }
                    struct igmp_v3_report_extra report;
                    memcpy(&report, pos, sizeof(report));
                    pos += sizeof(struct igmp_v3_report_extra);
                    len -= sizeof(struct igmp_v3_report_extra);
                    
                    // iterate records
                    uint16_t num_records = ntoh16(report.number_of_group_records);
                    for (int i = 0; i < num_records; i++) {
                        // check record
                        if (len < sizeof(struct igmp_v3_report_record)) {
                            PeerLog(o, BLOG_INFO, "analyze: IGMP: short record header");
                            goto out;
                        }
                        struct igmp_v3_report_record record;
                        memcpy(&record, pos, sizeof(record));
                        pos += sizeof(struct igmp_v3_report_record);
                        len -= sizeof(struct igmp_v3_report_record);
                        
                        // iterate sources
                        uint16_t num_sources = ntoh16(record.number_of_sources);
                        int j;
                        for (j = 0; j < num_sources; j++) {
                            // check source
                            if (len < sizeof(struct igmp_source)) {
                                PeerLog(o, BLOG_INFO, "analyze: IGMP: short source");
                                goto out;
                            }
                            pos += sizeof(struct igmp_source);
                            len -= sizeof(struct igmp_source);
                        }
                        
                        // check aux data
                        uint16_t aux_len = ntoh16(record.aux_data_len);
                        if (len < aux_len) {
                            PeerLog(o, BLOG_INFO, "analyze: IGMP: short record aux data");
                            goto out;
                        }
                        pos += aux_len;
                        len -= aux_len;
                        
                        switch (record.type) {
                            case IGMP_RECORD_TYPE_MODE_IS_INCLUDE:
                            case IGMP_RECORD_TYPE_CHANGE_TO_INCLUDE_MODE:
                                if (num_sources != 0) {
                                    add_group_to_peer(o, record.group);
                                }
                                break;
                            case IGMP_RECORD_TYPE_MODE_IS_EXCLUDE:
                            case IGMP_RECORD_TYPE_CHANGE_TO_EXCLUDE_MODE:
                                add_group_to_peer(o, record.group);
                                break;
                        }
                    }
                } break;
            }
        } break;
    }
    
out:;
}
