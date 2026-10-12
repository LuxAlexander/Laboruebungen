# SWD-LB-08: Implementation — Logging in Python/Django

**Course:** Software Design (SWD) Lab  
**Instructors:** Bernhard Danninger, Josef Lettner, Henrik Binggl  
**Version:** Updated for Current Codebase (2026)

---

## Goal of This Unit

> **You will be able to:** configure Python and Django logging, choose the right level per event and route each app's logs through its own named logger.
>
> **You hand in (as part of the final zip):**
> - the finished implementation of your two use cases
> - a `LOGGING` configuration with a formatter, a handler and a named logger for your app
> - log lines at the appropriate levels in your services and views
>
> The final submission is a zip file handed in before session 9, together with a short presentation.

---

## 1. Session Overview

| Unit | Topics | Tasks |
|---|---|---|
| 1 | Intro, Context Diagrams, Requirements, Demo Project | Modelling: project setting, context diagram, requirements. Coding: install tools and get starter project running |
| 2 | Use-Case-Diagrams | Modelling: 1 use-case diagram. Coding: get familiar with the demo project |
| 3 | Class-Diagrams & Unit-Tests | Modelling: 1 class diagram (business objects). Coding: start creating persistence layer and unit tests |
| 4 | Component-Diagrams | Modelling: 1 component diagram. Coding: start creating services/components on top of the models |
| 5 | Activity- & Sequence-Diagrams | Modelling: 1 activity & 1 sequence diagram. Coding: implement the service logic |
| 6 | Implementation | Rework the design. Coding: implement the frontend (Model-View-Template) and the UI unit tests |
| 7 | Design-Patterns | Identify and apply the design patterns in your implementation |
| 8 | Implementation | Finish implementation, add logging |
| 9 | Outlook | Short presentation. Final project submission (as zip file before session) |
| 10 | Test | — |

> **Note:** This is the overall planning for the lab. The order and the contents of the
> individual sessions may still change as the lab progresses — always follow the current
> handout and the announcements on Moodle.

---

## 2. Logging in Software Development

