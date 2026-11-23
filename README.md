# UnrealProspero

Unreal Engine 5.8.0 platform extension for PS5 via [AnyDevKit](https://github.com/boykopovar/AnyDevKit).

## Status

![UnrealGame running on PS5](https://github.com/user-attachments/assets/a09e0e82-a9d7-4a5c-8fbe-cb0748f92c80)

[UnrealGame](https://github.com/unicorn2022/UE5.8.0/blob/bbba9c6afb546b9053ce9eafa2d4973a71aefaa0/Engine/Source/UnrealGame.Target.cs) built with this extension runs on PS5.

## How it works

Adds PS5 as a build target without modifying Unreal Engine source. A custom RHI translates rendering commands to AGC. [AnyDevKit](https://github.com/boykopovar/AnyDevKit) provides the toolchain, runtime and packaging. HLSL shaders are compiled offline to SPIR-V, translated to native PS5 GPU code by [opengnm-psbc](https://github.com/boykopovar/opengnm-psbc), and packaged for AGC by AnyDevKit. Applications run directly on PS5 using its system libraries.

## License

This project is licensed under the GNU General Public License version 2 only.
