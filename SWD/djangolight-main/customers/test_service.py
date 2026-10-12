from decimal import Decimal

from django.test import TestCase

from core.domain import InsufficientCredit
from core.interfaces import Customers
from customers.models import Customer
from customers.services import CustomerService

"""
The CustomerService is tested through its interface against the SQLite test database,
because credit redemption is about locking and updating a row.
"""


class TestCustomerService(TestCase):
    def setUp(self):
        self.customer = Customer.objects.create(username="alice", credit=100)
        self.customer_service = CustomerService()

    def test_satisfies_customers(self):
        self.assertIsInstance(self.customer_service, Customers)

    def test_redeem_reduces_credit(self):
        self.customer_service.redeem_credit(self.customer, 30)

        self.customer.refresh_from_db()
        self.assertEqual(self.customer.credit, 70)

    def test_redeem_exact_credit_in_cents(self):
        # credit 0.30 must cover an order of 0.10 + 0.20; with floats the sum exceeds 0.3 and is rejected
        Customer.objects.filter(pk=self.customer.pk).update(credit=Decimal("0.30"))

        self.customer_service.redeem_credit(self.customer, Decimal("0.10") + Decimal("0.20"))

        self.customer.refresh_from_db()
        self.assertEqual(self.customer.credit, Decimal("0.00"))

    def test_insufficient_credit_raises_with_fresh_values(self):
        # the caller holds a stale object, the database row is what counts
        Customer.objects.filter(pk=self.customer.pk).update(credit=5)

        with self.assertRaises(InsufficientCredit) as ctx:
            self.customer_service.redeem_credit(self.customer, 10)

        self.assertEqual((ctx.exception.required, ctx.exception.available), (10, 5))
        self.customer.refresh_from_db()
        self.assertEqual(self.customer.credit, 5)

    def test_no_credit_raises(self):
        Customer.objects.filter(pk=self.customer.pk).update(credit=None)

        with self.assertRaises(InsufficientCredit) as ctx:
            self.customer_service.redeem_credit(self.customer, 1)

        self.assertEqual(ctx.exception.available, Decimal("0"))
