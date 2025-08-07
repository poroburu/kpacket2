<div align="center">
    <img width="128" src="https://github.com/AshitaXI/Ashita/raw/master/repo/ashita.png" alt="ashita">
    </br>
</div>

<div align="center">
    <a href="https://discord.gg/Ashita"><img src="https://img.shields.io/discord/264673946257850368.svg?style=for-the-badge" alt="Ashita Discord Server" /></a>
    <a href="LICENSE.md"><img src="https://img.shields.io/badge/License-LGPL_v3-blue?style=for-the-badge" alt="license" /></a>
    <a href="https://ashitaxi.com/"><img src="https://img.shields.io/badge/Homepage-link-blue?style=for-the-badge" alt="link" /></a>
    <a href="https://docs.ashitaxi.com/"><img src="https://img.shields.io/badge/Documentation-link-blue?style=for-the-badge" alt="link" /></a>
</div>

# Example Plugin

This repository contains the source code to the current template used to create plugins for Ashita v4.

## Requirements

Building this plugin template requires the use of CMake. _(CMake 3.22 or newer currently.)_

  - `CMake` - [https://cmake.org/](https://cmake.org/)

We recommend using either of the following development environments to create and compile plugins:

  - `Visual Studio Code` - [https://code.visualstudio.com/](https://code.visualstudio.com/)
  - `Visual Studio 2022` - [https://visualstudio.microsoft.com/](https://visualstudio.microsoft.com/)

### Using Visual Studio Code

In order to make use of VSCode, you will need to install a few extensions to properly compile this code:

  - `C/C++` - [https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
  - `CMake` - [https://marketplace.visualstudio.com/items?itemName=twxs.cmake](https://marketplace.visualstudio.com/items?itemName=twxs.cmake)
  - `CMake Tools` - [https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)

### Using Visual Studio 2022 _(or newer)_

In order to make use of Visual Studio 2022 _(or newer)_, you need to make sure that you install the following optional packages:

  - `Desktop development with C++`
  - `C++ CMake tools for Windows`

_Additional runtimes and platform targets are optional and up to you on which you wish to support. Please note that Ashita is a 32bit (x86) application and environment. All plugins must be compiled as 32bit (x86) as well!_

### Other Environments

While we do recommend using VSCode or VS2022+ in order to develop plugins for Ashita, it is not required. You may use any kind of environment that you prefer. Building plugins also does not require CMake and is simply a recommandation while following and using this plugin template.

Plugins can be built using any compiler that supports the following:

  - Must compile to 32bit (x86).
  - Must support exporting demangled function names.
  - Must support properly handling calling conventions. (ie. `__stdcall`, `__thiscall`, etc.)
  - Must support properly inheriting from C++ class objects. _(Properly implement interfaces/vtables.)_

## SDK Environment Variable Setup

To properly build this plugin template, it is required to have the environment variable `ASHITA4_SDK_PATH` set and pointed to your current Ashita v4 plugin sdk folder. This project includes a CMake package that helps automatically find and configure the Ashita v4 SDK for use by using this variable.

You can set this variable one of many ways such as:

  - As a global system environment variable.
  - As a user-specific environment variable.
  - As a local environment variable to the command line when building.

To add this environment variable at the user or system level on Windows, please follow these steps:

  1. Click `Start`.
  2. Type in `advanced system settings` and choose the option that shows. _(`View advanced system settings`)_
  3. If not already selected, click the `Advanced` tab at the top.
  4. Click the `Environment Variables` button near the bottom of the window.

Here you can decide if you wish to set this variable for just your local user or for the entire system.

  - To set the variable for just your user, follow the next steps under the `User variables for <your username>` section.
  - To set the variable for the entire system, follow the next steps under the `System variables` section.

  1. Click `New`.
  2. In the `Variable name` text box, enter: `ASHITA4_SDK_PATH`
  3. In the `Variable value` text box, enter the path to your Ashita SDK folder. _(This is located in the `/plugins/` folder where you have Ashita installed.)_
    - For example, `C:\Ashita\plugins\sdk\`
  4. Click `Ok` on all windows to save the changes.


This project template makes use of an expected environment variable `ASHITA4_SDK_PATH` to locate where you have the Ashita v4 SDK installed to. This path should point to your current Ashita v4 `/plugins/sdk/` folder location

# License

The Ashita plugin SDK, and thus, this plugins source code are both released under [GNU LGPL v3](LICENSE.md)
