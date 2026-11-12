# Project Structure (starter: SWD Django Demo Shop)

Scan of the starter project in `djangolight-main/`, as it is today (a coffee-gear web shop with prepaid credit).
Purpose of this file: know what exists, so we can change it step by step into our Ticketing (Soccer) system.
Our requirements are in [PROJECT.md](./PROJECT.md). The shop vocabulary is in [CONTEXT.md](./CONTEXT.md) and
the author's own overview with diagrams is in [PROJECT_OVERVIEW.md](./PROJECT_OVERVIEW.md).

Stack: Python 3.14, Django 6.1, `dependency-injector`, SQLite in development, MariaDB + gunicorn + nginx in
containers, `uv` (packages) and `just` (tasks). `djangorestframework` and `marshmallow` are installed but unused.
About 2,500 lines in total, 65 tests.

## 1. Directory tree

```
djangolight-main/
├── manage.py
├── pyproject.toml            dependencies (uv), setuptools app list (must name every new app)
├── uv.lock, .python-version  locked deps, Python 3.14
├── justfile                  recipes: migrate, adminuser, seed, test, test-mariadb, runserver, compose
├── db.sqlite3                development database (not tracked)
├── README.md                 how to run and develop the demo
├── CONTEXT.md                domain glossary of the SHOP (to be replaced by a ticketing glossary)
├── PROJECT_OVERVIEW.md       architecture write-up with Mermaid diagrams
├── PROJECT.md                OUR requirements and task tracker
├── PROJECT_STRUCTURE.md      this file
├── LICENSE, .editorconfig, .gitattributes, .gitignore, .dockerignore
├── .vscode/                  launch/settings, django-tests.env (Test Explorer via manage.py)
│
├── swd_django_demo/          the Django PROJECT (settings, wiring)
│   ├── settings.py           INSTALLED_APPS, AUTH_USER_MODEL, SQLite, LOGGING, TIME_ZONE Europe/Vienna
│   ├── settings_prod.py      container settings: secrets from env, MariaDB
│   ├── urls.py               "" -> core, products/, orders/, admin/
│   ├── apps.py               ProjectConfig.ready(): builds the DI container and wires views
│   ├── containers.py         DI container: product_service, customer_service, order_service
│   ├── wsgi.py, asgi.py, run_gunicorn.py
│   └── test_*.py             DI, settings modules, smoke tests
│
├── core/                     SHARED SEAMS (no business data of its own)
│   ├── interfaces.py         Protocols: ProductCatalog, Customers
│   ├── domain.py             frozen dataclasses CatalogItem, CustomerInfo, OrderLine, OrderSummary;
│   │                         exceptions OrderError, EmptyBasket, InsufficientCredit
│   ├── models.py             abstract Product (name, description), abstract Customer (AbstractUser)
│   ├── managers.py           CoreProductManager (empty)
│   ├── views.py              /health (DB probe, 200 or 503)
│   ├── index_view.py         "/" redirects to products:index
│   ├── urls.py               "", health, logout
│   ├── templates/core/base.html   Bootstrap layout, navbar, basket badge, login/logout
│   └── tests.py
│
├── products/                 CATALOG
│   ├── models.py             Product(CoreProduct) + price Decimal
│   ├── managers.py           ProductManager: get_by_id, get_by_name, get_by_price_range
│   ├── services.py           ProductService = adapter for ProductCatalog
│   ├── views.py              index (list all products), uses Provide["product_service"]
│   ├── urls.py               "" -> index
│   ├── seeding.py            read_catalog() + idempotent import (matched by name, Decimal prices)
│   ├── management/commands/seed_catalog.py
│   ├── data/catalog.json     5 sample products
│   ├── templates/products/index.html
│   └── test_*.py             models, seeding, services, views
│
├── customers/                CUSTOMER = Django user
│   ├── models.py             Customer(CoreCustomer) + credit Decimal (null)
│   ├── managers.py           CustomerManager: get_by_username
│   ├── services.py           CustomerService = adapter for Customers: redeem_credit (row lock), find_customer
│   ├── admin.py              admin for the user model
│   ├── views.py              empty
│   └── test_service.py, test_concurrency.py
│
├── orders/                   BASKET + ORDERS
│   ├── models.py             Order(user, total_price, created_at), OrderPosition(order, pos, product, quantity, price)
│   ├── basket.py             Basket: session list of product ids, prices into OrderSummary
│   ├── services.py           OrderService: get_order_summary, get_orders, create_order (one transaction)
│   ├── views.py              add_to_basket, basket_overview, place_order, clear_basket, order_list
│   ├── urls.py               /orders/add_to_basket, basket_overview, place_order, clear_basket, my
│   ├── context_processors.py basket_count for every template
│   ├── logging/__init__.py   app logger
│   ├── templates/orders/     basket_overview.html, order_list.html
│   └── test_basket.py, test_service.py, test_ui.py
│
├── containers/               DEPLOYMENT: compose.yaml, compose.test.yaml (MariaDB tests),
│   │                         django-app/Dockerfile + entrypoint.sh, nginx/, initial_data.sql
├── static/                   Bootstrap 5, bootstrap-icons, application.css, theme.js, favicons
└── doc/lab_instructions/     SWD-Project-Ticketing.pdf (assignment) and .docx (our report)
                              (the SWD-LB-01..08 lab guides are tracked in git but deleted from the working tree)
```

