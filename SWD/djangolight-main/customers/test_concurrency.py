import threading
from decimal import Decimal

from django.db import OperationalError, connection
from django.test import TransactionTestCase, skipUnlessDBFeature

from core.domain import InsufficientCredit
from customers.models import Customer
from customers.services import CustomerService

"""
Several customers' sessions redeem credit from the same account at the same time.
TransactionTestCase, because every thread uses its own database connection and must see committed data.
"""

def _is_lock_error(e: Exception) -> bool:
    return isinstance(e, OperationalError) and "locked" in str(e)


CREDIT = Decimal("100")
AMOUNT = Decimal("10")
THREADS = 11  # one more than the credit covers


class CustomerServiceConcurrencyTestCase(TransactionTestCase):
    def setUp(self):
        self.customer = Customer.objects.create(username="alice", credit=CREDIT)
        self.customer_service = CustomerService()

    def _redeem_concurrently(self) -> tuple[int, list[Exception]]:
        """Starts all redemptions together; returns the number of successes and the raised exceptions."""
        start = threading.Barrier(THREADS)
        lock = threading.Lock()
        successes: list[None] = []
        errors: list[Exception] = []

        def redeem():
            try:
                start.wait()
                self.customer_service.redeem_credit(self.customer, AMOUNT)
                with lock:
                    successes.append(None)
            except Exception as e:
                with lock:
                    errors.append(e)
            finally:
                # every thread opened its own connection
                connection.close()

        threads = [threading.Thread(target=redeem) for _ in range(THREADS)]
        for thread in threads:
            thread.start()
        for thread in threads:
            thread.join()
        return len(successes), errors

    def test_no_redemption_is_lost_and_credit_never_goes_negative(self):
        # holds on every database, also on SQLite where some threads fail with "database is locked"
        successes, errors = self._redeem_concurrently()

        # the only acceptable failures: the business rule, or SQLite refusing the concurrent write.
        # SQLite may even lock out every thread, so the number of successes is checked by the row-lock test below
        unexpected = [e for e in errors if not (isinstance(e, InsufficientCredit) or _is_lock_error(e))]
        self.assertEqual(unexpected, [])
        credit = Customer.objects.get().credit
        self.assertEqual(credit, CREDIT - successes * AMOUNT)
        self.assertGreaterEqual(credit, 0)

    @skipUnlessDBFeature("has_select_for_update")
    def test_row_lock_lets_exactly_the_covered_redemptions_through(self):
        # with a real row lock the redemptions are serialized: ten fit into the credit, the eleventh is rejected
        successes, errors = self._redeem_concurrently()

        self.assertEqual(successes, 10)
        self.assertEqual([type(e) for e in errors], [InsufficientCredit])
        self.assertEqual(Customer.objects.get().credit, Decimal("0"))
