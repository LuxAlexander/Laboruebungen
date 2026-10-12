from decimal import Decimal
from typing import Optional, Protocol, runtime_checkable

from core.domain import CustomerInfo, CatalogItem
from core.models import Customer

"""
The interfaces at the seams between the apps. They are Protocols (PEP 544): an adapter satisfies
an interface by having the right methods, no inheritance needed. Callers depend only on these
and on the records in core.domain, never on a concrete app's models.
"""


@runtime_checkable
class ProductCatalog(Protocol):
    def get_all_products(self) -> list[CatalogItem]: ...

    def get_product(self, product_id) -> Optional[CatalogItem]: ...


@runtime_checkable
class Customers(Protocol):
    def redeem_credit(self, customer: Customer, amount: Decimal) -> None:
        """Deducts `amount` from the customer's credit or raises InsufficientCredit; writes nothing on failure."""

    def find_customer(self, username: str) -> Optional[CustomerInfo]: ...
