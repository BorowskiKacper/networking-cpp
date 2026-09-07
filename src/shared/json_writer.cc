#include "shared/json_writer.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <ostream>

namespace bench
{
    JsonWriter::JsonWriter(std::ostream &out, bool pretty) : out_(out), pretty_(pretty) {}

    JsonWriter::~JsonWriter()
    {
        assert(stack_.empty() && "JsonWriter destroyed with an object or array still open");
    }

    bool JsonWriter::Compact() const
    {
        return !pretty_ || (!stack_.empty() && stack_.back().compact);
    }

    void JsonWriter::Indent()
    {
        if (Compact())
            return;
        NewlineIndent();
    }

    void JsonWriter::NewlineIndent()
    {
        out_ << '\n';
        for (size_t i = 0; i < stack_.size(); i++)
        {
            out_ << "  ";
        }
    }

    void JsonWriter::Separator()
    {
        assert(!stack_.empty() && stack_.back().is_array && "a bare value belongs in an array");
        if (!stack_.back().empty)
            out_ << ',';
        stack_.back().empty = false;
        Indent();
    }

    void JsonWriter::Separator(std::string_view key)
    {
        assert(!stack_.empty() && !stack_.back().is_array && "a keyed value belongs in an object");
        if (!stack_.back().empty)
            out_ << ',';
        stack_.back().empty = false;
        Indent();
        WriteEscaped(key);
        out_ << (Compact() ? ":" : ": ");
    }

    void JsonWriter::WriteEscaped(std::string_view text)
    {
        out_ << '"';
        for (char c : text)
        {
            switch (c)
            {
            case '"':
                out_ << "\\\"";
                break;
            case '\\':
                out_ << "\\\\";
                break;
            case '\n':
                out_ << "\\n";
                break;
            case '\r':
                out_ << "\\r";
                break;
            case '\t':
                out_ << "\\t";
                break;
            default:
                if (static_cast<unsigned char>(c) < 0x20)
                {
                    char escape[7];
                    std::snprintf(escape, sizeof(escape), "\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(c)));
                    out_ << escape;
                }
                else
                {
                    out_ << c;
                }
                break;
            }
        }
        out_ << '"';
    }

    void JsonWriter::WriteSigned(int64_t value)
    {
        out_ << value;
    }

    void JsonWriter::WriteUnsigned(uint64_t value)
    {
        out_ << value;
    }

    void JsonWriter::WriteDouble(double value)
    {
        // JSON has no NaN or Infinity: an undefined metric is recorded as absent rather than as a token no parser will accept.
        if (!std::isfinite(value))
        {
            out_ << "null";
            return;
        }

        // %.12g round-trips everything this project measures while staying readable to someone reading the document by eye.
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%.12g", value);
        out_ << buffer;
    }

    void JsonWriter::BeginObject()
    {
        // The document root takes no key and has no separator to emit.
        if (stack_.empty())
        {
            out_ << '{';
            stack_.push_back({false, !pretty_, true});
            return;
        }
        Separator();
        out_ << '{';
        stack_.push_back({false, Compact(), true});
    }

    void JsonWriter::BeginObject(std::string_view key)
    {
        Separator(key);
        out_ << '{';
        stack_.push_back({false, Compact(), true});
    }

    void JsonWriter::EndObject()
    {
        assert(!stack_.empty() && !stack_.back().is_array);
        // Read before the pop: the closing brace belongs to the frame being closed, not to its parent, so a compact frame inside a pretty one still closes on its own line.
        const bool was_empty = stack_.back().empty;
        const bool was_compact = Compact();
        stack_.pop_back();
        if (!was_empty && !was_compact)
            NewlineIndent();
        out_ << '}';
        if (stack_.empty() && pretty_)
            out_ << '\n';
    }

    void JsonWriter::BeginArray(bool compact)
    {
        Separator();
        out_ << '[';
        stack_.push_back({true, Compact() || compact, true});
    }

    void JsonWriter::BeginArray(std::string_view key, bool compact)
    {
        Separator(key);
        out_ << '[';
        stack_.push_back({true, Compact() || compact, true});
    }

    void JsonWriter::EndArray()
    {
        assert(!stack_.empty() && stack_.back().is_array);
        const bool was_empty = stack_.back().empty;
        const bool was_compact = Compact();
        stack_.pop_back();
        if (!was_empty && !was_compact)
            NewlineIndent();
        out_ << ']';
    }

    void JsonWriter::Field(std::string_view key, const char *value)
    {
        Field(key, std::string_view(value));
    }

    void JsonWriter::Field(std::string_view key, const std::string &value)
    {
        Field(key, std::string_view(value));
    }

    void JsonWriter::Field(std::string_view key, std::string_view value)
    {
        Separator(key);
        WriteEscaped(value);
    }

    void JsonWriter::Field(std::string_view key, bool value)
    {
        Separator(key);
        out_ << (value ? "true" : "false");
    }

    void JsonWriter::Field(std::string_view key, int value) { Separator(key); WriteSigned(value); }
    void JsonWriter::Field(std::string_view key, unsigned value) { Separator(key); WriteUnsigned(value); }
    void JsonWriter::Field(std::string_view key, long value) { Separator(key); WriteSigned(value); }
    void JsonWriter::Field(std::string_view key, unsigned long value) { Separator(key); WriteUnsigned(value); }
    void JsonWriter::Field(std::string_view key, long long value) { Separator(key); WriteSigned(value); }
    void JsonWriter::Field(std::string_view key, unsigned long long value) { Separator(key); WriteUnsigned(value); }
    void JsonWriter::Field(std::string_view key, double value) { Separator(key); WriteDouble(value); }

    void JsonWriter::NullField(std::string_view key)
    {
        Separator(key);
        out_ << "null";
    }

    void JsonWriter::Value(const char *value) { Value(std::string_view(value)); }
    void JsonWriter::Value(const std::string &value) { Value(std::string_view(value)); }

    void JsonWriter::Value(std::string_view value)
    {
        Separator();
        WriteEscaped(value);
    }

    void JsonWriter::Value(bool value)
    {
        Separator();
        out_ << (value ? "true" : "false");
    }

    void JsonWriter::Value(int value) { Separator(); WriteSigned(value); }
    void JsonWriter::Value(unsigned value) { Separator(); WriteUnsigned(value); }
    void JsonWriter::Value(long value) { Separator(); WriteSigned(value); }
    void JsonWriter::Value(unsigned long value) { Separator(); WriteUnsigned(value); }
    void JsonWriter::Value(long long value) { Separator(); WriteSigned(value); }
    void JsonWriter::Value(unsigned long long value) { Separator(); WriteUnsigned(value); }
    void JsonWriter::Value(double value) { Separator(); WriteDouble(value); }

    void JsonWriter::NullValue()
    {
        Separator();
        out_ << "null";
    }
}
