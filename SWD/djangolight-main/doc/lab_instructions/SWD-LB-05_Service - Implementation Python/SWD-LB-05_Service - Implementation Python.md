# SWD-LB-05: Service Implementation and Mocking in Python/Django

**Course:** Software Design (SWD) Lab  
**Instructors:** Bernhard Danninger, Josef Lettner, Henrik Binggl  
**Version:** Updated for Current Codebase (2026)

---

## Goal of This Unit

> **You will be able to:** implement the business logic of your use cases in services, and decide for each test whether to mock a dependency or run against the test database.
>
> **You hand in (as part of the final zip):**
> - 1 activity diagram and 1 sequence diagram for your own ITS project
> - the service / business logic for your two chosen use cases
> - at least one unit test per use case, isolating the service under test
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

## 2. Unit Testing with `unittest.mock`

### 2.1 Motivation
- Unit tests should be **isolated** from other dependencies
- Often a test needs a **specific setup** (e.g. records in the database) that is expensive or slow to create
- A test should exercise **one unit** and fail for one reason

### 2.2 Solution
- Only start the infrastructure needed for the test
- Create **test replacements (mocks)** for the real dependencies
- [unittest.mock documentation](https://docs.python.org/3/library/unittest.mock.html)

> **In this project:** Mocking is used **selectively**, not universally. Tests that are about database behaviour (credit redemption, product lookup, order persistence) run against the SQLite test database and inherit from `django.test.TestCase`. Mocks are used where a dependency has its **own** tests elsewhere and would only add noise — most notably the `CustomerService` is mocked in `orders/test_service.py`, because credit handling is covered by `customers/test_service.py`.

---

## 3. What This Project Mocks, and What It Does Not

There is no rule that says *mock everything*. The project mocks a dependency only when that dependency has its own tests elsewhere; otherwise the test runs against the SQLite test database. The three service test files show the split:

| Test file | Base class | What it tests | What is mocked |
|---|---|---|---|
| `customers/test_service.py` | `django.test.TestCase` | Credit redemption against the SQLite test DB (locking, updating a row) | Nothing — the database *is* the point |
| `products/test_services.py` | `django.test.TestCase` | The `ProductService` adapter against real products | Nothing |
| `orders/test_service.py` | `django.test.TestCase` | `OrderService` logic (pricing, ordering, summaries) against the test DB | `CustomerService` (a `Mock`) |

`django.test.TestCase` itself derives from `unittest.TestCase`, so all `unittest` assertion methods (`assertEqual`, `assertRaises`, `assertIsInstance`, ...) are still available. The difference is that Django sets up a test database and wraps each test in a transaction that is rolled back, which is why the persistence tests can run against the database without polluting each other.

---

## 4. Mocking a Dependency

### 4.1 The Order Service and Its Dependencies
The `OrderService` depends on a `ProductCatalog` and a `Customers` interface, injected through the constructor. It only talks to the **interfaces** from `core.interfaces`, never to concrete apps:

```python
# orders/services.py
from core.interfaces import Customers, ProductCatalog

class OrderService:
    def __init__(self, product_service: ProductCatalog, customer_service: Customers):
        self.product_service = product_service
        self.customer_service = customer_service
```

### 4.2 Mocking the Customer Service
The order test does not want to re-test credit redemption, so it replaces the `CustomerService` with a `Mock` and wires the real `ProductService` into the `OrderService`:

```python
# orders/test_service.py
from unittest.mock import Mock

from django.test import TestCase

from core.domain import EmptyBasket, InsufficientCredit
from customers.models import Customer
from orders.basket import Basket
from orders.models import Order, OrderPosition
from orders.services import OrderService
from products.models import Product
from products.services import ProductService


class TestOrderService(TestCase):
    def setUp(self):
        # real products in the test database, so pricing is real
        self.apple = Product.objects.create(name="apple", description="", price=1.5)
        self.pear = Product.objects.create(name="pear", description="", price=2.0)
        self.customer = Customer.objects.create(username="alice", credit=100)

        # the customer service is mocked — credit handling has its own tests
        self.customer_service = Mock()
        self.order_service = OrderService(ProductService(), self.customer_service)
        self.basket = Basket([self.apple.id, self.pear.id, self.apple.id])
```

The `setUp` method runs before each test method. It creates the real records the test needs and the `Mock` for the dependency it wants to isolate.

### 4.3 Setting Return Values and Asserting Calls
A `Mock` returns a new `Mock` by default. The test sets the return value or side effect it needs and then asserts that the expected method was called:

```python
    def test_create_order_customer_has_credit(self):
        order = self.order_service.create_order(self.customer, self.basket)

        self.assertEqual(Order.objects.get(), order)
        self.assertEqual(order.user, self.customer)
        self.assertEqual(order.total_price, 5.0)
        # the mock lets us assert that redeem_credit was called exactly once, with these args
        self.customer_service.redeem_credit.assert_called_once_with(self.customer, 5.0)
```

### 4.4 Raising an Exception from a Mock
To test the failure path, set `side_effect` to the exception the real service would raise. The mock then raises it when called:

```python
    def test_create_order_customer_without_credit(self):
        self.customer_service.redeem_credit.side_effect = InsufficientCredit(required=5.0, available=1.0)

        with self.assertRaises(InsufficientCredit):
            self.order_service.create_order(self.customer, self.basket)

        self.customer_service.redeem_credit.assert_called_once()
        # credit is redeemed before anything is written, so nothing is persisted
        self.assertEqual(Order.objects.count(), 0)
        self.assertEqual(OrderPosition.objects.count(), 0)
```

### 4.5 Asserting a Mock Was Not Called
For the case where the service must bail out before reaching the dependency, `assert_not_called()` checks the mock was never touched:

```python
    def test_empty_basket_is_rejected_before_touching_credit(self):
        with self.assertRaises(EmptyBasket):
            self.order_service.create_order(self.customer, Basket())

        self.customer_service.redeem_credit.assert_not_called()
        self.assertEqual(Order.objects.count(), 0)
```

---

## 5. Mocking the Model: the Repository Pattern

### 5.1 Patching vs. Injecting
A common alternative is to patch `Order` and `OrderPosition` where they are imported, using `unittest.mock.patch` with a `tearDown` to stop the patches. This project does **not** patch models. Instead the services accept an optional `repo` parameter, so a test can pass an in-memory fake instead of patching the ORM:

```python
# customers/services.py
class CustomerService:
    def __init__(self, repo=None):
        # default to Customer.objects, but allow tests to pass a fake
        self.repo = repo or Customer.objects

    def redeem_credit(self, customer: Customer, amount: Decimal) -> None:
        with transaction.atomic():
            locked: Customer = self.repo.select_for_update().get(pk=customer.pk)
            ...
```

```python
# products/services.py
class ProductService:
    def __init__(self, repo=None):
        # default to Product.objects, but allow tests to pass a fake
        self.repo = repo or Product.objects
```

### 5.2 Why This Replaces Patching
- No `patch` / `stop` bookkeeping and no `tearDown` needed
- The seam is explicit in the constructor, not hidden in an import
- The same mechanism works for both the model (`repo`) and other services (constructor arguments)
- The credit and product tests run against the real test database because that behaviour is what they want to verify

### 5.3 When You Do Want `patch`
`unittest.mock.patch` is still the right tool when you need to replace something you do not control through the constructor — for example a function called inside the code under test, or a module-level singleton. The pattern is to patch **where the name is looked up**, not where it is defined:

```python
from unittest.mock import patch

# if orders/services.py does `from orders.models import Order`,
# patch orders.services.Order, not orders.models.Order
with patch("orders.services.Order") as mock_order:
    ...
```

---

## 6. Common Mock Assertions

| Assertion | Meaning |
|---|---|
| `mock.assert_called_once()` | The mock was called exactly once |
| `mock.assert_called_once_with(a, b)` | Called exactly once, with these arguments |
| `mock.assert_called_with(a, b)` | The most recent call used these arguments |
| `mock.assert_not_called()` | The mock was never called |
| `mock.call_count` | How many times the mock was called |
| `mock.return_value` | The value returned when the mock is called |
| `mock.side_effect = Exception(...)` | The mock raises this exception when called |

[Full `unittest.mock` documentation](https://docs.python.org/3/library/unittest.mock.html)

---

## 7. Running Tests

```bash
# Run all tests from the project root
just test
# or
uv run python manage.py test -v 2

# Run the service tests for a specific app
uv run python manage.py test orders.test_service -v 2
uv run python manage.py test customers.test_service -v 2
uv run python manage.py test products.test_services -v 2

# Run a single test method
uv run python manage.py test orders.test_service.TestOrderService.test_create_order_customer_has_credit -v 2
```

**PyCharm:** Right-click the test file or class and "Run 'Test:'"

---

## 8. Assignment: Project Implementation

### 8.1 Tasks
- [ ] Implement the service / business logic for your use cases
  - [ ] interacting with persistence (your models)
  - [ ] and with other services
- [ ] Create at least one unit test for each selected use case to test the service logic
  - [ ] Decide **per dependency** whether to mock it. The rule used in this project:
    - [ ] **Mock** another service that the service under test calls, when that service already has its own tests — pass a `Mock` through the constructor
    - [ ] **Do not mock** the database when the database behaviour *is* what you are testing — inherit from `django.test.TestCase` and let Django give you a test database
    - [ ] Keep the `repo` parameter on your services as a seam, so you *can* pass a fake when a test would otherwise need an expensive setup
  - [ ] Justify one of your choices in a comment: why is this dependency mocked, and that one not?
  - [ ] Use `assert_called_once_with` / `assert_not_called` to verify interactions
  - [ ] Use `side_effect` to simulate exceptions from a mocked dependency

### 8.2 Key Concepts to Demonstrate
- **Service layer** separate from models and views
- **Dependency injection** through constructor parameters
- **Mocking** of dependencies with `unittest.mock.Mock`
- **Repository pattern** as an alternative to `patch` for the persistence layer
- **Assertion** of interactions and exception paths

---

*Updated to match current djangolight codebase*  
*Last updated: 2026-09-06*
