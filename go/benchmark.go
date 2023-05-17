package main

import (
    "bufio"
    "fmt"
    "math"
    "net"
    "os"
    "os/exec"
    "strconv"
    "strings"
    "time"
)

const DEFAULT_TRIALS = 10

/**
 *  basic .ini file parser for reading parameter files
 */
type Config struct {
	Sections   []string
	Parameters map[string]map[string]uint64
}

func NewConfig(filename string) (*Config) {
	cfg := &Config{Parameters: make(map[string]map[string]uint64)}
	file, err := os.Open(filename)
	if err != nil { panic(err) }
	defer file.Close()

	section := "default"
	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())
		if len(line) == 0 || line[0] == ';' {
			continue // empty lines or comments
		} else if line[0] == '[' && line[len(line) - 1] == ']' {
            // if the previous section is marked as "skip" then erase it
            if _, ok := cfg.Parameters[section]["skip"]; ok {
                delete(cfg.Parameters, section)
                cfg.Sections =  cfg.Sections[:len(cfg.Sections) - 1]
            }
			section = line[1 : len(line) - 1]
			cfg.Sections = append(cfg.Sections, section)
		} else {
			split := strings.Index(line, "=")
			if split == -1 { continue }
			key := strings.TrimSpace(line[:split])
			val, err := strconv.ParseUint(strings.TrimSpace(line[split+1:]), 10, 64)
			if err != nil {
			    panic(fmt.Sprintf("%s could not be converted to uint64", val));
			}
			if _, ok := cfg.Parameters[section]; !ok {
				cfg.Parameters[section] = make(map[string]uint64)
			}
			cfg.Parameters[section][key] = val
		}
	}
	if err := scanner.Err(); err != nil { panic(err) }
	return cfg
}

func (cfg *Config) Get(section, key string) (uint64) {
	if val, ok := cfg.Parameters[section][key]; ok {
		return val
	} else if val, ok := cfg.Parameters["default"][key]; ok {
		return val
	}
	panic(fmt.Sprintf("key error: %s not defined for %s", key, section))
}

func (cfg *Config) Has(section, key string) (bool) {
	_, sec := cfg.Parameters[section][key]
	_, def := cfg.Parameters["default"][key]
    return sec || def
}

/**
 * print a stat and color if its the best / worst
 */
func PrintStat(name string, stats []float64, i int) {
    min := math.MaxFloat64
    max := float64(0)
    for _, stat := range stats {
        if (stat < min) { min = stat }
        if (stat > max) { max = stat }
    }
    color := ""
    reset := ""
    diff  := "         "
    if stats[i] == min {
        color = GREEN
        reset = RESET
    } else {
        diff = fmt.Sprintf("(+%.3f)", stats[i] - min)
    }
    if stats[i] == max {
        color = RED
        reset = RESET
    }
    fmt.Printf("   %s%s=%.3f %s%s\t", color, name, stats[i], diff, reset)
}

/**
 * run oprf to generate input files for a benchmark
 */
var lastCmd string
func PrepInputs(cfg *Config, section string) {
    server := exec.Command(
        "./bin/oprf",
        "--server",
        "--cuckoo-n",
        strconv.FormatUint(cfg.Get(section, "cuckoo_n"), 10),
        "--cuckoo-pad",
        strconv.FormatUint(cfg.Get(section, "cuckoo_pad"), 10),
        "--cuckoo-hashes",
        strconv.FormatUint(cfg.Get(section, "cuckoo_hashes"), 10),
        "--hashtable-n",
        strconv.FormatUint(cfg.Get(section, "hashtable_n"), 10),
        "--hashtable-pad",
        strconv.FormatUint(cfg.Get(section, "hashtable_pad"), 10),
        "--threads",
        "16",
    )
    client := exec.Command(
        "./bin/oprf",
        "--client",
        "--cuckoo-n",
        strconv.FormatUint(cfg.Get(section, "cuckoo_n"), 10),
        "--cuckoo-pad",
        strconv.FormatUint(cfg.Get(section, "cuckoo_pad"), 10),
        "--cuckoo-hashes",
        strconv.FormatUint(cfg.Get(section, "cuckoo_hashes"), 10),
        "--hashtable-n",
        strconv.FormatUint(cfg.Get(section, "hashtable_n"), 10),
    )

    // if we've already generated these files, no need to again
    if (server.String() == lastCmd) { return; }
    lastCmd = server.String()

    server.Start()
    client.Start()
    fmt.Printf("generating input files ")
    server.Wait()
    client.Wait()
    fmt.Printf("%sDONE%s\n", GREEN, RESET)
}

