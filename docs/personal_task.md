# Отчет по индивидуальному кафедральному заданию

**Формулировка:** "Реализация системы мониторинга безопасности на базе ОС Windows"

**Задачи:**
> - Настройка Windows Event Log для сбора и анализа событий безопасности (например, попытки входа, изменения настроек
системы, доступ к конфиденциальным данным).
> - Установка и настройка системы для мониторинга безопасности (например, OSSEC, AlienVault, использование встроенных
возможностей Windows для логирования).
> - Разработка плана реагирования на инциденты безопасности, создание сценариев для обработки событий (например,
автоматическое уведомление администратора о попытке несанкционированного входа).
> - Проведение тестирования: симуляция атак на систему с целью проверки реакции и эффективности системы мониторинга.

---

## Оборудование и ПО

- **Хостовая ОС**: macOS Sequoia
- **Гостевая ОС (VM)**: Windows 11 Pro
- **Система мониторинга**: Graylog 6.x _(в Docker)_
- **Агент логирования**: Winlogbeat 8.x
- **Инструменты**: Docker, PowerShell, Telegram, Event Viewer

---

## Выбор инструментов

### Почему Graylog?

**Graylog** — это мощная система логирования и корреляции событий, идеально подходящая для учебных и малых боевых 
инфраструктур. **Он позволяет:**

- Централизовать сбор логов из разных источников.
- Фильтровать и обрабатывать события по заданным правилам.
- Отправлять уведомления через Telegram, Email, Webhook.
- Использовать открытые стандарты (Beats, GELF, Syslog).
- Работать через понятный веб-интерфейс.

![изображение-пример интерфейса graylog](media/personal_task/graylog_example_screenshot.png)

### Почему Winlogbeat?

**Winlogbeat** — лёгкий и эффективный агент от Elastic для передачи логов Windows:

- Прост в установке и настройке.
- Поддерживает выбор конкретных журналов (Security, System и др.).
- Хорошо работает с Graylog через Logstash/Beats.
- Низкое потребление ресурсов.

![изображение github-репозитория winlogbeat](media/personal_task/winlogbeat_page.png)

---

## Ход работы

### 1. Настройка аудита в Windows

Через `secpol.msc` включены политики:

- Аудит входа в систему (успешный и неуспешный)
- Аудит использования привилегий
- Аудит создания/удаления пользователей
- Аудит изменений политик безопасности

> Это обеспечило генерацию событий: 4624, 4625, 4672, 4720, 4719.

![скриншот настроенного secpol в wm wim](media/personal_task/task_1/secpool_setup.png)

---

### 2. Развёртывание Graylog

**Graylog** развёрнут в **Docker** (с MongoDB и Elasticsearch), открыт порт 5044 для Beats. Использован 
**docker-compose**, что упростило управление сервисами.

![скриншот настроенного graylog](media/personal_task/task_2/graylog_done.png)

**Для начала**, согласно документации, нужно было сгенерировать ключи шифрования и вписать их в переменные среды для 
**docker**.

![скриншот настройки переменных среды для docker-а graylog](media/personal_task/task_2/ssh_setup.png)

**Далее**, нужно было развернуть **graylog** при помощи конфига **docker-compose**.

![скриншот запуска graylog через docker-compose](media/personal_task/task_2/graylog_startup.png)

**После чего**, используя пароль из логов **docker**, зайти в веб-интерфейс и выполнить первоначальную настройку.

![скриншот выполненной начальной настройки graylog](media/personal_task/task_2/graylog_startup_guide.png)

---

### 3. Установка Winlogbeat

На виртуальной Windows:

- Установлен **Winlogbeat**
- Настроен `winlogbeat.yml` (output → логстэш на хосте)
- Установлен как сервис
- Запущен

![скриншот настроенного winlogbeat](media/personal_task/task_3/winlogbeat_done.png)

**Для начала** нужно было скачать архив с **winlogbeat** с сайта и написать правильный конфиг для подключения к серверу 
**graylog** в **docker**.

![скриншот сделанного конфига для winlogbeat (часть 1)](media/personal_task/task_3/winlogbeat_config_1.png)

![скриншот сделанного конфига для winlogbeat (часть 2)](media/personal_task/task_3/winlogbeat_config_2.png)

Далее, нужно было через **powershell** установить **winlogbeat** как сервис в **Windows 11** и запустить.

![скриншот установки winlogbeat и его запуска](media/personal_task/task_3/winlogbeat_install.png)

---

### 4. Создание Stream'ов

Для каждого события был создан отдельный stream (для дальней реализации автоматических уведомлений в **telegram** через
**alert-ы**).

![скриншот настроенных stream-ов в graylog](media/personal_task/task_4/stream_done.png)

| Событие               | Event ID | Условие для алерта |
|-----------------------|----------|--------------------|
| Неудачные входы       | 4625     | >3 за 5 минут      |
| Успешный вход         | 4624     | >1 за 1 минуту     |
| Админский вход        | 4672     | каждое событие     |
| Создание пользователя | 4720     | каждое событие     |
| Изменение аудита      | 4719     | каждое событие     |

**Подробное описание событий и план реагирования:**

