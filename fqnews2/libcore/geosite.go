package libcore

import (
	"github.com/sagernet/sing-box/common/geosite"
	"github.com/sagernet/sing-box/common/srs"
	C "github.com/sagernet/sing-box/constant"
	"github.com/sagernet/sing-box/option"
	"os"

	"log"
)

type Geosite struct {
	geositeReader *geosite.Reader
}

func (g *Geosite) CheckGeositeCode(path string, code string) bool {
	geositeReader, codes, err := geosite.Open(path)
	g.geositeReader = geositeReader
	if err != nil {
		log.Println("failed to open geosite file:", err)
		return false
	} else {
		log.Println("loaded geosite database: ", len(codes), " codes")
	}
	sourceSet, err := geositeReader.Read(code)
	if err != nil {
		log.Println("failed to read geosite code:", code, err)
		return false
	}
	return len(sourceSet) >= 1
}

// ConvertGeosite need to run CheckGeositeCode first
func (g *Geosite) ConvertGeosite(code string, outputPath string) {

	sourceSet, err := g.geositeReader.Read(code)
	if err != nil {
		log.Println("failed to read geosite code:", code, err)
		return
	}

	var headlessRule option.DefaultHeadlessRule

	defaultRule := geosite.Compile(sourceSet)

	headlessRule.Domain = defaultRule.Domain
	headlessRule.DomainSuffix = defaultRule.DomainSuffix
	headlessRule.DomainKeyword = defaultRule.DomainKeyword
	headlessRule.DomainRegex = defaultRule.DomainRegex
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

func newGeosite() *Geosite {
	return new(Geosite)
}
