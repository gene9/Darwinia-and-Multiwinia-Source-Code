
DEF_CONTROL_TYPE( MenuLeft,                          INPUT_TYPE_BOOL ) // Select button to the left
DEF_CONTROL_TYPE( MenuRight,                         INPUT_TYPE_BOOL ) // Select button to the right
DEF_CONTROL_TYPE( MenuUp,                            INPUT_TYPE_BOOL ) // Select button above
DEF_CONTROL_TYPE( MenuDown,                          INPUT_TYPE_BOOL ) // Select button below
DEF_CONTROL_TYPE( MenuActivate,                      INPUT_TYPE_BOOL ) // Press menu button
DEF_CONTROL_TYPE( MenuEscape,                        INPUT_TYPE_BOOL ) // Escape out of this menu
DEF_CONTROL_TYPE( MenuClose,                         INPUT_TYPE_BOOL ) // close current window

//DEF_CONTROL_TYPE( ChatLogDisplay,                    INPUT_TYPE_BOOL ) // Show chat log
//DEF_CONTROL_TYPE( ChatLogHide,                       INPUT_TYPE_BOOL ) // Hide chat log
DEF_CONTROL_TYPE( SkipMessage,                       INPUT_TYPE_BOOL ) // Skip chat message
DEF_CONTROL_TYPE( SkipCutscene,                      INPUT_TYPE_BOOL )

DEF_CONTROL_TYPE( CameraLeft,                        INPUT_TYPE_BOOL ) // Move camera left (boolean)
DEF_CONTROL_TYPE( CameraRight,                       INPUT_TYPE_BOOL ) // Move camera right (boolean)
DEF_CONTROL_TYPE( CameraUp,                          INPUT_TYPE_BOOL ) // Move camera up (boolean)
DEF_CONTROL_TYPE( CameraDown,                        INPUT_TYPE_BOOL ) // Move camera down (boolean)
DEF_CONTROL_TYPE( CameraForwards,                    INPUT_TYPE_BOOL ) // Move camera forwards (boolean)
DEF_CONTROL_TYPE( CameraBackwards,                   INPUT_TYPE_BOOL ) // Move camera backwards (boolean)
DEF_CONTROL_TYPE( CameraZoom,                        INPUT_TYPE_BOOL ) // Zoom camera in (boolean)
DEF_CONTROL_TYPE( CameraDeZoom,                      INPUT_TYPE_BOOL ) // Zoom camera out (boolean)

