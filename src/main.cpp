#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>

#include "Snapshot.hpp"
#include "SpawnPopup.hpp"
#include "Storage.hpp"

using namespace geode::prelude;

// Les snapshots sont rangés par tranche de x (BUCKET_SIZE, dans Snapshot.hpp)
// plutôt que par temps : plusieurs runs complètent la même carte, et le run
// le plus récent écrase l'ancien au même endroit.

// Le snapshot le plus loin est souvent celui où tu es mort : on recule d'une
// marge (réglage, en blocs ; 10 par défaut, environ 1 seconde en vitesse 1x)
// pour ne pas respawn dans l'obstacle. Une tranche = un bloc.
static int respawnMarginBuckets() {
    return static_cast<int>(Mod::get()->getSettingValue<int64_t>("respawn-margin"));
}

// Les messages de suivi (un par seconde, à chaque reset...) ne servent qu'à
// déboguer : ils sont coupés par défaut.
static bool debugLogs() {
    return Mod::get()->getSettingValue<bool>("debug-logs");
}

class $modify(SPAPlayLayer, PlayLayer) {
    struct Fields {
        std::map<int, Snapshot> snaps; // clé = tranche de x
        float logTimer = 0.f;

        Ref<StartPosObject> customStart;   // garde notre start pos en vie
        Ref<StartPosObject> originalStart; // la start pos d'avant (souvent aucune)
        bool hasOriginal = false;
        float lastSpawnPercent = -1.f;     // pour rouvrir le slider au même endroit
        std::optional<Snapshot> activeSpawn; // le snapshot de la start pos active

        std::vector<Snapshot> manual;      // v0.6 : start pos posées à la main, triées par x
        int manualIndex = -1;              // celle qui est active (-1 = aucune)
    };

    bool init(GJGameLevel* level, bool dontCreateObjects) {
        if (!PlayLayer::init(level, dontCreateObjects)) return false;
        if (debugLogs()) {
            log::info("Niveau ouvert : {} (platformer = {})", std::string(level->m_levelName), m_isPlatformer);
        }

        // On recharge la carte des runs précédents et les start pos manuelles.
        if (!m_isPlatformer) {
            auto data = loadLevelData(level);
            m_fields->snaps = std::move(data.snaps);
            m_fields->manual = std::move(data.manual);
            if (!m_fields->snaps.empty() || !m_fields->manual.empty()) {
                log::info(
                    "{} snapshots et {} start pos manuelles recharges",
                    m_fields->snaps.size(), m_fields->manual.size()
                );
            }
        }
        return true;
    }

    void saveIfNeeded() {
        if (m_isPlatformer) return;
        saveLevelData(m_level, LevelData{m_fields->snaps, m_fields->manual});
    }

    void levelComplete() {
        PlayLayer::levelComplete();
        this->saveIfNeeded();
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        // Pas de plateforme pour l'instant, et rien pendant l'animation de mort.
        if (m_isPlatformer || !m_player1 || m_player1->m_isDead) return;

        int bucket = static_cast<int>(m_player1->getPositionX() / BUCKET_SIZE);
        auto& snap = m_fields->snaps[bucket];
        snap = takeSnapshot(this, m_player1);

        // Un résumé par seconde dans la console pour vérifier.
        m_fields->logTimer += dt;
        if (m_fields->logTimer < 1.f) return;
        m_fields->logTimer = 0.f;
        if (!debugLogs()) return;
        log::info(
            "{:.1f}% | {} snapshots | {} {}{}{}{}{}",
            snap.percent,
            m_fields->snaps.size(),
            gamemodeName(snap.gamemode),
            speedName(snap.speed),
            snap.mini ? " mini" : "",
            snap.upsideDown ? " gravite-inversee" : "",
            snap.dual ? " dual" : "",
            snap.mirror ? " mirror" : ""
        );
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        this->applySpawnHeight();
        if (debugLogs()) {
            log::info("Reset : {} snapshots en memoire", m_fields->snaps.size());
        }
    }

    // Dans un mode à couloir, notre start pos est posée au centre du couloir
    // (pour que GD place le sol, le plafond et la caméra au bon endroit) :
    // après chaque reset, on remet le joueur à la hauteur où il était vraiment.
    void applySpawnHeight() {
        auto& spawn = m_fields->activeSpawn;
        if (!spawn || !usesCorridor(spawn->gamemode) || !m_player1) return;
        if (m_startPosObject != static_cast<StartPosObject*>(m_fields->customStart)) return;
        // En practice avec des checkpoints, le reset ramène au checkpoint : on n'y touche pas.
        if (m_isPracticeMode && m_checkpointArray && m_checkpointArray->count() > 0) return;

        CCPoint pos{spawn->x, spawn->y};
        m_player1->m_position = pos;
        m_player1->setPosition(pos);
    }

