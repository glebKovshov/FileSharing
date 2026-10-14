# FileSharing 2

Кроссплатформенное приложение для обмена файлами в локальной сети на Qt 6
(PySide6). Поддерживает Windows, Linux и macOS; .NET и Windows Forms не нужны.

## Запуск из исходников

Нужен Python 3.11 или новее.

```sh
python -m venv .venv
# Linux / macOS
.venv/bin/python -m pip install -e '.[dev]'
.venv/bin/python -m filesharing.app
# Windows
.venv\Scripts\python -m pip install -e ".[dev]"
.venv\Scripts\python -m filesharing.app
```

Предыдущая версия C++/CLI сохранена в истории Git (тег `v1.0`).
