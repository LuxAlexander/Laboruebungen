from typing import Optional

from core.domain import CustomerInfo
from customers.models import Customer


class CustomerService:
    """Adapter for core.interfaces.Customers, backed by the customers app's model."""

    def __init__(self, repo=None):
        # default to Customer.objects, but allow tests to pass a fake
        self.repo = repo or Customer.objects

    def find_customer(self, username: str) -> Optional[CustomerInfo]:
        customer: Optional[Customer] = self.repo.get_by_username(username=username)
        if customer:
            return CustomerInfo(id=customer.id, username=customer.username, email=customer.email,
                                first_name=customer.first_name, last_name=customer.last_name)
        return None
