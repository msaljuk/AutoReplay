# AutoReplay
An automated input record-and-replay system for Unreal Engine

[Setup](#Setup)
- [Installation](#Installation)
- [Project Settings](#project-settings)
  
[Architecture](#Architecture)

[Usage](#Usage)
- [Console Interface](#Console-Interface)
- [Blueprint Interface](#Blueprint-Interface)
- [Code Interface](#Code-Interface)

[Demo](#Demo)

[FAQ](#FAQ)
<br>[Development](#Development)

### Setup

#### Installation
You can use this plug-in as an Engine or a Project plugin. Clone this repository into either your engine or project's `/Plugins/` directory and compile your game in Visual Studio. A C++ code project is required for this to work. Set the plugin to be enabled in your engine's project settings or in your project's .uproject file.

#### Project Settings
Once enabled, you can access a dedicated settings tab (JTAutoReplaySettings) for the plugin in your Project Settings menu. Here you can customize some core settings for the plugin, including
- Setting your export directory for all input recordings. By default, this is set to `{Project}/Content/JTInputRecordingSessions/`.
- Setting an escape key that you can trigger during input recordings to pause input capture (useful for cases when you want to not have something show up in the recording such as exit cases). By default, this is set to Left Bracket `[`.

### Architecture
There are three components of this plugin that work in tandem to build the entire record-and-replay system:
- *Input Recorder*: Existing as a singleton subsystem on the game instance, this fields all requests to start and stop recording player(s) input. 
- *Input Serializer*: This is a standalone util library that can take a recorded session from the input recorder and serialize it to the user's export directory as a .JSON file.
- *Input Player*: A singleton subsystem existing in the world, this fields all requests to take previously recorded and serialized input sessions and play them for the current user.

### Usage
As of right now, there are three interfaces to these systems from record to replay. (A fourth, an easy-to-use Editor GUI, is planned for the future).

#### Console Interface
This is the simplest no-frills way to utilize this plugin:
- To record, simple type `jt.autoreplay.inputrecorder.requestrecording` into your console window to start recording your in-game inputs. When you want to stop recording, hit your escape key (Left Bracket or `[` by default - can be changed in your project settings). Then open your console window and type `jt.autoreplay.inputrecorder.stoprecording`.
- Your recording will be serialized to your export directory (`{Project}/Content/JTInputRecordingSessions/` by default - can be changed in your project settings) as a JSON file (e.g. `IRS2024.02.18-14.59.25.json`)
- To replay, copy the name of your serialized recording session JSON file (`IRS2024.02.18-14.59.25.json`), go to your console window and type `jt.autoreplay.inputplayer.requestplay {paste copied name of recording session}`. Your recording will start playing.

There's a lot more options for these commands to add delays, replay multiple times etc. For more detail, look at the help text for these commands in your console window or go look at `AutoReplay/Source/AutoReplay/Private/JTAutoReplayConsoleMenu.cpp`.

#### Blueprint Interface
Both the input recorder and input replayer subsystems can be accessed in any Blueprint script. Simply look for `JTInputRecorder` or `JTInputPlayer` in your Blueprint, and use the given public functions available to script the start/stop of recording/replay sessions:

##### JTInputRecorder
- `RequestRecording`: Call to request the start of a recording session
- `StopRecording`: Call to request termination of an ongoing recording session

##### JTInputPlayer
- `RequestPlay`: Call to request the start of a play session
- `StopPlaying`: Call to request termination of an ongoing play session

#### Code Interface
You can access the same BP functions mentioned above through code. Additional functionality for the input serializer library is also accessible in code.

### Demo
You can watch a demo of the plugin in use here: https://www.saljuk.com/code/AutoReplay

### FAQ

- *Does the input recording work for mouse/keyboard/controller/touch/custom input device?*
<br>The input recorder hooks into the highest Slate (the Unreal Engine core UI framework) viewport level of incoming input. Therefore, it works generically for all inputs that are accepted by the engine itself.

- *Does the input recording work for multiple players?*
<br>All local players currently active for the local game instance are recorded (meaning co-op/split-screen games *should* work). This workflow is however untested as of right now.

- *How accurate is the input replay system?*
<br>Unreal Engine is a non-determinstic engine. While efforts have been made to ensure that the actual input record-and-replay system is frame accurate, we cannot ensure that, even when fed the same input, the engine will always respond in the same manner. In my experience, averaging results of an input session being replayed multiple times yields a high reliability of the same outcome being replicated. However, your mileage may vary based on your specific testing scenario and other factors (variablity in framerate, physics etc.)

### Development
This plugin was developed by me (Saljuk Gondal) as part of my own suite of personal tools and projects (JukiTech). It's being hosted on a standard MIT License and is available for anyone's use under MIT license rules, completely free of charge (citations/creditations are always welcome and appreciated!). This plugin is provided as is without any warranty.

For any feature requests or changes to the plugin, please reach out at saljukgondal@gmail.com. Issues/pull requests also welcome.

For more about me, visit https://saljuk.com