    void onQuit() {
        auto& snaps = m_fields->snaps;
        if (!snaps.empty() && debugLogs()) {
            log::info(
                "Sortie du niveau : {} snapshots, de {:.1f}% a {:.1f}%",
                snaps.size(),
                snaps.begin()->second.percent,
                snaps.rbegin()->second.percent
            );
        }
        // Avant l'original : après, le PlayLayer est en train d'être détruit.
        this->saveIfNeeded();
        PlayLayer::onQuit();
    }

    // --- v0.3 : le spawn ---------------------------------------------------

    // Crée une start pos à partir d'un snapshot et relance le niveau dessus.
    // Même recette que GJBaseGameLayer::createLiveStartPosObject dans GD.
    void spawnAt(Snapshot const& s) {
        auto obj = static_cast<StartPosObject*>(GameObject::createWithKey(31)); // 31 = start pos
        if (!obj) return;
        obj->firstSetup();
        obj->customSetup();
        // Dans un couloir, GD place sol/plafond/caméra autour du y de la start pos,
        // comme pour un portail : on la pose donc au centre du couloir enregistré.
        bool corridor = usesCorridor(s.gamemode) && s.portalY != 0.f;
        obj->setStartPos({s.x, corridor ? s.portalY : s.y});

        auto set = obj->m_startSettings;
        set->m_startMode = s.gamemode;
        set->m_startSpeed = s.speed;
        set->m_startMini = s.mini;
        set->m_startDual = s.dual;
        set->m_isFlipped = s.upsideDown;
        set->m_mirrorMode = s.mirror;
        set->m_reverseGameplay = s.goingLeft;
        set->m_rotateGameplay = s.sideways;
        set->m_yVelocity = s.yVelocity;
        set->m_disableStartPos = true;

        // Au premier spawn, on retient la start pos d'origine pour pouvoir y revenir.
        if (!m_fields->hasOriginal) {
            m_fields->originalStart = m_startPosObject;
            m_fields->hasOriginal = true;
        }
        m_fields->customStart = obj;
        m_fields->lastSpawnPercent = s.percent;
        m_fields->activeSpawn = s;

        log::info(
            "Spawn a {:.1f}% ({} {}) | y joueur = {:.1f} | centre couloir = {:.1f}",
            s.percent, gamemodeName(s.gamemode), speedName(s.speed), s.y, s.portalY
        );
        this->switchToStartPosObject(obj);
        this->applySpawnHeight();
    }

    // --- v0.4 : du % au snapshot -------------------------------------------

    // Le dernier snapshot dont le % est <= pct, sans dépasser le point le plus
    // loin moins la marge de sécurité. nullptr si rien n'est enregistré.
    Snapshot const* snapshotForPercent(float pct) {
        auto& snaps = m_fields->snaps;
        if (snaps.empty()) return nullptr;
        int maxBucket = std::max(snaps.rbegin()->first - respawnMarginBuckets(), snaps.begin()->first);
        Snapshot const* best = &snaps.begin()->second;
        for (auto const& [bucket, snap] : snaps) {
            if (bucket > maxBucket || snap.percent > pct) break;
            best = &snap;
        }
        return best;
    }

    // Le % le plus loin qu'on peut proposer, ou -1 si rien n'est enregistré.
    float safeMaxPercent() {
        auto s = this->snapshotForPercent(100.f);
        return s ? s->percent : -1.f;
    }

    void spawnAtPercent(float pct) {
        auto s = this->snapshotForPercent(pct);
        if (!s) {
            Notification::create("Aucun snapshot enregistre", NotificationIcon::Warning)->show();
            return;
        }
        // Copie : spawnAt relance le niveau, qui continue d'écrire dans la carte.
        Snapshot copy = *s;
        m_fields->manualIndex = -1;
        this->spawnAt(copy);
    }

    // Respawn un peu avant le point le plus loin atteint.
    void spawnNearFurthest() {
        this->spawnAtPercent(100.f);
    }

    // Revient à la start pos d'origine (le début du niveau, en général).
    void spawnAtOriginalStart() {
        if (!m_fields->hasOriginal) {
            this->resetLevel();
            return;
        }
        log::info("Retour au debut du niveau");
        m_fields->activeSpawn.reset();
        m_fields->manualIndex = -1;
        this->switchToStartPosObject(m_fields->originalStart);
    }

    // --- v0.6 : le mode manuel ---------------------------------------------

    bool canUseManualKeys() {
        return !m_isPlatformer && !m_isPaused && !m_hasCompletedLevel && m_player1;
    }

    static void notify(std::string const& text, NotificationIcon icon = NotificationIcon::Info) {
        Notification::create(text, icon, 1.2f)->show();
    }

