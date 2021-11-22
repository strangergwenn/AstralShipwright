# Astral Shipwright

Astral Shipwright is a spaceship building game coming to Steam in 2022.

 - [Website](http:/astralshipwright.com)
 - [Store page](https://store.steampowered.com/app/1728180/Astral_Shipwright)

![Game screenshot](https://astralshipwright.com/gallery_data/2.jpg)

## Building the game

Game sources are intended for game customers and Unreal Engine developers. **You won't be able to run the game from this repository alone**, as the game contents are not included. Building from source is only useful if you want to replace the game executable with your own modifications.

### Dependencies
You will need the following tools to build from sources:

* Astral Shipwright uses Unreal Engine 5 as a game engine. You can get it for free at [unrealengine.com](http://unrealengine.com). You need to download the appropriate Engine Engine 5 version from sources on GitHub - check the .uproject file as text for the current version in the "EngineAssociation" field.
* [Visual Studio Community 2022](https://www.visualstudio.com/downloads) will be used to build the game. You need the latest MSVC compiler and the Windows 10 SDK.

### Build process
Follow those steps to build the game:

* In the Windows explorer, right-click *AstralShipwright.uproject* and pick the "Generate Visual Studio Project Files" option.
* An *AstralShipwright.sln* file will appear - double-click it to open Visual Studio.
* Select the "Shipping" build type.
* You can now build the game by hitting F7 or using the Build menu. This should take a few minutes.

The resulting binary will be generated as *Binaries\Win64\AstralShipwright-Win64-Shipping.exe* and can replace the equivalent file in your existing game folder.