### 2.1 What Logging Is
[Logging](https://www.merriam-webster.com/dictionary/log) captures and monitors events during execution of a program. Logs are stored for later review — in a file, a database, or a remote system.

- Especially important for **headless** processes (no UI to show errors) and **asynchronous** work
- Standard for every modern technology; multiple libraries and toolkits are available
- [Headless software (Wikipedia)](https://en.wikipedia.org/wiki/Headless_software)

### 2.2 The Simplest Approach
The simplest form of logging is printing to `STDOUT` (e.g. `print(...)`). Its disadvantage: `STDOUT` is ephemeral and needs to be captured by something else. Writing to a destination (a file, a remote system) is better — and this problem has already been solved by logging frameworks.

### 2.3 Common Characteristics of Modern Logging Systems

| Concern | What it covers |
|---|---|
| **Log levels** | Control verbosity: `TRACE` / `DEBUG` / `INFO` / `WARN` / `ERROR` / `FATAL`. Filter messages by severity threshold |
| **Loggers / categories** | Group logs by component / module / class. Hierarchical naming (e.g. `app.db.connection`). Independent configuration for each part |
| **Handlers / appenders / sinks** | Console, file, rolling file. Remote log servers / collectors. Multiple outputs simultaneously |
| **Formatters** | Configurable layout (timestamp, level, logger, message). Support for JSON / structured logging. Correlation IDs / request IDs |
| **Configuration** | Central config via file, code or environment. Environment-specific settings (dev / staging / prod). Hot-reload supported in many frameworks |
| **Contextual logging** | Attach metadata (user / session / transaction IDs). Thread- or request-local context |
| **Performance** | Async logging, buffered I/O, control log volume to avoid overhead |

---

## 3. Python's Built-in Logging

Python ships a logging module in the standard library. It provides the building blocks listed above:

- [Python `logging` documentation](https://docs.python.org/3/library/logging.html)

| Concept | Python `logging` |
|---|---|
| Log levels | `DEBUG`, `INFO`, `WARNING`, `ERROR`, `CRITICAL` |
| Loggers | `logging.getLogger("name")`, hierarchical by dotted name |
| Handlers | `StreamHandler`, `FileHandler`, `RotatingFileHandler`, ... |
| Formatters | `logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")` |
| Configuration | `logging.basicConfig(...)` or `logging.config.dictConfig(...)` |

A logger is created per module using its dotted name. Because the name is hierarchical, a logger named `orders` is the parent of `orders.services`, so configuration on `orders` propagates to its children unless they override it.

```python
import logging

logger = logging.getLogger(__name__)   # __name__ is the module's dotted path
logger.debug("detailed information, only visible when the level is DEBUG")
logger.info("something expected happened")
logger.warning("something unexpected, but the program still works")
logger.error("a real problem")
logger.critical("the program cannot continue")
```

---

## 4. Django Logging

### 4.1 Django Uses and Extends Python's Logging
Django does not invent a new logging system. It configures Python's `logging` module through the `LOGGING` setting in `settings.py` and adds a few handlers of its own (e.g. `AdminEmailHandler`).

- [Django logging documentation](https://docs.djangoproject.com/en/stable/topics/logging/)
- [Python logging documentation](https://docs.python.org/3/library/logging.html)

### 4.2 Configuring Logging in `settings.py`
The demo project defines a `LOGGING` dictionary in `swd_django_demo/settings.py`. It uses the standard `dictConfig` schema with formatters, handlers and loggers:

```python
# swd_django_demo/settings.py
LOGGING = {
    "version": 1,
    "disable_existing_loggers": False,
    "formatters": {
        "verbose": {
            "format": "%(asctime)s - %(name)s - %(levelname)s - %(message)s",
        },
    },
    "handlers": {
        "console": {
            "class": "logging.StreamHandler",
        },
        "console-verbose": {
            "class": "logging.StreamHandler",
            "formatter": "verbose",
        },
    },
    "loggers": {
        "django": {
            "handlers": ["console"],
            "level": "INFO",
        },
        # Configure custom logger for the app
        "orders": {
            "handlers": ["console-verbose"],
            "level": "DEBUG",
        },
    },
}
```

The configuration has four parts:

| Part | What it does | Example in the project |
|---|---|---|
| `formatters` | Define the layout of a log line | `"verbose"` adds timestamp, logger name, level and message |
| `handlers` | Define where logs go and which formatter they use | `"console"` (plain) and `"console-verbose"` (formatted) both write to the console via `StreamHandler` |
| `loggers` | Named loggers, each with its own handlers and level | `"django"` logs at `INFO` to the plain console; `"orders"` logs at `DEBUG` to the formatted console |
| `disable_existing_loggers` | Keep Django's default loggers alive | Set to `False` so other framework loggers still work |

### 4.3 Environment-Specific Configuration
The project keeps a separate `swd_django_demo/settings_prod.py` for production. It starts with `from .settings import *`, so it inherits the `LOGGING` block, and then overrides only the production-only settings (`DEBUG = False`, a MariaDB database, `ALLOWED_HOSTS`, secrets from the environment). This lets logging be tuned per environment without touching the development settings — for example, raising the `orders` level from `DEBUG` to `INFO` in production:

```python
# swd_django_demo/settings_prod.py
LOGGING["loggers"]["orders"]["level"] = "INFO"
```

### 4.4 App-Based Logging
Each app creates a **named logger** and uses it throughout its services and views. The demo project keeps the logger in a small `logging` package inside the `orders` app:

```python
# orders/logging/__init__.py
import logging

logger = logging.getLogger(__name__.split('.')[0])
```

`__name__` is `"orders.logging"`, so `__name__.split('.')[0]` is `"orders"`. This matches the `"orders"` logger configured in `settings.py`, so the app's logs are routed by that configuration.

### 4.5 Producing Logs
The `orders` service and views import that logger and emit log lines at different levels. Debug calls trace the flow through the service; an info call records an expected business outcome (an order rejected by a business rule):

```python
# orders/services.py
from orders.logging import logger

class OrderService:
    def __init__(self, product_service: ProductCatalog, customer_service: Customers):
        logger.debug(f"OrderService.__init__(product_service: {product_service}, "
                     f"customer_service: {customer_service}")
        self.product_service = product_service
        self.customer_service = customer_service

    def create_order(self, customer: Customer, basket: Basket) -> Order:
        logger.debug(f"OrderService.create_order(customer: {customer}, basket: {len(basket)} items)")
        ...
```

```python
# orders/views.py
from orders.logging import logger

@login_required
@require_POST
@inject
def place_order(request, order_service: OrderService = Provide["order_service"]) -> HttpResponse:
    try:
        ...
    except OrderError as e:
        # expected business outcomes are shown to the customer; anything else is a bug and surfaces as 500
        logger.info(f"order rejected: {e}")
        messages.error(request, str(e))
```

> **f-strings vs. lazy formatting.** The demo project writes `logger.debug(f"...{value}")`, which is
> easy to read but builds the string **even when the level is switched off**. The standard-library
> style passes the values as arguments instead, so the formatting only happens if the line is
> actually emitted:
>
> ```python
> logger.debug("create_order(customer: %s, basket: %s items)", customer, len(basket))
> ```
>
> For a lab project the f-string is fine. In production code, or anywhere in a hot path, prefer the
> `%s` form. Use it in your own project — it costs nothing and it is what reviewers expect.

### 4.6 Log Levels in Practice

| Level | Used for | Example in the project |
|---|---|---|
| `DEBUG` | Detailed flow tracing, only visible at `DEBUG` level | `logger.debug(f"OrderService.create_order(...)")` |
| `INFO` | Expected, significant events | `logger.info(f"order rejected: {e}")` |
| `WARNING` | Something unexpected, but the program still works | (not used in the demo — add for your project) |
| `ERROR` | A real problem, some functionality failed | (not used in the demo — add for your project) |
| `CRITICAL` | The program cannot continue | (not used in the demo — add for your project) |

The `"orders"` logger is set to `DEBUG` in development, so all these lines appear in the console. In production you would typically raise the level to `INFO` so the `DEBUG` flow tracing is suppressed.

---

## 5. Adding Logging to Your Own App

### 5.1 Steps
1. Add a `LOGGING` block to `settings.py` (or reuse the existing one and add a logger for your app):
   ```python
   "loggers": {
       "myapp": {
           "handlers": ["console-verbose"],
           "level": "DEBUG",
       },
   }
   ```
2. Create a named logger in your app — a small `logging/__init__.py` keeps it in one place:
   ```python
   # myapp/logging/__init__.py
   import logging

   logger = logging.getLogger(__name__.split('.')[0])
   ```
3. Import the logger in your services and views and emit log lines at the right level:
   ```python
   from myapp.logging import logger

   def do_something(...):
       logger.debug(f"do_something entered with ...")
       ...
       logger.info(f"do_something completed: ...")
   ```
4. Choose the level per event: `DEBUG` for flow tracing, `INFO` for expected events, `WARNING`/`ERROR` for problems.

---

## 6. Running Tests

Logging does not affect test outcomes, but log lines appear in the test output when the level is set to `DEBUG`:

```bash
just test
# or
uv run python manage.py test -v 2
```

If the log output is noisy during tests, raise the logger level in a test settings file or filter it in the `LOGGING` configuration.

---

## 7. Assignment: Project Implementation

### 7.1 Tasks
- [ ] Complete the project implementation based on your chosen use cases
  - [ ] Use logging within your implementation
    - [ ] Add a `LOGGING` block to `settings.py` with a formatter, a handler and a logger for your app
    - [ ] Create a named logger in your app (e.g. `myapp/logging/__init__.py`)
    - [ ] Emit `debug` lines for flow tracing in your services
    - [ ] Emit `info` lines for expected business outcomes
    - [ ] Emit `warning` / `error` lines for unexpected conditions
    - [ ] Prefer lazy `%s` formatting over f-strings in logging calls
  - [ ] Consider environment-specific logging (e.g. `DEBUG` in dev, `INFO` in prod)

### 7.2 Key Concepts to Demonstrate
- **Log levels** to control verbosity
- **Named loggers** grouped per app
- **Handlers and formatters** for output destination and layout
- **Configuration** in `settings.py` via the `LOGGING` dictionary
- **Environment-specific** settings (dev vs. prod)

---

*Updated to match current djangolight codebase*  
*Last updated: 2026-09-06*
