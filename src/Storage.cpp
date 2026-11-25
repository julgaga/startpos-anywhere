#include "Storage.hpp"

#include <Geode/utils/file.hpp>

using namespace geode::prelude;

// Format du fichier :
// {"format":1, "levelVersion":N, "bucketSize":30,
//  "snaps":[[tranche, x, y, %, vitesseY, gamemode, vitesse, drapeaux, portalY], ...]}
// Un tableau par snapshot plutôt qu'un objet : le fichier reste petit.
// v2 : ajout de portalY. Les fichiers v1 sont ignorés (ils n'ont pas le couloir,
// et ceux des tests peuvent contenir des runs enregistrés dans le mauvais mode).
constexpr int FORMAT_VERSION = 2;

enum SnapFlags : int {
    FlagMini = 1 << 0,
    FlagUpsideDown = 1 << 1,
    FlagDual = 1 << 2,
    FlagMirror = 1 << 3,
    FlagGoingLeft = 1 << 4,
    FlagSideways = 1 << 5,
    FlagOnGround = 1 << 6,
};

static int packFlags(Snapshot const& s) {
    return (s.mini ? FlagMini : 0) | (s.upsideDown ? FlagUpsideDown : 0) | (s.dual ? FlagDual : 0)
        | (s.mirror ? FlagMirror : 0) | (s.goingLeft ? FlagGoingLeft : 0)
        | (s.sideways ? FlagSideways : 0) | (s.onGround ? FlagOnGround : 0);
}

static void unpackFlags(Snapshot& s, int flags) {
    s.mini = flags & FlagMini;
    s.upsideDown = flags & FlagUpsideDown;
    s.dual = flags & FlagDual;
    s.mirror = flags & FlagMirror;
    s.goingLeft = flags & FlagGoingLeft;
    s.sideways = flags & FlagSideways;
    s.onGround = flags & FlagOnGround;
}

std::optional<std::string> storageKey(GJGameLevel* level) {
    if (!level) return std::nullopt;
    int id = level->m_levelID.value();
    switch (level->m_levelType) {
        case GJLevelType::Main: return fmt::format("main-{}", id);
        case GJLevelType::Editor: return std::nullopt;
        default: return id > 0 ? std::optional(fmt::format("online-{}", id)) : std::nullopt;
    }
}

static asp::fs::Path pathFor(std::string const& key) {
    return Mod::get()->getSaveDir() / (key + ".json");
}

// Une entrée = [tranche, x, y, %, vitesseY, gamemode, vitesse, drapeaux, portalY].
// Les start pos manuelles utilisent le même format (la tranche y est indicative).
static matjson::Value snapToJson(int bucket, Snapshot const& s) {
    // Arrondi à 2 décimales : largement assez précis, et le fichier reste lisible.
    auto round2 = [](float v) { return std::round(v * 100.0) / 100.0; };
    auto e = matjson::Value::array();
    e.push(bucket);
    e.push(round2(s.x));
    e.push(round2(s.y));
    e.push(round2(s.percent));
    e.push(round2(s.yVelocity));
    e.push(s.gamemode);
    e.push(static_cast<int>(s.speed));
    e.push(packFlags(s));
    e.push(round2(s.portalY));
    return e;
}

static std::optional<Snapshot> snapFromJson(matjson::Value const& e, int& bucket) {
    if (!e.isArray() || e.size() < 9) return std::nullopt;
    Snapshot s;
    bucket = static_cast<int>(e[0].asInt().unwrapOr(0));
    s.x = static_cast<float>(e[1].asDouble().unwrapOr(0.0));
    s.y = static_cast<float>(e[2].asDouble().unwrapOr(0.0));
    s.percent = static_cast<float>(e[3].asDouble().unwrapOr(0.0));
    s.yVelocity = static_cast<float>(e[4].asDouble().unwrapOr(0.0));
    s.gamemode = static_cast<int>(e[5].asInt().unwrapOr(0));
    s.speed = static_cast<Speed>(e[6].asInt().unwrapOr(0));
    unpackFlags(s, static_cast<int>(e[7].asInt().unwrapOr(0)));
    s.portalY = static_cast<float>(e[8].asDouble().unwrapOr(0.0));
    return s;
}

LevelData loadLevelData(GJGameLevel* level) {
    LevelData data;
    auto key = storageKey(level);
    if (!key) return data;

    auto path = pathFor(*key);
    if (!std::filesystem::exists(path.string())) return data;

    auto res = file::readJson(path);
    if (!res) {
        log::warn("Fichier de snapshots illisible ({}), on repart de zero", *key);
        return data;
    }
    auto json = std::move(res).unwrap();

    if (json["format"].asInt().unwrapOr(0) != FORMAT_VERSION) return data;
    if (json["bucketSize"].asDouble().unwrapOr(0.0) != BUCKET_SIZE) return data;
    if (json["levelVersion"].asInt().unwrapOr(-1) != level->m_levelVersion) {
        log::info("Le niveau a ete mis a jour depuis la sauvegarde : snapshots ignores");
        return data;
    }

    int bucket = 0;
    auto const& list = json["snaps"];
    if (list.isArray()) {
        for (std::size_t i = 0; i < list.size(); ++i) {
            if (auto s = snapFromJson(list[i], bucket)) data.snaps[bucket] = *s;
        }
    }
    // "manual" est optionnel : les fichiers écrits avant la v0.6 ne l'ont pas.
    if (json.contains("manual") && json["manual"].isArray()) {
        auto const& manual = json["manual"];
        for (std::size_t i = 0; i < manual.size(); ++i) {
            if (auto s = snapFromJson(manual[i], bucket)) data.manual.push_back(*s);
        }
        std::sort(data.manual.begin(), data.manual.end(), [](auto const& a, auto const& b) { return a.x < b.x; });
    }
    return data;
}

void saveLevelData(GJGameLevel* level, LevelData const& data) {
    auto key = storageKey(level);
    if (!key || (data.snaps.empty() && data.manual.empty())) return;

    auto list = matjson::Value::array();
    for (auto const& [bucket, s] : data.snaps) {
        list.push(snapToJson(bucket, s));
    }
    auto manual = matjson::Value::array();
    for (auto const& s : data.manual) {
        manual.push(snapToJson(static_cast<int>(s.x / BUCKET_SIZE), s));
    }

    auto json = matjson::Value::object();
    json["format"] = FORMAT_VERSION;
    json["levelVersion"] = level->m_levelVersion;
    json["bucketSize"] = BUCKET_SIZE;
    json["snaps"] = std::move(list);
    json["manual"] = std::move(manual);

    auto res = file::writeStringSafe(pathFor(*key), json.dump(matjson::NO_INDENTATION));
    if (!res) {
        log::error("Impossible de sauvegarder les snapshots ({})", *key);
        return;
    }
    log::info(
        "{} snapshots et {} start pos manuelles sauvegardes ({})",
        data.snaps.size(), data.manual.size(), *key
    );
}
