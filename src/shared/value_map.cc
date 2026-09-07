#include "shared/value_map.h"

#include "shared/json_writer.h"

#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <set>

namespace bench
{
    namespace
    {
        std::string_view SectionOf(std::string_view key)
        {
            const size_t dot = key.find('.');
            return dot == std::string_view::npos ? std::string_view{} : key.substr(0, dot);
        }

        std::string_view LeafOf(std::string_view key)
        {
            const size_t dot = key.find('.');
            return dot == std::string_view::npos ? key : key.substr(dot + 1);
        }

        const char *TypeName(ValueMap::Type type)
        {
            switch (type)
            {
            case ValueMap::Type::kBool:
                return "bool";
            case ValueMap::Type::kInt:
                return "int";
            case ValueMap::Type::kUInt:
                return "uint";
            case ValueMap::Type::kDouble:
                return "double";
            case ValueMap::Type::kString:
                return "string";
            }
            return "?";
        }

        [[noreturn]] void Die(const std::string &message)
        {
            std::cerr << "config: " << message << std::endl;
            std::abort();
        }

        bool ParseBoolText(std::string_view text, bool *out)
        {
            if (text == "1" || text == "true" || text == "yes" || text == "on")
            {
                *out = true;
                return true;
            }
            if (text == "0" || text == "false" || text == "no" || text == "off")
            {
                *out = false;
                return true;
            }
            return false;
        }
    }

