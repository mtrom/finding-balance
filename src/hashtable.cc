#include "hashtable.h"
#include "utils.h"

namespace unbalanced_psi {

    Hashtable::Hashtable(u64 buckets) :
        size(0), width(0), table(buckets, vector<u8>()) { }

    u64 Hashtable::hash(const vector<u8>& entry, u64 table_size) {
        return Hashtable::hash(entry.data(), table_size);
    }

    u64 Hashtable::hash(const u8* entry, u64 table_size) {
        // just interpret the first eight bytes as the hash value
        u64 value = *reinterpret_cast<const u64*>(&entry[0]);
        return value % table_size;
    }

    void Hashtable::insert(const vector<u8>& entry) {
        u64 index = hash(entry, table.size());
        table[index].insert(table[index].begin(), entry.begin(), entry.end());
        if (table[index].size() > width) { width = table[index].size(); }
        size++;
    }

    void Hashtable::pad() {
        for (auto i = 0; i < table.size(); i++) {
            table[i].resize(width);
        }
    }

    void Hashtable::to_file(string filename) {
        std::ofstream file(filename, std::ios::out | std::ios::binary);
        if (!file) { throw std::runtime_error("cannot open " + filename); }
        file.write((const char*) &width, sizeof(u64));
        for (auto i = 0; i < table.size(); i++) {
            file.write((const char*) table[i].data(), table[i].size());
        }
        file.close();
    }

    u64 Hashtable::buckets() {
        return table.size();
    }

    void Hashtable::log() {
        for (auto i = 0; i < table.size(); i++) {
            std::clog << "[  hash  ] bucket (" << i << ")\t:";
            std::clog << to_hex(table[i].data(), table[i].size()) << std::endl;
        }
    }
}
