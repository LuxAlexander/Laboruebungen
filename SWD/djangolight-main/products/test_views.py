from decimal import Decimal
from unittest.mock import MagicMock

from dependency_injector import containers, providers
from django.test import TestCase
from django.urls import reverse

import swd_django_demo
from core.domain import CatalogItem
from core.interfaces import ProductCatalog


class TestContainer(containers.DeclarativeContainer):
    """replaces the catalog adapter, so the view is tested without a database"""
    product_service = providers.Singleton(MagicMock, spec=ProductCatalog)


class TestProductIndex(TestCase):
    def setUp(self):
        self.items = [
            CatalogItem(id=1, name="testproduct1", description="testdescription1", price=Decimal("1.0")),
            CatalogItem(id=2, name="testproduct2", description="testdescription2", price=Decimal("2.5")),
        ]
        container = TestContainer()
        container.product_service().get_all_products.return_value = self.items
        container.wire(modules=["products.views"])

    def tearDown(self):
        swd_django_demo.get_container().wire(modules=["products.views"])

    def test_index_lists_the_catalog(self):
        response = self.client.get(reverse("products:index"))

        self.assertEqual(response.status_code, 200)
        self.assertTemplateUsed(response, "products/index.html")
        self.assertEqual(list(response.context["products_list"]), self.items)
        self.assertContains(response, "testproduct2")
        self.assertContains(response, "2.50 &euro;")

    def test_index_is_mounted_at_products(self):
        self.assertEqual(reverse("products:index"), "/products/")
