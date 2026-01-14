# Developer Console & Debugging Tools

The SDFX Engine includes a robust set of debugging tools, a system event logger, and a hidden cheat input system to assist in development and testing.

## 📜 System Console

The System Console provides real-time feedback on engine initialization, asset loading, and game state changes. It is useful for tracking errors (missing files) and verifying system behavior.

### Controls

| Action | Input | Description |
| :--- | :--- | :--- |
| **Toggle Console** | `F10` | Opens/Closes the console overlay. |
| **Scroll History** | `Mouse Wheel` | Navigates through the last 100 log entries. |
| **Clear Log** | `Click "CLEAR"` | Wipes the current session history. |
| **Help** | `Click "HELP"` | Opens the documentation in the web browser. |

### Logged Events
The console automatically captures and displays the following information:
* **System Initialization:** GPU OpenGL version, Audio device status.
* **Asset Loading:** Status of Textures (`.png`), Sounds, and Music (`.mp3`).
    * *Logs warnings if files are missing.*
* **Game State:** Save/Load confirmations, Pause states, and UI toggles.
* **Player Events:** Position resets, deaths (void fall), and gear changes.
* **Screenshot:** Confirmations of saved screenshots.

---

## ⌨️ Cheat Console

A secondary command input is available for modifying gameplay mechanics on the fly. This system uses numeric codes entered via a dedicated UI.

### Usage
1.  Press **`K`** to open the Cheat Input overlay.
2.  Type the numeric code using `0-9`.
3.  The active cheats will appear in yellow text below the input line.
4.  Press **`SHIFT + K`** to clear all active cheats instantly.

### Available Codes

| Code | Effect | Description |
| :--- | :--- | :--- |
| **29103** | `FLY MODE` | Disables gravity and allows free movement with Arrow Keys. |
| **84721** | `INF JUMP` | Allows the player to jump infinitely in mid-air. |
| **112233** | `NOCLIP` | Disables collision with blocks (walls/floors). |

---

## 📊 Visual Debug Overlay

For performance monitoring and real-time state values, the engine renders a lightweight Heads-Up Display (HUD).

**Activation:** Press `F3`

### Displayed Metrics:
* **Performance:** Current FPS and Frame Time.
* **World Info:** Player Position (X, Y) and Active Block Count.
* **State:** Current Camera Mode, Weather Type, and Day/Night cycle.
* **Asset Status:** Verifies if `player.png`, `cursor.png`, or `gear` textures are loaded.
* **Audio:** Displays the currently playing music track filename.
