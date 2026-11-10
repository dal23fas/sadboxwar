// Loads a content package (definitions + initial state) from JSON (Content/Data/*.json).
#pragma once
#include "EFWorldCore/Domain.h"
#include "EFWorldCore/Result.h"
#include <string>

namespace ef {

Status loadContentFromJson(const Json& j, WorldContent& out);
Status loadContentFromString(const std::string& text, WorldContent& out);
Status loadContentFromFile(const std::string& path, WorldContent& out);

} // namespace ef