    // Pose une start pos là où tu es, sans interrompre le run.
    void placeManual() {
        if (!this->canUseManualKeys() || m_player1->m_isDead) return;

        auto s = takeSnapshot(this, m_player1);
        auto& list = m_fields->manual;
        auto it = std::upper_bound(list.begin(), list.end(), s.x, [](float x, Snapshot const& o) {
            return x < o.x;
        });

        // Écart minimum (réglage, en blocs) : évite d'en empiler plusieurs en
        // appuyant vite sur la touche. La liste est triée, il suffit de
        // regarder les deux voisines.
        float minGap = static_cast<float>(Mod::get()->getSettingValue<int64_t>("manual-min-gap")) * BUCKET_SIZE;
        if (minGap > 0.f) {
            Snapshot const* tooClose = nullptr;
            if (it != list.end() && it->x - s.x < minGap) tooClose = &*it;
            if (it != list.begin() && s.x - std::prev(it)->x < minGap) tooClose = &*std::prev(it);
            if (tooClose) {
                notify(
                    fmt::format("Trop proche de la start pos a {:.1f} %", tooClose->percent),
                    NotificationIcon::Warning
                );
                return;
            }
        }

        int index = static_cast<int>(it - list.begin());
        list.insert(it, s);
        // Si on insère avant la start pos active, son numéro se décale.
        if (m_fields->manualIndex >= index) m_fields->manualIndex++;

        log::info("Start pos manuelle posee a {:.1f}% ({})", s.percent, gamemodeName(s.gamemode));
        notify(
            fmt::format("Start pos {}/{} posee a {:.1f} %", index + 1, list.size(), s.percent),
            NotificationIcon::Success
        );
    }

    // Va sur la start pos manuelle n° index ; -1 (ou hors liste) = début du niveau.
    void goToManual(int index, bool showNotif = true) {
        auto& list = m_fields->manual;
        if (index < 0 || index >= static_cast<int>(list.size())) {
            this->spawnAtOriginalStart();
            if (showNotif) notify("Debut du niveau");
            return;
        }
        Snapshot copy = list[index];
        this->spawnAt(copy);
        m_fields->manualIndex = index;
        if (showNotif) {
            notify(fmt::format("Start pos {}/{} ({:.1f} %)", index + 1, list.size(), copy.percent));
        }
    }

    // Suivante (+1) ou précédente (-1). On boucle : ... dernière -> début -> première ...
    void cycleManual(int dir) {
        if (!this->canUseManualKeys()) return;
        auto& list = m_fields->manual;
        if (list.empty()) {
            notify("Aucune start pos manuelle : pose-en une d'abord", NotificationIcon::Warning);
            return;
        }
        int count = static_cast<int>(list.size());
        int next = m_fields->manualIndex + dir;
        if (next >= count) next = -1;
        if (next < -1) next = count - 1;
        this->goToManual(next);
    }

    // Supprime la start pos manuelle active et recule sur la précédente.
    void deleteManual() {
        if (!this->canUseManualKeys()) return;
        auto& list = m_fields->manual;
        int index = m_fields->manualIndex;
        if (index < 0 || index >= static_cast<int>(list.size())) {
            notify("Va sur une start pos manuelle pour la supprimer", NotificationIcon::Warning);
            return;
        }
        float pct = list[index].percent;
        list.erase(list.begin() + index);
        log::info("Start pos manuelle a {:.1f}% supprimee", pct);
        notify(fmt::format("Start pos a {:.1f} % supprimee ({} restantes)", pct, list.size()));
        this->goToManual(index - 1, false);
    }

    // Supprime toutes les start pos manuelles du niveau. Renvoie combien.
    // On ne relance pas le niveau (on est dans le menu pause) : le spawn en
    // cours reste actif jusqu'au prochain « Debut » ou changement de start pos.
    int deleteAllManual() {
        int count = static_cast<int>(m_fields->manual.size());
        m_fields->manual.clear();
        m_fields->manualIndex = -1;
        log::info("{} start pos manuelles supprimees", count);
        return count;
    }
};

// Les touches sont des réglages « keybind » de Geode (voir mod.json) :
// tu peux les changer dans les réglages du mod ou dans les touches du jeu.
$on_mod(Loaded) {
    auto onPress = [](auto action) {
        return [action](Keybind const&, bool down, bool repeat, double) {
            if (!down || repeat) return;
            if (auto pl = static_cast<SPAPlayLayer*>(PlayLayer::get())) action(pl);
        };
    };
    listenForKeybindSettingPresses("place-startpos", onPress([](SPAPlayLayer* pl) { pl->placeManual(); }));
    listenForKeybindSettingPresses("next-startpos", onPress([](SPAPlayLayer* pl) { pl->cycleManual(1); }));
    listenForKeybindSettingPresses("prev-startpos", onPress([](SPAPlayLayer* pl) { pl->cycleManual(-1); }));
    listenForKeybindSettingPresses("delete-startpos", onPress([](SPAPlayLayer* pl) { pl->deleteManual(); }));
}

