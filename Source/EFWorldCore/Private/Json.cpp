#include "EFWorldCore/Json.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

namespace ef {

const Json& Json::get(const std::string& key) const {
    static const Json null;
    if (!isObject()) return null;
    for (const auto& kv : obj_) if (kv.first == key) return kv.second;
    return null;
}

bool Json::has(const std::string& key) const {
    if (!isObject()) return false;
    for (const auto& kv : obj_) if (kv.first == key) return true;
    return false;
}

Json& Json::set(const std::string& key, Json value) {
    if (!isObject()) { *this = object(); }
    for (auto& kv : obj_) if (kv.first == key) { kv.second = std::move(value); return *this; }
    obj_.emplace_back(key, std::move(value));
    return *this;
}

Json& Json::push(Json value) {
    if (!isArray()) { *this = array(); }
    arr_.push_back(std::move(value));
    return *this;
}

// ---------------- writer ----------------
static void writeString(std::string& out, const std::string& s) {
    out.push_back('"');
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) { char buf[8]; std::snprintf(buf, sizeof buf, "\\u%04x", c); out += buf; }
                else out.push_back(static_cast<char>(c));
        }
    }
    out.push_back('"');
}

static void writeNumber(std::string& out, double v) {
    if (std::isfinite(v) && std::floor(v) == v && std::fabs(v) < 9.0e15) {
        char buf[32]; std::snprintf(buf, sizeof buf, "%lld", static_cast<long long>(v)); out += buf;
    } else if (std::isfinite(v)) {
        char buf[64]; std::snprintf(buf, sizeof buf, "%.17g", v); out += buf;
    } else {
        out += "null";
    }
}

void Json::dumpInto(std::string& out, int indent, int depth) const {
    auto newline = [&](int d) {
        if (indent <= 0) return;
        out.push_back('\n');
        out.append(static_cast<size_t>(d * indent), ' ');
    };
    switch (type_) {
        case Type::Null: out += "null"; break;
        case Type::Bool: out += bool_ ? "true" : "false"; break;
        case Type::Number: writeNumber(out, num_); break;
        case Type::String: writeString(out, str_); break;
        case Type::Array:
            out.push_back('[');
            for (size_t i = 0; i < arr_.size(); ++i) {
                if (i) out.push_back(',');
                newline(depth + 1);
                arr_[i].dumpInto(out, indent, depth + 1);
            }
            if (!arr_.empty()) newline(depth);
            out.push_back(']');
            break;
        case Type::Object:
            out.push_back('{');
            for (size_t i = 0; i < obj_.size(); ++i) {
                if (i) out.push_back(',');
                newline(depth + 1);
                writeString(out, obj_[i].first);
                out += indent > 0 ? ": " : ":";
                obj_[i].second.dumpInto(out, indent, depth + 1);
            }
            if (!obj_.empty()) newline(depth);
            out.push_back('}');
            break;
    }
}

std::string Json::dump(int indent) const {
    std::string out;
    dumpInto(out, indent, 0);
    return out;
}

// ---------------- parser ----------------
namespace {
struct Parser {
    const std::string& s;
    size_t i = 0;
    std::string err;
    int depth = 0;

    explicit Parser(const std::string& text) : s(text) {}

    void skipWs() { while (i < s.size() && (s[i] == ' ' || s[i] == '\n' || s[i] == '\r' || s[i] == '\t')) ++i; }
    bool fail(const char* m) { if (err.empty()) { err = m; err += " at offset "; err += std::to_string(i); } return false; }

    bool parseValue(Json& out) {
        if (++depth > 256) return fail("nesting too deep");
        skipWs();
        if (i >= s.size()) return fail("unexpected end");
        char c = s[i];
        bool ok = false;
        if (c == '{') ok = parseObject(out);
        else if (c == '[') ok = parseArray(out);
        else if (c == '"') { std::string str; ok = parseString(str); if (ok) out = Json(std::move(str)); }
        else if (c == 't' && s.compare(i, 4, "true") == 0) { i += 4; out = Json(true); ok = true; }
        else if (c == 'f' && s.compare(i, 5, "false") == 0) { i += 5; out = Json(false); ok = true; }
        else if (c == 'n' && s.compare(i, 4, "null") == 0) { i += 4; out = Json(); ok = true; }
        else if (c == '-' || (c >= '0' && c <= '9')) ok = parseNumber(out);
        else return fail("unexpected character");
        --depth;
        return ok;
    }

