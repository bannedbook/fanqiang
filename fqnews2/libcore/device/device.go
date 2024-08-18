package device

import (
	"runtime"
)

func NumUDPWorkers() int {
	numUDPWorkers := 4
	if num := runtime.GOMAXPROCS(0); num > numUDPWorkers {
		numUDPWorkers = num
	}
	return numUDPWorkers
}
