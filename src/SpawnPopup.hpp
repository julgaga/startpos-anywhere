#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/SliderNode.hpp>

#include <functional>
#include <string>

// Popup « Spawn au % » : un slider de 0 au % le plus loin enregistré,
// un aperçu du snapshot choisi et un bouton pour spawn.
// Il ne connaît pas le PlayLayer : tout passe par les deux callbacks.
class SpawnPopup : public geode::Popup {
public:
    using DescribeFn = std::function<std::string(float percent)>;
    using SpawnFn = std::function<void(float percent)>;

    static SpawnPopup* create(float maxPercent, float initialPercent, DescribeFn describe, SpawnFn onSpawn);

protected:
    float m_percent = 0.f;
    DescribeFn m_describe;
    SpawnFn m_onSpawn;
    cocos2d::CCLabelBMFont* m_valueLabel = nullptr;
    cocos2d::CCLabelBMFont* m_infoLabel = nullptr;

    bool init(float maxPercent, float initialPercent, DescribeFn describe, SpawnFn onSpawn);
    void updateLabels();
    void onSpawn(cocos2d::CCObject*);
};
