# Start Pos Anywhere

Start a level **from any percentage**, without placing anything by hand.

While you play, the mod quietly records your run: position, gamemode, speed, gravity, mini, dual, mirror and vertical speed. From the pause menu you can then restart from any point you have already reached, using a start position built from your own run.

## Pause menu

- **Respawn**: restart a little before the furthest point you reached (the safety margin is a setting).
- **Choisir %**: open a slider, pick a percentage and see which state you will spawn in before you confirm.
- **Debut**: go back to the real start of the level.
- **Suppr. SP**: delete all your manual start positions on this level.

## Manual start positions

| Key (default) | Action |
|---|---|
| `C` | Place a start position where you are, without stopping the run |
| `E` / `Q` | Restart from the next / previous one (loops through the level start) |
| `Shift+C` | Delete the active one |

All keys can be changed in the mod settings.

## Saving

Recordings and manual start positions are saved per level and reloaded when you open it again. If the creator updates the level, the old recording is discarded. Editor levels are not saved.

## Limitations

- A start position only knows the **player** state, not the **level** state: blocks moved by triggers, colors and toggled groups are not restored. Levels heavy on triggers may look wrong after spawning.
- Platformer levels are not supported.
- Like any start position, runs started from one do not count towards your progress.
