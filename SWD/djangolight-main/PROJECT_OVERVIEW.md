# Project Big Picture

This project is a small but complete Django web application: a shop where visitors browse a product
catalog, fill a basket and place an order that is paid from prepaid credit. The point is not to show
individual Django features, but **how the parts of a server-side Python application fit together**.

The diagrams in this document are [Mermaid](https://mermaid.js.org/) diagrams. GitHub, GitLab, VS Code
(with a Markdown preview extension) and PyCharm render them directly; nothing has to be generated.

The one idea to take away: **separation of responsibilities**. Every module has one job. When the
application grows, you do not add code to the biggest file — you add it to the layer that owns the
responsibility.

---

## 1. What The Application Does

The domain is a shop. Its vocabulary is defined in [CONTEXT.md](./CONTEXT.md), and code, templates and
documentation all use those words.

- A **Product** is one offering in the catalog, with a name, a description and a price.
- A **Customer** is a person who can log in and place orders; they own a **Credit**.
- A **Basket** holds the products a visitor intends to order. It lives in the session, not in the database.
- An **Order** is a placed and paid basket. It consists of **Order Positions** (one per distinct product).

```mermaid
classDiagram
    class Customer {
        +int id
        +str username
        +str email
        +Decimal credit
    }
    class Product {
        +int id
        +str name
        +str description
        +Decimal price
    }
    class Order {
        +int id
        +Decimal total_price
        +datetime created_at
    }
    class OrderPosition {
        +int pos
        +int quantity
        +Decimal price
    }

    Customer "1" --> "0..*" Order : places
    Order "1" *-- "1..*" OrderPosition : order_positions
    OrderPosition "0..*" --> "1" Product : product

    note for OrderPosition "price is copied at order time:\na later price change must not\nrewrite past orders"
```

One customer can place many orders; each order belongs to exactly one customer. An order consists of
several positions, and each position refers to one product with a quantity and the unit price **at the
time of ordering**. That copied price is a deliberate domain decision, not an optimisation.

The application is reached in one way only: **server-rendered HTML pages**. There is no REST API in this
project (`djangorestframework` is installed but currently unused) — adding one is a natural exercise,
and it would reuse exactly the same services as the pages.

---

## 2. The Big Picture

Read the diagram from the outside (the browser) toward the database and back. Arrows mean *"may call"*,
not *"every request goes through here"*.

```mermaid
flowchart TB
    browser["Browser<br/><i>HTML over HTTP</i>"]

    subgraph django["Django framework"]
        middleware["Middleware<br/><i>session, authentication, CSRF, messages</i>"]
        urls["URL configuration<br/><code>swd_django_demo/urls.py</code> + per-app <code>urls.py</code>"]
    end

    subgraph presentation["Presentation"]
        views["Views<br/><code>products/views.py</code>, <code>orders/views.py</code>"]
        templates["Templates<br/><code>*/templates/**.html</code>"]
    end

    subgraph seam["Seam — core"]
        interfaces["Interfaces (Protocols)<br/><code>core/interfaces.py</code>"]
        domain["Records (frozen dataclasses)<br/><code>core/domain.py</code>"]
    end

    subgraph business["Business logic"]
        basket["Basket<br/><code>orders/basket.py</code>"]
        services["Services / adapters<br/><code>ProductService</code>, <code>CustomerService</code>, <code>OrderService</code>"]
    end

    subgraph data["Data access"]
        managers["Managers = repositories<br/><code>*/managers.py</code>"]
        models["Models (ORM)<br/><code>*/models.py</code>"]
    end

    db[("Database<br/>SQLite in dev, MariaDB in containers")]
    container["DI container<br/><code>swd_django_demo/containers.py</code>"]

    browser --> middleware --> urls --> views
    views --> templates --> browser
    views -.depends on.-> interfaces
    views --> basket
    services -.implements.-> interfaces
    basket --> interfaces
    views --> services
    services --> managers --> models --> db
    container -. injects .-> views
    domain -.exchanged by.-> views
    domain -.exchanged by.-> services

    style seam fill:#fff6e0,stroke:#e0a800
    style container fill:#eef6ff,stroke:#4a90d9
```

Two things distinguish this project from a textbook Django app:

1. **The views never see a model.** They receive `CatalogItem`, `CustomerInfo`, `OrderSummary` — plain
   frozen dataclasses from `core/domain.py`. Only the adapters (services) touch Django models.
2. **The views never construct a service.** The dependency-injection container hands the service in.

---

## 3. The Django Apps

A Django project is a collection of *apps*. Each app is a Python package with its own models, views,
URLs, templates and tests.

```mermaid
flowchart LR
    subgraph project["swd_django_demo — the project"]
        settings["settings.py<br/>urls.py<br/>containers.py<br/>apps.py"]
    end

    core["<b>core</b><br/>interfaces, domain records,<br/>abstract models, site root, health"]
    products["<b>products</b><br/>catalog model, manager,<br/>ProductService, catalog page, seeding"]
    customers["<b>customers</b><br/>Customer = user model,<br/>CustomerService, credit"]
    orders["<b>orders</b><br/>Basket, OrderService,<br/>Order + OrderPosition, pages"]

    products --> core
    customers --> core
    orders --> core
    project --> products
    project --> customers
    project --> orders
    orders -. "FK (schema only)" .-> products
    orders -. "FK (schema only)" .-> customers

    style core fill:#fff6e0,stroke:#e0a800
```

Note what the dashed arrows mean: `orders` depends on `products` and `customers` only through the
foreign keys of its models (and therefore its migrations). Its code never imports another app's models:
the services, views and basket depend on `core.interfaces` and get the concrete adapter injected. That is
the architectural idea of this project — data may be linked, behaviour goes through the seams.

| App | Responsibility |
|---|---|
| `core` | The seams: `interfaces.py` (Protocols), `domain.py` (records + errors), abstract base models, site root redirect, `/health`. |
| `products` | Product catalog: `Product` model, `ProductManager` (repository), `ProductService` (adapter), `/products/` page, `seed_catalog` command. |
| `customers` | `Customer` — at the same time the Django user model — plus `CustomerService` for credit redemption. |
| `orders` | `Basket` (session), `OrderService` (placement + history), `Order`/`OrderPosition` models, pages under `/orders/`. |
| `swd_django_demo` | The project itself: settings, root URLs, the DI container and the composition root. |

---

## 4. Application Startup

There is no `main()`. Django is started by a *management command* (`manage.py runserver`) or by a WSGI
server (gunicorn in the container setup). Startup does this:

```mermaid
sequenceDiagram
    autonumber
    participant CLI as manage.py / gunicorn
    participant Django as django.setup()
    participant Apps as INSTALLED_APPS
    participant Project as ProjectConfig.ready()
    participant C as Container

    CLI->>Django: load DJANGO_SETTINGS_MODULE
    Django->>Apps: import each app, load its models
    Note over Apps: core, customers, products, orders …<br/>swd_django_demo.apps.ProjectConfig is last
    Apps->>Project: ready()
    Project->>C: Container()
    Project->>C: config.from_dict(settings)
    Project->>C: wire(modules=["products.views", "orders.views"])
    Note over C: every Provide[...] default in those<br/>modules is now bound to a singleton
    CLI->>CLI: serve requests
```

`swd_django_demo/apps.py` is the **composition root** — the single place where the application decides
which concrete implementation satisfies which interface. It must stay **last** in `INSTALLED_APPS`,
because `ready()` is only allowed to import models after every app has been loaded.

### Dependency Injection

```mermaid
flowchart LR
    subgraph cont["Container (swd_django_demo/containers.py)"]
        ps["product_service<br/><i>Singleton(ProductService)</i>"]
        cs["customer_service<br/><i>Singleton(CustomerService)</i>"]
        os["order_service<br/><i>Singleton(OrderService)</i>"]
    end

    ps --> os
    cs --> os

    pv["products.views.index"]
    ov1["orders.views.add_to_basket"]
    ov2["orders.views.basket_overview<br/>place_order · order_list"]

    ps -. "Provide['product_service']" .-> pv
    ps -. "Provide['product_service']" .-> ov1
    os -. "Provide['order_service']" .-> ov2
```

A view declares what it needs as a parameter with a default:

```python
@inject
def index(request, product_service: ProductCatalog = Provide["product_service"]) -> HttpResponse:
    ...
```

Why bother? The view depends on the **interface** `ProductCatalog`, never on `ProductService`. A test can
call `index(request, product_service=FakeCatalog())` without a database, and production can swap the
catalog implementation by changing one line in the container. This is the same reason Spring uses
`@Autowired` — Python just needs an explicit library
([dependency-injector](https://python-dependency-injector.ets-labs.org/)) because it has no built-in one.

---

## 5. The Layers

```mermaid
flowchart TB
    L0["<b>Client</b> — browser"]
    L1["<b>Routing</b> — urls.py, middleware (session, auth, CSRF)"]
    L2["<b>Presentation</b> — views + Django templates"]
    L3["<b>Business logic</b> — Basket, OrderService, ProductService, CustomerService"]
    L4["<b>Data access</b> — managers (repositories)"]
    L5["<b>Persistence</b> — models (ORM) → database"]

    L0 --> L1 --> L2 --> L3 --> L4 --> L5
    L5 -. results .-> L4 -. records .-> L3 -. records .-> L2 -. HTML .-> L0
```

Requests travel down, results travel back up. Settings, the DI container and the tests support the whole
stack and are therefore not drawn as part of the runtime path.

### 5.1 URL Configuration

```text
swd_django_demo/urls.py      the root URLconf
core/urls.py                 ""  → redirect to products, /health, /logout
products/urls.py             /products/
orders/urls.py               /orders/…
```

The root URLconf only `include()`s the apps' own URL modules, so every app owns its URLs. Each app sets
an `app_name`, which is why templates can write `{% url 'orders:basket_overview' %}` instead of hard-coded
paths. Change a path, and every link follows.

| URL | View | Purpose |
|---|---|---|
| `/` | `core.index_view.index` | Redirects to the catalog. |
| `/health` | `core.views.health_check` | JSON health probe for the container healthcheck: 200 when the database answers, 503 otherwise. |
| `/products/` | `products.views.index` | The catalog page. |
| `/orders/add_to_basket` | `orders.views.add_to_basket` | POST, adds one product to the session basket. |
| `/orders/basket_overview` | `orders.views.basket_overview` | The priced basket. |
| `/orders/place_order` | `orders.views.place_order` | Login required, turns the basket into an order. |
| `/orders/clear_basket` | `orders.views.clear_basket` | POST, empties the basket. |
| `/orders/my` | `orders.views.order_list` | Login required, the customer's order history. |
| `/admin/` | Django admin | Product and customer maintenance, and the login page. |

### 5.2 View Layer

Views are the entry point for HTTP requests. A view in this project does three things and nothing more:
read the request, call a service, render a template.

```python
@login_required
@inject
def order_list(request, order_service: OrderService = Provide["order_service"]) -> HttpResponse:
    context = {"orders": order_service.get_orders(request.user)}
    return HttpResponse(loader.get_template("orders/order_list.html").render(context, request))
```

Notice what is **not** there: no query, no pricing rule, no transaction. `place_order` is the one view
with visible control flow, and even that is only translation — it catches `OrderError` and turns it into a
user message:

```python
try:
    order = order_service.create_order(request.user, basket)
    basket.clear()
    messages.success(request, f"Thank you! Your order #{order.pk} has been placed.")
except OrderError as e:                    # expected business outcome → message
    messages.error(request, str(e))        # anything else is a bug → 500
```

### 5.3 Service Layer

Services hold the business logic. Each one is a plain Python class — no Django base class, no framework
magic.

| Service | Responsibility |
|---|---|
| `products.services.ProductService` | Adapter for `ProductCatalog`: reads products and converts them to `CatalogItem` records. The conversion is where a pricing policy would live. |
| `customers.services.CustomerService` | Adapter for `Customers`: `redeem_credit()` locks the customer row, checks the credit and deducts it, or raises `InsufficientCredit` without writing anything. |
| `orders.services.OrderService` | Prices a basket, lists the order history and — in `create_order` — owns the one transaction that places an order. |
| `orders.basket.Basket` | Not injected but constructed from the session. Owns the session key and the pricing of raw product ids into `OrderLine`s. |

`OrderService` shows the rule most clearly:

```python
def __init__(self, product_service: ProductCatalog, customer_service: Customers):
```

It is typed against the *interfaces*. It never imports `products.models` or `customers.models`, which is
why its tests can hand it an in-memory catalog or a mock instead of the real adapter.

### 5.4 Repository Layer — Managers

Django has no separate repository class; the equivalent is a model **manager**, reachable as
`Product.objects`. The default manager already provides `all()`, `filter()`, `get()`, `create()`,
`update_or_create()`, `delete()`. Custom query methods are added by subclassing:

```python
class ProductManager(CoreProductManager):
    def get_by_id(self, id: int) -> Optional["Product"]:
        return self.get_queryset().filter(id=id).first()

    def get_by_price_range(self, min_price: Decimal, max_price: Decimal) -> QuerySet["Product"]:
        return self.get_queryset().filter(price__gte=min_price, price__lte=max_price)
```

Keeping queries in the manager means an SQL-shaped concern stays in one place instead of being spread
over the services. `CustomerManager` additionally extends Django's `UserManager`, because
`manage.py createsuperuser` needs `create_user`/`create_superuser`.

### 5.5 Model Layer

Models are the ORM classes; each non-abstract model maps to one database table.

```mermaid
classDiagram
    class `core.Product` {
        <<abstract>>
        name
        description
    }
    class `products.Product` {
        price
    }
    class `core.Customer` {
        <<abstract, AbstractUser>>
    }
    class `customers.Customer` {
        credit
    }

    `core.Product` <|-- `products.Product`
    `core.Customer` <|-- `customers.Customer`
```

The important mechanisms:

- **Abstract models** (`class Meta: abstract = True`) get no table of their own; their fields are copied
  into every subclass. `core.models.Product` and `core.models.Customer` define what *every* product or
  customer has.
- **Foreign keys across apps.** `orders/models.py` references the other apps by string:
  `ForeignKey("products.Product", …)` and `ForeignKey(settings.AUTH_USER_MODEL, …)`. A string reference
  needs no import, so the orders code stays free of other apps' models; the dependency shows up only in the
  schema and in the migration's `dependencies`. `AUTH_USER_MODEL = "customers.Customer"` is Django's
  documented way to replace the built-in user model.
- **`on_delete=PROTECT`** on the product and customer foreign keys: a product that appears in an order
  must not vanish, or the order history would be falsified. `OrderPosition` uses `CASCADE` towards its
  order, because a position has no meaning without it.
- **`unique_together = (("order", "pos"),)`** substitutes for the composite primary key Django does not
  support.

### 5.6 The Seam — Interfaces and Records

This is the layer a Spring project would call "interfaces and DTOs", and it is worth a close look.

**Interfaces are `typing.Protocol`s** (PEP 544). An adapter satisfies one simply by having the right
methods — no `implements`, no inheritance:

```python
@runtime_checkable
class ProductCatalog(Protocol):
    def get_all_products(self) -> list[CatalogItem]: ...
    def get_product(self, product_id) -> Optional[CatalogItem]: ...
```

**Records are frozen dataclasses** in `core/domain.py`: `CatalogItem`, `CustomerInfo`, `OrderLine`,
`OrderSummary`. They are the Python counterpart of DTOs, and they exist for concrete reasons:

- A Django model is an *Active Record*: bound to the database, no `id` before saving, foreign keys not
  followable before saving. The basket must be **priced without writing anything**, so it cannot use
  models.
- `frozen=True` means a record cannot change under the caller's feet — the price shown in the basket is
  the price that was looked up.
- `slots=True` turns a typo like `item.pric = 1` into an error instead of a silent new attribute.

**Business failures are exceptions**, also in `core/domain.py`:

```mermaid
flowchart LR
    OE["OrderError"] --> EB["EmptyBasket"]
    OE --> IC["InsufficientCredit<br/><i>required, available</i>"]
```

Services raise the specific type; views catch the base type `OrderError`. Anything else that escapes is a
bug and correctly becomes a 500.

An **entity describes how data is stored, a record describes how data is transferred.** Mixing the two is
the most common way a layered application quietly loses its layers.

### 5.7 Templates And Static Files

```text
core/templates/core/base.html            the layout: navigation, messages, footer
products/templates/products/index.html   the catalog
orders/templates/orders/basket_overview.html
orders/templates/orders/order_list.html
static/                                  Bootstrap, icons, application.css, theme.js
static_dir/                              collectstatic output, served by nginx in the container setup
```

Templates are rendered **on the server**. The view fills a context dictionary, the template reads it with
`{{ … }}`, `{% for %}`, `{% if %}` and extends the layout with `{% extends "core/base.html" %}`.

One value reaches every template without any view passing it: `basket_count`. That is a **context
processor** (`orders/context_processors.py`, registered in `settings.TEMPLATES`), which is the right tool
for something the layout needs everywhere — here the badge in the navigation bar.

### 5.8 Authentication And Access Control

There is no separate security layer in this project; Django's middleware and decorators do the work.

```mermaid
flowchart TB
    req["Request"] --> sess["SessionMiddleware<br/><i>loads request.session</i>"]
    sess --> auth["AuthenticationMiddleware<br/><i>sets request.user</i>"]
    auth --> csrf["CsrfViewMiddleware<br/><i>validates POST tokens</i>"]
    csrf --> dec{"@login_required<br/>on the view?"}
    dec -- "no" --> view["View runs<br/><i>anonymous visitors welcome</i>"]
    dec -- "yes, and authenticated" --> view
    dec -- "yes, but anonymous" --> login["Redirect to LOGIN_URL<br/><code>/admin/login/</code>"]
```

- Browsing the catalog and filling a basket work **without login** — a visitor has a session, not an
  account.
- `place_order` and `order_list` carry `@login_required`: they need a customer with credit and an owner
  for the order.
- The `/admin/` site is protected by Django's own staff check, and it doubles as the login page
  (`LOGIN_URL = "/admin/login/"` — convenient for a lab, not something to copy into production).
- Every view that changes state (`add_to_basket`, `clear_basket`, `place_order`) accepts only POST and is
  protected by Django's CSRF middleware; the forms render `{% csrf_token %}`. A good discussion topic: what
  could an attacker do if `clear_basket` were a plain GET link, or if a view were `@csrf_exempt`?

### 5.9 Settings And Configuration

`swd_django_demo/settings.py` is the development configuration (SQLite, `DEBUG = True`, a checked-in
development key that `DJANGO_SECRET_KEY` overrides). `settings_prod.py` is the container variant: it imports
everything from `settings.py` and only overrides what differs (MariaDB, no debug, allowed hosts). Its secrets
come from the environment — `DJANGO_SECRET_KEY` and `DJANGO_DB_PASSWORD` are required, the container refuses
to start without them. The Dockerfile selects it with `DJANGO_SETTINGS_MODULE=swd_django_demo.settings_prod`,
and `containers/compose.yaml` passes the secrets (with local-only defaults, override them in your shell or in
`containers/.env`). Settings worth knowing:

| Setting | Effect |
|---|---|
| `INSTALLED_APPS` | Which apps exist. `ProjectConfig` must remain last. |
| `AUTH_USER_MODEL = "customers.Customer"` | Makes the shop's customer *the* user of the project. |
| `TEMPLATES.OPTIONS.context_processors` | Includes `orders.context_processors.basket`. |
| `SESSION_SERIALIZER` (JSON) | Why the basket stores product ids as **strings**: session data must be JSON serializable. |
| `LOGGING` | The `orders` logger runs at `DEBUG` with a verbose formatter, so service calls are visible in the console. |

### 5.10 Tests

```text
orders/test_basket.py         pure unit tests: a dict as session, an in-memory catalog, no database
products/test_services.py     adapter tests on SQLite, real models
customers/test_service.py     "
customers/test_concurrency.py credit redemption under concurrent access
orders/test_service.py        "
orders/test_ui.py             view tests through the Django test client, with a test container
products/test_models.py       model and manager behaviour
products/test_seeding.py      catalog import
products/test_views.py        the catalog page
swd_django_demo/test_dependency.py   the container wires what it claims to wire
```

Run them with `just test` (`uv run python manage.py test -v 2`), a subset with a dotted label:

```bash
uv run python manage.py test orders
uv run python manage.py test orders.test_basket.TestBasketLines
```

`just test-mariadb` runs the same suite inside the app image against MariaDB, like production (needs Docker,
accepts the same labels). It is the only way the strict row-lock test in `customers/test_concurrency.py`
runs; SQLite has no `select_for_update()`, so `just test` skips it.

Two habits are worth copying. First, the tests follow the same responsibility split as the production
code — basket logic is tested without a database, adapters against real models, pages through the test
client. Second, **every adapter has a test asserting it satisfies its Protocol** (`isinstance(service,
ProductCatalog)` works thanks to `@runtime_checkable`), so an incomplete adapter fails a test instead of
failing in front of a user.

---

## 6. Example Flow: Opening The Catalog

`GET /products/`

```mermaid
sequenceDiagram
    autonumber
    actor V as Visitor
    participant U as URLconf
    participant Vw as products.views.index
    participant PS as ProductService
    participant M as Product.objects
    participant DB as Database
    participant T as products/index.html

    V->>U: GET /products/
    U->>Vw: index(request, product_service=<injected>)
    Vw->>PS: get_all_products()
    PS->>M: all()
    M->>DB: SELECT … FROM products_product
    DB-->>M: rows
    M-->>PS: Product instances
    PS-->>Vw: list[CatalogItem]
    Note over PS,Vw: models stop here —<br/>the view only sees records
    Vw->>T: render({"products_list": …})
    T-->>V: HTML (with basket_count from the context processor)
```

## 7. Example Flow: Adding To The Basket

`POST /orders/add_to_basket` with `product_id`

```mermaid
sequenceDiagram
    autonumber
    actor V as Visitor
    participant Vw as orders.views.add_to_basket
    participant PS as ProductService
    participant B as Basket
    participant S as request.session

    V->>Vw: POST product_id=3
    alt product_id missing
        Vw-->>V: 400 Bad Request
    else product unknown
        Vw->>PS: get_product(3)
        PS-->>Vw: None
        Vw-->>V: 404 Not Found
    else product exists
        Vw->>PS: get_product(3)
        PS-->>Vw: CatalogItem
        Vw->>B: Basket.load(session).add("3")
        B->>S: session["product_list"] = ["3", …]
        Vw-->>V: 302 back to the referring page
    end
```

Nothing is written to the database. The basket is **session state**: a list of product id strings under
one key. Pricing happens later and on demand.

## 8. Example Flow: Placing An Order

`POST /orders/place_order` — the most interesting path in the project, because it is where a transaction,
a business rule and error handling meet.

```mermaid
sequenceDiagram
    autonumber
    actor C as Customer
    participant Vw as orders.views.place_order
    participant OS as OrderService
    participant B as Basket
    participant PS as ProductService
    participant CS as CustomerService
    participant DB as Database

    C->>Vw: POST /orders/place_order
    Note over Vw: @login_required — anonymous users<br/>are redirected to the login page
    Vw->>OS: create_order(request.user, basket)
    OS->>B: lines(product_service)
    B->>PS: get_product(id) per distinct id
    PS-->>B: CatalogItem
    B-->>OS: OrderSummary (lines, total_price)

    alt basket empty
        OS-->>Vw: raise EmptyBasket
    else basket priced
        rect rgb(238, 246, 255)
            Note over OS,DB: transaction.atomic() — all or nothing
            OS->>CS: redeem_credit(customer, total_price)
            CS->>DB: SELECT … FOR UPDATE (lock the customer row)
            alt credit < total
                CS-->>OS: raise InsufficientCredit
                Note over OS,DB: rollback — nothing was written
            else credit sufficient
                CS->>DB: UPDATE customer SET credit = credit - total
                OS->>DB: INSERT INTO orders_order
                OS->>DB: bulk INSERT INTO orders_orderposition
            end
        end
        OS-->>Vw: Order
    end

    alt success
        Vw->>B: clear()
        Vw-->>C: redirect to catalog + success message
    else OrderError
        Vw-->>C: redirect to basket + error message
    end
```

Three design decisions are visible here, and they are the ones to remember:

1. **One transaction owner.** `OrderService.create_order` opens the single `transaction.atomic()` block.
   `CustomerService.redeem_credit` also uses `atomic()`, but nested inside a caller's transaction that
   becomes a *savepoint*, not a second transaction. Exactly one place decides what "all or nothing" means.
2. **Credit first.** Redemption happens before the order rows are written, so an `InsufficientCredit`
   aborts before anything exists.
3. **`select_for_update()`.** The customer row is locked while credit is checked and deducted. Without
   it, two concurrent orders could both read the same balance and both succeed — the classic lost-update
   race. `customers/test_concurrency.py` demonstrates it: eleven threads redeem from a credit that covers
   ten. Only a database with row locks serializes them (`just test-mariadb`); SQLite has no
   `select_for_update()`, so there the test checks only that no redemption is lost.

---

## 9. Running The Application

```bash
uv sync            # create .venv and install dependencies
just migrate       # create the database tables
just adminuser     # create an admin/customer
just seed          # load products/data/catalog.json
just runserver     # http://localhost:8000
```

Give the new customer some credit in the admin site (`/admin/`), or orders will be rejected — which is
itself a useful thing to see once.

**Migrations** deserve a note. Every model change needs `manage.py makemigrations`, which writes a file
into `<app>/migrations/`. Those files are versioned code: they are the history of the schema, are
committed to git, and `manage.py migrate` replays them. Never edit a migration that has been shared —
add a new one instead. The only exception is a deliberate reset of the whole history, as done once for
`customers`, `products` and `orders`: every existing database must then be recreated (see README, section 3.4).

`just compose` starts the deployment shape of the application, which is worth seeing at least once:

```mermaid
flowchart LR
    b["Browser<br/>localhost:8000"] --> n["nginx<br/><i>static files + reverse proxy</i>"]
    n --> a["app<br/><i>gunicorn + Django</i>"]
    a --> d[("MariaDB")]
    n -. reads .-> s["static_dir/<br/><i>collectstatic output</i>"]

    subgraph net["docker network 'backend' — not exposed"]
        a
        d
    end
```

Only nginx publishes a port. The development server (`runserver`) is a convenience tool and serves static
files itself; in the container setup those two jobs are separated, which is how it is done in production.

---

## 10. Where To Add Code For A New Feature

```mermaid
flowchart TB
    A["1 · Vocabulary<br/>Is the concept in CONTEXT.md?<br/>Name it there first."] --> B
    B["2 · Model<br/>New or changed field/entity in <code>*/models.py</code><br/>+ <code>makemigrations</code>"] --> C
    C["3 · Manager<br/>New query? Add a method to <code>*/managers.py</code>"] --> D
    D["4 · Seam<br/>Does another app need it?<br/>Extend the Protocol in <code>core/interfaces.py</code><br/>and the record in <code>core/domain.py</code>"] --> E
    E["5 · Service<br/>Business rule in the owning service.<br/>New failure? Subclass <code>OrderError</code>."] --> F
    F["6 · Container<br/>New service? Register it in <code>containers.py</code><br/>and wire the module."] --> G
    G["7 · View + URL<br/>Read request, call service, render."] --> H
    H["8 · Template<br/>Extend <code>core/base.html</code>."] --> I
    I["9 · Tests<br/>Unit test the logic, adapter test the queries,<br/>view test the page."]
```

Step 4 is the one that is easy to skip and expensive to skip. If you extend a Protocol, **every** adapter
must follow — including the test doubles such as `InMemoryProducts` in `orders/test_basket.py`, otherwise
the interface is a lie.

Worked example, "products belong to a category": a `category` field on `products.Product` plus a
migration; a `get_by_category` method on `ProductManager`; a `category` field on `CatalogItem` and a
`get_products_by_category` method on the `ProductCatalog` Protocol, implemented in **both**
`ProductService` and the `InMemoryProducts` test double; a filter parameter in `products.views.index`; a filter
control in the template; tests at each level.

---

## 11. What To Remember

- **URLs** map paths to views; each app owns its own `urls.py`.
- **Views** are thin: read the request, call a service, render a template.
- **Services** hold the business rules and own the transactions.
- **Managers** hold the queries — they are this project's repositories.
- **Models** describe stored data; **records** in `core/domain.py` describe transferred data.
- **Interfaces** (`Protocol`s) are the seams between apps; an app never imports another app's models.
- **The container** decides which implementation fills which seam, in one place, at startup.
- **Templates** render HTML on the server; the layout lives in `core/base.html`.
- **Tests** mirror the layers, and every adapter proves it satisfies its Protocol.

When something is confusing, follow one request from the outside inwards and back: URL → view → service →
manager → model → database → record → template → HTML. That path is the backbone of the whole
application.
