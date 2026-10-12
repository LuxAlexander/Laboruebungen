from dataclasses import dataclass
from datetime import datetime
from decimal import Decimal
from typing import Optional

"""
The records that cross the seams between the apps.

Why not pass Django models around? A model instance is an Active Record: it is bound to the database,
its id is unset until saved, and foreign keys cannot be followed before saving. Views and templates
only need to *read* data, and the basket must be priced without writing anything, so the apps
exchange these plain records instead. Only the adapters (products, customers, orders) touch models.

Why `frozen=True`? Once a record has crossed a seam it must not change under the caller's feet;
a catalog item shown in the basket has exactly the price it had when it was looked up. Frozen
dataclasses raise on attribute assignment, so accidental mutation fails loudly instead of silently
leaking across apps. Frozen records are also hashable and can be used as set members and dict keys.

Why `slots=True`? Fixed attributes, no per-instance __dict__: a typo like `item.pric = 1` raises
instead of creating a new attribute, and the records are smaller and faster to create.

Naming: the records are named for their role at the seam (what the caller sees), not for the
model they are copied from. `CatalogItem` is what the ProductCatalog shows, `CustomerInfo` is what
the Customers interface knows about a customer.

Why `Decimal` for money? Binary floats cannot represent most cent amounts exactly (0.1 + 0.2 != 0.3),
so sums drift and comparisons such as "is the credit high enough?" can flip. `Decimal` keeps cents exact
and matches the `DecimalField`s in the models.
"""


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


class OrderError(Exception):
    """Base class for the reasons an order cannot be placed. Views catch this one name."""


class EmptyBasket(OrderError):
    def __init__(self) -> None:
        super().__init__("cannot place an order from an empty basket")


class InsufficientCredit(OrderError):
    def __init__(self, required: Decimal, available: Decimal) -> None:
        self.required = required
        self.available = available
        super().__init__(f"cannot perform order without enough credit, total order price of [{required}] "
                         f"is higher than credit of [{available}]")
