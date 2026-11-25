#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "Snapshot.hpp"

using SnapshotMap = std::map<int, Snapshot>; // clé = tranche de x

// Tout ce qu'on garde pour un niveau.
struct LevelData {
    SnapshotMap snaps;            // l'enregistrement automatique
    std::vector<Snapshot> manual; // les start pos posées à la main, triées par x
};

// Nom du fichier de sauvegarde pour ce niveau (« main-1 », « online-12345 »),
// ou rien pour les niveaux de l'éditeur, qui changent pendant que tu les modifies.
std::optional<std::string> storageKey(GJGameLevel* level);

// Lit les données du niveau. Vide si pas de fichier, fichier illisible,
// ou si le créateur a mis le niveau à jour depuis.
LevelData loadLevelData(GJGameLevel* level);

// Écrit les données du niveau dans le dossier de sauvegarde du mod.
void saveLevelData(GJGameLevel* level, LevelData const& data);
