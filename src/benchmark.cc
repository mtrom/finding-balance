#include <algorithm>
#include <fstream>
#include <limits>
#include <numeric>

#include "defines.h"
#include "server.h"
#include "utils.h"

#define DEFAULT_TRIALS 10

namespace unbalanced_psi {

    /**
     * basic .ini file parser for reading parameter files
     */
    struct Config {
        vector<string> sections;
        map<string, map<string, u64>> parameters;

        Config(std::string filename) {
            std::ifstream file(filename);

            string line;
            string section = "default";
            while (std::getline(file, line)) {
                auto split = line.find('=');

                if (line.empty() || line[0] == ';') {
                    continue; // empty lines or comments
                } else if (line[0] == '[' and line[line.length() - 1] == ']') {
                    section = line.substr(1, line.length() - 2);
                    sections.push_back(section);
                } else if (split != std::string::npos) {
                    auto key = line.substr(0, split);
                    parameters[section][key] = std::stoul(line.substr(split + 1));
                }
            }
            file.close();
        }

        u64 get(string section, string key) {
            if (parameters[section].count(key)) {
                return parameters[section][key];
            } else if (parameters["default"].count(key)) {
                return parameters["default"][key];
            } else {
                throw std::runtime_error("key error: " + key + " not defined for " + section);
            }
        }

        bool has(string section, string key) {
            return parameters[section].count(key) or parameters["default"].count(key);
        }

        void print() {
            for (auto const& [name, section] : parameters) {
                std::cout << name << std::endl;
                for (auto const& [key, value] : section) {
                    std::cout << "\t" + key << " = " << value << std::endl;
                }
            }
        }
    };
}


using namespace unbalanced_psi;

void print_stat(string name, vector<float> stats, int i) {
    auto min = *std::min_element(stats.begin(), stats.end());
    auto max = *std::max_element(stats.begin(), stats.end());
    if (stats[i] == min) { std::cout << GREEN; }
    if (stats[i] == max) { std::cout << RED; }
    std::cout << "   " << name << "=" << stats[i];
    if (stats[i] > min) { std::cout << " (+" << stats[i] - min << ")"; }
    std::cout << RESET << std::endl;
}

int main(int argc, char *argv[]) {
    Config config("params/10.ini");


    vector<float> averages(config.sections.size());
    vector<float> minimums(config.sections.size());
    vector<float> maximums(config.sections.size());
    vector<float> stddevs(config.sections.size());
    for (auto i = 0; i < config.sections.size(); i++) {
        auto section = config.sections[i];
        auto dataset = generate_dataset(1 << config.get(section, "server_log"));
        PSIParams params(
            config.get(section, "cuckoo_n"),
            config.get(section, "cuckoo_pad"),
            config.get(section, "cuckoo_hashes"),
            config.get(section, "hashtable_n"),
            config.get(section, "hashtable_pad"),
            int(config.get(section, "threads"))
        );

        std::cout << section << "\t" << std::flush;

        Server server(dataset, params);

        auto trials = config.has(section, "trials") ? config.get(section, "trials") : DEFAULT_TRIALS;
        vector<float> runtimes(trials);
        for (auto i = 0; i < trials; i++) {
            std::cout << "." << std::flush;
            auto start = high_resolution_clock::now();
            server.offline();
            auto stop = high_resolution_clock::now();
            duration<float, std::milli> elapsed = stop - start;
            runtimes[i] = elapsed.count();
        }
        std::cout << std::endl;
        averages[i] = std::reduce(runtimes.begin(), runtimes.end()) / runtimes.size();
        minimums[i] = *std::min_element(runtimes.begin(), runtimes.end());
        maximums[i] = *std::max_element(runtimes.begin(), runtimes.end());
        stddevs[i] = 0;
        for (float time : runtimes) {
            stddevs[i] += (time - averages[i]) * (time - averages[i]);
        }
        stddevs[i] /= (runtimes.size() - 1);
        stddevs[i] = sqrt(stddevs[i]);
    }
    std::cout << std::endl;

    for (auto i = 0; i < config.sections.size(); i++) {
        std::cout << std::fixed << std::setprecision(3);
        std::cout << ">>> " << config.sections[i] << " <<<" << std::endl;
        std::cout << " DDH OFFLINE (ms)\n";
        print_stat("avg", averages, i);
        print_stat("min", minimums, i);
        print_stat("max", maximums, i);
        print_stat("std", stddevs, i);
        std::cout << std::endl;
    }
}
