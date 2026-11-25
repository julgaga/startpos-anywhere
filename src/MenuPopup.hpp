#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

#include <functional>
#include <string>

// Ce que font les boutons du menu. Le popup ne connaît pas le PlayLayer :
// main.cpp lui passe les actions.
struct MenuActions {
    std::function<void()> respawn;
    std::function<void()> choosePercent;
    std::function<void()> backToStart;
    std::function<void()> deleteManual;

    // Les start pos manuelles, déjà mises en texte (« 2/5 - 37.5 % - ship »),
    // celle qui est sélectionnée au départ, et quoi faire pour y aller.
    std::vector<std::string> manualLabels;
    int manualSelected = 0;
    std::function<void(int index)> goToManual;
};

// Le menu « Start Pos Anywhere », ouvert par l'icône du menu pause.
class MenuPopup : public geode::Popup {
public:
    static MenuPopup* create(std::string const& status, MenuActions actions);

protected:
    MenuActions m_actions;
    int m_selected = 0;
    cocos2d::CCLabelBMFont* m_selectedLabel = nullptr;

    void updateSelectedLabel();
    void onPrevManual(cocos2d::CCObject*);
    void onNextManual(cocos2d::CCObject*);
    void onGoToManual(cocos2d::CCObject*);

    bool init(std::string const& status, MenuActions actions);
    void addActionButton(char const* text, char const* texture, float y, cocos2d::SEL_MenuHandler handler);

    // Ferme le menu puis lance l'action (copiée avant : onClose peut détruire le popup).
    void run(std::function<void()> const& action);

    void onRespawn(cocos2d::CCObject*);
    void onChoosePercent(cocos2d::CCObject*);
    void onBackToStart(cocos2d::CCObject*);
    void onDeleteManual(cocos2d::CCObject*);
    void onSettings(cocos2d::CCObject*);
};
