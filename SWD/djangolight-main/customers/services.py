from decimal import Decimal
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
            # in many other ORMs we do not need to call save() explicitly, as the ORM will keep track of
            # the changes and commit them to the database when the transaction is committed
            # personal remark (bid): Django ORM is not the best ORM out there ...

    def get_by_username(self, username: str) -> Optional[Customer]:
        return self.repo.get_by_username(username=username)

    def find_customer(self, username: str) -> Optional[CustomerInfo]:
        customer: Optional[Customer] = self.get_by_username(username=username)
        if customer:
            return CustomerInfo(id=customer.id, username=customer.username, email=customer.email,
                                first_name=customer.first_name, last_name=customer.last_name)
        return None
