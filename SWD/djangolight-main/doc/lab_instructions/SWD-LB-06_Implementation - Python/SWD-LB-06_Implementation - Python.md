# SWD-LB-06: Implementation in Python/Django

**Course:** Software Design (SWD) Lab  
**Instructors:** Bernhard Danninger, Josef Lettner, Henrik Binggl  
**Version:** Updated for Current Codebase (2026)

---

## Goal of This Unit

> **You will be able to:** build the frontend of your use cases with Django's Model-View-Template pattern, pass records instead of models across the layers, and test views with mocked services.
>
> **You hand in (as part of the final zip):**
> - the user interface for your two chosen use cases, following MVT
> - at least one UI unit test per use case, using a test container with mocked services
> - the reworked design documents, updated to match what you actually implemented
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

## 2. Model-View-Template (MVT) Pattern

### 2.1 Overview
Django implements the **Model-View-Template** pattern, its variant of MVC:
- **Model** — the data layer (Django ORM models)
- **View** — the function/class that receives an HTTP request, calls the service logic and prepares data
- **Template** — the HTML that renders the data

- [Django MVT introduction](https://dev.to/akolade/django-mvt-an-introduction-to-the-model-view-template-architecture-of-a-django-app-23b9)
- [Django tutorial part 3](https://docs.djangoproject.com/en/stable/intro/tutorial03/)
- [Django templates](https://docs.djangoproject.com/en/stable/topics/templates/)

### 2.2 URL Routing
An incoming request is matched against the URL patterns defined in `urls.py`. Each pattern maps to a view:

```
http://127.0.0.1:8000/products/
        │
        ▼
  swd_django_demo/urls.py      path("products/", include("products.urls"))
        │
        ▼
  products/urls.py              path("", views.index, name="index")
        │
        ▼
  products/views.py            def index(request, ...): ...
```

```python
# swd_django_demo/urls.py
from django.contrib import admin
from django.urls import include, path

urlpatterns = [
    path("", include("core.urls")),
    path("products/", include("products.urls")),
    path("orders/", include("orders.urls")),
    path("admin/", admin.site.urls),
]
```

### 2.3 The View
A view receives the request, calls the service logic, builds the context and renders the template. When using dependency injection, the view is wired in the project's `apps.py` so the container can inject the services:

```python
# products/views.py
from dependency_injector.wiring import Provide, inject
from django.http import HttpRequest, HttpResponse
from django.template import loader

from core.interfaces import ProductCatalog


@inject
def index(
    request: HttpRequest,
    product_service: ProductCatalog = Provide["product_service"],
) -> HttpResponse:
    """the product catalog: all available offerings (basket_count comes from orders.context_processors.basket)"""
    context = {
        "products_list": product_service.get_all_products(),
    }
    template = loader.get_template("products/index.html")
    return HttpResponse(template.render(context, request))
```

The `@inject` decorator and `Provide["product_service"]` tell the dependency-injection container which service to inject. The wiring happens once in the project's `apps.py`:

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

> **CSRF protection:** every view that changes state (`add_to_basket`, `clear_basket`, `place_order`)
> accepts only POST, and every form renders `{% csrf_token %}`. Never turn the protection off with
> `@csrf_exempt`, and never change state on a GET request: any other website could trigger it in the
> customer's browser. `orders/test_ui.py` (`BasketCsrfTests`) shows how to test this with
> `Client(enforce_csrf_checks=True)`.

A view that needs to handle success and failure paths can use Django's `messages` framework and redirect:

```python
# orders/views.py
@login_required
@require_POST
@inject
def place_order(request: HttpRequest, order_service: OrderService = Provide["order_service"]) -> HttpResponse:
    try:
        basket = Basket.load(request.session)
        order = order_service.create_order(request.user, basket)
        basket.clear()
        messages.success(request, f"Thank you! Your order #{order.pk} has been placed.")
        return redirect("products:index")

    except OrderError as e:
        # expected business outcomes are shown to the customer; anything else is a bug and surfaces as 500
        logger.info(f"order rejected: {e}")
        messages.error(request, str(e))

    return redirect("orders:basket_overview")
```

---

## 3. Templates

### 3.1 Template Inheritance
Each HTML page derives from a base template defined in the `core` app. The base template defines the navigation, message alerts and a `{% block content %}` placeholder that child templates override:

```html
<!-- core/templates/core/base.html -->
<!doctype html>
<html lang="en">
  <head>
    <title>{% block title %}SWD Demo Shop{% endblock %}</title>
    ...
  </head>
  <body>
    <nav>...</nav>
    <main class="container py-4 flex-grow-1">
      {% block content %}{% endblock %}
    </main>
    <footer>...</footer>
  </body>
</html>
```

```html
<!-- products/templates/products/index.html -->
{% extends 'core/base.html' %}
{% block title %}Products · SWD Demo Shop{% endblock %}

{% block content %}
  {% for product in products_list %}
    <h2>{{ product.name }}</h2>
    <span>{{ product.price|floatformat:2 }} &euro;</span>
  {% endfor %}
{% endblock %}
```

### 3.2 Template Tags and Variables
- `{% %}` tags control flow: `{% for %}`, `{% if %}`, `{% extends %}`, `{% block %}`, `{% url %}`, `{% csrf_token %}`
- `{{ }}` variables render values from the context provided by the view: `{{ product.name }}`, `{{ order.total_price }}`
- Filters modify values: `{{ product.price|floatformat:2 }}`, `{{ basket_count|pluralize }}`

### 3.3 Context Processors
A context processor makes data available to **every** template without each view having to pass it. The shop uses one to show the basket count in the navigation badge on every page:

```python
# orders/context_processors.py
from orders.basket import Basket


def basket(request) -> dict:
    """Makes the basket item count available to every template (navigation badge)."""
    session = getattr(request, "session", None)
    return {"basket_count": len(Basket.load(session)) if session is not None else 0}
```

---

## 4. Sessions

### 4.1 Storing Data in the Session
- The session is a dictionary-like object; you can do all normal dictionary operations on it
- Data stored in the session must be **JSON-serializable**
- By default you cannot store custom objects in the session (that would need additional serializers such as Marshmallow)
- For this project it is sufficient to store only the **IDs** of model objects (and primitive datatypes)

```python
# orders/basket.py
class Basket:
    SESSION_KEY = "product_list"

    def __init__(self, product_ids: Iterable = (), session: Optional[MutableMapping] = None):
        # session data must be JSON serializable, so ids are kept as strings
        self._product_ids: list[str] = [str(product_id) for product_id in product_ids]
        self._session = session

    @classmethod
    def load(cls, session: MutableMapping) -> "Basket":
        return cls(session.get(cls.SESSION_KEY, []), session)

    def add(self, product_id) -> None:
        self._product_ids.append(str(product_id))
        if self._session is not None:
            self._session[self.SESSION_KEY] = list(self._product_ids)
```

The basket is loaded from and stored in the session inside the views:

```python
# orders/views.py — adding to the basket
Basket.load(request.session).add(product_id)

# placing the order
basket = Basket.load(request.session)
order = order_service.create_order(request.user, basket)
basket.clear()
```

---

## 5. Records: Data Transfer Between Layers

### 5.1 Why Not Pass ORM Models Around
Django ORM models are **Active Records**: bound to the database, with `.pk` unset until saved and foreign keys that cannot be followed before saving. Exposing them to views, templates or other apps couples the layers to the database and to a specific app. Views and templates only need to **read** data, and the basket must be priced without writing anything.

### 5.2 Records as Frozen Dataclasses
The demo project uses **frozen dataclasses** as plain, immutable records that cross the seams between apps. They are defined in `core/domain.py` as a stable contract between layers:

```python
# core/domain.py
from dataclasses import dataclass
from datetime import datetime
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
class OrderLine:
    pos: int
    product: CatalogItem
    quantity: int
    price: Decimal  # unit price at the time the basket was priced

    @property
    def total(self) -> Decimal:
        return self.price * self.quantity


@dataclass(frozen=True, slots=True)
class OrderSummary:
    """A priced basket: what the customer is about to order, before anything is written to the database."""
    order_number: int  # 0 until the order is persisted
    customer: Optional[CustomerInfo]
    lines: list[OrderLine]
    total_price: Decimal
    created_at: Optional[datetime] = None  # None until the order is persisted
```

### 5.3 Why `frozen=True` and `slots=True`

| Feature | What it gives the records |
|---|---|
| `@dataclass` decorator | Auto-generates `__init__`, `__repr__`, `__eq__` |
| Type annotations | Enforce structure (`id: int`, `name: str`, ...) |
| `frozen=True` | Prevents accidental modification; a catalog item shown in the basket keeps exactly the price it had when looked up. Frozen records are also hashable |
| `slots=True` | Fixed attributes, no per-instance `__dict__`; a typo like `item.pric = 1` raises instead of creating a new attribute, and the records are smaller and faster |
| `dataclasses.asdict()` | Easy conversion to plain dicts if needed for JSON |
| No Django ORM dependency | Lightweight, framework-independent, safe to use anywhere |

### 5.4 Alternatives
Plain classes, `NamedTuple`, [Pydantic](https://docs.pydantic.dev/) or serializer frameworks are valid alternatives. The project uses frozen dataclasses because they are standard-library, zero-dependency and expressive enough for the stable contract at the seams.

### 5.5 Naming
The records are named for their **role at the seam** (what the caller sees), not for the model they are copied from: `CatalogItem` is what the `ProductCatalog` interface shows, `CustomerInfo` is what the `Customers` interface knows about a customer.

---

## 6. Unit Testing the UI

### 6.1 What UI Tests Cover
UI tests exercise the view and the template together: the HTTP result, the template used and the context data. They need parts of Django to run, so they inherit from `django.test.TestCase` and use the Django test client:

```python
# products/test_views.py
from django.test import TestCase
from django.urls import reverse
```

### 6.2 Mocking Services with a Test Container
Views do not have constructors to pass dependencies into — the container injects them. To isolate the view, the test builds a **test container** that provides a mocked service, then wires it into the view modules:

```python
# products/test_views.py
from unittest.mock import MagicMock

from dependency_injector import containers, providers
from django.test import TestCase
from django.urls import reverse

import swd_django_demo
from core.domain import CatalogItem
from core.interfaces import ProductCatalog


class TestContainer(containers.DeclarativeContainer):
    """replaces the catalog adapter, so the view is tested without a database"""
    product_service = providers.Singleton(MagicMock, spec=ProductCatalog)


class TestProductIndex(TestCase):
    def setUp(self):
        self.items = [
            CatalogItem(id=1, name="testproduct1", description="testdescription1", price=1.0),
            CatalogItem(id=2, name="testproduct2", description="testdescription2", price=2.5),
        ]
        container = TestContainer()
        container.product_service().get_all_products.return_value = self.items
        container.wire(modules=["products.views"])

    def tearDown(self):
        # restore the production wiring so later tests do not see the mocks
        swd_django_demo.get_container().wire(modules=["products.views"])
```

### 6.3 Asserting the HTTP Result, Template and Context
The test client issues a request; the test then checks the status code, the template used and the context data:

```python
    def test_index_lists_the_catalog(self):
        response = self.client.get(reverse("products:index"))

        self.assertEqual(response.status_code, 200)
        self.assertTemplateUsed(response, "products/index.html")
        self.assertEqual(list(response.context["products_list"]), self.items)
        self.assertContains(response, "testproduct2")
        self.assertContains(response, "2.50 &euro;")
```

### 6.4 Mocking Models Too
The orders UI test goes further and also mocks the product service with a `spec`, so only `ProductCatalog` methods are allowed. A factory provider builds `CatalogItem` records as test data:

```python
# orders/test_ui.py
class TestContainer(containers.DeclarativeContainer):
    product_factory = providers.Factory(CatalogItem, price=0.0)

    product_service = providers.Singleton(
        MagicMock,
        spec=ProductCatalog
    )


class ViewTests(TestCase):
    def setUp(self):
        self.mock_product_list = [
            TestContainer.product_factory(id=1, name="testproduct1", description="testdescription1"),
            TestContainer.product_factory(id=2, name="testproduct2", description="testdescription2"),
        ]
        c = TestContainer()
        c.product_service().get_all_products.return_value = self.mock_product_list
        c.product_service().get_product.side_effect = lambda product_id: self.mock_product_list[int(product_id) - 1]
        c.wire(modules=["products.views", "orders.views"])
```

Some order tests use the **real** `OrderService` and real products in the test database, because the basket overview is priced by the production `OrderService` — the mock is only for the catalog the view lists, not for the order logic itself.

### 6.5 Common UI Test Assertions

| Assertion | Meaning |
|---|---|
| `self.client.get(url)` | Issue a GET request through the test client |
| `response.status_code` | The HTTP status code (200, 302, 404, ...) |
| `self.assertTemplateUsed(response, "x.html")` | The template was rendered |
| `response.context["key"]` | The value the view passed to the template |
| `self.assertContains(response, "text")` | The rendered HTML contains the text |
| `self.assertNotContains(response, "text")` | The rendered HTML does not contain the text |
| `self.assertRedirects(response, url)` | The response redirects to the given URL |

---

## 7. Running Tests

```bash
# Run all tests
just test
# or
uv run python manage.py test -v 2

# Run the UI tests for a specific app
uv run python manage.py test products.test_views -v 2
uv run python manage.py test orders.test_ui -v 2

# Run a single test method
uv run python manage.py test products.test_views.TestProductIndex.test_index_lists_the_catalog -v 2
```

**PyCharm:** Right-click the test file or class and "Run 'Test:'"

---

## 8. Assignment: Project Implementation

### 8.1 Requirements
- Client-server application according to the UML definition
- Persistence is obtained using the ORM (Django ORM)
- Implementation of the chosen two (non-trivial) use cases
- According to the Model-View-Template pattern

### 8.2 Tasks
- [ ] Implement the user interface for your two use cases according to the UML definition
  - [ ] Wire the views in the project's `apps.py` for dependency injection
  - [ ] Define templates that extend the base template in `core/templates/core/base.html`
  - [ ] Use records (frozen dataclasses) to pass data from services to views and templates
  - [ ] Use the session for any client-side state (e.g. basket), storing only JSON-serializable IDs
- [ ] Create at least one unit test for each selected use case to test the MVT logic
  - [ ] Isolate the view from the business logic: build a test container whose providers are `MagicMock(spec=YourInterface)` and wire it into your view modules
  - [ ] Use a **factory provider** to build records as test data, so the test needs no database rows
  - [ ] You do **not** have to mock everything: where a view is priced or driven by a service you also want to exercise, use the real service and real rows in the test database, as `orders/test_ui.py` does. Mock the parts that have their own tests elsewhere
  - [ ] Restore the production wiring in `tearDown` — otherwise your mocks leak into every later test
  - [ ] Assert the HTTP result, template used and context data

### 8.3 Key Concepts to Demonstrate
- **Model-View-Template** pattern in Django
- **Dependency injection** wired in the project's `apps.py`
- **Records** (frozen dataclasses) as the contract between layers instead of ORM models
- **Sessions** for client-side state, storing only JSON-serializable data
- **Template inheritance** from a base template
- **UI testing** with mocked services and a test container

---

*Updated to match current djangolight codebase*  
*Last updated: 2026-09-06*
