# SWD-LB-07: Design Patterns in Python/Django

**Course:** Software Design (SWD) Lab  
**Instructors:** Bernhard Danninger, Josef Lettner, Henrik Binggl  
**Version:** Updated for Current Codebase (2026)

---

## Goal of This Unit

> **You will be able to:** name the design patterns already present in the demo project, explain what each one buys you, and apply them deliberately in your own code.
>
> **You hand in (as part of the final zip):**
> - your implementation, with the design patterns you used identified and justified
> - constructor injection for essential dependencies and container-managed singletons
> - your services written as adapters that depend on interfaces, not on models
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

## 2. What Are Design Patterns?

- A software design pattern is a **general, reusable solution** to a commonly occurring problem within a given context in software design
- It is a description or **template** for how to solve a problem that can be used in many different situations
- Design patterns are **formalized best practices** that the programmer can use to solve common problems when designing an application or system

The approach became popular with the book *"Design Patterns – Elements of Reusable Object-Oriented Software"* by the **Gang of Four** (Gamma et al., "GoF", ISBN 978-0201633610). It describes 23 patterns with implementations in C++ and Smalltalk. The approach is tied to **object-oriented** languages — not everything applies directly to functional languages (Lisp, Haskell, Elixir, Scala, Clojure, ...).

Further reading:
- [GoF Design Patterns](https://en.wikipedia.org/wiki/Design_Patterns)
- [Design Patterns Cheat Sheet](https://refactoring.guru/design-patterns)

---

## 3. The Three Categories

The GoF book defines three categories of design patterns:

### 3.1 Creational Patterns
> "... are ones that create objects, rather than having to instantiate objects directly."

These decouple object creation from use. Relevant to this project: **Dependency Injection** and **Singleton**.

### 3.2 Structural Patterns
> "These concern class and object composition."

These describe how classes and objects are composed to form larger structures. Relevant to this project: the **Adapter** pattern (services adapt the core interfaces to their models) and **Interface/Protocol** based composition.

### 3.3 Behavioral Patterns
> "Most of these design patterns are specifically concerned with communication between objects."

These define how objects interact and distribute responsibility. The demo project does not use a behavioral pattern explicitly.

Not every GoF pattern is relevant to a given project. This lab focuses on the patterns the demo project actually uses: **Dependency Injection** and **Singleton**.

---

## 4. Dependency Injection

### 4.1 The Goal
- Make a class **independent of its dependencies**
- Allow services to be used completely independent of any client consumption — **swap a service without modifying its clients**

### 4.2 How It Is Achieved
- **Decouple** the usage of an object from its creation
- **Provide** dependencies to objects instead of creating the dependencies inside the objects themselves

### 4.3 Constructor Injection
Dependencies are provided through the constructor. This is the form the project uses for all **essential** dependencies:

```python
# orders/services.py
from core.interfaces import Customers, ProductCatalog

class OrderService:
    def __init__(self, product_service: ProductCatalog, customer_service: Customers):
        self.product_service = product_service
        self.customer_service = customer_service
```

Note that `OrderService` depends only on the **interfaces** (`ProductCatalog`, `Customers`) from `core.interfaces`, not on any concrete app. Any adapter that satisfies the interface can be injected — including a `Mock` in tests.

### 4.4 Interface Injection (Protocol)
The project uses Python's `typing.Protocol` (PEP 544) for interface injection. An adapter satisfies the interface by having the right methods; no inheritance is needed:

```python
# core/interfaces.py
from typing import Optional, Protocol, runtime_checkable
from core.domain import CustomerInfo, CatalogItem
from core.models import Customer

@runtime_checkable
class ProductCatalog(Protocol):
    def get_all_products(self) -> list[CatalogItem]: ...
    def get_product(self, product_id) -> Optional[CatalogItem]: ...

@runtime_checkable
class Customers(Protocol):
    def redeem_credit(self, customer: Customer, amount: Decimal) -> None: ...
    def find_customer(self, username: str) -> Optional[CustomerInfo]: ...
```

`@runtime_checkable` allows `isinstance(service, ProductCatalog)` to verify at runtime that an adapter satisfies the interface — used in the test suite:

```python
# products/test_services.py
def test_satisfies_product_catalog(self):
    self.assertIsInstance(self.service, ProductCatalog)
```

### 4.5 Setter / Optional Injection
For **optional** dependencies the project uses a default value in the constructor instead of a separate setter method. The repository (`repo`) is optional — the service defaults to the Django model manager, but a test can pass an in-memory fake:

```python
# products/services.py
class ProductService:
    def __init__(self, repo=None):
        # default to Product.objects, but allow tests to pass a fake
        self.repo = repo or Product.objects
```

```python
# customers/services.py
class CustomerService:
    def __init__(self, repo=None):
        # default to Customer.objects, but allow tests to pass a fake
        self.repo = repo or Customer.objects
```

### 4.6 The Dependency Injection Container
The project uses the [dependency-injector](https://python-dependency-injector.ets-labs.org/) framework. The container declares the concrete adapters at the seams and provides them to the views through wiring:

```python
# swd_django_demo/containers.py
from dependency_injector import containers, providers
from customers.services import CustomerService
from orders.services import OrderService
from products.services import ProductService


class Container(containers.DeclarativeContainer):
    config = providers.Configuration()

    product_service = providers.Singleton(ProductService)
    customer_service = providers.Singleton(CustomerService)

    # OrderService receives its dependencies through constructor injection
    order_service = providers.Singleton(
        OrderService, product_service=product_service, customer_service=customer_service
    )
```

The container is built once in the project's `apps.py` and wired into the view modules:

```python
# swd_django_demo/apps.py
class ProjectConfig(AppConfig):
    """Composition root: builds and wires the dependency-injection container."""

    name: str = "swd_django_demo"

    def ready(self) -> None:
        from swd_django_demo.containers import Container

        self.container = Container()
        self.container.config.from_dict(settings.__dict__)
        self.container.wire(modules=["products.views", "orders.views"])
```

The views declare the injected services with `@inject` and `Provide`:

```python
# products/views.py
from dependency_injector.wiring import Provide, inject
from core.interfaces import ProductCatalog

@inject
def index(
    request: HttpRequest,
    product_service: ProductCatalog = Provide["product_service"],
) -> HttpResponse:
    ...
```

### 4.7 Three Forms of Injection Compared

| Form | Where dependencies arrive | When to use | Used in project |
|---|---|---|---|
| **Constructor injection** | Constructor parameters | All essential dependencies | `OrderService.__init__(product_service, customer_service)` |
| **Interface injection** | Through an interface / Protocol the dependent satisfies | When the adapter is chosen at runtime | `ProductCatalog`, `Customers` Protocols |
| **Setter / optional injection** | Optional constructor parameter with a default | Optional dependencies, test seams | `ProductService.__init__(repo=None)`, `CustomerService.__init__(repo=None)` |

---

## 5. Singleton

### 5.1 When It Is Used
- When **at most one instance** per class should exist (e.g. interface to a printer, a file, or a shared service)
- It allows **access control**, dealing with **concurrency** and **subclassing**
- It can **prevent global variables**: the object does not need to exist from the beginning, it is created on first access

### 5.2 Drawbacks
- Introduces **global state**
- Can be **hard to test** and **difficult to stub**

### 5.3 The Classic Implementation
A classic singleton has:
- A **private class variable** holding the instance
- A **private constructor** so the class cannot be instantiated directly
- A **public `get_instance()` method** that creates the instance on first call and returns it thereafter

```python
class Singleton:
    _instance = None

    def __init__(self):
        # in Python the constructor cannot be truly private,
        # so this pattern relies on convention and get_instance()
        if Singleton._instance is not None:
            raise RuntimeError("Use get_instance()")

    @classmethod
    def get_instance(cls):
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance
```

Different implementations with different trade-offs exist (decorator, Borg/monostate pattern, module-level singleton, ...).

### 5.4 How the Project Uses Singleton
The project does **not** hand-roll the singleton pattern. Instead, the **dependency-injection container** provides each service as a `providers.Singleton`, which guarantees one instance for the lifetime of the application:

```python
# swd_django_demo/containers.py
class Container(containers.DeclarativeContainer):
    product_service = providers.Singleton(ProductService)
    customer_service = providers.Singleton(CustomerService)
    order_service = providers.Singleton(
        OrderService, product_service=product_service, customer_service=customer_service
    )
```

This keeps the singleton behaviour (one shared instance) without the drawbacks of a hand-rolled singleton:
- The services themselves are **plain classes** with no global state — they have no knowledge that they are singletons
- **Testability** is preserved: tests build their own test container with mocks, or construct a service directly with `OrderService(ProductService(), Mock())`
- There is no hidden `get_instance()` coupling — the container is the only place that knows how services are wired

### 5.5 Classic Singleton vs. Container Singleton

| Aspect | Classic singleton (hand-rolled) | Container singleton (`providers.Singleton`) |
|---|---|---|
| Instance count | One, enforced by the class itself | One, enforced by the container |
| Global state | Yes — class-level `_instance` | No — the service class is plain |
| Testability | Hard to stub | Easy — build a test container or pass a `Mock` |
| Coupling | Callers know the singleton class | Callers depend on the interface, not the class |
| Swapability | Requires changing the class | Swap the provider in the container |

---

## 6. Adapter Pattern (Bonus)

The services in the demo project are **adapters**: they adapt the core interfaces (`ProductCatalog`, `Customers`) to the app's concrete models. The adapter holds the model-specific logic (how to read from the database, how to convert a model to a record) and exposes only the interface methods:

```python
# products/services.py
from core.domain import CatalogItem
from products.models import Product

# adapter for the ProductCatalog interface, backed by the products app's model
class ProductService:
    def get_all_products(self) -> list[CatalogItem]:
        return [self._to_item(product) for product in self.repo.all()]

    def get_product(self, product_id) -> Optional[CatalogItem]:
        product = self.repo.get_by_id(product_id)
        return self._to_item(product) if product else None

    def _to_item(self, product: Product) -> CatalogItem:
        # in another product app, the logic for getting the price could be different
        return CatalogItem(id=product.id, name=product.name,
                           description=product.description, price=product.price)
```

Because the `OrderService`, the `Basket` and the views depend only on the `ProductCatalog` interface, any object with the same methods can take the adapter's place. The project uses this for **test doubles** — stand-ins that satisfy the interface without a database.

A hand-written in-memory implementation (a *fake*) for the pure unit tests of the basket:

```python
# orders/test_basket.py
class InMemoryProducts:
    def __init__(self, *products: CatalogItem):
        self._by_id = {str(product.id): product for product in products}

    def get_all_products(self) -> list[CatalogItem]:
        return list(self._by_id.values())

    def get_product(self, product_id) -> CatalogItem | None:
        return self._by_id.get(str(product_id))

order = Basket(["1", "2", "1"]).lines(InMemoryProducts(APPLE, PEAR))
```

A generated *mock* restricted to the interface, wired in through a test container for the view tests:

```python
# orders/test_ui.py
class TestContainer(containers.DeclarativeContainer):
    product_service = providers.Singleton(MagicMock, spec=ProductCatalog)

c = TestContainer()
c.product_service().get_all_products.return_value = self.mock_product_list
c.wire(modules=["products.views", "orders.views"])
```

What matters is what does **not** change: neither the `Basket`, the `OrderService` nor the views are touched to run against the double. Since `ProductCatalog` is a `typing.Protocol`, `InMemoryProducts` does not even inherit from it — `test_satisfies_product_catalog` checks with `isinstance` that it still has the right methods. If you extend the Protocol, the doubles must follow.

---

## 7. Patterns in This Project at a Glance

| Pattern | Category | Where in the project |
|---|---|---|
| **Dependency Injection** (constructor) | Creational | `OrderService.__init__(product_service, customer_service)` |
| **Dependency Injection** (optional) | Creational | `ProductService.__init__(repo=None)`, `CustomerService.__init__(repo=None)` |
| **Dependency Injection Container** | Creational | `swd_django_demo/containers.py`, wired in `apps.py` |
| **Singleton** | Creational | `providers.Singleton` in the container |
| **Adapter** | Structural | `ProductService`, `CustomerService` adapt core interfaces to their models |
| **Interface / Protocol** | Structural | `ProductCatalog`, `Customers` in `core/interfaces.py` |
| **Test Double** (fake, mock) | Testing | `InMemoryProducts` in `orders/test_basket.py`, `MagicMock(spec=ProductCatalog)` in `orders/test_ui.py` |

---

## 8. Assignment: Project Implementation

### 8.1 Tasks
- [ ] Continue implementing your use cases
  - [ ] Identify which design patterns are already present in your code
  - [ ] Use **constructor injection** for essential service dependencies
  - [ ] Use an **optional `repo` parameter** where you want a test seam for the persistence layer
  - [ ] Declare your services as **singletons** in the DI container
  - [ ] Make your services **adapters**: depend on interfaces, hide the models behind the adapter

### 8.2 Key Concepts to Demonstrate
- **Dependency injection** decouples service usage from creation
- **Protocols** as interfaces for interface injection
- **Container-managed singletons** instead of hand-rolled singletons
- **Adapter pattern** for services that bridge interfaces and models
- **Testability** through injectable seams (`repo`, `Mock`, test container)

---

*Updated to match current djangolight codebase*  
*Last updated: 2026-09-06*
