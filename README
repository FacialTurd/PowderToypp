The Powder Toy (Subframe Chipmaker Mod) - August 2016

This is a mod of The Powder Toy for subframe chipmakers. Subframe technology refers to a collection of tricks used to reduce computation cycles in electronic components to a single frame.

Features
========

These features are enabled when you enter the Lua command "tpt.setdebug(0x8)" into the console (or you can put it in autorun.lua):

- Particle order reloading: if you use the brush or do a copy-paste, then advance the simulation, the frame will be completed and the particle order will be reloaded automatically. You can reload the particle order manually with Shift-F5.
- Shift-Space runs the simulation particle-by-particle rather than frame-by-frame.
- Shift-R starts a recording of the particle-by-particle simulation and automatically stops recording at the end of the frame. Recording can still be stopped with R but particle-by-particle simulation will still continue.
- Alt-F continues updating particles until it encouters an "interesting" update (create_part, delete_part or part_change_type called), and does not display any log messages until the frame is completed.
- Overwriting a local save "<save>.cps" will create a backup of the file you're overwriting at "<save>.cps.backup". Subframe development is dangerous.

These features will be present without enabling:

- Stacked particles are shown in the debug HUD.
- P opens the property tool window rather than takes a screenshot (you can still take screenshots with F2).
- The property tool sets temp values in celsius rather than kelvin.
- Ctrl-Y reverses an undo (merged in vanilla snapshot).
- The ctype of FILTs will be displayed in the debug HUD. In the original game, this was only shown for non-FILT coloured particles like BRAY.
- 30th bit handling: Setting the ctype of coloured particles (e.g. FILT) with the property tool will XOR the input value with 0x20000000 first (so it will set the 30th bit automatically, unless you set it). When hovering the mouse over coloured particles, the ctype displayed in the debug HUD will ignore the 30th bit and, if the 29th bit is set, be displayed as a negative number.
- DRAY is coloured bright yellow to distinguish it from ARAY and DTEC.
- When pasting (or placing a stamp), the pasted particles can be moved out of the initial paste boundary without getting deleted. Additionally, the white dotted paste boundary will not be shown.
- If you attempt to open a save or close the window with the ESC button (the case for closing with the 'X' button has not been implemented) when there are unsaved changes, a confirmation warning will be displayed.
- The position in frame is saved into undo history during subframe debugging, so you can undo into the middle of a frame without destroying everything.
- (v1.1) Stack tool (Shortcut: Shift-S): Clicking on a stack of particles will unstack them, provided there's enough space; selecting multiple particles in different positions will stack them in order of their positions.
- (v1.1) Creating CRAY with the brush gives it a ctype of SPRK automatically. If you want to create a ctype-less CRAY, use the property tool.
- (v1.1, experimental) Shift-; (semicolon) toggles stack mode. It allows you to draw over existing particles, and makes right-click delete one stacked particle at a time. This helps to make transparent DTEC.
- (v1.2) DTEC is coloured a darker red to distinguish it from ARAY.

Note that the original game already supports the following subframe debugging features:

- Shift-F updates all particles up to the particle where your mouse is at.
- Alt-F updates a single particle. This behaviour has been modified in this mod to continue updating particles until an "interesting" update occurs.

The following simulation features are from the vanilla TPT snapshot, so don't publish anything that depends on these:

- Two new FILT modes, variable red shift and variable blue shift, allows ctype-based bitshifting that can be set by DTEC.
- CRAY copies its own life to particles it creates.

Changelog
=========

v1.0:
Original release.

v1.1:
Merge changes in snapshot.
Add stack tool.
Add stack mode.
Give drawn CRAY a ctype of SPRK.

v1.2 (not released):
Change DTEC colour.
