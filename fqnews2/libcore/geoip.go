package libcore

import (
	"github.com/oschwald/maxminddb-golang"
	"github.com/sagernet/sing-box/common/srs"
	C "github.com/sagernet/sing-box/constant"
	"github.com/sagernet/sing-box/option"
	"log"
	"net"
	"os"
	"strings"
)

type Geoip struct {
	geoipReader *maxminddb.Reader
}

func (g *Geoip) OpenGeosite(path string) bool {
	geoipReader, err := maxminddb.Open(path)
	g.geoipReader = geoipReader
	if err != nil {
		log.Println("failed to open geoip file:", err)
		return false
	} else {
		log.Println("loaded geoip database")
	}
	return true
}

func (g *Geoip) ConvertGeoip(countryCode, outputPath string) {
	networks := g.geoipReader.Networks(maxminddb.SkipAliasedNetworks)
	countryMap := make(map[string][]*net.IPNet)
	var (
		ipNet           *net.IPNet
		nextCountryCode string
		err             error
	)
	for networks.Next() {
		ipNet, err = networks.Network(&nextCountryCode)
		if err != nil {
			log.Println("failed to get network:", err)
			return
		}
		countryMap[nextCountryCode] = append(countryMap[nextCountryCode], ipNet)
	}

	ipNets := countryMap[strings.ToLower(countryCode)]

	if len(ipNets) == 0 {
		log.Println("no networks found for country code:", countryCode)
		return
	}

	var headlessRule option.DefaultHeadlessRule
	headlessRule.IPCIDR = make([]string, 0, len(ipNets))
	for _, cidr := range ipNets {
		headlessRule.IPCIDR = append(headlessRule.IPCIDR, cidr.String())
	}
	var plainRuleSet option.PlainRuleSetCompat
	plainRuleSet.Version = C.RuleSetVersion1
	plainRuleSet.Options.Rules = []option.HeadlessRule{
		{
			Type:           C.RuleTypeDefault,
			DefaultOptions: headlessRule,
		},
	}

	outputFile, err := os.Create(outputPath)
	err = srs.Write(outputFile, plainRuleSet.Upgrade())
	if err != nil {
		log.Println("failed to write geosite file:", err)
		return
	}
}

func NewGeoip() *Geoip {
	return new(Geoip)
}
