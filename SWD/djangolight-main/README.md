# SWD Django Demo

A small web shop built with [Django](https://www.djangoproject.com/), used as the reference project for the
Software-Design (SWD) lab course. Customers browse a product catalog, fill a basket, place orders and pay with
their prepaid credit. The shop is deliberately simple; the point of the project is *how* it is built.

## 1. What this application is about

The project shows how to structure a Django application into loosely coupled apps that talk to each other
through small interfaces instead of importing each other's models.

| App | Responsibility |
|---|---|
| `core` | The seams between the apps: interfaces (`core/interfaces.py`), the records that cross them (`core/domain.py`), abstract base models, site root and health check. |
| `products` | The product catalog: model, repository (manager), the `ProductService` adapter, the listing page at `/products/` and the `seed_catalog` command. |
| `customers` | The customer, which is also the Django user model, and the `CustomerService` adapter for credit handling. |
| `orders` | The basket (session), order placement and the order history, with their pages under `/orders/`. |
| `swd_django_demo` | The Django project: settings, URL configuration and the dependency-injection container. |

Concepts demonstrated:

- **Interfaces as Protocols.** `ProductCatalog` and `Customers` are `typing.Protocol`s. An adapter satisfies
  them by having the right methods, no inheritance needed. Tests use in-memory stand-ins or mocks the same way.
- **Records instead of models across seams.** Views and the basket work with frozen dataclasses such as
  `CatalogItem`, `OrderLine` and `OrderSummary`. Only the adapters touch Django models. The docstring in
  `core/domain.py` explains why.
- **Dependency injection.** The container in `swd_django_demo/containers.py` wires the adapters into the views.
  It is built in `swd_django_demo/apps.py` once all apps are loaded. A good introduction is the video
  ["Dependency Injection, The Best Pattern"](https://www.youtube.com/embed/J1f5b4vcxCQ?si=Qb4pFAWWazOvzPjm); the
  library used is [dependency-injector](https://python-dependency-injector.ets-labs.org/examples/django.html).
- **One transaction owner.** `OrderService.create_order` prices the basket, redeems credit and writes the order in
  a single transaction. Business failures are typed exceptions (`InsufficientCredit`, `EmptyBasket`) that the
  view turns into messages.
- **Plain foreign keys at the data level.** `Order` and `OrderPosition` reference `settings.AUTH_USER_MODEL`
  and `"products.Product"` directly. The coupling between the apps is in the database schema only; the orders
  code itself works against the interfaces.

The vocabulary of the shop itself (catalog, basket, order line, credit, ...) is defined in
[CONTEXT.md](./CONTEXT.md). Code and documentation use those terms.

Further reading: [Business logic in a Django project](https://emcarrio.medium.com/business-logic-in-a-django-project-a25abc64718c),
the [official Django tutorial](https://www.djangoproject.com/start/) and [CS50 Web, Django](https://cs50.harvard.edu/web/2020/weeks/3/).

## 2. Prerequisites

### 2.1 A decent terminal

Software development means working with command-line tools, so a good shell and terminal matter.

- **Linux and macOS** ship a capable shell (bash, zsh) and terminal. Any of Terminal, iTerm2, Konsole,
  GNOME Terminal or Alacritty works well.
- **Windows**: use [PowerShell 7](https://github.com/PowerShell/PowerShell) inside
  [Windows Terminal](https://learn.microsoft.com/en-us/windows/terminal/), not the legacy console.
  Two settings save time:
  - Allow local scripts: `Set-ExecutionPolicy RemoteSigned -Scope CurrentUser`
  - If typing `python` opens the Microsoft Store, disable the
    [app execution alias](https://stackoverflow.com/questions/58754860/cmd-opens-windows-store-when-i-type-python).
- **Windows Defender** can slow down package installs and test runs considerably. Consider
  [excluding the project folder](https://support.microsoft.com/en-us/windows/add-an-exclusion-to-windows-security-811816c0-4dfd-af4a-47e4-c301afe13b26)
  from real-time scanning, and be aware of the security trade-off.

### 2.2 Tools

| Tool | Purpose | Install |
|---|---|---|
| [uv](https://docs.astral.sh/uv/) | Python version, virtual environment and package manager. It installs Python for you. | [Installation guide](https://docs.astral.sh/uv/getting-started/installation/), see 2.3 below |
| Python 3.14 | Runtime, pinned in `.python-version`. | `uv python install 3.14` (uv also does this on demand) |
| [just](https://github.com/casey/just) | Task runner, the `justfile` holds the common commands. Works the same on Linux, macOS and Windows. | [Installation guide](https://just.systems/man/en/packages.html), see 2.4 below |
| [Docker](https://www.docker.com/) with Compose | Only for the deployment exercise (MariaDB, gunicorn, nginx). | Docker Desktop or the engine of your distribution |

### 2.3 Installing uv

`uv` is a single binary as well. The standalone installer is the recommended way and works on every platform; it
places the binary in `~/.local/bin` (Linux, macOS) or `%USERPROFILE%\.local\bin` (Windows) and adds that directory
to your `PATH`. Package managers are an alternative, the full list is in the
[installation guide](https://docs.astral.sh/uv/getting-started/installation/).

| Platform | Command |
|---|---|
| Linux, macOS (installer) | `curl -LsSf https://astral.sh/uv/install.sh \| sh` |
| Windows (installer) | `powershell -ExecutionPolicy ByPass -c "irm https://astral.sh/uv/install.ps1 \| iex"` |
| macOS | `brew install uv` |
| Windows | `winget install --id astral-sh.uv --exact` (or `scoop install uv`) |
| Any platform with Rust | `cargo install --git https://github.com/astral-sh/uv uv` |

Do not install `uv` with `pip` into a project environment; it should live outside of any virtual environment.
Once installed, `uv self update` keeps it current (installer builds only, package manager builds update through
the package manager).

### 2.4 Installing just

`just` is a single binary with no runtime dependencies. Pick one of the options below, the full list is in the
[installation guide](https://just.systems/man/en/packages.html).

| Platform | Command |
|---|---|
| macOS | `brew install just` |
| Windows | `winget install --id Casey.Just --exact` (or `scoop install just`, `choco install just`) |
| Debian 13+, Ubuntu 24.04+ | `sudo apt install just` |
| Fedora | `sudo dnf install just` |
| Arch Linux | `sudo pacman -S just` |
| Alpine | `sudo apk add just` |
| Any platform with Rust | `cargo install just` |

If your package manager ships an old version or none at all, download a prebuilt binary into a directory on your
`PATH`, for example:

```bash
mkdir -p ~/.local/bin
curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash -s -- --to ~/.local/bin
```

Check the setup:

```bash
uv --version
just --version
```

## 3. How to run the application

### 3.1 With just

Run `just` without arguments to list the recipes.

| Recipe | What it does |
|---|---|
| `just migrate` | Creates or updates the database tables (SQLite file `db.sqlite3` in development). |
| `just adminuser` | Creates an admin user interactively. |
| `just seed` | Loads the product catalog from `products/data/catalog.json`. Safe to re-run, items are matched by name. |
| `just test` | Runs the test suite verbosely. |
| `just test-mariadb` | Runs the test suite inside the app image against MariaDB, like production (needs Docker). Accepts test labels, e.g. `just test-mariadb customers`. |
| `just runserver` | Starts the development server on `http://localhost:8000`. |
| `just compose` | Collects static files and starts the container stack (MariaDB, app, nginx) on `http://localhost:8000`. |

First start:

```bash
uv sync           # creates .venv and installs the dependencies
just migrate
just adminuser
just seed
just runserver
```

Then open `http://localhost:8000`. The site root redirects to the shop. Log in with the admin user to place
orders and to see the order history; the admin site is at `http://localhost:8000/admin`.

### 3.2 The Django commands behind the recipes

Every recipe is a `manage.py` call, run through `uv run` so the virtual environment does not need to be activated.

```bash
uv run python manage.py migrate
uv run python manage.py createsuperuser
uv run python manage.py seed_catalog [--file path/to/items.json]
uv run python manage.py test -v 2
uv run python manage.py runserver
uv run python manage.py showmigrations      # which migrations are applied
uv run python manage.py makemigrations      # after a model change
uv run python manage.py shell               # a Python shell with Django loaded
```

The catalog file is a plain JSON list; each item has `name`, `description` and `price`:

```json
[{"name": "Espresso Cup", "description": "Porcelain cup, 80 ml", "price": 7.90}]
```

Prices are read as `Decimal` (exact cents), not as binary floats.

Customers need credit to order. Set it in the admin site on the customer record.

### 3.3 Containers (deployment exercise)

`containers/compose.yaml` runs the application the way it would be deployed: MariaDB as the database, the app
behind gunicorn, nginx serving static files and proxying requests. `containers/initial_data.sql` seeds MariaDB with
the schema, the five catalog products and the admin user (`admin` / `admin`, credit 100).

```bash
just compose
```

After model changes the SQL file must be regenerated; the recipe is in its header comment.

The container uses `swd_django_demo/settings_prod.py`, which extends `settings.py`. Its secrets come from the
environment: `DJANGO_SECRET_KEY`, `DJANGO_DB_PASSWORD` and `DJANGO_SUPERUSER_PASSWORD`. `compose.yaml` has
local-only defaults for them; set your own values in the shell or in `containers/.env` (ignored by git and by
`.dockerignore`). The defaults are a deliberate lab convenience so that `just compose` works without any setup;
they also mean that the "refuse to start without secrets" check of `settings_prod.py` never triggers under compose.
A real deployment must not rely on them.

### 3.4 Upgrading an existing checkout after a migration reset

The migration history of `customers`, `products` and `orders` was reset to a single `0001_initial` per app
(removal of the swappable `ProductBase`/`CustomerBase` models, money as `Decimal`). A database created with the
old migrations considers the new `0001_initial` as already applied, so `migrate` reports *No migrations to apply*
and the first request fails with `no such table`. Recreate the database:

```bash
rm db.sqlite3     # PowerShell: Remove-Item db.sqlite3
just migrate
just adminuser
just seed
```

The container database is rebuilt from `containers/initial_data.sql` on every `just compose`, because the recipe
runs `docker compose down` first. If you started the stack by hand, run
`docker compose -f ./containers/compose.yaml down` before starting it again.

## 4. How to develop this application

### 4.1 Environment

`uv sync` creates the virtual environment in `.venv` and installs everything from `uv.lock`. Prefer
`uv run <command>` over activating the environment; if you want a shell with it activated, use
`source .venv/bin/activate` (Linux, macOS) or `.\.venv\Scripts\Activate.ps1` (Windows).

Dependency groups: the default group holds the runtime, `dev` the development tools, `prod` gunicorn and the
MariaDB driver. `uv sync --group prod` installs the latter, which needs the MariaDB client headers on the host.

Add a dependency with `uv add <package>`, update all with `uv lock --upgrade` followed by `uv sync`.

### 4.2 Where things are

```
core/interfaces.py        ProductCatalog, Customers       (Protocols)
core/domain.py            CatalogItem, CustomerInfo, OrderLine, OrderSummary, order errors
products/services.py      ProductService     adapter for ProductCatalog
customers/services.py     CustomerService    adapter for Customers
orders/basket.py          Basket             session state and pricing
orders/services.py        OrderService       order placement and history
products/views.py         the catalog page, dependencies injected via Provide[...]
orders/views.py           basket, order placement and order history pages
swd_django_demo/containers.py   the DI container
swd_django_demo/apps.py         builds and wires the container on startup
```

Rules of thumb when extending the project:

- An app talks to another app only through `core.interfaces` and `core.domain`, never through its models.
- A new capability at a seam is added to the Protocol first, then to every adapter, then to its callers.
- Business failures are exceptions derived from `OrderError`; views catch that one name.
- Model changes need a migration (`makemigrations`) and, for the container exercise, a regenerated
  `containers/initial_data.sql`.

### 4.3 Testing

Tests are discovered by Django's test runner from every `test*.py` module, run with `just test`. To run a subset,
give a dotted label:

```bash
uv run python manage.py test orders
uv run python manage.py test orders.test_basket.TestBasketLines
```

The suite mixes three styles on purpose, each testing through the interface of a module:

- **Pure unit tests** (`orders/test_basket.py`): plain `unittest`, a dict as session, an in-memory
  `ProductCatalog`. No database, no patching.
- **Adapter tests on SQLite** (`products/test_services.py`, `customers/test_service.py`, `orders/test_service.py`):
  Django `TestCase`, real models, mocks only for the neighbouring interface.
- **View tests** (`orders/test_ui.py`): the Django test client against the templates, with a test container
  wired into the views to replace the catalog.

Every adapter has a test asserting it satisfies its Protocol, so an incomplete adapter fails a test instead of
failing at runtime. `customers/test_concurrency.py` shows credit redemption under concurrent access. SQLite has
no row locks (concurrent writers fail with *database is locked*), so it only checks that no redemption is lost;
the strict check — exactly ten of eleven redemptions fit into the credit — runs on MariaDB with `just test-mariadb`.

### 4.4 IDE

- **VS Code**: the repository ships settings, launch configurations and extension recommendations in `.vscode/`.
  Unit tests appear in the Test Explorer: `.vscode/django-tests.env` sets `MANAGE_PY_PATH`, which makes the Python
  extension discover and run tests through `manage.py test` instead of plain `unittest` (see the
  [VS Code docs](https://code.visualstudio.com/docs/python/testing#_django-unit-tests)). The Django server can be
  started with the debugger.
- **PyCharm**: use a 2025 release or newer for debugging. After `uv sync`, add an interpreter under
  Settings, Python, Interpreter, Add Local Interpreter, type *uv*, existing environment, pointing to
  `.venv/bin/python` (or `.venv\Scripts\python.exe` on Windows).

### 4.5 Starting a Django project from scratch

For reference, the steps this project began with:

```bash
uv init --python 3.14
uv add django
uv run django-admin startproject swd_django_demo .
uv run python manage.py startapp <app_name>
```

### 4.6 Platform-specific line=endings

Line-Endings are treated differently on Platforms (Windows vs. Mac vs. Linux). To ensure the proper handling of file-endings git provides the possibility to define the handling in `.gitattributes`. If the git attributes were added later, existing clones need to be `renormalized`.

```bash
# Windows users must re-normalize their existing checkout once
git add --renormalize .
git checkout -- .
# verify: git ls-files --eol containers/django-app/entrypoint.sh  -> w/lf
```

## License

[MIT](./LICENSE)
