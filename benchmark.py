import configparser
import re
import subprocess
import sys

from collections import defaultdict
from statistics import mean, stdev
from os import path, fsync

RED    = "\033[0;31m"
GREEN  = "\033[0;32m"
BLUE   = "\033[0;34m"
WHITE  = "\033[0;37m"
YELLOW = "\033[0;33m"
RESET  = "\033[0m"

# all metrics collected
METRICS = []

# helps keep metric output in line
COLUMN_WIDTH = 40

SUCCESS = "\033[0;32m>>>>>>>>>>>>>>> SUCCESS <<<<<<<<<<<<<<<\033[0m"

def main(fn, from_logs):
    config = configparser.ConfigParser()
    config.read(fn)

    # filename without .ini
    fname = path.splitext(path.basename(fn))[0]
    subprocess.run(f"mkdir -p logs/{fname}", shell=True, cwd=path.dirname(path.realpath(__file__)))

    # run trials
    results = defaultdict(lambda: defaultdict(list))
    for name in config:
        if name == "DEFAULT": continue
        print(name)
        file = open(f"logs/{fname}/{name}.log", "r" if from_logs else "w")
        if not from_logs:
            for i in range(config[name].getint("trials")):
                output = run_protocol(config, name, print_cmds=(i == 0))

                file.write(output)
                # make sure file is written in case of failures
                file.flush()
                fsync(file.fileno())

                print(".", end='', flush=True)
                parse_output(output, results[name])
        else:
            for output in file.read().split(SUCCESS):
                parse_output(output, results[name])

        print("")

    # gether statistics
    stats = defaultdict(lambda: defaultdict(dict))
    for name in results:
        for metric in results[name]:
            stats[metric][name]["avg"] = mean(results[name][metric])
            stats[metric][name]["min"] = min(results[name][metric])
            stats[metric][name]["max"] = max(results[name][metric])
            if len(results[name][metric]) > 1:
                stats[metric][name]["dev"] = stdev(results[name][metric])

    # report statistics
    for metric in METRICS:
        if "client" in metric:
            print(f"{YELLOW}{metric}{RESET}")
        elif "server" in metric:
            print(f"{BLUE}{metric}{RESET}")
        else:
            print(f"{metric}")

        for name in results:
            print(f"  {name}")
            report_dev = config[name].getint("trials") > 1
            report_stat(stats, metric, name, "avg")
            report_stat(stats, metric, name, "min")
            report_stat(stats, metric, name, "max")
            if report_dev: report_stat(stats, metric, name, "dev")
            print()

        print("")

def report_stat(stats, metric, name, stat):
    # in case some runs have more robust metrics than others
    if stat not in stats[metric][name]:
        return
    best = min(
        stats[metric][n][stat] if stat in stats[metric][n]
        else float('inf')
        for n in stats[metric]
    )
    worst = max(
        stats[metric][n][stat] if stat in stats[metric][n]
        else float('inf')
        for n in stats[metric]
    )
    diff = stats[metric][name][stat] - best
    if stats[metric][name][stat] == best:
        output = f"{GREEN}{stat}={stats[metric][name][stat]:.3f}{RESET}"
    elif stats[metric][name][stat] == worst:
        output = f"{RED}{stat}={stats[metric][name][stat]:.3f} (+{diff:.3f}){RESET}"
    else:
        output = f"{WHITE}{stat}={stats[metric][name][stat]:.3f} (+{diff:.3f}){RESET}"
    return print(f"    {output}".ljust(COLUMN_WIDTH), end="")

def parse_output(output, results):
    metrics = defaultdict(float)
    for line in output.split("\n"):
        line = re.sub(r"\x1b\[0(;..)?m", "", line)
        line = line.strip()
        if not re.search("\(.*\)\s*:\s*\d+(\.\d+)*$", line):
            if "FAILURE" in line: raise Exception("one of the trials failed")
            continue
        # remove color indicators and extra spacing
        line = re.sub("[\t\n]", "", line)
        metric, value = line.split(":")
        metrics[metric.strip()] += float(value.strip())
        if metric.strip() not in METRICS: METRICS.append(metric.strip())

    for metric in metrics:
        results[metric].append(metrics[metric])

def run_protocol(config, name, print_cmds=True):
    subprocess.run(
        "rm -r out/*",
        shell=True,
        stderr=subprocess.DEVNULL,
        cwd=path.dirname(path.realpath(__file__))
    )
    subprocess.run(
        f"seq 0 1 {config[name]['cuckoo_size']} | xargs -n 1 mkdir",
        shell=True,
        cwd=path.join(path.dirname(path.realpath(__file__)), "out")
    )

    output = ""
    datagen = subprocess.run([
        "./bin/datagen",
        "--server-log", config[name]["server_log"],
        "--client", config[name]["client_size"],
        "--overlap", config[name]["overlap"],
    ])

    args = [
        "--cuckoo-size", config[name]["cuckoo_size"],
        "--cuckoo-hashes",config[name]["cuckoo_hashes"],
        "--hashtable-size",config[name]["hashtable_size"],
    ]

    server = subprocess.Popen(
        [
            "./bin/oprf", "--server",
        ] + args + [
            "--threads", config[name]["threads"],
        ], stdout = subprocess.PIPE, text=True
    )
    client = subprocess.Popen(
        [ "./bin/oprf", "--client", ] + args,
        stdout = subprocess.PIPE,
        text=True
    )
    sout, serr = server.communicate()
    cout, cerr = client.communicate()

    if serr or cerr:
        print(serr)
        print(cerr)
        raise Exception("failure when running oprf")

    output += sout
    output += cout
    if print_cmds:
        print(' '.join(server.args))
        print(' '.join(client.args))

    args = [
        "--cuckoo-size", config[name]["cuckoo_size"],
        "--hashtable-size",config[name]["hashtable_size"],
        "--buckets-per-col",config[name]["buckets_per_col"],
    ]

    if "lwe_n" in config[name]:
        args += [
            "--lwe-n", config[name]["lwe_n"],
            "--lwe-sigma", config[name]["lwe_sigma"],
            "--mod", config[name]["mod"],
        ]

    server = subprocess.Popen(
        [
            "./bin/pir", "--server",
        ] + args + [
            "--queries", config[name]["client_size"],
            "--threads", config[name]["threads"],
        ], stdout = subprocess.PIPE, text=True
    )

    client = subprocess.Popen(
        [
            "./bin/pir", "--client",
        ] + args + [
            "--expected", config[name]["overlap"],
        ], stdout = subprocess.PIPE, text=True
    )

    sout, serr = server.communicate()
    cout, cerr = client.communicate()

    if serr or cerr:
        print(serr)
        print(cerr)
        raise Exception("failure when running pir")

    output += sout
    output += cout
    if print_cmds:
        print(' '.join(server.args))
        print(' '.join(client.args))

    return output

if __name__ == "__main__":
    fn = sys.argv[1]
    main(sys.argv[1], len(sys.argv) > 2 and "from-logs" in sys.argv[2])
