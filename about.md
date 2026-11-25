# Start Pos Anywhere

Start a level **from any percentage**, without placing anything by hand.

While you play, the mod quietly records your run: position, gamemode, speed, gravity, mini, dual, mirror and vertical speed. From the pause menu you can then restart from any point you have already reached, using a start position built from your own run.

## Pause menu

A small icon with the mod's logo sits in the left column of the pause menu. It opens the mod menu, which shows how far your run is recorded and holds:

- **Respawn**: restart a little before the furthest point you reached (the safety margin is a setting).
- **Choisir %**: open a slider, pick a percentage and see which state you will spawn in before you confirm.
- **Debut**: go back to the real start of the level.
- **Suppr. SP**: delete all your manual start positions on this level.
- **Start pos manuelles**: browse your manual start positions with the arrows (number, %, gamemode) and press **Aller** to restart from the one you picked.
- A gear button that opens the mod settings.

## Manual start positions

| Key (default) | Action |
|---|---|
| `C` | Place a start position where you are, without stopping the run |
| `E` / `Q` | Restart from the next / previous one (loops through the level start) |
| `Shift+C` | Delete the active one |

All keys can be changed in the mod settings.

## Saving

Recordings and manual start positions are saved per level and reloaded when you open it again. If the creator updates the level, the old recording is discarded. Editor levels are not saved.

## Precise mode (practice)

A start position only knows the **player** state, not the **level** state: blocks moved by triggers, colors and toggled groups are not restored. In practice mode, the mod therefore also records **full checkpoints** every few blocks (10 by default) and respawns from them when one is available. The level is then exactly as it was, even in trigger-heavy levels. The % slider shows "checkpoint precis" when that is the case.

- Only runs started from the real level start (or from a precise checkpoint) are recorded, since a run started from a start position already has the wrong trigger state.
- Precise checkpoints are kept while you stay in practice mode and are not saved between sessions.
- If you place your own practice checkpoint after a precise respawn, yours takes over.
- The checkpoint injection technique comes from [Practice Checkpoint Permanence](https://geode-sdk.org/mods/kevadroz.practicecheckpointpermanence) by Kevadroz.

## Limitations

- Outside practice mode, spawns use start positions, so trigger-heavy levels may look wrong after spawning.
- Platformer levels are not supported.
- Like any start position, runs started from one do not count towards your progress.
