# SWD-LB-03: Classes and Unit Tests in Python/Django

**Course:** Software Design (SWD) Lab  
**Instructors:** Bernhard Danninger, Josef Lettner, Henrik Binggl  
**Version:** Updated for Current Codebase (2026)

---

## Goal of This Unit

> **You will be able to:** map business entities to Django models, connect the models of different apps, and write unit tests that verify persistence and associations.
>
> **You hand in (as part of the final zip):**
> - 1 class diagram of your business objects
> - the models of your first business entities, registered in the admin site
> - two unit tests: one for creating and reloading an entity, one for its associations
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

## 2. Persisting Data with Django Models

### 2.1 Django ORM Framework
- Django comes with its own ORM framework based on the **Active Record Pattern**
- For this project we use **SQLite Database** to store information
- Business entities are defined in `models.py`
- [Django Model Documentation](https://docs.djangoproject.com/en/stable/topics/models/)
- **Important:** Models contain only the **structure** of business objects, not the business **behavior**. A `__str__` method and a custom manager with query methods are fine; business rules (may this customer order? what does this order cost?) belong in a service

### 2.2 Loose Coupling with Models
- Goal: Keep apps loosely coupled
- Example: The orders app contains references to products and customers
- In a relational database, these references are **foreign keys** — and that is what we use
- The coupling stays at the **data level**: the orders *code* (services, views, basket) never imports
  another app's models. How the apps talk to each other without importing each other's code — interfaces
  in `core/interfaces.py` and services — is the topic of SWD-LB-04, which builds on the models from this unit

---

## 3. Defining the Models

### 3.1 Core Model Definition
- The demo project has a special app called `core`. It holds what all other apps share; every other app
  depends on it, and it depends on none of them. In this unit it contributes the **abstract models**
- Define the **minimum fields** a product should have in `core/models.py`
- Each Django model derives from `models.Model`
- Make the class **abstract** so it cannot be instantiated directly and gets no table of its own
- The common fields for a product are defined here

```python
# core/models.py
from django.contrib.auth.models import AbstractUser
from django.db import models

from core.managers import CoreProductManager

# we define an abstract class Product here, so that we can use different types of products later
class Product(models.Model):
    # Use custom manager to inherit the queryset methods from the CoreProductManager
    objects = CoreProductManager()

    # Make this class abstract to prevent it from being instantiated directly as it serves as an interface
    class Meta:
        abstract = True

    # each product has a name and a description
    name: str = models.CharField(max_length=200)
    description: str = models.TextField()

    def __str__(self) -> str:
        return self.name

class Customer(AbstractUser):
    class Meta:
        abstract = True

    def __str__(self) -> str:
        return self.username
```

### 3.2 Concrete Model Implementation
- Implement the concrete product in the product app's `models.py`
- It inherits from the abstract `core.Product`; the inherited and the additional fields end up in **one** table
- Store money in a `DecimalField` and work with `decimal.Decimal`, never `FloatField`/`float`: binary floats cannot
  represent most cent amounts exactly (`0.1 + 0.2 != 0.3`), so sums drift and credit checks can give the wrong answer

```python
# products/models.py
from decimal import Decimal

from django.db import models

from core.models import Product as CoreProduct
from products.managers import ProductManager

# this is the concrete model that we will use for the project
class Product(CoreProduct):
    objects: ProductManager = ProductManager()

    # we are adding a price field to the product model
    price: Decimal = models.DecimalField(max_digits=10, decimal_places=2)
```

```python
# customers/models.py
from decimal import Decimal

from django.db import models

from core.models import Customer as CoreCustomer
from customers.managers import CustomerManager

# this is the concrete model that we will use for the project
class Customer(CoreCustomer):
    objects: CustomerManager = CustomerManager()

    # we are adding a credit field to the customer model
    credit: Decimal = models.DecimalField(max_digits=10, decimal_places=2, null=True)
```

### 3.3 The Custom User Model
- The customer is at the same time the Django **user** model, so it can log in
- Django's documented way to replace the built-in user is the `AUTH_USER_MODEL` setting
- Set it **before the first `migrate`** — changing it later requires resetting the database (see 5.3)
- [Django: Substituting a custom User model](https://docs.djangoproject.com/en/stable/topics/auth/customizing/#substituting-a-custom-user-model)

```python
# swd_django_demo/settings.py
AUTH_USER_MODEL = "customers.Customer"
```

### 3.4 Referencing Models of Other Apps
- Reference other apps' models with a **string** `"app_label.ModelName"` — no import needed
- Reference the user model through `settings.AUTH_USER_MODEL`, never through the concrete class
- `on_delete=models.PROTECT` prevents deleting a product or customer that is referenced by an order

```python
# orders/models.py
from django.conf import settings
from django.db import models

from orders.managers import OrderManager

class Order(models.Model):
    # We use our own manager to manage instances of the Order model
    objects: OrderManager = OrderManager()
    # the customer who placed the order
    user: models.ForeignKey = models.ForeignKey(settings.AUTH_USER_MODEL, on_delete=models.PROTECT)
    total_price: models.DecimalField = models.DecimalField(max_digits=10, decimal_places=2)
    created_at: models.DateTimeField = models.DateTimeField(auto_now_add=True)

class OrderPosition(models.Model):
    # Django does not support composite primary keys, so we have to use a unique_together constraint
    class Meta:
        # This constraint ensures that each (order, pos) pair is unique
        unique_together = (('order', 'pos'),)

    # the related name lets us access the positions of an order as order.order_positions
    order: models.ForeignKey[Order] = models.ForeignKey(Order, on_delete=models.CASCADE,
                                                        related_name='order_positions')
    # the position of this order position within the order
    pos: models.PositiveIntegerField = models.PositiveIntegerField()
    # the ordered product, referenced by string to avoid importing the products app
    product: models.ForeignKey = models.ForeignKey("products.Product", on_delete=models.PROTECT)
    quantity: models.IntegerField = models.IntegerField()
    price: models.DecimalField = models.DecimalField(max_digits=10, decimal_places=2)  # unit price at the time the order was placed
```

---

## 4. Where the Apps Depend on Each Other

- **Data level:** foreign keys from `orders` to `products` and `customers`. `makemigrations` records them as
  migration `dependencies`, so the products and customers tables are created first
- **Code level:** keeping the apps' *code* independent is the topic of SWD-LB-04 (interfaces, services and
  dependency injection)
- **Configuration:** the main project's `settings.py` decides which apps exist (`INSTALLED_APPS`) and which
  model is the user (`AUTH_USER_MODEL`)

---

## 5. Database Migrations

### 5.1 Creating Migrations
- Migrations are Django's way to create and update the database according to your models
- [Django Migrations Documentation](https://docs.djangoproject.com/en/stable/topics/migrations/)

```bash
# Create SQL DDL statements reflecting model changes
uv run python manage.py makemigrations

# Apply the changes to the database
uv run python manage.py migrate
```

### 5.2 Connecting to the Database
- Connect to the SQLite database with any SQL client
- Use the SQLite3 JDBC driver opening the file `db.sqlite3` in the project directory
- Some IDEs provide direct database access

### 5.3 Resetting Migrations
If migrations or database get corrupted:

> **Warning:** this throws away the database, including your admin user and any data you entered
> through the admin site. Because the project uses a custom user model (`AUTH_USER_MODEL =
> "customers.Customer"`), deleting the `customers` migrations without also deleting `db.sqlite3`
> leaves the database in a state Django cannot migrate — always do both steps together.
>
> **Never delete migrations recursively from the project root.** Django and every third-party
> package ship their own `migrations/` folders inside `.venv/`. A recursive delete from the root
> matches those too and destroys your virtual environment, including Django's migration framework
> itself. Always name your own apps explicitly, as below.

```bash
# 1. Delete the migration files of YOUR apps only, keeping __init__.py (bash/zsh)
#    List your own apps here — never run this with "." as the search path.
find core customers products orders -path "*/migrations/*.py" -not -name "__init__.py" -delete
find core customers products orders -path "*/migrations/*.pyc" -delete

# 2. Delete the database file
rm db.sqlite3

# 3. Recreate everything
uv run python manage.py makemigrations
uv run python manage.py migrate
uv run python manage.py createsuperuser
uv run python manage.py seed_catalog     # reload the product catalog
```

On Windows (PowerShell 7) the first two steps look like this:

```powershell
# 1. Delete the migration files of YOUR apps only — list them explicitly
Get-ChildItem -Path .\core, .\customers, .\products, .\orders -Recurse -Include *.py, *.pyc |
    Where-Object { $_.DirectoryName -like '*migrations*' -and $_.Name -ne '__init__.py' } |
    Remove-Item

# 2. Delete the database file
Remove-Item db.sqlite3
```

If your virtual environment ever does get damaged this way, `rm -rf .venv && uv sync` rebuilds it.

---

## 6. Django Admin Site

### 6.1 Registering Models
- Django provides an easy way to populate the database using the admin site
- Register models in the app's `admin.py` to make them available in the admin interface

```python
# products/admin.py
from django.contrib import admin
from .models import Product

admin.site.register(Product)
```

The customer model replaces Django's user model, so it is registered with a `UserAdmin` subclass
instead of the plain registration — otherwise the admin form loses the password handling and the
custom `credit` field:

```python
# customers/admin.py
from django.contrib import admin
from django.contrib.auth.admin import UserAdmin
from .models import Customer


class CustomerAdmin(UserAdmin):
    model = Customer
    # If you have any custom fields in your Customer model, make sure to add them here
    fieldsets = UserAdmin.fieldsets + (
        (None, {'fields': ('credit',)}),
    )


# Register your custom admin with the Customer model
admin.site.register(Customer, CustomerAdmin)
```

### 6.2 Accessing Admin Site
- Access through your browser: [http://localhost:8000/admin](http://localhost:8000/admin)
- Log in with admin credentials created via `just adminuser` or `uv run python manage.py createsuperuser`

---

## 7. Unit Testing

### 7.1 unittest Framework
- `unittest` is Python's de-facto standard unit testing framework
- A unit test may be re-tested at any time
- Execution time should be small (inner loop optimization)
- [Python unittest Documentation](https://docs.python.org/3/library/unittest.html)
- [Django Testing Documentation](https://docs.djangoproject.com/en/stable/topics/testing/overview/)

> **In this project:** All tests inherit from `django.test.TestCase` (or `django.test.TransactionTestCase` for concurrency tests). `django.test.TestCase` itself derives from `unittest.TestCase`, so all `unittest` features (`assertEqual`, `assertTrue`, `setUp`, etc.) are still available. Plain `unittest.TestCase` is only used in the pure-Python demo `swd_django_demo/test_simple.py` to illustrate the framework without starting Django services.

### 7.2 Test Structure
- Create files starting with `test_` in the app directory
- Test classes should inherit from `unittest.TestCase` or `django.test.TestCase`
- Name test methods with `test_*` prefix — this is what the test runner discovers
- Naming test classes with a `Test*` prefix is a convention, not a requirement; the project's
  `customers/test_concurrency.py` uses a `...TestCase` suffix instead
- Use `self.assert*` methods to check expected behavior

### 7.3 Simple Unit Test Example (Pure Python)
- Plain `unittest.TestCase` - does not start Django services
- For testing logic without database interaction

```python
# Example of a simple unit test (no database)
import unittest

class TestStringMethods(unittest.TestCase):
    def test_upper(self):
        self.assertEqual('foo'.upper(), 'FOO')

    def test_isupper(self):
        self.assertTrue('FOO'.isupper())
        self.assertFalse('Foo'.isupper())

if __name__ == '__main__':
    unittest.main()
```

### 7.4 Custom Managers: Where Query Methods Live

Before the next example: a Django **manager** is the object behind `Model.objects`. Putting your
queries into a manager keeps them out of the services and gives them a name. The demo project does
this in `products/managers.py`:

```python
# products/managers.py
# this can be seen as our repository for our model
from decimal import Decimal
from typing import TYPE_CHECKING, Optional

from django.db import models

from core.managers import CoreProductManager

if TYPE_CHECKING:
    # products/models.py imports this module, so a runtime import would be circular
    from products.models import Product


class ProductManager(CoreProductManager):
    def get_by_id(self, id: int) -> Optional["Product"]:
        return self.get_queryset().filter(id=id).first()

    def get_by_name(self, name: str) -> Optional["Product"]:
        return self.get_queryset().filter(name=name).first()

    def get_by_price_range(self, min_price: Decimal, max_price: Decimal) -> models.QuerySet["Product"]:
        # Query for Product objects by price range
        return self.get_queryset().filter(price__gte=min_price, price__lte=max_price)
```

`ProductManager` extends `CoreProductManager` from the core app, so the concrete product inherits
the query methods the core app already defines and adds its own on top.

The manager is attached to the model with `objects = ProductManager()` (see `products/models.py`
above), which is what makes `Product.objects.get_by_name(...)` work in the test below. If you
copy that test onto your own model without writing a manager first, you will get an
`AttributeError` — use the built-in `Model.objects.get(name=...)` instead, or write your own manager.

### 7.5 Persistence Test Example (Django TestCase)
- Inherit from `django.test.TestCase` when Django ORM is needed
- Django sets up a test database automatically
- Test database is separate from development database

```python
# products/test_models.py
from decimal import Decimal

from django.test import TestCase
from products.models import Product

class TestProduct(TestCase):
    def setUp(self) -> None:
        # Setup code runs before each test
        pass

    def test_new_product_saved(self) -> None:
        product = Product.objects.create(name="testproduct1", price=Decimal("19.99"), description="testdescription1")
        self.assertEqual(Product.objects.get_by_name("testproduct1"), product)

    def test_get_products_in_price_range(self) -> None:
        p1 = Product.objects.create(name="testproduct1", price=Decimal("11.99"), description="testdescription1")
        p2 = Product.objects.create(name="testproduct2", price=Decimal("19.99"), description="testdescription2")
        p3 = Product.objects.create(name="testproduct3", price=Decimal("29.99"), description="testdescription3")
        p4 = Product.objects.create(name="testproduct4", price=Decimal("30.00"), description="testdescription4")

        expected_products = [p2, p3]
        # two QuerySets will never be equal, even if they contain the same objects
        self.assertEqual(list(Product.objects.get_by_price_range(Decimal("19.99"), Decimal("29.99"))), expected_products)
```

### 7.6 Running Tests
```bash
# Run all tests from project root
uv run python manage.py test -v 2

# Run tests for a specific app
uv run python manage.py test products -v 2

# Run a specific test class
uv run python manage.py test products.test_models.TestProduct -v 2

# Run a specific test method
uv run python manage.py test products.test_models.TestProduct.test_new_product_saved -v 2
```

**PyCharm:** Right-click file or root directory and "Run 'Test:'"

---

## 8. Assignment: Project Implementation

### 8.1 Tasks
- [ ] Create classes for your first business entities within the app where they belong (models.py)
  - [ ] See `models.py` from product and order apps as reference
  - [ ] Reference models of other apps by string (`"app_label.ModelName"`), not by import
- [ ] Register your master-data entities to be editable in the admin site (admin.py)
  - [ ] Create some entities through the admin site for testing
- [ ] Create two unit tests:
  - [ ] Test to create new business entities, persist them to the database, reload the entity from the database and check for validity
  - [ ] Test whether associations to other objects are valid

### 8.2 Example Test Requirements
```python
# Example test for model creation and persistence
from django.test import TestCase
from myapp.models import MyModel, RelatedModel

class TestMyModelPersistence(TestCase):
    def test_create_and_retrieve_model(self):
        # Create a model instance
        obj = MyModel.objects.create(name="test", value=100)
        
        # Retrieve it from database
        retrieved = MyModel.objects.get(pk=obj.pk)
        
        # Verify it was saved correctly
        self.assertEqual(retrieved.name, "test")
        self.assertEqual(retrieved.value, 100)

    def test_model_associations(self):
        # Create related objects
        related = RelatedModel.objects.create(name="related")
        obj = MyModel.objects.create(name="test", related=related)
        
        # Retrieve and verify association
        retrieved = MyModel.objects.get(pk=obj.pk)
        self.assertEqual(retrieved.related, related)
```

### 8.3 Key Concepts to Demonstrate
- **Model definition** and inheritance
- **Cross-app references** via string foreign keys
- **Admin site** registration
- **Database persistence** testing
- **Model associations** testing

---

*Updated to match current djangolight codebase*  
*Last updated: 2026-10-03*