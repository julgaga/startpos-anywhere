#pragma once

#include <Geode/Geode.hpp>

// Un snapshot par tranche de 30 unités de x (une case de GD fait 30 unités).
inline constexpr float BUCKET_SIZE = 30.f;

// Tout ce qu'une start pos sait stocker sur le joueur.
struct Snapshot {
    float x = 0.f;
    float y = 0.f;
    float percent = 0.f;
    float yVelocity = 0.f;      // GD 2.2091 sait la remettre dans une start pos
    float portalY = 0.f;        // centre du couloir (ship, ball, ufo, wave, spider, swing)
    int gamemode = 0;           // même ordre que LevelSettingsObject::m_startMode
    Speed speed = Speed::Normal;
    bool mini = false;
    bool upsideDown = false;
    bool dual = false;
    bool mirror = false;
    bool goingLeft = false;     // gameplay inversé (portail de sens)
    bool sideways = false;      // gameplay tourné de 90°
    bool onGround = false;      // utile plus tard pour les « zones sûres »
};

// 0 cube, 1 ship, 2 ball, 3 ufo, 4 wave, 5 robot, 6 spider, 7 swing
inline int gamemodeOf(PlayerObject* p) {
    if (p->m_isShip) return 1;
    if (p->m_isBall) return 2;
    if (p->m_isBird) return 3;
    if (p->m_isDart) return 4;
    if (p->m_isRobot) return 5;
    if (p->m_isSpider) return 6;
    if (p->m_isSwing) return 7;
    return 0;
}

// Ces modes vivent dans un couloir (sol + plafond) que GD place autour du y du
// portail... ou de la start pos. Le cube et le robot n'en ont pas.
inline bool usesCorridor(int mode) {
    return mode != 0 && mode != 5;
}

inline char const* gamemodeName(int mode) {
    static char const* names[] = {"cube", "ship", "ball", "ufo", "wave", "robot", "spider", "swing"};
    return (mode >= 0 && mode < 8) ? names[mode] : "?";
}

// m_playerSpeed vaut environ 0.7 / 0.9 / 1.1 / 1.3 / 1.6 selon le portail de vitesse.
inline Speed speedOf(PlayerObject* p) {
    float s = p->m_playerSpeed;
    if (s < 0.8f) return Speed::Slow;
    if (s < 1.0f) return Speed::Normal;
    if (s < 1.2f) return Speed::Fast;
    if (s < 1.45f) return Speed::Faster;
    return Speed::Fastest;
}

inline char const* speedName(Speed speed) {
    switch (speed) {
        case Speed::Slow: return "0.5x";
        case Speed::Normal: return "1x";
        case Speed::Fast: return "2x";
        case Speed::Faster: return "3x";
        case Speed::Fastest: return "4x";
    }
    return "?";
}

inline Snapshot takeSnapshot(PlayLayer* layer, PlayerObject* p) {
    Snapshot s;
    s.x = p->getPositionX();
    s.y = p->getPositionY();
    s.percent = layer->getCurrentPercent();
    s.yVelocity = static_cast<float>(p->getYVelocity());
    s.portalY = layer->m_gameState.m_portalY;
    s.gamemode = gamemodeOf(p);
    s.speed = speedOf(p);
    s.mini = p->m_vehicleSize < 1.f;
    s.upsideDown = p->m_isUpsideDown;
    s.dual = layer->m_gameState.m_isDualMode;
    s.mirror = layer->m_gameState.m_isFlipped;
    s.goingLeft = p->m_isGoingLeft;
    s.sideways = p->m_isSideways;
    s.onGround = p->m_isOnGround;
    return s;
}