    bool parseNumber(Json& out) {
        size_t start = i;
        if (s[i] == '-') ++i;
        while (i < s.size() && ((s[i] >= '0' && s[i] <= '9') || s[i] == '.' || s[i] == 'e' || s[i] == 'E' || s[i] == '+' || s[i] == '-')) ++i;
        std::string tok = s.substr(start, i - start);
        char* end = nullptr;
        double v = std::strtod(tok.c_str(), &end);
        if (end == tok.c_str() || *end != '\0') return fail("bad number");
        out = Json(v);
        return true;
    }

    static void appendUtf8(std::string& o, unsigned cp) {
        if (cp < 0x80) o.push_back(static_cast<char>(cp));
        else if (cp < 0x800) { o.push_back(static_cast<char>(0xC0 | (cp >> 6))); o.push_back(static_cast<char>(0x80 | (cp & 0x3F))); }
        else if (cp < 0x10000) { o.push_back(static_cast<char>(0xE0 | (cp >> 12))); o.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F))); o.push_back(static_cast<char>(0x80 | (cp & 0x3F))); }
        else { o.push_back(static_cast<char>(0xF0 | (cp >> 18))); o.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F))); o.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F))); o.push_back(static_cast<char>(0x80 | (cp & 0x3F))); }
    }

    bool parseHex4(unsigned& cp) {
        if (i + 4 > s.size()) return fail("bad \\u escape");
        cp = 0;
        for (int k = 0; k < 4; ++k) {
            char c = s[i++]; cp <<= 4;
            if (c >= '0' && c <= '9') cp |= static_cast<unsigned>(c - '0');
            else if (c >= 'a' && c <= 'f') cp |= static_cast<unsigned>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') cp |= static_cast<unsigned>(c - 'A' + 10);
            else return fail("bad hex digit");
        }
        return true;
    }

    bool parseString(std::string& out) {
        ++i; // opening quote
        while (i < s.size()) {
            char c = s[i++];
            if (c == '"') return true;
            if (c == '\\') {
                if (i >= s.size()) return fail("bad escape");
                char e = s[i++];
                switch (e) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    case 'u': {
                        unsigned cp = 0; if (!parseHex4(cp)) return false;
                        if (cp >= 0xD800 && cp <= 0xDBFF && i + 6 <= s.size() && s[i] == '\\' && s[i + 1] == 'u') {
                            i += 2; unsigned lo = 0; if (!parseHex4(lo)) return false;
                            cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                        }
                        appendUtf8(out, cp);
                        break;
                    }
                    default: return fail("bad escape char");
                }
            } else {
                out.push_back(c);
            }
        }
        return fail("unterminated string");
    }

    bool parseArray(Json& out) {
        ++i; out = Json::array();
        skipWs();
        if (i < s.size() && s[i] == ']') { ++i; return true; }
        while (true) {
            Json v; if (!parseValue(v)) return false;
            out.push(std::move(v));
            skipWs();
            if (i >= s.size()) return fail("unterminated array");
            if (s[i] == ',') { ++i; continue; }
            if (s[i] == ']') { ++i; return true; }
            return fail("expected , or ]");
        }
    }

    bool parseObject(Json& out) {
        ++i; out = Json::object();
        skipWs();
        if (i < s.size() && s[i] == '}') { ++i; return true; }
        while (true) {
            skipWs();
            if (i >= s.size() || s[i] != '"') return fail("expected key string");
            std::string key; if (!parseString(key)) return false;
            skipWs();
            if (i >= s.size() || s[i] != ':') return fail("expected :");
            ++i;
            Json v; if (!parseValue(v)) return false;
            out.set(key, std::move(v));
            skipWs();
            if (i >= s.size()) return fail("unterminated object");
            if (s[i] == ',') { ++i; continue; }
            if (s[i] == '}') { ++i; return true; }
            return fail("expected , or }");
        }
    }
};
} // namespace

bool Json::parse(const std::string& text, Json& out, std::string* error) {
    Parser p(text);
    bool ok = p.parseValue(out);
    if (ok) { p.skipWs(); if (p.i != text.size()) { ok = false; p.fail("trailing characters"); } }
    if (!ok) { if (error) *error = p.err; out = Json(); }
    return ok;
}

} // namespace ef