| Event ID | Название                  | Смысл события                             | Угроза                     | План реагирования                              |
|----------|---------------------------|-------------------------------------------|----------------------------|------------------------------------------------|
| 4625     | Неудачный вход            | Неправильный логин/пароль                 | Подбор пароля, brute-force | Алерт >3 за 5 мин, блокировка IP или учётки    |
| 4624     | Успешный вход             | Успешная авторизация                      | Неожиданный доступ         | Отслеживание активности, алерт по времени      |
| 4672     | Вход с привилегиями       | Вход с правами администратора             | Эскалация привилегий       | Алерт при каждом случае, проверка пользователя |
| 4720     | Создание пользователя     | Добавление нового локального пользователя | Внедрение злоумышленника   | Алерт, проверка кто создал, удаление           |
| 4719     | Изменение политики аудита | Изменены настройки безопасности           | Скрытие активности         | Алерт, восстановление, разбор по пользователю  |

**Сначала**, был настроен **input** для получения логов с виртуальной машины.

![скриншот настройки input в graylog](media/personal_task/task_4/input_setup.png)

**Далее**, для каждого из событий был создан отдельный **stream**, куда приходили бы нужные логи (согласно event_id).

![скриншот настроенного stream для успешного входа](media/personal_task/task_4/stream_success_login.png)
![скриншот настроенного stream для неудачного входа](media/personal_task/task_4/stream_failed_login.png)
![скриншот настроенного stream для входа с привилегиями](media/personal_task/task_4/stream_admin_login.png)
![скриншот настроенного stream для создания пользователя](media/personal_task/task_4/stream_create_user.png)
![скриншот настроенного stream для изменений в политике аудита](media/personal_task/task_4/stream_audit_change.png)

---

### 5. Настройка Telegram-уведомлений

- Создан бот в **@BotFather**
- Получен chat_id
- Настроено HTTP Notification:
    - URL: `https://api.telegram.org/bot<TOKEN>/sendMessage`
    - Метод: POST
    - Content-Type: application/json
    - Body:
      ```json
      {
        "chat_id": "123456789",
        "text": "🚨 Graylog Alert!\nStream: ${event.stream_title}\nEvent: ${event.event_definition_title}\nTime: ${event.timestamp}\n\nMessage:\n${event.message}"
      }
      ```

Ниже показано как в **graylog** были добавлены уведомления с **api telegram** и сделан проверочный **alert**.

![скриншот конфига для уведомлений в telegram](media/personal_task/task_5/notificator_config_1.png)
![скриншот конфига для уведомлений в telegram](media/personal_task/task_5/notificator_config_2.png)

---

### 6. Создание Alert'ов

Для каждого события был создан отдельный **alert** и настроена нотификация в **telegram** бот.

![скриншот со всеми готовыми alert-ами](media/personal_task/task_6/alert_done.png)

Ниже скриншоты с подробной настройкой для каждого **alert-а**.

![скриншот настройки alert-ов 1](media/personal_task/task_6/alert_1.png)
![скриншот настройки alert-ов 2](media/personal_task/task_6/alert_2.png)
![скриншот настройки alert-ов 3](media/personal_task/task_6/alert_3.png)
![скриншот настройки alert-ов 4](media/personal_task/task_6/alert_4.png)
![скриншот настройки alert-ов 5](media/personal_task/task_6/alert_5.png)
![скриншот настройки alert-ов 6](media/personal_task/task_6/alert_6.png)
![скриншот настройки alert-ов 7](media/personal_task/task_6/alert_7.png)
![скриншот настройки alert-ов 8](media/personal_task/task_6/alert_8.png)
![скриншот настройки alert-ов 9](media/personal_task/task_6/alert_9.png)
![скриншот настройки alert-ов 10](media/personal_task/task_6/alert_10.png)
![скриншот настройки alert-ов 11](media/personal_task/task_6/alert_11.png)
![скриншот настройки alert-ов 12](media/personal_task/task_6/alert_12.png)
![скриншот настройки alert-ов 13](media/personal_task/task_6/alert_13.png)
![скриншот настройки alert-ов 14](media/personal_task/task_6/alert_14.png)
![скриншот настройки alert-ов 15](media/personal_task/task_6/alert_15.png)
![скриншот настройки alert-ов 16](media/personal_task/task_6/alert_16.png)
![скриншот настройки alert-ов 17](media/personal_task/task_6/alert_17.png)
![скриншот настройки alert-ов 18](media/personal_task/task_6/alert_18.png)

---

### 7. Тестирование (симуляция атак)

- Ввод неверного пароля (4625)
- Вход с правами администратора (4672)
- Создание пользователя через **PowerShell**:
  `net user test P@ssword123 /add`
- Изменение политики аудита в `secpol.msc`

Все события были зафиксированы и переданы в Graylog, где сработали алерты и отправились уведомления в Telegram.

![скриншот с отображением всех логов](media/personal_task/task_7/test_all_messages.png)

Для проверки неудачного входа на виртуальную машину был установлен пароль и выполнено несколько неудачных попыток.
![скриншот теста с неудачным входом](media/personal_task/task_7/test_failed_login.png)

Для проверки удачного входа вышел из сеанса и выполнил вход заново.
![скриншот теста с удачным входом](media/personal_task/task_7/test_success_login.png)

Для проверки входа с привилегиями так же вышел из сеанса и вошел заново (работу вел из пользователя с правами админа).
![скриншот теста с входом админа 1](media/personal_task/task_7/test_admin_login_1.png)
![скриншот теста с входом админа 2](media/personal_task/task_7/test_admin_login_2.png)

Для проверки создания пользователя ввел в **powershell** соответствующую команду.
![скриншот с тестом создания пользователя](media/personal_task/task_7/test_create_user.png)

Для проверки изменений политик аудита несколько раз сделал правки в один из параметров через `secpol.msc`.
![скриншот с тестом политик аудита](media/personal_task/task_7/test_audit.png)

---

## Вывод
