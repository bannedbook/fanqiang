package libcore

import (
	"net/netip"

	tun "github.com/sagernet/sing-tun"
	"github.com/sagernet/sing/common/x/list"
)

// wtf

type interfaceMonitor struct {
}

func (i *interfaceMonitor) Start() error {
	return nil
}

func (i *interfaceMonitor) Close() error {
	return nil
}

func (i *interfaceMonitor) DefaultInterfaceName(destination netip.Addr) string {
	return ""
}

func (i *interfaceMonitor) DefaultInterfaceIndex(destination netip.Addr) int {
	return 0
}

func (i *interfaceMonitor) DefaultInterface(destination netip.Addr) (string, int) {
	return "", 0
}

func (i *interfaceMonitor) OverrideAndroidVPN() bool {
	return false
}

func (i *interfaceMonitor) AndroidVPNEnabled() bool {
	return false
}

func (i *interfaceMonitor) RegisterCallback(callback tun.DefaultInterfaceUpdateCallback) *list.Element[tun.DefaultInterfaceUpdateCallback] {
	return nil
}

func (i *interfaceMonitor) UnregisterCallback(element *list.Element[tun.DefaultInterfaceUpdateCallback]) {
}
