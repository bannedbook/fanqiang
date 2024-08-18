package libcore

import (
	"fmt"
	"strings"

	"libcore/stun"
)

type StunResult struct {
	Text    string
	Success bool
}

func StunTest(server string) *StunResult {
	//note: this library doesn't support stun1.l.google.com:19302
	ret := &StunResult{}
	var text string

	// Old NAT Type Test
	client := stun.NewClient()
	client.SetServerAddr(server)
	nat, host, err, fakeFullCone := client.Discover()
	if err != nil {
		text += fmt.Sprintln("Discover Error:", err.Error())
	}

	if fakeFullCone {
		text += fmt.Sprintln("Fake fullcone (no endpoint IP change) detected!!")
	}

	if host != nil {
		text += fmt.Sprintln("NAT Type:", nat)
		text += fmt.Sprintln("External IP Family:", host.Family())
		text += fmt.Sprintln("External IP:", host.IP())
		text += fmt.Sprintln("External Port:", host.Port())
	}

	// New NAT Test

	natBehavior, err := client.BehaviorTest()
	if err != nil {
		text += fmt.Sprintln("BehaviorTest Error:", err.Error())
	}

	if natBehavior != nil {
		text += fmt.Sprintln("Mapping Behavior:", natBehavior.MappingType)
		text += fmt.Sprintln("Filtering Behavior:", natBehavior.FilteringType)
		text += fmt.Sprintln("Normal NAT Type:", natBehavior.NormalType())
	}

	ret.Success = true
	ret.Text = strings.TrimRight(text, "\n")
	return ret
}
