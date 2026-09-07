#pragma once

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

namespace bench
{
    class JsonWriter;

    // An ordered, typed key -> value map used for two things: the configuration a benchmark was asked to run with, and the metrics it produced.
    //
    // Configuration is registered here with its defaults, overridden from the command line, and then serialized verbatim into the run document. Because the same map is the one the benchmark reads its settings out of, the recorded configuration cannot drift from the one that actually ran.
    //
    // Keys may carry a single "section." prefix (e.g. "lob.max_locates"), which becomes a nested object in JSON. Keys of one section must be registered contiguously.
    class ValueMap
    {
    public:
        enum class Type
        {
            kBool,
            kInt,
            kUInt,
            kDouble,
            kString,
        };

        struct Entry
        {
            std::string key;
            std::string doc;
            Type type = Type::kString;
            // Recorded like any other key but rejected on the command line: a fact about the build (a template parameter, say) rather than a knob.
            bool fixed = false;
            bool bool_value = false;
            int64_t int_value = 0;
            uint64_t uint_value = 0;
            double double_value = 0.0;
            std::string string_value;
        };

        // Registers a key, or overwrites the value of one already registered. doc is kept from the first registration.
        void AddBool(std::string_view key, bool value, std::string_view doc = {});
        void AddInt(std::string_view key, int64_t value, std::string_view doc = {});
        void AddUInt(std::string_view key, uint64_t value, std::string_view doc = {});
        void AddDouble(std::string_view key, double value, std::string_view doc = {});
        void AddString(std::string_view key, std::string_view value, std::string_view doc = {});

        // Same, for values that are recorded but not settable from the command line.
        void AddFixedUInt(std::string_view key, uint64_t value, std::string_view doc = {});
        void AddFixedString(std::string_view key, std::string_view value, std::string_view doc = {});

        // A missing key or a type mismatch is a programming error, not a runtime condition: these abort rather than returning a fallback that would silently misconfigure a benchmark run.
        bool GetBool(std::string_view key) const;
        int64_t GetInt(std::string_view key) const;
        uint64_t GetUInt(std::string_view key) const;
        double GetDouble(std::string_view key) const;
        const std::string &GetString(std::string_view key) const;

        bool Has(std::string_view key) const;
        const std::vector<Entry> &entries() const { return entries_; }

        // Applies one "--key=value" / "--key value" / "--flag" / "--no-flag" token. Returns false with error filled if the key is unknown here; consumed is set when the following argument was taken as the value.
        bool ApplyOption(std::string_view token, const char *next, bool *consumed, std::string *error);

        // Renders each key with its type, default and doc string, for --help.
        void AppendUsage(std::string *out) const;

        // Nested one level on '.'.
        void WriteJson(JsonWriter &writer) const;
        // Flat, one field per entry, with '.' replaced by '_' so the result is safe as a column name. Used for the one-line-per-run index.
        void WriteFlatFields(JsonWriter &writer, std::string_view prefix) const;

        // Stable over key order and value formatting; used to build the run id so that runs of the same configuration are recognisable at a glance.
        uint64_t Hash() const;

        std::string ValueToString(const Entry &entry) const;

    private:
        Entry *Find(std::string_view key);
        const Entry *Find(std::string_view key) const;
        Entry &Register(std::string_view key, Type type, std::string_view doc, bool fixed);

        std::vector<Entry> entries_;
    };

    // Parses the whole command line across several maps -- typically the benchmark's configuration and the output plumbing, kept apart so that where a run wrote its results never ends up inside the configuration that identifies it. Returns false with error filled on an unknown option, an unparsable value, an attempt to set a fixed key, or a stray positional argument.
    bool ParseArgs(int argc, char **argv, std::initializer_list<ValueMap *> maps, std::string *error);

    // True if -h or --help appears anywhere on the command line.
    bool HelpRequested(int argc, char **argv);

    std::string UsageString(std::string_view program, std::initializer_list<const ValueMap *> maps);

    // FNV-1a, used for the configuration hash and the input-file fingerprint.
    uint64_t Fnv1a(const void *data, size_t length, uint64_t seed = 1469598103934665603ULL);
}
