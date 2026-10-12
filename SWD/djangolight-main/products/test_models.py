from decimal import Decimal
from django.test import TestCase
from products.models import Product


# This test class inherits from the TestCase class, which is provided by Django (not form unittest),
# as it provides some additional functionality that we need for testing Django models.
class TestProduct(TestCase):
    def setUp(self) -> None:
        pass

    def test_new_product_saved(self) -> None:
        product = Product.objects.create(name="testproduct1", price=Decimal("19.99"), description="testdescription1")
        self.assertEqual(Product.objects.get_by_name("testproduct1"), product)

    def test_get_products_in_price_range(self) -> None:
        p1 = Product.objects.create(name="testproduct1", price=Decimal("11.99"), description="testdescription1")
        p2 = Product.objects.create(name="testproduct2", price=Decimal("19.99"), description="testdescription2")
        p3 = Product.objects.create(name="testproduct3", price=Decimal("29.99"), description="testdescription3")
        p4 = Product.objects.create(name="testproduct4", price=Decimal("30.00"), description="testdescription4")

        expected_products = list([p2, p3])
        # two QuerySets will never be equal, even if they contain the same objects, so we need to convert them to lists
        self.assertEqual(list(Product.objects.get_by_price_range(Decimal("19.99"), Decimal("29.99"))), expected_products)
