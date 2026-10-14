# FileSharing 2

Приложение для обмена файлами в локальной сети на **C++17 и Qt 6**.
Windows, Linux и macOS. Без Python, .NET, C++/CLI и Windows Forms.

## Сборка

Нужны CMake 3.21+, компилятор C++17 и Qt 6.2+ (Core, Network, Widgets, Test).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

На Linux: `./build/FileSharing`. На macOS: `open build/FileSharing.app`.
На Windows: `build\Release\FileSharing.exe` (для генератора Visual Studio).
Укажите `-DCMAKE_PREFIX_PATH=/путь/к/Qt` при необходимости.

Предыдущая версия C++/CLI сохранена в истории Git (тег `v1.0`).