    uint64_t Fnv1a(const void *data, size_t length, uint64_t seed)
    {
        const unsigned char *bytes = static_cast<const unsigned char *>(data);
        uint64_t hash = seed;
        for (size_t i = 0; i < length; i++)
        {
            hash ^= bytes[i];
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    ValueMap::Entry *ValueMap::Find(std::string_view key)
    {
        for (Entry &entry : entries_)
        {
            if (entry.key == key)
                return &entry;
        }
        return nullptr;
    }

    const ValueMap::Entry *ValueMap::Find(std::string_view key) const
    {
        return const_cast<ValueMap *>(this)->Find(key);
    }

    bool ValueMap::Has(std::string_view key) const
    {
        return Find(key) != nullptr;
    }

    ValueMap::Entry &ValueMap::Register(std::string_view key, Type type, std::string_view doc, bool fixed)
    {
        Entry *existing = Find(key);
        if (existing != nullptr)
        {
            if (existing->type != type)
                Die("key " + std::string(key) + " re-registered with a different type");
            return *existing;
        }

        entries_.push_back(Entry{});
        Entry &entry = entries_.back();
        entry.key = std::string(key);
        entry.doc = std::string(doc);
        entry.type = type;
        entry.fixed = fixed;
        return entry;
    }

    void ValueMap::AddBool(std::string_view key, bool value, std::string_view doc)
    {
        Register(key, Type::kBool, doc, false).bool_value = value;
    }

    void ValueMap::AddInt(std::string_view key, int64_t value, std::string_view doc)
    {
        Register(key, Type::kInt, doc, false).int_value = value;
    }

    void ValueMap::AddUInt(std::string_view key, uint64_t value, std::string_view doc)
    {
        Register(key, Type::kUInt, doc, false).uint_value = value;
    }

    void ValueMap::AddDouble(std::string_view key, double value, std::string_view doc)
    {
        Register(key, Type::kDouble, doc, false).double_value = value;
    }

    void ValueMap::AddString(std::string_view key, std::string_view value, std::string_view doc)
    {
        Register(key, Type::kString, doc, false).string_value = std::string(value);
    }

    void ValueMap::AddFixedUInt(std::string_view key, uint64_t value, std::string_view doc)
    {
        Register(key, Type::kUInt, doc, true).uint_value = value;
    }

    void ValueMap::AddFixedString(std::string_view key, std::string_view value, std::string_view doc)
    {
        Register(key, Type::kString, doc, true).string_value = std::string(value);
    }

    bool ValueMap::GetBool(std::string_view key) const
    {
        const Entry *entry = Find(key);
        if (entry == nullptr || entry->type != Type::kBool)
            Die("no bool key " + std::string(key));
        return entry->bool_value;
    }

    int64_t ValueMap::GetInt(std::string_view key) const
    {
        const Entry *entry = Find(key);
        if (entry == nullptr || entry->type != Type::kInt)
            Die("no int key " + std::string(key));
        return entry->int_value;
    }

    uint64_t ValueMap::GetUInt(std::string_view key) const
    {
        const Entry *entry = Find(key);
        if (entry == nullptr || entry->type != Type::kUInt)
            Die("no uint key " + std::string(key));
        return entry->uint_value;
    }

    double ValueMap::GetDouble(std::string_view key) const
    {
        const Entry *entry = Find(key);
        if (entry == nullptr || entry->type != Type::kDouble)
            Die("no double key " + std::string(key));
        return entry->double_value;
    }

    const std::string &ValueMap::GetString(std::string_view key) const
    {
        const Entry *entry = Find(key);
        if (entry == nullptr || entry->type != Type::kString)
            Die("no string key " + std::string(key));
        return entry->string_value;
    }

    std::string ValueMap::ValueToString(const Entry &entry) const
    {
        char buffer[64];
        switch (entry.type)
        {
        case Type::kBool:
            return entry.bool_value ? "true" : "false";
        case Type::kInt:
            std::snprintf(buffer, sizeof(buffer), "%" PRId64, entry.int_value);
            return buffer;
        case Type::kUInt:
            std::snprintf(buffer, sizeof(buffer), "%" PRIu64, entry.uint_value);
            return buffer;
        case Type::kDouble:
            std::snprintf(buffer, sizeof(buffer), "%.12g", entry.double_value);
            return buffer;
        case Type::kString:
            return entry.string_value;
        }
        return {};
    }

    bool ValueMap::ApplyOption(std::string_view token, const char *next, bool *consumed, std::string *error)
    {
        *consumed = false;

        std::string_view body = token;
        std::string_view inline_value;
        bool has_inline = false;

        const size_t equals = body.find('=');
        if (equals != std::string_view::npos)
        {
            inline_value = body.substr(equals + 1);
            body = body.substr(0, equals);
            has_inline = true;
        }

        // --no-<flag> is only meaningful when it names a registered key, so it is resolved after the --key=value split and before the lookup.
        bool negated = false;
        std::string_view key = body;
        if (key.rfind("no-", 0) == 0 && Find(key.substr(3)) != nullptr)
        {
            negated = true;
            key = key.substr(3);
        }

        Entry *entry = Find(key);
        if (entry == nullptr)
            return false; // another map may own it; ParseArgs decides whether that is an error

        if (entry->fixed)
        {
            *error = "--" + std::string(key) + " is fixed at build time and cannot be set";
            return false;
        }

        std::string_view text = inline_value;
        if (!has_inline)
        {
            if (entry->type == Type::kBool)
            {
                entry->bool_value = !negated;
                return true;
            }
            if (next == nullptr)
            {
                *error = "--" + std::string(key) + " needs a value";
                return false;
            }
            text = next;
            *consumed = true;
        }

        if (negated && entry->type != Type::kBool)
        {
            *error = "--no- only applies to flags, and --" + std::string(key) + " is a " + TypeName(entry->type);
            return false;
        }

        const std::string value(text);
        char *end = nullptr;
        errno = 0;
        switch (entry->type)
        {
        case Type::kBool:
        {
            bool parsed = false;
            if (!ParseBoolText(text, &parsed))
            {
                *error = "--" + std::string(key) + ": " + value + " is not a boolean";
                return false;
            }
            entry->bool_value = negated ? !parsed : parsed;
            return true;
        }
        case Type::kInt:
            entry->int_value = std::strtoll(value.c_str(), &end, 10);
            break;
        case Type::kUInt:
            if (!value.empty() && value[0] == '-')
            {
                *error = "--" + std::string(key) + ": " + value + " must not be negative";
                return false;
            }
            entry->uint_value = std::strtoull(value.c_str(), &end, 10);
            break;
        case Type::kDouble:
            entry->double_value = std::strtod(value.c_str(), &end);
            break;
        case Type::kString:
            entry->string_value = value;
            return true;
        }

        if (value.empty() || end == nullptr || *end != '\0' || errno == ERANGE)
        {
            *error = "--" + std::string(key) + ": " + value + " is not a valid " + TypeName(entry->type);
            return false;
        }
        return true;
    }

    void ValueMap::AppendUsage(std::string *out) const
    {
        for (const Entry &entry : entries_)
        {
            std::string line = "  --" + entry.key;
            if (entry.type != Type::kBool)
                line += "=<" + std::string(TypeName(entry.type)) + ">";
            while (line.size() < 46)
                line += ' ';
            line += entry.doc;
            line += entry.fixed ? "  [fixed at build time: " + ValueToString(entry) + "]"
                                : "  [default: " + ValueToString(entry) + "]";
            *out += line;
            *out += '\n';
        }
    }

    void ValueMap::WriteJson(JsonWriter &writer) const
    {
        std::set<std::string> closed_sections;
        std::string open_section;
        bool in_section = false;

        for (const Entry &entry : entries_)
        {
            const std::string section(SectionOf(entry.key));
            if (in_section && section != open_section)
            {
                writer.EndObject();
                closed_sections.insert(open_section);
                in_section = false;
            }
            if (!section.empty() && !in_section)
            {
                if (closed_sections.count(section) != 0)
                    Die("section " + section + " was registered in two non-adjacent runs of keys");
                writer.BeginObject(section);
                open_section = section;
                in_section = true;
            }

            const std::string leaf(LeafOf(entry.key));
            switch (entry.type)
            {
            case Type::kBool:
                writer.Field(leaf, entry.bool_value);
                break;
            case Type::kInt:
                writer.Field(leaf, entry.int_value);
                break;
            case Type::kUInt:
                writer.Field(leaf, entry.uint_value);
                break;
            case Type::kDouble:
                writer.Field(leaf, entry.double_value);
                break;
            case Type::kString:
                writer.Field(leaf, entry.string_value);
                break;
            }
        }

        if (in_section)
            writer.EndObject();
    }

    void ValueMap::WriteFlatFields(JsonWriter &writer, std::string_view prefix) const
    {
        for (const Entry &entry : entries_)
        {
            std::string name(prefix);
            name += entry.key;
            for (char &c : name)
            {
                if (c == '.')
                    c = '_';
            }

            switch (entry.type)
            {
            case Type::kBool:
                writer.Field(name, entry.bool_value);
                break;
            case Type::kInt:
                writer.Field(name, entry.int_value);
                break;
            case Type::kUInt:
                writer.Field(name, entry.uint_value);
                break;
            case Type::kDouble:
                writer.Field(name, entry.double_value);
                break;
            case Type::kString:
                writer.Field(name, entry.string_value);
                break;
            }
        }
    }

    uint64_t ValueMap::Hash() const
    {
        uint64_t hash = Fnv1a("", 0);
        for (const Entry &entry : entries_)
        {
            const std::string pair = entry.key + "=" + ValueToString(entry) + "\n";
            hash = Fnv1a(pair.data(), pair.size(), hash);
        }
        return hash;
    }

    bool HelpRequested(int argc, char **argv)
    {
        for (int i = 1; i < argc; i++)
        {
            if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0)
                return true;
        }
        return false;
    }

    bool ParseArgs(int argc, char **argv, std::initializer_list<ValueMap *> maps, std::string *error)
    {
        for (int i = 1; i < argc; i++)
        {
            const std::string_view token(argv[i]);
            if (token.rfind("--", 0) != 0)
            {
                *error = "unexpected argument " + std::string(token) + " (every setting is a --key=value option)";
                return false;
            }

            const std::string_view option = token.substr(2);
            const char *next = (i + 1 < argc) ? argv[i + 1] : nullptr;

            bool handled = false;
            for (ValueMap *map : maps)
            {
                bool consumed = false;
                std::string local_error;
                if (map->ApplyOption(option, next, &consumed, &local_error))
                {
                    if (consumed)
                        i++;
                    handled = true;
                    break;
                }
                if (!local_error.empty())
                {
                    *error = local_error;
                    return false;
                }
            }

            if (!handled)
            {
                *error = "unknown option " + std::string(token) + " (try --help)";
                return false;
            }
        }
        return true;
    }

    std::string UsageString(std::string_view program, std::initializer_list<const ValueMap *> maps)
    {
        std::string usage = "Usage: " + std::string(program) + " [--key=value ...]\n\n";
        bool first = true;
        for (const ValueMap *map : maps)
        {
            if (!first)
                usage += '\n';
            first = false;
            map->AppendUsage(&usage);
        }
        return usage;
    }
}