## 2. Architecture in one picture

```
Browser -> urls.py -> views (@inject) -> services (adapters) -> managers (repositories) -> models -> DB
                          |                   ^
                          |                   | implements
                          +--> core.interfaces (Protocols) + core.domain (frozen records)
```

Rules the starter follows (and we should keep):
1. Apps talk to each other only through `core.interfaces` (Protocols) and `core.domain` (frozen dataclasses),
   never through each other's models. Only adapters touch models.
2. Views get their services injected with `Provide["name"]`. The container is in `swd_django_demo/containers.py`
   and is wired in `swd_django_demo/apps.py` (the module list there must include every view module that injects).
3. Business failures are typed exceptions derived from one base (`OrderError`); the view catches the base and
   shows a message.
4. One service method owns a transaction (`create_order`). Locks via `select_for_update()` inside
   `transaction.atomic()` (`CustomerService.redeem_credit`).
5. Money is `Decimal` everywhere.
6. Data entry is by seed command and admin, no CRUD UI.
7. Foreign keys between apps are plain FKs (`settings.AUTH_USER_MODEL`, `"products.Product"`).

Request flow of the existing purchase: `add_to_basket` (session) -> `basket_overview` (price via catalog) ->
`place_order` -> `OrderService.create_order` -> `redeem_credit` (locked) -> `Order` + `OrderPosition` rows.

## 3. Mapping: shop -> ticketing

| Shop (today) | Ticketing (ours) | Action |
|---|---|---|
| `products.Product` (name, description, price) | `Match` plus `Seat`; price comes from a pricing rule (sector x competition type), not from a field | replace |
| `core.models.Product` (abstract) | not needed (or an abstract base for stadium entities) | remove or repurpose |
| `customers.Customer` (+ credit) | `Customer` (login, optional for anonymous counter sales); credit is not needed | keep user model, drop `credit` |
| `CustomerService.redeem_credit` | payment simulation (cash or card) and the seat-lock pattern | adapt, keep the `select_for_update` idea |
| `orders.Basket` (session, product ids) | seat selection for one match, max. 4 seats per person | adapt |
| `orders.Order` / `OrderPosition` | `Ticket` (bound to one fixed seat, single-match or season) and a purchase record | replace |
| `OrderService.create_order` | `TicketService.purchase`: re-check availability, lock, create tickets, update statistics, all in one transaction | adapt |
| `InsufficientCredit`, `EmptyBasket` | `SeatNotAvailable`, `TicketLimitExceeded`, `PaymentFailed`, `EntryDenied` family | replace |
| `ProductCatalog` Protocol | `MatchCatalog` (upcoming matches, seat availability per sector) | replace |
| `Customers` Protocol | keep, drop `redeem_credit`; add a `Payments` Protocol | adapt |
| `seed_catalog` + `catalog.json` | seed stadium (8 sectors, 32,000 seats), teams, competitions, matches, season tickets | replace |
| `core/views.py` health check, `index_view`, `base.html` | keep as they are, rename the shop branding | keep |
| containers, justfile, static, settings | keep; add new apps to `INSTALLED_APPS` and to `pyproject.toml` | keep, extend |

