from decimal import Decimal
from django.test import TestCase

from core.domain import CatalogItem
from core.interfaces import ProductCatalog
from products.models import Product
from products.services import ProductService


class TestProductService(TestCase):
    def setUp(self):
        self.apple = Product.objects.create(name="apple", description="fruit", price=Decimal("1.5"))
        self.service = ProductService()

    def test_satisfies_product_catalog(self):
        self.assertIsInstance(self.service, ProductCatalog)

    def test_get_all_products_returns_catalog_items(self):
        self.assertEqual(self.service.get_all_products(),
                         [CatalogItem(id=self.apple.id, name="apple", description="fruit", price=Decimal("1.5"))])

    def test_get_product_by_string_id(self):
        self.assertEqual(self.service.get_product(str(self.apple.id)).name, "apple")

    def test_unknown_product_is_none(self):
        self.assertIsNone(self.service.get_product(999))
