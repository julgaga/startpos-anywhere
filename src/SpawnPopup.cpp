#include "SpawnPopup.hpp"

using namespace geode::prelude;

SpawnPopup* SpawnPopup::create(float maxPercent, float initialPercent, DescribeFn describe, SpawnFn onSpawn) {
    auto ret = new SpawnPopup();
    if (ret->init(maxPercent, initialPercent, std::move(describe), std::move(onSpawn))) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool SpawnPopup::init(float maxPercent, float initialPercent, DescribeFn describe, SpawnFn onSpawn) {
    if (!Popup::init(300.f, 200.f)) return false;

    m_describe = std::move(describe);
    m_onSpawn = std::move(onSpawn);
    m_percent = std::clamp(initialPercent, 0.f, maxPercent);

    this->setTitle("Spawn au %");

    // Les labels d'abord : le callback du slider s'en sert.
    m_valueLabel = CCLabelBMFont::create("", "bigFont.fnt");
    m_valueLabel->setScale(.7f);
    m_mainLayer->addChildAtPosition(m_valueLabel, Anchor::Center, {0.f, 35.f});

    m_infoLabel = CCLabelBMFont::create("", "goldFont.fnt");
    m_infoLabel->setScale(.55f);
    m_mainLayer->addChildAtPosition(m_infoLabel, Anchor::Center, {0.f, -22.f});

    auto rangeLabel = CCLabelBMFont::create(
        fmt::format("Enregistre jusqu'a {:.1f} %", maxPercent).c_str(), "chatFont.fnt"
    );
    rangeLabel->setScale(.6f);
    rangeLabel->setOpacity(180);
    rangeLabel->setColor({255, 255, 255});
    m_mainLayer->addChildAtPosition(rangeLabel, Anchor::Center, {0.f, -42.f});

    auto slider = SliderNode::create([this](SliderNode*, float value) {
        m_percent = value;
        this->updateLabels();
    });
    slider->setMin(0.f);
    slider->setMax(std::max(maxPercent, 0.1f));
    slider->setValue(m_percent);
    m_mainLayer->addChildAtPosition(slider, Anchor::Center, {0.f, 5.f});

    auto spawnBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Spawn", "goldFont.fnt", "GJ_button_01.png", .8f),
        this, menu_selector(SpawnPopup::onSpawn)
    );
    m_buttonMenu->addChildAtPosition(spawnBtn, Anchor::Bottom, {0.f, 28.f});

    this->updateLabels();
    return true;
}

void SpawnPopup::updateLabels() {
    m_valueLabel->setString(fmt::format("{:.1f} %", m_percent).c_str());
    if (m_describe) {
        m_infoLabel->setString(m_describe(m_percent).c_str());
    }
}

void SpawnPopup::onSpawn(CCObject*) {
    // onClose peut détruire le popup : on copie ce dont on a besoin avant.
    auto callback = m_onSpawn;
    float percent = m_percent;
    this->onClose(nullptr);
    if (callback) callback(percent);
}
