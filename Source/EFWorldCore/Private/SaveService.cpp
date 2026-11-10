#include "EFWorldCore/SaveService.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <system_error>

namespace fs = std::filesystem;

namespace ef {

FileSaveStorage::FileSaveStorage(std::string rootDir) : root_(std::move(rootDir)) {}

std::string FileSaveStorage::pathFor(const std::string& key) const { return (fs::path(root_) / key).string(); }

Status FileSaveStorage::writeAtomic(const std::string& key, const std::string& bytes) {
    std::error_code ec;
    fs::create_directories(root_, ec);
    if (ec) return Status::Fail("save_io", "cannot create save directory: " + ec.message());
    const fs::path final = fs::path(root_) / key;
    const fs::path tmp = fs::path(root_) / (key + ".tmp");
    const fs::path bak = fs::path(root_) / (key + ".bak");
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) return Status::Fail("save_io", "cannot open temp file");
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        out.flush();
        if (!out) return Status::Fail("save_io", "write failed");
    }
    // Verify the temp file re-parses before it replaces anything.
    {
        std::ifstream in(tmp, std::ios::binary); std::stringstream ss; ss << in.rdbuf();
        Json check; if (ss.str() != bytes || !Json::parse(ss.str(), check)) { fs::remove(tmp, ec); return Status::Fail("save_io", "temp file verification failed"); }
    }
    if (fs::exists(final)) {
        fs::remove(bak, ec); ec.clear();
        fs::rename(final, bak, ec);
        if (ec) return Status::Fail("save_io", "cannot rotate backup: " + ec.message());
    }
    fs::rename(tmp, final, ec);
    if (ec) {
        // Restore previous version if the final rename failed.
        std::error_code ec2; if (fs::exists(bak)) fs::rename(bak, final, ec2);
        return Status::Fail("save_io", "cannot finalize save: " + ec.message());
    }
    return Status::Ok();
}

Status FileSaveStorage::read(const std::string& key, std::string& out) {
    std::ifstream in(pathFor(key), std::ios::binary);
    if (!in) return Status::Fail("save_missing", key);
    std::stringstream ss; ss << in.rdbuf(); out = ss.str();
    return Status::Ok();
}

bool FileSaveStorage::exists(const std::string& key) { std::error_code ec; return fs::exists(pathFor(key), ec); }

// ------------------------------------------------------------------
Status SaveService::save(const WorldState& world, const std::string& slot, const std::string& buildLabel) {
    if (slot.empty()) return Status::Fail("save_invalid_slot");
    Json doc = Json::object();
    Json meta = Json::object();
    meta.set("slot_name", slot);
    meta.set("content_id", world.content().content_id);
    meta.set("schema_version", kSaveSchemaVersion);
    meta.set("world_tick", static_cast<int64_t>(world.time().tick));
    meta.set("saved_at_unix", static_cast<int64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));
    meta.set("build_label", buildLabel);
    doc.set("meta", meta);
    doc.set("snapshot", world.toSnapshot());
    return storage_.writeAtomic(slotKey(slot), doc.dump(2));
}

Status SaveService::parseSaveDocument(const std::string& bytes, SaveSlotMeta& meta, Json& snapshot) {
    Json doc; std::string err;
    if (!Json::parse(bytes, doc, &err)) return Status::Fail("save_corrupt", err);
    if (!doc.isObject() || !doc.get("snapshot").isObject()) return Status::Fail("save_corrupt", "missing snapshot");
    const Json& m = doc.get("meta");
    meta.slot_name = m.get("slot_name").asString(); meta.content_id = m.get("content_id").asString();
    meta.schema_version = static_cast<int>(m.get("schema_version").asInt(-1)); meta.world_tick = m.get("world_tick").asInt(0);
    meta.saved_at_unix = m.get("saved_at_unix").asInt(0); meta.build_label = m.get("build_label").asString();
    snapshot = doc.get("snapshot");
    return Status::Ok();
}

Status SaveService::load(WorldState& world, const std::string& slot, std::vector<std::string>* warnings, SaveSlotMeta* metaOut) {
    const std::string key = slotKey(slot);
    std::string bytes; SaveSlotMeta meta; Json snapshot;
    Status rd = storage_.read(key, bytes);
    Status parsed = rd ? parseSaveDocument(bytes, meta, snapshot) : rd;
    if (!parsed) {
        // Rollback path: try the previous version.
        std::string bakBytes;
        Status bakRd = storage_.read(key + ".bak", bakBytes);
        if (!bakRd) return Status::Fail("save_unreadable", "primary: " + parsed.message + "; no backup available");
        Status bakParsed = parseSaveDocument(bakBytes, meta, snapshot);
        if (!bakParsed) return Status::Fail("save_unreadable", "primary and backup both corrupt");
        if (warnings) warnings->push_back("primary save corrupt (" + parsed.message + "); rolled back to backup");
    }
    Status restored = world.restoreSnapshot(snapshot, warnings);
    if (!restored) return restored;
    if (metaOut) *metaOut = meta;
    return Status::Ok();
}

} // namespace ef
