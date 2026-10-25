#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace bench
{
    // Minimal streaming JSON writer: enough for the run-result documents and nothing more. No parsing, no DOM, no dependency -- reading these files is Python's job. Structure mistakes (a bare value where a key belongs, an unbalanced End) are programming errors and abort under assert.
    class JsonWriter
    {
    public:
        // pretty=false emits everything on one line, which is what the index file and --stdout_json want.
        explicit JsonWriter(std::ostream &out, bool pretty = true);
        ~JsonWriter();

        JsonWriter(const JsonWriter &) = delete;
        JsonWriter &operator=(const JsonWriter &) = delete;

        void BeginObject();
        void BeginObject(std::string_view key);
        void EndObject();

        // compact keeps an array on a single line regardless of pretty printing, which is what the sparse histogram buckets want.
        void BeginArray(bool compact = false);
        void BeginArray(std::string_view key, bool compact = false);
        void EndArray();

        void Field(std::string_view key, const char *value);
        void Field(std::string_view key, const std::string &value);
        void Field(std::string_view key, std::string_view value);
        void Field(std::string_view key, bool value);
        void Field(std::string_view key, int value);
        void Field(std::string_view key, unsigned value);
        void Field(std::string_view key, long value);
        void Field(std::string_view key, unsigned long value);
        void Field(std::string_view key, long long value);
        void Field(std::string_view key, unsigned long long value);
        void Field(std::string_view key, double value);
        void NullField(std::string_view key);

        void Value(const char *value);
        void Value(const std::string &value);
        void Value(std::string_view value);
        void Value(bool value);
        void Value(int value);
        void Value(unsigned value);
        void Value(long value);
        void Value(unsigned long value);
        void Value(long long value);
        void Value(unsigned long long value);
        void Value(double value);
        void NullValue();

    private:
        struct Frame
        {
            bool is_array;
            bool compact;
            bool empty;
        };

        // Emits the separator and indentation that must precede the next entry.
        void Separator();
        void Separator(std::string_view key);
        void Indent();
        void NewlineIndent();
        void WriteEscaped(std::string_view text);
        void WriteSigned(int64_t value);
        void WriteUnsigned(uint64_t value);
        void WriteDouble(double value);
        bool Compact() const;

        std::ostream &out_;
        bool pretty_;
        std::vector<Frame> stack_;
    };
}
