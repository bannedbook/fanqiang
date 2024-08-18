// Copyright 2016 Cong Ding
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package stun

import (
	"errors"
	"net"
)

// Follow RFC 3489 and RFC 5389.
// Figure 2: Flow for type discovery process (from RFC 3489).
//                        +--------+
//                        |  Test  |
//                        |   I    |
//                        +--------+
//                             |
//                             |
//                             V
//                            /\              /\
//                         N /  \ Y          /  \ Y             +--------+
//          UDP     <-------/Resp\--------->/ IP \------------->|  Test  |
//          Blocked         \ ?  /          \Same/              |   II   |
//                           \  /            \? /               +--------+
//                            \/              \/                    |
//                                             | N                  |
//                                             |                    V
//                                             V                    /\
//                                         +--------+  Sym.      N /  \
//                                         |  Test  |  UDP    <---/Resp\
//                                         |   II   |  Firewall   \ ?  /
//                                         +--------+              \  /
//                                             |                    \/
//                                             V                     |Y
//                  /\                         /\                    |
//   Symmetric  N  /  \       +--------+   N  /  \                   V
//      NAT  <--- / IP \<-----|  Test  |<--- /Resp\               Open
//                \Same/      |   I    |     \ ?  /               Internet
//                 \? /       +--------+      \  /
//                  \/                         \/
//                  |Y                          |Y
//                  |                           |
//                  |                           V
//                  |                           Full
//                  |                           Cone
//                  V              /\
//              +--------+        /  \ Y
//              |  Test  |------>/Resp\---->Restricted
//              |   III  |       \ ?  /
//              +--------+        \  /
//                                 \/
//                                  |N
//                                  |       Port
//                                  +------>Restricted
func (c *Client) discover(conn net.PacketConn, addr *net.UDPAddr) (_ NATType, _ *Host, _ error, fakeFullCone bool) {
	// Perform test1 to check if it is under NAT.
	c.logger.Debugln("Do Test1")
	c.logger.Debugln("Send To:", addr)
	resp, err := c.test1(conn, addr)
	if err != nil {
		return NATError, nil, err, fakeFullCone
	}
	c.logger.Debugln("Received:", resp)
	if resp == nil {
		return NATBlocked, nil, nil, fakeFullCone
	}
	// identical used to check if it is open Internet or not.
	identical := resp.identical
	// changedAddr is used to perform second time test1 and test3.
	changedAddr := resp.changedAddr
	// mappedAddr is used as the return value, its IP is used for tests
	mappedAddr := resp.mappedAddr
	// Make sure IP and port are not changed.
	if resp.serverAddr.IP() != addr.IP.String() ||
		resp.serverAddr.Port() != uint16(addr.Port) {
		fakeFullCone = true
	}
	// if changedAddr is not available, use otherAddr as changedAddr,
	// which is updated in RFC 5780
	if changedAddr == nil {
		changedAddr = resp.otherAddr
	}
	// changedAddr shall not be nil
	if changedAddr == nil {
		return NATError, mappedAddr, errors.New("Server error: no changed address."), fakeFullCone
	}
	// Perform test2 to see if the client can receive packet sent from
	// another IP and port.
	c.logger.Debugln("Do Test2")
	c.logger.Debugln("Send To:", addr)
	resp, err = c.test2(conn, addr)
	if err != nil {
		return NATError, mappedAddr, err, fakeFullCone
	}
	c.logger.Debugln("Received:", resp)
	// Make sure IP and port are changed.
	if resp != nil &&
		(resp.serverAddr.IP() == addr.IP.String() ||
			resp.serverAddr.Port() == uint16(addr.Port)) {
		fakeFullCone = true
	}
	if identical {
		if resp == nil {
			return SymmetricUDPFirewall, mappedAddr, nil, fakeFullCone
		}
		return NATNone, mappedAddr, nil, fakeFullCone
	}
	if resp != nil {
		return NATFull, mappedAddr, nil, fakeFullCone
	}
	// Perform test1 to another IP and port to see if the NAT use the same
	// external IP.
	c.logger.Debugln("Do Test1")
	c.logger.Debugln("Send To:", changedAddr)
	caddr, err := net.ResolveUDPAddr("udp", changedAddr.String())
	if err != nil {
		c.logger.Debugf("ResolveUDPAddr error: %v", err)
	}
	resp, err = c.test1(conn, caddr)
	if err != nil {
		return NATError, mappedAddr, err, fakeFullCone
	}
	c.logger.Debugln("Received:", resp)
	if resp == nil {
		// It should be NAT_BLOCKED, but will be detected in the first
		// step. So this will never happen.
		return NATUnknown, mappedAddr, nil, fakeFullCone
	}
	// Make sure IP/port is not changed.
	if resp.serverAddr.IP() != caddr.IP.String() ||
		resp.serverAddr.Port() != uint16(caddr.Port) {
		fakeFullCone = true
	}
	if mappedAddr.IP() == resp.mappedAddr.IP() && mappedAddr.Port() == resp.mappedAddr.Port() {
		// Perform test3 to see if the client can receive packet sent
		// from another port.
		c.logger.Debugln("Do Test3")
		c.logger.Debugln("Send To:", caddr)
		resp, err = c.test3(conn, caddr)
		if err != nil {
			return NATError, mappedAddr, err, fakeFullCone
		}
		c.logger.Debugln("Received:", resp)
		if resp == nil {
			return NATPortRestricted, mappedAddr, nil, fakeFullCone
		}
		// Make sure IP is not changed, and port is changed.
		if resp.serverAddr.IP() != caddr.IP.String() ||
			resp.serverAddr.Port() == uint16(caddr.Port) {
			fakeFullCone = true
		}
		return NATRestricted, mappedAddr, nil, fakeFullCone
	}
	return NATSymmetric, mappedAddr, nil, fakeFullCone
}

