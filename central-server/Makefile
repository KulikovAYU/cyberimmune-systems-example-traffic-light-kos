environment:
	pipenv install -r requirements.txt

prepare: environment

# Запуск сервера с использованием pipenv
run:
	pipenv run python central-server.py

# Локальный запуск сервера без использования pipenv
local-run:
	python central-server.py