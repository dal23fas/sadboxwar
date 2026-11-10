// Versioned save/load with atomic writes, backup and rollback (master prompt §36, TDD §12).
// Storage is abstracted so the UE module can route through IFileManager; the default uses std::filesystem.
#pragma once
#include "EFWorldCore/WorldState.h"
#include <string>
#include <vector>

namespace ef {

struct SaveSlotMeta {
    std::string slot_name;
    std::string content_id;
    int schema_version = kSaveSchemaVersion;
    Tick world_tick = 0;
    int64_t saved_at_unix = 0;
    std::string build_label = "phase0";
};

class ISaveStorage {
public:
    virtual ~ISaveStorage() = default;
    // Must be atomic: the previous version of `key` must survive if writing fails midway.
    virtual Status writeAtomic(const std::string& key, const std::string& bytes) = 0;
    virtual Status read(const std::string& key, std::string& out) = 0;
    virtual bool exists(const std::string& key) = 0;
};

// Files under rootDir: <slot>.efsave, <slot>.efsave.bak (previous version), <slot>.efsave.tmp (transient).
class FileSaveStorage : public ISaveStorage {
public:
    explicit FileSaveStorage(std::string rootDir);
    Status writeAtomic(const std::string& key, const std::string& bytes) override;
    Status read(const std::string& key, std::string& out) override;
    bool exists(const std::string& key) override;
    std::string pathFor(const std::string& key) const;
private:
    std::string root_;
};

class SaveService {
public:
    explicit SaveService(ISaveStorage& storage) : storage_(storage) {}
    Status save(const WorldState& world, const std::string& slot, const std::string& buildLabel = "phase0");
    // Loads <slot>; if corrupt/unreadable, rolls back to <slot>.bak and reports it in warnings.
    Status load(WorldState& world, const std::string& slot, std::vector<std::string>* warnings = nullptr, SaveSlotMeta* meta = nullptr);
    static Status parseSaveDocument(const std::string& bytes, SaveSlotMeta& meta, Json& snapshot);
    static std::string slotKey(const std::string& slot) { return slot + ".efsave"; }
private:
    ISaveStorage& storage_;
};

} // namespace ef
