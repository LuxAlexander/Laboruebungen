# SWD-LB-04: Component Implementation in Python/Django

**Course:** Software Design (SWD) Lab  
**Instructors:** Bernhard Danninger, Josef Lettner, Henrik Binggl  
**Version:** Updated for Current Codebase (2026)

---

## Goal of This Unit

> **You will be able to:** cut a system into components that depend on interfaces instead of on each other, build services on top of the models from SWD-LB-03, and wire them together with a dependency-injection container.
>
> **You hand in (as part of the final zip):**
> - 1 component diagram for your own ITS project
> - the interface definitions for your services in `core/interfaces.py`
> - at least one service app with a `services.py` and its provider in `containers.py`
> - a test in `swd_django_demo/test_dependency.py` showing your services can be instantiated
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

## 2. Five Words You Will Meet in This Unit

This unit introduces several terms at once, and they are easy to mix up. Each one lives in its own file:

| Term | What it is | Where it lives in the demo |
|---|---|---|
| **Interface** (Protocol) | A list of method signatures a caller may rely on. No implementation. | `core/interfaces.py` — `ProductCatalog`, `Customers` |
| **Record** | A small immutable data object that crosses between apps instead of a model. | `core/domain.py` — `CatalogItem`, `CustomerInfo` |
| **Service** | The class that holds the business logic of an app. | `orders/services.py` — `OrderService` |
| **Adapter** | A service whose job is to satisfy a core interface on behalf of its app's models. | `products/services.py`, `customers/services.py` |
| **Repository** | The object a service reads and writes models through — here simply a Django model manager, passed in so a test can replace it. | the `repo` parameter of `ProductService` / `CustomerService` |

Read this table again after working through the unit; it is the map for everything below.

---

## 3. Django Applications as Components