DEF_CONTROL_TYPE( CameraMove,                        INPUT_TYPE_2D   ) // Analog camera control
DEF_CONTROL_TYPE( TargetMove,                        INPUT_TYPE_2D   ) // Analog target control
DEF_CONTROL_TYPE( TargetMoveSecondary,               INPUT_TYPE_2D   ) // Analog target control
DEF_CONTROL_TYPE( TargetMoveZ,                       INPUT_TYPE_1D   ) // Analog target control
DEF_CONTROL_TYPE( CameraZoomChange,                  INPUT_TYPE_1D   ) // Analog camera zoom
DEF_CONTROL_TYPE( CameraSpeedup,                     INPUT_TYPE_BOOL ) // Hold to go faster
DEF_CONTROL_TYPE( CameraSlowdown,                    INPUT_TYPE_BOOL ) // Hold to go slower
DEF_CONTROL_TYPE( CameraRotate,                      INPUT_TYPE_BOOL ) // rotate the cam in entity track mode
DEF_CONTROL_TYPE( CameraRotateLeft,                  INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( CameraRotateRight,                 INPUT_TYPE_BOOL )

DEF_CONTROL_TYPE( CameraSwitchMode,                  INPUT_TYPE_BOOL ) // Switch from free to track cam

DEF_CONTROL_TYPE( CameraDebugRotate,                 INPUT_TYPE_BOOL ) // Hold for debug rotate mode
DEF_CONTROL_TYPE( GestureActive,                     INPUT_TYPE_BOOL ) // Hold to write a gesture

DEF_CONTROL_TYPE( UnitDeselect,                      INPUT_TYPE_BOOL ) // Escape unit control mode
DEF_CONTROL_TYPE( UnitCycleLeft,                     INPUT_TYPE_BOOL ) // Cycle left through tasks
DEF_CONTROL_TYPE( UnitCycleRight,                    INPUT_TYPE_BOOL ) // Cycle right through tasks

DEF_CONTROL_TYPE( UnitEndSetTarget,                INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( UnitSetTarget,                     INPUT_TYPE_BOOL ) // "Click to go to target"
DEF_CONTROL_TYPE( UnitMove,                          INPUT_TYPE_2D   ) // Analog unit movement
DEF_CONTROL_TYPE( UnitMoveDirectionChange,           INPUT_TYPE_2D   )
DEF_CONTROL_TYPE( UnitPrimaryFireTarget,             INPUT_TYPE_BOOL ) // "Press button to fire at target"
DEF_CONTROL_TYPE( UnitPrimaryFireDirected,           INPUT_TYPE_2D   ) // "Move joystick to fire"
DEF_CONTROL_TYPE( UnitSecondaryFireTarget,           INPUT_TYPE_BOOL ) // "Press button to fire at target"
DEF_CONTROL_TYPE( UnitSecondaryAim,                  INPUT_TYPE_BOOL )

DEF_CONTROL_TYPE( UnitStartSecondaryFireDirected,    INPUT_TYPE_2D   ) // Start moving power arc
DEF_CONTROL_TYPE( UnitSecondaryFireDirected,         INPUT_TYPE_2D   ) // Fire wherever power arc is

DEF_CONTROL_TYPE( UnitCreate,                        INPUT_TYPE_BOOL )

DEF_CONTROL_TYPE( WeaponCycleLeft,                   INPUT_TYPE_BOOL ) // Cycle left through weapons or unit modes
DEF_CONTROL_TYPE( WeaponCycleRight,                  INPUT_TYPE_BOOL ) // Cycle right through weapons or unit modes

DEF_CONTROL_TYPE( TaskManagerSelectNextTask,         INPUT_TYPE_BOOL ) // Select Next active task
DEF_CONTROL_TYPE( TaskManagerDisplay,                INPUT_TYPE_BOOL ) // Show the task manager
DEF_CONTROL_TYPE( TaskManagerHide,                   INPUT_TYPE_BOOL ) // Hide the task manager
DEF_CONTROL_TYPE( TaskManagerSwitchTask,             INPUT_TYPE_BOOL ) // Switch task
DEF_CONTROL_TYPE( TaskManagerEndTask,                INPUT_TYPE_BOOL ) // Kill task

DEF_CONTROL_TYPE( GesturesTaskManagerDisplay,        INPUT_TYPE_BOOL ) // Show the gestural task manager
DEF_CONTROL_TYPE( GesturesTaskManagerHide,           INPUT_TYPE_BOOL ) // Hide the gestural task manager
DEF_CONTROL_TYPE( GesturesTaskManagerEndTask,        INPUT_TYPE_BOOL ) // Kill task
DEF_CONTROL_TYPE( GesturesChatLog,                   INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( GesturesSwitchUnit,                INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( GestureLeft,                       INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( GestureRight,                      INPUT_TYPE_BOOL )

DEF_CONTROL_TYPE( IconsTaskManagerDisplay,           INPUT_TYPE_BOOL ) // Show the iconic task manager
DEF_CONTROL_TYPE( IconsTaskManagerDisplayDown,       INPUT_TYPE_BOOL ) // When icon task manager key has been pressed down
DEF_CONTROL_TYPE( IconsTaskManagerDisplayPressed,    INPUT_TYPE_BOOL ) // When icon task manager key is pressed
DEF_CONTROL_TYPE( IconsTaskManagerDisplayUp,         INPUT_TYPE_BOOL ) // When icon task manager key is released
DEF_CONTROL_TYPE( IconsTaskManagerHide,              INPUT_TYPE_BOOL ) // Hide the iconic task manager
DEF_CONTROL_TYPE( IconsTaskManagerEndTask,           INPUT_TYPE_BOOL ) // Kill task
DEF_CONTROL_TYPE( IconsChatLog,                      INPUT_TYPE_BOOL ) // Show chat log

DEF_CONTROL_TYPE( ActivateTMButton,                  INPUT_TYPE_BOOL ) // Activate Task Manager buttons

//DEF_CONTROL_TYPE( EclipseMouseMove,                  INPUT_TYPE_2D    ) // Eclipse thinks the mouse is moving
DEF_CONTROL_TYPE( EclipseLMouseDown,                 INPUT_TYPE_BOOL ) // Eclipse left mouse button down
DEF_CONTROL_TYPE( EclipseLMousePressed,              INPUT_TYPE_BOOL ) // Eclipse left mouse button is pressed
DEF_CONTROL_TYPE( EclipseRMousePressed,              INPUT_TYPE_BOOL ) // Eclipse right mouse button is pressed
DEF_CONTROL_TYPE( ScrollSpeedup,                     INPUT_TYPE_BOOL ) // Hold to scroll faster

DEF_CONTROL_TYPE( SelectLocation,                    INPUT_TYPE_BOOL ) // Select a location on the world screen
DEF_CONTROL_TYPE( LocationDragActive,                INPUT_TYPE_BOOL ) // Hold to drag a location
DEF_CONTROL_TYPE( DeselectLocation,                  INPUT_TYPE_BOOL ) // Deselect a location after dragging

DEF_CONTROL_TYPE( TileSelect,                        INPUT_TYPE_BOOL ) // Location editor tile select
DEF_CONTROL_TYPE( TileDrop,                          INPUT_TYPE_BOOL ) // Location editor tile drop
DEF_CONTROL_TYPE( TileDrag,                          INPUT_TYPE_BOOL ) // Location editor tile drag

DEF_CONTROL_TYPE( FileMultiSelect,                   INPUT_TYPE_BOOL ) // Hold to select multiple files

DEF_CONTROL_TYPE( DebugDropPacket,                   INPUT_TYPE_BOOL ) // For stress testing the network system

DEF_CONTROL_TYPE( TaskManagerSelectTask1,            INPUT_TYPE_BOOL ) // Go to task manager screenzone 0
DEF_CONTROL_TYPE( TaskManagerSelectTask2,            INPUT_TYPE_BOOL ) // Go to task manager screenzone 1
DEF_CONTROL_TYPE( TaskManagerSelectTask3,            INPUT_TYPE_BOOL ) // Go to task manager screenzone 2
DEF_CONTROL_TYPE( TaskManagerSelectTask4,            INPUT_TYPE_BOOL ) // Go to task manager screenzone 3
DEF_CONTROL_TYPE( TaskManagerSelectTask5,            INPUT_TYPE_BOOL ) // Go to task manager screenzone 4
DEF_CONTROL_TYPE( TaskManagerSelectTask6,            INPUT_TYPE_BOOL ) // Go to task manager screenzone 5
DEF_CONTROL_TYPE( TaskManagerSelectTask7,            INPUT_TYPE_BOOL ) // Go to task manager screenzone 6
DEF_CONTROL_TYPE( TaskManagerSelectTask8,            INPUT_TYPE_BOOL ) // Go to task manager screenzone 7
DEF_CONTROL_TYPE( TaskManagerSelectTask9,            INPUT_TYPE_BOOL ) // Go to task manager screenzone 8

DEF_CONTROL_TYPE( IconsTaskManagerNewSquad,          INPUT_TYPE_BOOL ) // Make a new Insertion Squad
DEF_CONTROL_TYPE( IconsTaskManagerNewEngineer,       INPUT_TYPE_BOOL ) // Make a new Engineer
DEF_CONTROL_TYPE( IconsTaskManagerNewOfficer,        INPUT_TYPE_BOOL ) // Make a new Officer
DEF_CONTROL_TYPE( IconsTaskManagerNewArmour,         INPUT_TYPE_BOOL ) // Make a new Armour

DEF_CONTROL_TYPE( IconsTaskManagerSelectGrenade,     INPUT_TYPE_BOOL ) // Select Grenade as the current secondary weapon
DEF_CONTROL_TYPE( IconsTaskManagerSelectRocket,      INPUT_TYPE_BOOL ) // Select Rocket as the current secondary weapon
DEF_CONTROL_TYPE( IconsTaskManagerSelectAirStrike,   INPUT_TYPE_BOOL ) // Select AirStrike as the current secondary weapon
DEF_CONTROL_TYPE( IconsTaskManagerSelectController,  INPUT_TYPE_BOOL ) // Select Controller as the current secondary weapon

DEF_CONTROL_TYPE( LoaderHelp,                        INPUT_TYPE_BOOL ) // Hold for a help screen
DEF_CONTROL_TYPE( SepulvedaHelp,                     INPUT_TYPE_BOOL ) // Activate Sepulveda

DEF_CONTROL_TYPE( GOLLoaderSpeedup,                  INPUT_TYPE_BOOL ) // Game of Life Loader keys
DEF_CONTROL_TYPE( GOLLoaderSlowdown,                 INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( GOLLoaderReset,                    INPUT_TYPE_BOOL )

DEF_CONTROL_TYPE( RTLoaderReflectivenessIncrease,    INPUT_TYPE_BOOL ) // Ray Tracer Loader keys
DEF_CONTROL_TYPE( RTLoaderReflectivenessDecrease,    INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( RTLoaderPixelWaveIncrease,         INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( RTLoaderPixelWaveDecrease,         INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( RTLoaderMotionBlurIncrease,        INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( RTLoaderMotionBlurDecrease,        INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( RTLoaderPixelBlurIncrease,         INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( RTLoaderPixelBlurDecrease,         INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( RTLoaderBrightnessIncrease,        INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( RTLoaderBrightnessDecrease,        INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( RTLoaderResolutionIncrease,        INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( RTLoaderResolutionDecrease,        INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( RTLoaderCameraMode1,               INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( RTLoaderCameraMode2,               INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( RTLoaderCameraMode3,               INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( RTLoaderLMB,                       INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( RTLoaderRMB,                       INPUT_TYPE_BOOL )

DEF_CONTROL_TYPE( StartAttractMode,                  INPUT_TYPE_BOOL ) // Activate Attract Mode
DEF_CONTROL_TYPE( StopAttractMode,                   INPUT_TYPE_BOOL ) // Deactivate Attract Mode

DEF_CONTROL_TYPE( CameraFreeMovement,                INPUT_TYPE_BOOL ) // Change to free from first person

DEF_CONTROL_TYPE( DebugCameraFixUp,                  INPUT_TYPE_BOOL )

DEF_CONTROL_TYPE( SkipTutorial,                      INPUT_TYPE_BOOL )

DEF_CONTROL_TYPE( GamePause,                         INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( ToggleEditor,                      INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( CreateScreenShot,                  INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( ToggleCheatMenu,                   INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( MouseMovement,                     INPUT_TYPE_2D   )
DEF_CONTROL_TYPE( IconsTaskManagerQuickUnit,         INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( IconsTaskManagerQuickUnitCreate,   INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( IconsTaskManagerQuickUnitLeft,     INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( IconsTaskManagerQuickUnitRight,     INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( ControllerUnplugged,               INPUT_TYPE_BOOL )
DEF_CONTROL_TYPE( ControllerPlugged,               INPUT_TYPE_BOOL )