## 4. Suggested target layout

Keeps the starter's app pattern (models, managers, services, views, urls, tests, seed) and our report's components
(Data Management, Core, Customer, Ticket, Orders, Security Alert).

```
core/             interfaces.py (MatchCatalog, Customers, Payments, SecurityAlerts), domain.py (SeatInfo,
                  TicketInfo, EntryResult, errors), health, base.html            [exists, rewrite contents]
stadiums/         NEW  Stadium, Sector, Seat, Team, Competition, Match(status), seed command + fixtures
customers/        keep  Customer (user); optional season-ticket owner link
tickets/          NEW  Ticket (seat, match or competition, type, number), pricing rules, availability,
                  TicketService, seat selection (basket)  [replaces products + part of orders]
orders/           KEEP/ADAPT  purchase flow, payment simulation, order history  [or merge into tickets]
access/           NEW  TicketUsage (entry/leave log), AccessService (UC 4.2), gate HTML page,
                  StaffNotification (Security Alert), match statistics
swd_django_demo/  containers.py: register new services; apps.py: wire new view modules
```

New URLs to add (proposal): `matches/`, `matches/<id>/seats/?sector=`, `tickets/buy`, `gate/` (HTML simulation),
`gate/enter`, `gate/leave`, `staff/alerts/`, `stats/`.

## 5. Where the concurrency requirements land

| Requirement | Starter code to extend | Django technique |
|---|---|---|
| No double sale of a seat (UC 4.1) | `OrderService.create_order` + `CustomerService.redeem_credit` | `transaction.atomic()` + `select_for_update()` on the seat/ticket rows, unique constraint (match, seat) |
| No double entry (UC 4.2) | new `AccessService.check_entry` | `select_for_update()` on `Ticket`, version field for optimistic check, unique constraint "one open ENTRY per ticket and match" |
| Denied attempts survive rollback | none yet | write the log row outside the failed `atomic()` block (own transaction) |
| Atomic statistics | none yet | `F("used") + 1` update in the same transaction |

Note: SQLite has no row locks (see `customers/test_concurrency.py`). The strict concurrency tests must run on
MariaDB via `just test-mariadb`.

## 6. Tests in the starter (pattern to copy)

| Style | Example | Use for |
|---|---|---|
| Pure unit test, fake session and in-memory catalog | `orders/test_basket.py` | seat selection, price rules |
| Adapter test with real models | `products/test_services.py`, `orders/test_service.py` | services |
| View test with Django client and a test container | `orders/test_ui.py` | purchase flow, gate page |
| Protocol conformance test per adapter | `*/test_service*.py` | every new adapter |
| Concurrency test with threads | `customers/test_concurrency.py` | double sale, double entry |

## 7. Practical notes for the change

- New apps must be added in four places: `INSTALLED_APPS` (settings.py), the `include` list in `pyproject.toml`,
  `swd_django_demo/urls.py`, and the `wire(modules=[...])` call in `swd_django_demo/apps.py`.
- The migration history is one `0001_initial` per app. Replacing models means deleting `db.sqlite3` and the old
  migrations, then `makemigrations` (the README section 3.4 describes the same reset).
- `containers/initial_data.sql` holds the schema and sample data for MariaDB and must be regenerated after model changes.
- `LOGIN_URL` points to the admin login; the starter has no register/login pages of its own (a user story asks for them).
- Not in the starter and needed by us: unique constraints on seats, enumerations for match and ticket status,
  a notification mechanism, the gate page, statistics, and all stadium master data.
