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

func (c *Client) sendWithLog(conn net.PacketConn, addr *net.UDPAddr, changeIP bool, changePort bool) (*response, error) {
	c.logger.Debugln("Send To:", addr)
	resp, err := c.sendBindingReq(conn, addr, changeIP, changePort)
	if err != nil {
		return nil, err
	}
	c.logger.Debugln("Received:", resp)
	if resp == nil && changeIP == false && changePort == false {
		return nil, errors.New("NAT blocked.")
	}
	if resp != nil && !addrCompare(resp.serverAddr, addr, changeIP, changePort) {
		return nil, errors.New("Server error: response IP/port")
	}
	return resp, err
}

// Make sure IP and port  have or haven't change
func addrCompare(host *Host, addr *net.UDPAddr, IPChange, portChange bool) bool {
	isIPChange := host.IP() != addr.IP.String()
	isPortChange := host.Port() != uint16(addr.Port)
	return isIPChange == IPChange && isPortChange == portChange
}

func (c *Client) test(conn net.PacketConn, addr *net.UDPAddr) (*response, error) {
	return c.sendWithLog(conn, addr, false, false)
}

func (c *Client) testChangePort(conn net.PacketConn, addr *net.UDPAddr) (*response, error) {
	return c.sendWithLog(conn, addr, false, true)
}

func (c *Client) testChangeBoth(conn net.PacketConn, addr *net.UDPAddr) (*response, error) {
	return c.sendWithLog(conn, addr, true, true)
}

func (c *Client) test1(conn net.PacketConn, addr net.Addr) (*response, error) {
	return c.sendBindingReq(conn, addr, false, false)
}

func (c *Client) test2(conn net.PacketConn, addr net.Addr) (*response, error) {
	return c.sendBindingReq(conn, addr, true, true)
}

func (c *Client) test3(conn net.PacketConn, addr net.Addr) (*response, error) {
	return c.sendBindingReq(conn, addr, false, true)
}
