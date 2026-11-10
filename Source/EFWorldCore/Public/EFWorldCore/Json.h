// Minimal JSON DOM (ordered objects for deterministic save files).
// No exceptions: parse returns bool; accessors return defaults on type mismatch.
#pragma once
#include <string>
#include <vector>
#include <utility>
#include <cstdint>

namespace ef {

class Json {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };
    using Array = std::vector<Json>;
    using Object = std::vector<std::pair<std::string, Json>>;

    Json() = default;
    Json(bool b) : type_(Type::Bool), bool_(b) {}
    Json(int v) : type_(Type::Number), num_(v) {}
    Json(int64_t v) : type_(Type::Number), num_(static_cast<double>(v)) {}
    Json(uint64_t v) : type_(Type::Number), num_(static_cast<double>(v)) {}
    Json(double v) : type_(Type::Number), num_(v) {}
    Json(const char* s) : type_(Type::String), str_(s) {}
    Json(std::string s) : type_(Type::String), str_(std::move(s)) {}
    Json(Array a) : type_(Type::Array), arr_(std::move(a)) {}
    Json(Object o) : type_(Type::Object), obj_(std::move(o)) {}

    static Json object() { return Json(Object{}); }
    static Json array() { return Json(Array{}); }

    Type type() const { return type_; }
    bool isNull() const { return type_ == Type::Null; }
    bool isBool() const { return type_ == Type::Bool; }
    bool isNumber() const { return type_ == Type::Number; }
    bool isString() const { return type_ == Type::String; }
    bool isArray() const { return type_ == Type::Array; }
    bool isObject() const { return type_ == Type::Object; }

    bool asBool(bool def = false) const { return isBool() ? bool_ : def; }
    double asNumber(double def = 0.0) const { return isNumber() ? num_ : def; }
    int64_t asInt(int64_t def = 0) const { return isNumber() ? static_cast<int64_t>(num_) : def; }
    const std::string& asString() const { static const std::string empty; return isString() ? str_ : empty; }
    const Array& asArray() const { static const Array empty; return isArray() ? arr_ : empty; }
    const Object& asObject() const { static const Object empty; return isObject() ? obj_ : empty; }
    Array& asArrayMut() { if (!isArray()) { *this = array(); } return arr_; }

    // Object access. get() returns a static Null when the key is missing.
    const Json& get(const std::string& key) const;
    bool has(const std::string& key) const;
    Json& set(const std::string& key, Json value);   // insert or replace, keeps order
    Json& push(Json value);                            // array append

    size_t size() const { return isArray() ? arr_.size() : (isObject() ? obj_.size() : 0); }

    // Serialization
    std::string dump(int indent = 0) const;
    static bool parse(const std::string& text, Json& out, std::string* error = nullptr);

private:
    void dumpInto(std::string& out, int indent, int depth) const;
    Type type_ = Type::Null;
    bool bool_ = false;
    double num_ = 0.0;
    std::string str_;
    Array arr_;
    Object obj_;
};

} // namespace ef
