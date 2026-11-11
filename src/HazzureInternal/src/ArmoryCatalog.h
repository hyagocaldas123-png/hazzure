// IDs/names from the installed items_game schema; no remote inventory mutation.
#pragma once
namespace HazzureArmory {
struct CatalogEntry {int id;const char* name;};
inline constexpr CatalogEntry stickersCatalog[]={
#include "catalogs/armory/stickers-000.inc"
#include "catalogs/armory/stickers-001.inc"
#include "catalogs/armory/stickers-002.inc"
#include "catalogs/armory/stickers-003.inc"
#include "catalogs/armory/stickers-004.inc"
#include "catalogs/armory/stickers-005.inc"
#include "catalogs/armory/stickers-006.inc"
#include "catalogs/armory/stickers-007.inc"
#include "catalogs/armory/stickers-008.inc"
#include "catalogs/armory/stickers-009.inc"
#include "catalogs/armory/stickers-010.inc"
#include "catalogs/armory/stickers-011.inc"
#include "catalogs/armory/stickers-012.inc"
#include "catalogs/armory/stickers-013.inc"
#include "catalogs/armory/stickers-014.inc"
#include "catalogs/armory/stickers-015.inc"
#include "catalogs/armory/stickers-016.inc"
#include "catalogs/armory/stickers-017.inc"
#include "catalogs/armory/stickers-018.inc"
#include "catalogs/armory/stickers-019.inc"
#include "catalogs/armory/stickers-020.inc"
#include "catalogs/armory/stickers-021.inc"
#include "catalogs/armory/stickers-022.inc"
#include "catalogs/armory/stickers-023.inc"
};
inline constexpr CatalogEntry charmsCatalog[]={
#include "catalogs/armory/charms-000.inc"
};
}
