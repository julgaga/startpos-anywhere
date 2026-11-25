#include "MenuPopup.hpp"

#include <Geode/ui/GeodeUI.hpp>

using namespace geode::prelude;

MenuPopup* MenuPopup::create(std::string const& status, MenuActions actions) {
    auto ret = new MenuPopup();
    if (ret->init(status, std::move(actions))) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool MenuPopup::init(std::string const& status, MenuActions actions) {
    if (!Popup::init(280.f, 340.f)) return false;
    m_actions = std::move(actions);

    this->setTitle("Start Pos Anywhere");

    // Petit résumé : jusqu'où c'est enregistré, combien de start pos manuelles...
    auto statusLabel = CCLabelBMFont::create(status.c_str(), "chatFont.fnt");
    statusLabel->setAlignment(kCCTextAlignmentCenter);
    statusLabel->setScale(.6f);
    m_mainLayer->addChildAtPosition(statusLabel, Anchor::Top, {0.f, -58.f});

    this->addActionButton("Respawn", "GJ_button_01.png", 75.f, menu_selector(MenuPopup::onRespawn));
    this->addActionButton("Choisir %", "GJ_button_02.png", 40.f, menu_selector(MenuPopup::onChoosePercent));
    this->addActionButton("Debut", "GJ_button_04.png", 5.f, menu_selector(MenuPopup::onBackToStart));
    this->addActionButton("Suppr. SP", "GJ_button_06.png", -30.f, menu_selector(MenuPopup::onDeleteManual));

    // --- Choisir une start pos manuelle --------------------------------------
    auto sectionLabel = CCLabelBMFont::create("Start pos manuelles", "goldFont.fnt");
    sectionLabel->setScale(.55f);
    m_mainLayer->addChildAtPosition(sectionLabel, Anchor::Center, {0.f, -66.f});

    bool hasManual = !m_actions.manualLabels.empty();
    m_selected = hasManual
        ? std::clamp(m_actions.manualSelected, 0, static_cast<int>(m_actions.manualLabels.size()) - 1)
        : 0;

    m_selectedLabel = CCLabelBMFont::create("", "bigFont.fnt");
    m_mainLayer->addChildAtPosition(m_selectedLabel, Anchor::Center, {0.f, -94.f});

    if (hasManual) {
        auto prevSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        prevSpr->setScale(.45f);
        auto prevBtn = CCMenuItemSpriteExtra::create(prevSpr, this, menu_selector(MenuPopup::onPrevManual));
        m_buttonMenu->addChildAtPosition(prevBtn, Anchor::Center, {-118.f, -94.f});

        auto nextSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        nextSpr->setScale(.45f);
        nextSpr->setFlipX(true);
        auto nextBtn = CCMenuItemSpriteExtra::create(nextSpr, this, menu_selector(MenuPopup::onNextManual));
        m_buttonMenu->addChildAtPosition(nextBtn, Anchor::Center, {118.f, -94.f});

        auto goSprite = ButtonSprite::create("Aller", 90, true, "goldFont.fnt", "GJ_button_03.png", 26.f, .7f);
        auto goBtn = CCMenuItemSpriteExtra::create(goSprite, this, menu_selector(MenuPopup::onGoToManual));
        m_buttonMenu->addChildAtPosition(goBtn, Anchor::Center, {0.f, -126.f});
    }
    this->updateSelectedLabel();

    // Engrenage en haut à droite : les réglages du mod (touches, marge, mode précis).
    auto gear = CCSprite::createWithSpriteFrameName("GJ_optionsBtn02_001.png");
    gear->setScale(.7f);
    auto settingsBtn = CCMenuItemSpriteExtra::create(gear, this, menu_selector(MenuPopup::onSettings));
    m_buttonMenu->addChildAtPosition(settingsBtn, Anchor::TopRight, {-22.f, -22.f});

    return true;
}

void MenuPopup::addActionButton(char const* text, char const* texture, float y, SEL_MenuHandler handler) {
    // Même largeur pour tous les boutons : la colonne reste propre.
    auto sprite = ButtonSprite::create(text, 170, true, "goldFont.fnt", texture, 30.f, .8f);
    auto btn = CCMenuItemSpriteExtra::create(sprite, this, handler);
    m_buttonMenu->addChildAtPosition(btn, Anchor::Center, {0.f, y});
}

void MenuPopup::updateSelectedLabel() {
    auto& labels = m_actions.manualLabels;
    if (labels.empty()) {
        m_selectedLabel->setString("Aucune : touche C en jeu");
        m_selectedLabel->setOpacity(150);
    }
    else {
        m_selectedLabel->setString(labels[m_selected].c_str());
        m_selectedLabel->setOpacity(255);
    }
    // Le texte ne doit pas passer sous les flèches.
    limitNodeSize(m_selectedLabel, {200.f, 20.f}, .5f, .1f);
}

void MenuPopup::onPrevManual(CCObject*) {
    int count = static_cast<int>(m_actions.manualLabels.size());
    if (count == 0) return;
    m_selected = (m_selected - 1 + count) % count;
    this->updateSelectedLabel();
}

void MenuPopup::onNextManual(CCObject*) {
    int count = static_cast<int>(m_actions.manualLabels.size());
    if (count == 0) return;
    m_selected = (m_selected + 1) % count;
    this->updateSelectedLabel();
}

void MenuPopup::onGoToManual(CCObject*) {
    auto action = m_actions.goToManual;
    int index = m_selected;
    this->run([action, index] {
        if (action) action(index);
    });
}

void MenuPopup::run(std::function<void()> const& action) {
    auto copy = action;
    this->onClose(nullptr);
    if (copy) copy();
}

void MenuPopup::onRespawn(CCObject*) { this->run(m_actions.respawn); }
void MenuPopup::onChoosePercent(CCObject*) { this->run(m_actions.choosePercent); }
void MenuPopup::onBackToStart(CCObject*) { this->run(m_actions.backToStart); }
void MenuPopup::onDeleteManual(CCObject*) { this->run(m_actions.deleteManual); }

void MenuPopup::onSettings(CCObject*) {
    openSettingsPopup(Mod::get());
}