func SetupConnections() (net.Conn) {
    go func() {
        connection, err := net.Listen(SERVER_TYPE, SERVER_HOST)
        if err != nil { panic(err) }
        defer connection.Close()

        server, err := connection.Accept()
        if err != nil { panic(err) }

        // read whatever we send over the network
        for ; ; {
            read := ReadOverNetwork(server, CHUNK_SIZE)
            if len(read) == 0 { break }
        }
    }()

    // connect to client
    client, err := net.Dial(SERVER_TYPE, SERVER_HOST)
    if err != nil { panic(err) }

    return client
}

/**
 * run a benchmark with config
 */
func RunBenchmark(
    cfg *Config,
    benchmarkName string,
    fn func([][]uint64, *PSIParams, net.Conn),
) {
    averages := make([]float64, len(cfg.Sections))
    minimums := make([]float64, len(cfg.Sections))
    maximums := make([]float64, len(cfg.Sections))
    // stddevs  := make([]float64, len(cfg.Sections))
    for s, section := range cfg.Sections {
        psiParams := PSIParams{
            CuckooN: cfg.Get(section, "cuckoo_n"),
            BucketN: cfg.Get(section, "hashtable_n"),
            BucketSize: cfg.Get(section, "hashtable_pad"),
            BucketsPerCol: cfg.Get(section, "buckets_per_col"),
            Threads: uint(cfg.Get(section, "threads")),
        }
        trials := DEFAULT_TRIALS
        if cfg.Has(section, "trials") { trials = int(cfg.Get(section, "trials")) }

        PrepInputs(cfg, section)
        conn := SetupConnections()

        datasets := ReadServerInputs(int64(psiParams.CuckooN), &psiParams)

        fmt.Printf("%s : %s\t", benchmarkName, section)

        runtimes := make([]float64, trials)
        for i := 0; i < trials; i++ {
            fmt.Printf(".")
            start := time.Now()
            fn(datasets, &psiParams, conn)
            runtimes[i] = time.Since(start).Seconds()
        }
        fmt.Printf("\n")
        conn.Close()

        avg := float64(0)
        min := math.MaxFloat64
        max := float64(0)
        for _, runtime := range runtimes {
            avg += runtime
            if (runtime < min) { min = runtime }
            if (runtime > max) { max = runtime }
        }
        averages[s] = avg / float64(len(runtimes))
        minimums[s] = min
        maximums[s] = max
    }
    fmt.Printf("\n")

    for s, section := range cfg.Sections {
        fmt.Printf(">>> %s <<<\n %s (s)\n", section, benchmarkName)
        PrintStat("avg", averages, s)
        PrintStat("min", averages, s)
        PrintStat("max", averages, s)
        fmt.Printf("\n\n")
    }
}

func main() {

    if len(os.Args) < 2 {
        fmt.Printf("%sno parameter file specified%s\n", RED, RESET)
        os.Exit(1)
    }
    cfg := NewConfig(os.Args[1])

    RunBenchmark(
        cfg,
        "PIR OFFLINE LOCAL",
        func(datasets [][]uint64, psiParams *PSIParams, _ net.Conn) {
            if psiParams.Threads == 1 {
                for _, dataset := range datasets {
                    RunServerOffline(psiParams, dataset)
                }
            } else {
                channels := make([]chan ServerOfflineResult, psiParams.CuckooN)
                for j := range datasets {
                    channels[j] = make(chan ServerOfflineResult)
                    go func(i int) {
                        channels[j] <- RunServerOffline(psiParams, datasets[j])
                    }(j)
                }
                for _, channel := range channels {
                    <-channel
                }
            }
        },
    )

    RunBenchmark(
        cfg,
        "PIR OFFLINE OVER NETWORK",
        func(datasets [][]uint64, psiParams *PSIParams, conn net.Conn) {
            if psiParams.Threads == 1 {
                payloads := make([][]byte, psiParams.CuckooN)
                for i, dataset := range datasets {
                    result := RunServerOffline(psiParams, dataset)
                    payloads[i] = result.payload
                }
                for _, payload := range payloads {
                    WriteOverNetwork(conn, payload)
                }
            } else {
                channels := make([]chan ServerOfflineResult, psiParams.CuckooN)
                for i := range datasets {
                    channels[i] = make(chan ServerOfflineResult)
                    go func(i int) {
                        channels[i] <- RunServerOffline(psiParams, datasets[i])
                    }(i)
                }
                for _, channel := range channels {
                    result := <-channel
                    WriteOverNetwork(conn, result.payload)
                }
            }
        },
    )
}
