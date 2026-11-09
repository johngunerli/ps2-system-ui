# GameCube Web UI Specification

## Overview
A high-fidelity, interactive 3D web reproduction of the Nintendo GameCube BIOS system interface with full Gamepad API support, interactive sub-screens (Memory Card, Calendar, Options, Gameplay/Disc Launcher), custom App Launcher integration, and Web Audio sound effects.

## System Architecture

### Tech Stack
- **Framework**: React 18+ with Vite and TypeScript
- **3D Graphics Engine**: Three.js & @react-three/fiber & @react-three/drei
- **Styling**: Tailwind CSS + custom retro Gamecube font/styling
- **Audio**: Web Audio API (procedural synthesis for ambient hums and sound effects)
- **Input**: Standard Web Gamepad API + Keyboard/Mouse controls

### Navigation & State Machine
1. **Boot Screen**:
   - Rolling 3D cube tracing the "G" pattern on an orthogonal grid.
   - Bounce and logo reveal with audio chime.
   - Any key / button skips straight to Main Menu.
2. **Main 3D Menu**:
   - Central cube floating on its corner vertex.
   - Atmospheric deep indigo/purple space void with gently floating background crystal prisms.
   - 4 cardinal directions:
     - **Top (North)**: Game Play (Disc & App Launcher)
     - **Left (West)**: Memory Card
     - **Right (East)**: Calendar
     - **Bottom (South)**: Option
   - Smooth rotation transitions when pressing Up/Down/Left/Right or moving analog stick.
   - Pressing **A** (or Enter) zooms camera into the selected sub-screen.
   - Pressing **B** (or Escape) dollies camera back to the main menu.
3. **Sub-Screens**:
   - **Game Play / App Launcher**:
     - 3D rotating Mini-DVD disc model with dynamic disc face artwork.
     - Play disc / Launch selected app.
     - Disc library selector to switch between registered apps.
     - "Add App" modal to register custom web apps/URLs with persistent localStorage saving.
     - Launching opens the app in a GameCube styled retro overlay frame with a quick "B to Exit" shortcut.
   - **Memory Card**:
     - Slot A and Slot B switcher.
     - Free/Used block counter (e.g. 251 blocks free).
     - Save blocks with game banners/icons (Smash Melee, Wind Waker, Mario Sunshine, plus user apps).
   - **Calendar / Clock**:
     - Real-time digital and analog clock with date, month, year, day of week matching GameCube font and layout.
   - **Option**:
     - Sound output toggle: Mono, Stereo, Surround.
     - Sound test: Left/Right channel test sounds.
     - Screen position adjuster.
4. **Gamepad Controller Support**:
   - Listens to `gamepadconnected` and `gamepaddisconnected`.
   - **Pop-up Banner**: Displays a stylish GameCube OS notification toast (e.g. *"Controller connected in Socket 1"*).
   - Standard button mapping:
     - D-Pad / Left Stick: Navigate / Rotate cube
     - A button: Confirm / Select / Open
     - B button: Back / Cancel
     - X button: Switch App / Change Disc
     - Y button: Add New App
     - Start button: Skip Boot / Open Menu
5. **Audio Engine**:
   - Synthesizer generating the ambient Gamecube BIOS background drone.
   - Procedural UI sounds:
     - Menu whoosh / rotation
     - Cursor blip / tick
     - Confirm chime
     - Cancel chime
     - Disc laser spin-up sound