func (c *Client) behaviorTest(conn net.PacketConn, addr *net.UDPAddr) (*NATBehavior, error) {
	natBehavior := &NATBehavior{}

	// Test1   ->(IP1,port1)
	// Perform test to check if it is under NAT.
	c.logger.Debugln("Do Test1")
	resp1, err := c.test(conn, addr)
	if err != nil {
		return nil, err
	}
	// identical used to check if it is open Internet or not.
	if resp1.identical {
		return nil, errors.New("Not behind a NAT.")
	}
	// use otherAddr or changedAddr
	otherAddr := resp1.otherAddr
	if otherAddr == nil {
		if resp1.changedAddr != nil {
			otherAddr = resp1.changedAddr
		} else {
			return nil, errors.New("Server error: no other address and changed address.")
		}
	}

	// Test2   ->(IP2,port1)
	// Perform test to see if mapping to the same IP and port when
	// send to another IP.
	c.logger.Debugln("Do Test2")
	tmpAddr := &net.UDPAddr{IP: net.ParseIP(otherAddr.IP()), Port: addr.Port}
	resp2, err := c.test(conn, tmpAddr)
	if err != nil {
		return nil, err
	}
	if resp2.mappedAddr.IP() == resp1.mappedAddr.IP() &&
		resp2.mappedAddr.Port() == resp1.mappedAddr.Port() {
		natBehavior.MappingType = BehaviorTypeEndpoint
	}

	// Test3   ->(IP2,port2)
	// Perform test to see if mapping to the same IP and port when
	// send to another port.
	if natBehavior.MappingType == BehaviorTypeUnknown {
		c.logger.Debugln("Do Test3")
		tmpAddr.Port = int(otherAddr.Port())
		resp3, err := c.test(conn, tmpAddr)
		if err != nil {
			return nil, err
		}
		if resp3.mappedAddr.IP() == resp2.mappedAddr.IP() &&
			resp3.mappedAddr.Port() == resp2.mappedAddr.Port() {
			natBehavior.MappingType = BehaviorTypeAddr
		} else {
			natBehavior.MappingType = BehaviorTypeAddrAndPort
		}
	}

	// Test4   ->(IP1,port1)   (IP2,port2)->
	// Perform test to see if the client can receive packet sent from
	// another IP and port.
	c.logger.Debugln("Do Test4")
	resp4, err := c.testChangeBoth(conn, addr)
	if err != nil {
		return natBehavior, err
	}
	if resp4 != nil {
		natBehavior.FilteringType = BehaviorTypeEndpoint
	}

	// Test5   ->(IP1,port1)   (IP1,port2)->
	// Perform test to see if the client can receive packet sent from
	// another port.
	if natBehavior.FilteringType == BehaviorTypeUnknown {
		c.logger.Debugln("Do Test5")
		resp5, err := c.testChangePort(conn, addr)
		if err != nil {
			return natBehavior, err
		}
		if resp5 != nil {
			natBehavior.FilteringType = BehaviorTypeAddr
		} else {
			natBehavior.FilteringType = BehaviorTypeAddrAndPort
		}
	}

	return natBehavior, nil
}
