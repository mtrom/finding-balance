import configparser
import re
import subprocess
import sys

from collections import defaultdict
from statistics import mean, stdev
from os import path

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

def main(fn):
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
        file = open(f"logs/{fname}/{name}.log", "w")
        for i in range(config[name].getint("trials")):
            output = run_protocol(config, name, print_cmds=(i == 0))
            file.write(output)
            parse_output(output, results[name])
            print(".", end='', flush=True)
        print("")

    # gether statistics
    stats = defaultdict(lambda: defaultdict(dict))
    for name in results:
        for metric in results[name]:
            stats[metric][name]["avg"] = mean(results[name][metric])
            stats[metric][name]["min"] = min(results[name][metric])
            stats[metric][name]["max"] = max(results[name][metric])
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
            report_stat(stats, metric, name, "min", end="")
            report_stat(stats, metric, name, "max", end="")
            report_stat(stats, metric, name, "avg", end="")
            report_stat(stats, metric, name, "dev")

        print("")

def report_stat(stats, metric, name, stat, end="\n"):
    best = min(stats[metric][n][stat] for n in stats[metric])
    diff = stats[metric][name][stat] - best
    if stats[metric][name][stat] == best:
        output = f"{GREEN}{stat}={stats[metric][name][stat]:.3f}{RESET}"
    elif stats[metric][name][stat] == max(stats[metric][n][stat] for n in stats[metric]):
        output = f"{RED}{stat}={stats[metric][name][stat]:.3f} (+{diff:.3f}){RESET}"
    else:
        output = f"{WHITE}{stat}={stats[metric][name][stat]:.3f} (+{diff:.3f}){RESET}"
    return print(f"    {output}".ljust(COLUMN_WIDTH), end=end)

def parse_output(output, results):
    for line in output.split("\n"):
        if not re.search("\(.*\)\s*:\s*\d+(\.\d+)*$", line):
            if "FAILURE" in line: raise Exception("one of the trials failed")
            continue
        # remove color indicators and extra spacing
        line = re.sub(r"\x1b\[0(;..)?m", "", line)
        line = re.sub("[\t\n]", "", line)
        metric, value = line.split(":")
        results[metric.strip()].append(float(value.strip()))
        if metric.strip() not in METRICS: METRICS.append(metric.strip())

def run_protocol(config, name, print_cmds=True):
    subprocess.run(
        "rm -r out/*",
        shell=True,
        stderr=subprocess.DEVNULL,
        cwd=path.dirname(path.realpath(__file__))
    )
    subprocess.run(
        "seq 0 1 $1 | xargs -n 1 mkdir",
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
    sout, _ = server.communicate()
    cout, _ = client.communicate()
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
            "--lwe-sigma", config[name]["lwe_n"],
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

    sout, _ = server.communicate()
    cout, _ = client.communicate()
    output += sout
    output += cout
    if print_cmds:
        print(' '.join(server.args))
        print(' '.join(client.args))

    return output

if __name__ == "__main__":
    main(sys.argv[1])