### 3.1 Component Concept
- Some of your identified components are software components that we implement as Django Applications
- A Django Application may be reused in various projects
- [Official Django Documentation](https://docs.djangoproject.com/en/stable/ref/applications/)

### 3.2 Creating a Django Application
```bash
# Create a new Django app
uv run python manage.py startapp <AppName>
```

### 3.3 Application Configuration
To configure the application:
1. Create a config class subclassing from `AppConfig` in your application's `apps.py`
2. Register the application in the project's `settings.py` under `INSTALLED_APPS`

---

## 4. Core Application Architecture

### 4.1 Purpose of the Core App
- You met the `core` app in SWD-LB-03 as the home of the abstract models
- Beyond the models, it defines the **interfaces** and **common functionalities** all apps share
- **Goal:** All apps should only depend on the core app and its interfaces, not on specific implementations
- The orders code (services, views, basket) can then run against any implementation of an interface,
  e.g. a test double instead of the real products app

**Example:** The orders app needs to know about products. Its models reference the product by foreign key
(a dependency at the data level), but its services and views only use the `ProductCatalog` interface and
never import the products app's code.

### 4.2 Core App: models.py
- The abstract models `core.models.Product` and `core.models.Customer` were introduced in SWD-LB-03
  (section 3.1): they define the **structure** of the business entities, never their **behavior**
- This unit adds the behavior: business rules live in the **services** below, not in the models

### 4.3 Core App: interfaces.py
- Contains all **interfaces** (Protocols) needed for the application
- Interfaces are defined using Python's `typing.Protocol` (PEP 544)
- Only **method signatures** are defined, no implementation
- Adapters satisfy interfaces by having the right methods, no inheritance needed

```python
# Example from core/interfaces.py
from typing import Optional, Protocol, runtime_checkable
from core.domain import CustomerInfo, CatalogItem
from core.models import Customer

@runtime_checkable
class ProductCatalog(Protocol):
    def get_all_products(self) -> list[CatalogItem]: ...

    def get_product(self, product_id) -> Optional[CatalogItem]: ...

@runtime_checkable
class Customers(Protocol):
    def redeem_credit(self, customer: Customer, amount: Decimal) -> None:
        """Deducts `amount` from the customer's credit or raises InsufficientCredit; writes nothing on failure."""
        ...

    def find_customer(self, username: str) -> Optional[CustomerInfo]: ...
```

### 4.4 Core App: domain.py
- Contains the **records** that cross the seams between apps
- Uses frozen dataclasses (`frozen=True`) to prevent mutation after creation
- Records represent data transfer objects (DTOs) at the boundaries

```python
# Example from core/domain.py
from dataclasses import dataclass
from typing import Optional

@dataclass(frozen=True, slots=True)
class CatalogItem:
    id: int
    name: str
    description: str
    price: Decimal

    def __str__(self) -> str:
        return self.name

@dataclass(frozen=True, slots=True)
class CustomerInfo:
    id: int
    username: str
    email: str
    first_name: str
    last_name: str

    def __str__(self) -> str:
        return self.username
```

### 4.5 Inversion of Control
- The order service needs to know about products
- To keep it flexible, we **inject** a product service instance through the constructor
- **Important:** We only bind to the **interface** defined in core, not to specific implementations
- This is called **Inversion of Control** - dependencies are provided from an external source

---

## 5. Concrete Services Implementation

### 5.1 Implementing Core Interfaces
- Concrete apps (like `products`, `customers`) implement the interfaces defined in core
- The concrete service **does not derive** from the interface. A `Protocol` is satisfied **structurally**: a class satisfies it by simply having methods with the right names and signatures
- Note in the code below that `class ProductService:` inherits from nothing — yet it *is* a `ProductCatalog`
- A missing or misspelled method is caught by your type checker, and at runtime by `isinstance(service, ProductCatalog)`, which works because the Protocol is `@runtime_checkable`
- Services act as **adapters** between the core interfaces and the app's models

```python
# products/services.py
from typing import Optional
from core.domain import CatalogItem
from products.models import Product

# adapter for the ProductCatalog interface, backed by the products app's model
class ProductService:
    def __init__(self, repo=None):
        # default to Product.objects, but allow tests to pass a fake
        self.repo = repo or Product.objects

    def get_all_products(self) -> list[CatalogItem]:
        return [self._to_item(product) for product in self.repo.all()]

    def get_product(self, product_id) -> Optional[CatalogItem]:
        product: Optional[Product] = self.repo.get_by_id(product_id)
        return self._to_item(product) if product else None

    def _to_item(self, product: Product) -> CatalogItem:
        # in another product app, the logic for getting the price could be different
        return CatalogItem(id=product.id, name=product.name, description=product.description, price=product.price)
```

```python
# customers/services.py
from typing import Optional
from django.db import transaction
from core.domain import CustomerInfo, InsufficientCredit
from customers.models import Customer

# adapter for the Customers interface, backed by the customers app's model
class CustomerService:
    def __init__(self, repo=None):
        # default to Customer.objects, but allow tests to pass a fake
        self.repo = repo or Customer.objects

    def redeem_credit(self, customer: Customer, amount: Decimal) -> None:
        # atomic() nests: standalone it is a transaction, inside a caller's transaction it is a savepoint
        with transaction.atomic():
            # Lock the customer record for update, so that no other transaction can modify it
            locked: Customer = self.repo.select_for_update().get(pk=customer.pk)
            available: Decimal = locked.credit or Decimal("0")

            if available < amount:
                raise InsufficientCredit(required=amount, available=available)

            locked.credit = available - amount
            locked.save()

    def get_by_username(self, username: str) -> Optional[Customer]:
        return self.repo.get_by_username(username=username)

    def find_customer(self, username: str) -> Optional[CustomerInfo]:
        customer: Optional[Customer] = self.get_by_username(username=username)
        if customer:
            return CustomerInfo(id=customer.id, username=customer.username, email=customer.email,
                                first_name=customer.first_name, last_name=customer.last_name)
        return None
```

---

## 6. Dependency Injection

### 6.1 Service Dependencies
- The order service depends on functionality from product and customer services
- **Concrete instances** of these services are provided through the constructor
- We don't want to explicitly create dependencies during object creation
- A **dependency injection framework** handles this for us at runtime

### 6.2 Dependency Injection Framework
- [Dependency Injector Documentation](https://python-dependency-injector.ets-labs.org/)
- The framework injects dependencies automatically
- Services become **singletons** - only one instance exists during runtime

---

## 7. Dependency Injection Container

### 7.1 Container Configuration
- The DI framework needs to know which concrete implementations to use
- In the main project (`swd_django_demo`), we define a **container** in `containers.py`
- The container provides concrete instances of services
- The DI framework injects the instances defined in the container

```python
# swd_django_demo/containers.py
from dependency_injector import containers, providers
from customers.services import CustomerService
from orders.services import OrderService
from products.services import ProductService

# define a container class and declare its dependencies
class Container(containers.DeclarativeContainer):
    # configuration provider
    config = providers.Configuration()

    # Singleton provider for ProductService
    product_service = providers.Singleton(
        ProductService,
    )

    customer_service = providers.Singleton(
        CustomerService,
    )

    # Singleton provider for OrderService with product_service as a dependency
    order_service = providers.Singleton(
        OrderService, product_service=product_service, customer_service=customer_service
    )
```

### 7.2 Container Initialization
- The container needs to be **created** in the main project's `apps.py`
- **Wiring** is the process of injecting container providers into functions and methods
- Services themselves don't need wiring because they're already contained in the Container

```python
# swd_django_demo/apps.py
from django.apps import AppConfig
from django.conf import settings

class ProjectConfig(AppConfig):
    """Composition root: builds and wires the dependency-injection container."""

    name: str = "swd_django_demo"

    def ready(self) -> None:
        # ready() runs after every app's models are loaded, so the container
        # can import models from all apps without ordering issues.
        # The import stays inside ready() for the same reason: at module import
        # time the app registry is not populated yet.
        from swd_django_demo.containers import Container

        self.container = Container()
        self.container.config.from_dict(settings.__dict__)
        self.container.wire(modules=["products.views", "orders.views"])
```

---

## 8. Testing Dependencies

### 8.1 Unit Test Structure
- Test file: `swd_django_demo/test_dependency.py`
- The test structure and the test runner were introduced in SWD-LB-03 (section 7)

### 8.2 Testing Service Instantiation
```python
# swd_django_demo/test_dependency.py
from django.test import TestCase
import swd_django_demo

class TestDependencyContainer(TestCase):

    # this method is called before each test to set up the test environment
    # here we are getting the container from the swd_django_demo package
    def setUp(self):
        self.container = swd_django_demo.get_container()

    def test_product_service(self):
        # fetch the ProductService from the container
        product_service = self.container.product_service()
        # check that product_service is not None
        self.assertIsNotNone(product_service)

    def test_customer_service(self):
        # fetch the CustomerService from the container
        customer_service = self.container.customer_service()
        # check that customer_service is not None
        self.assertIsNotNone(customer_service)

    def test_order_service(self):
        # fetch the OrderService from the container
        order_service = self.container.order_service()
        # check that order_service is not None
        self.assertIsNotNone(order_service)
        # check that order_service has a product_service
        self.assertIsNotNone(order_service.product_service)
        # check that order_service has a customer_service
        self.assertIsNotNone(order_service.customer_service)
```

Run tests with:
```bash
just test
# or
uv run python manage.py test swd_django_demo.test_dependency -v 2
```

---

## 9. Assignment: Project Implementation

### 9.1 Tasks
- [ ] Create the interface definitions for your services in `core/interfaces.py`
  - [ ] Add protocol methods you are likely going to need
  - [ ] Use `typing.Protocol` and `@runtime_checkable` for interface definitions
  - [ ] Define the method signatures using the domain records from `core.domain`
- [ ] Create a new Django app for your services and register them in your project
- [ ] Create a `services.py` in your service apps
  - [ ] Create a class implementing the service interface from the core app
  - [ ] Create a concrete implementation of the methods defined in the interface
  - [ ] Use repository pattern with optional `repo` parameter for testability
- [ ] If your service depends on another service
  - [ ] Accept dependencies through constructor parameters
  - [ ] Define the providers in the project's `containers.py`
- [ ] Test if your services can be instantiated by adding a test method to `swd_django_demo/test_dependency.py` and run the tests

### 9.2 Key Concepts to Demonstrate
- **Loose coupling** between apps
- **Interface-based design** using Protocols
- **Dependency injection** pattern
- **Inversion of Control**
- **Testability** of components using repository pattern

---

*Updated to match current djangolight codebase*  
*Last updated: 2026-10-03*