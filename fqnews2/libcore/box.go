package libcore

import (
	"context"
	"errors"
	"fmt"
	"io"
	"libcore/device"
	"log"
	"runtime"
	"runtime/debug"
	"strings"
	"time"

	"github.com/matsuridayo/sing-box-extra/boxbox"
	_ "github.com/matsuridayo/sing-box-extra/distro/all"

	"github.com/matsuridayo/libneko/protect_server"
	"github.com/matsuridayo/libneko/speedtest"
	"github.com/matsuridayo/sing-box-extra/boxapi"

	"github.com/sagernet/sing-box/common/conntrack"
	"github.com/sagernet/sing-box/option"
	"github.com/sagernet/sing-box/outbound"
	"github.com/sagernet/sing/service/pause"
)

var mainInstance *BoxInstance

func VersionBox() string {
	version := []string{
		"sing-box-extra: " + boxbox.Version,
		runtime.Version() + "@" + runtime.GOOS + "/" + runtime.GOARCH,
	}

	var tags string
	debugInfo, loaded := debug.ReadBuildInfo()
	if loaded {
		for _, setting := range debugInfo.Settings {
			switch setting.Key {
			case "-tags":
				tags = setting.Value
			}
		}
	}

	if tags != "" {
		version = append(version, tags)
	}

	return strings.Join(version, "\n")
}

func ResetAllConnections(system bool) {
	if system {
		conntrack.Close()
		log.Println("[Debug] Reset system connections done")
	}
}

type BoxInstance struct {
	*boxbox.Box
	cancel context.CancelFunc
	state  int

	v2api        *boxapi.SbV2rayServer
	selector     *outbound.Selector
	pauseManager pause.Manager

	ForTest bool
}

func NewSingBoxInstance(config string) (b *BoxInstance, err error) {
	defer device.DeferPanicToError("NewSingBoxInstance", func(err_ error) { err = err_ })

	// parse options
	var options option.Options
	err = options.UnmarshalJSON([]byte(config))
	if err != nil {
		return nil, fmt.Errorf("decode config: %v", err)
	}

	// create box
	ctx, cancel := context.WithCancel(context.Background())
	sleepManager := pause.ManagerFromContext(ctx)
	//sleepManager := pause.NewDefaultManager(ctx)
	ctx = pause.ContextWithManager(ctx, sleepManager)
	instance, err := boxbox.New(boxbox.Options{
		Options:           options,
		Context:           ctx,
		PlatformInterface: boxPlatformInterfaceInstance,
	})
	if err != nil {
		cancel()
		return nil, fmt.Errorf("create service: %v", err)
	}

	b = &BoxInstance{
		Box:          instance,
		cancel:       cancel,
		pauseManager: sleepManager,
	}

	// fuck your sing-box platformFormatter
	pf := instance.GetLogPlatformFormatter()
	pf.DisableColors = true
	pf.DisableLineBreak = false

	// selector
	if proxy, ok := b.Router().Outbound("proxy"); ok {
		if selector, ok := proxy.(*outbound.Selector); ok {
			b.selector = selector
		}
	}

	return b, nil
}

func (b *BoxInstance) Start() (err error) {
	defer device.DeferPanicToError("box.Start", func(err_ error) { err = err_ })

	if b.state == 0 {
		b.state = 1
		return b.Box.Start()
	}
	return errors.New("already started")
}

func (b *BoxInstance) Close() (err error) {
	defer device.DeferPanicToError("box.Close", func(err_ error) { err = err_ })

	// no double close
	if b.state == 2 {
		return nil
	}
	b.state = 2

	// clear main instance
	if mainInstance == b {
		mainInstance = nil
		goServeProtect(false)
	}

	// close box
	b.CloseWithTimeout(b.cancel, time.Second*2, log.Println)

	return nil
}

func (b *BoxInstance) Sleep() {
	b.pauseManager.DevicePause()
	_ = b.Box.Router().ResetNetwork()
}

func (b *BoxInstance) Wake() {
	b.pauseManager.DeviceWake()
}

func (b *BoxInstance) SetAsMain() {
	mainInstance = b
	goServeProtect(true)
}

func (b *BoxInstance) SetConnectionPoolEnabled(enable bool) {
	// TODO api
}

func (b *BoxInstance) SetV2rayStats(outbounds string) {
	b.v2api = boxapi.NewSbV2rayServer(option.V2RayStatsServiceOptions{
		Enabled:   true,
		Outbounds: strings.Split(outbounds, "\n"),
	})
	b.Box.Router().SetV2RayServer(b.v2api)
}

func (b *BoxInstance) QueryStats(tag, direct string) int64 {
	if b.v2api == nil {
		return 0
	}
	return b.v2api.QueryStats(fmt.Sprintf("outbound>>>%s>>>traffic>>>%s", tag, direct))
}

func (b *BoxInstance) SelectOutbound(tag string) bool {
	if b.selector != nil {
		return b.selector.SelectOutbound(tag)
	}
	return false
}

func UrlTest(i *BoxInstance, link string, timeout int32) (latency int32, err error) {
	defer device.DeferPanicToError("box.UrlTest", func(err_ error) { err = err_ })
	if i == nil {
		// test current
		return speedtest.UrlTest(boxapi.CreateProxyHttpClient(mainInstance.Box), link, timeout)
	}
	return speedtest.UrlTest(boxapi.CreateProxyHttpClient(i.Box), link, timeout)
}

var protectCloser io.Closer

func goServeProtect(start bool) {
	if protectCloser != nil {
		protectCloser.Close()
		protectCloser = nil
	}
	if start {
		protectCloser = protect_server.ServeProtect("protect_path", false, 0, func(fd int) {
			intfBox.AutoDetectInterfaceControl(int32(fd))
		})
	}
}
