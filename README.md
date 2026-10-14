<img src="assets/icon.svg" width="64" height="64" alt="FileSharing">

# FileSharing 2

Простой обмен файлами между компьютерами в локальной сети.
Приложение полностью написано на **C++17 и Qt 6**: Linux, Windows и macOS.
Python, .NET, C++/CLI и Windows Forms для запуска не нужны.

## Установка

Готовые установщики доступны в [GitHub Releases](https://github.com/glebKovshov/FileSharing/releases/latest).
Откройте раздел **Assets** и скачайте файл для своей платформы:

| Платформа | Пакет | Установка |
| --- | --- | --- |
| Windows 10/11, x64 | `.exe` | Мастер установки, ярлыки и удаление через настройки ОС |
| macOS 12+, Intel / Apple Silicon | `.dmg` | Перетащите FileSharing в Applications |
| Linux x64, glibc 2.35+ | `.AppImage` | `chmod +x FileSharing-*.AppImage`, затем запустите файл |
| Ubuntu / Debian, x64 / ARM64 | `.deb` | `sudo apt install ./filesharing_*.deb` |
| Fedora / RPM-дистрибутивы | `.rpm` | `sudo dnf install ./filesharing-*.rpm` |

Windows и macOS пакеты включают Qt; AppImage включает библиотеки Qt.
DEB/RPM устанавливают зависимости через пакетный менеджер. Пакет должен
соответствовать архитектуре и доступным библиотекам вашего дистрибутива.
ARM64 DEB собирается на Ubuntu 24.04; x64 DEB/AppImage — на Ubuntu 22.04.
Нельзя обещать один бинарный файл для всех дистрибутивов и процессоров.
Для других архитектур используйте сборку из исходников.

macOS-пакет имеет локальную ad-hoc подпись; сертификат Apple Developer и
notarization пока не настроены. Первый запуск скачанного пакета может потребовать
подтверждения через настройки безопасности macOS. Если Linux не поддерживает
FUSE, запускайте AppImage с `APPIMAGE_EXTRACT_AND_RUN=1`.

## Как передать файл

1. Подключите оба компьютера к одной Wi-Fi или Ethernet-сети.
2. На отправителе выберите или перетащите файл и нажмите **Начать раздачу**.
3. На получателе откройте **Получить**, выберите устройство и нажмите **Получить файл**.
4. Проверьте имя, размер и папку, затем подтвердите приём.

По умолчанию файлы сохраняются в `Загрузки/FileSharing`. Папку можно изменить;
выбор запоминается. Кнопка **Открыть папку** появляется после успешного приёма.
Существующие файлы сохраняются: повторная копия получает суффикс `(1)`, `(2)` и т. д.

Отмена и обрыв сети удаляют неполный файл. Передача проверяется SHA-256;
отправитель показывает успех после подтверждения сохранения получателем.
Файл перестаёт раздаваться после завершения передачи; при необходимости нажмите
**Начать раздачу** снова. Передача после обрыва запускается заново.

Для поиска используется UDP 12345, для файла — динамический TCP-порт.
Разрешите приложению работу в локальной сети в брандмауэре и, на macOS,
разрешите доступ к локальной сети. Гостевые Wi-Fi-сети с изоляцией клиентов
могут блокировать поиск. Приложение рассчитано на доверенную локальную сеть:
передача не шифруется, устройства не аутентифицируются.
Оба устройства должны использовать версию 2; версия 1 несовместима с новым протоколом.

## Сборка из исходников

Нужны CMake 3.21+, компилятор C++17 и Qt 6.2+ (Core, Network, Widgets, Test).
На Windows используйте MSVC 2022 и соответствующий Qt SDK; на macOS — Xcode
Command Line Tools и Qt SDK. На Ubuntu / Debian:

```sh
sudo apt install build-essential cmake ninja-build qt6-base-dev libqt6svg6-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/FileSharing
```

Для Visual Studio добавьте `--config Release` к сборке; программа будет
в `build/Release/FileSharing.exe`. На macOS: `open build/FileSharing.app`.
При необходимости укажите `-DCMAKE_PREFIX_PATH=/путь/к/Qt`.
Без тестов: `-DBUILD_TESTING=OFF` (Qt Test тогда не требуется).

Упаковка: `cpack --config build/CPackConfig.cmake -C Release -B artifacts`.
Windows требует NSIS и `windeployqt`, macOS — `macdeployqt`, Linux — `dpkg-dev`
и `rpm`. AppImage: `packaging/appimage.sh build artifacts` (Linux x64).

[Протокол и защита от повреждения данных](docs/protocol.md) ·
[Проверки и тестирование](docs/testing.md).
Предыдущая версия C++/CLI сохранена в истории Git (тег `v1.0`).