class $modify(SPAPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto pl = PlayLayer::get();
        if (!pl || pl->m_isPlatformer) return;

        auto winSize = CCDirector::get()->getWinSize();
        auto menu = CCMenu::create();
        menu->setID("startpos-anywhere-menu"_spr);
        menu->setPosition({0.f, 0.f});
        this->addChild(menu);

        auto spawnBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Respawn", "goldFont.fnt", "GJ_button_01.png", .7f),
            this, menu_selector(SPAPauseLayer::onSpawnFurthest)
        );
        spawnBtn->setPosition({70.f, winSize.height / 2.f + 40.f});
        menu->addChild(spawnBtn);

        auto percentBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Choisir %", "goldFont.fnt", "GJ_button_02.png", .7f),
            this, menu_selector(SPAPauseLayer::onChoosePercent)
        );
        percentBtn->setPosition({70.f, winSize.height / 2.f});
        menu->addChild(percentBtn);

        auto startBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Debut", "goldFont.fnt", "GJ_button_04.png", .7f),
            this, menu_selector(SPAPauseLayer::onSpawnStart)
        );
        startBtn->setPosition({70.f, winSize.height / 2.f - 40.f});
        menu->addChild(startBtn);

        auto clearBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Suppr. SP", "goldFont.fnt", "GJ_button_06.png", .7f),
            this, menu_selector(SPAPauseLayer::onDeleteAllManual)
        );
        clearBtn->setPosition({70.f, winSize.height / 2.f - 80.f});
        menu->addChild(clearBtn);
    }

    void onDeleteAllManual(CCObject*) {
        auto pl = static_cast<SPAPlayLayer*>(PlayLayer::get());
        if (!pl) return;
        auto count = pl->m_fields->manual.size();
        if (count == 0) {
            Notification::create("Aucune start pos manuelle sur ce niveau", NotificationIcon::Info)->show();
            return;
        }
        createQuickPopup(
            "Tout supprimer",
            fmt::format("Supprimer les <cy>{}</c> start pos manuelles de ce niveau ?", count),
            "Annuler", "Supprimer",
            [](FLAlertLayer*, bool confirmed) {
                if (!confirmed) return;
                if (auto pl = static_cast<SPAPlayLayer*>(PlayLayer::get())) {
                    int n = pl->deleteAllManual();
                    Notification::create(
                        fmt::format("{} start pos supprimees", n), NotificationIcon::Success
                    )->show();
                }
            }
        );
    }

    void onChoosePercent(CCObject*) {
        auto pl = static_cast<SPAPlayLayer*>(PlayLayer::get());
        if (!pl) return;

        float maxPct = pl->safeMaxPercent();
        if (maxPct < 0.f) {
            Notification::create("Joue un peu d'abord : rien n'est enregistre", NotificationIcon::Warning)->show();
            return;
        }
        float initial = pl->m_fields->lastSpawnPercent >= 0.f ? pl->m_fields->lastSpawnPercent : maxPct;

        auto describe = [](float pct) -> std::string {
            auto pl = static_cast<SPAPlayLayer*>(PlayLayer::get());
            auto s = pl ? pl->snapshotForPercent(pct) : nullptr;
            if (!s) return "";
            return fmt::format(
                "-> {:.1f} % : {} {}{}{}",
                s->percent, gamemodeName(s->gamemode), speedName(s->speed),
                s->mini ? " mini" : "", s->dual ? " dual" : ""
            );
        };

        // Le menu pause peut avoir changé pendant que le popup était ouvert :
        // on le retrouve dans la scène au moment du spawn.
        auto spawn = [](float pct) {
            auto scene = CCDirector::get()->getRunningScene();
            if (auto pause = scene ? scene->getChildByType<PauseLayer>(0) : nullptr) {
                pause->onResume(nullptr);
            }
            if (auto pl = static_cast<SPAPlayLayer*>(PlayLayer::get())) {
                pl->spawnAtPercent(pct);
            }
        };

        if (auto popup = SpawnPopup::create(maxPct, initial, describe, spawn)) {
            popup->show();
        }
    }

    // onResume retire le menu pause : on récupère le PlayLayer avant,
    // et on ne touche plus à `this` après.
    void onSpawnFurthest(CCObject*) {
        auto pl = static_cast<SPAPlayLayer*>(PlayLayer::get());
        this->onResume(nullptr);
        if (pl) pl->spawnNearFurthest();
    }

    void onSpawnStart(CCObject*) {
        auto pl = static_cast<SPAPlayLayer*>(PlayLayer::get());
        this->onResume(nullptr);
        if (pl) pl->spawnAtOriginalStart();
    }
};
