from typing import Optional

from core.domain import CatalogItem
from products.models import Product


# adapter for the ProductCatalog interface, backed by the products app's model
class ProductService:
    def __init__(self, repo=None):
        # default to Product.objects, but allow tests to pass a fake
        self.repo = repo or Product.objects

    def get_all_products(self) -> list[CatalogItem]:
        return [self._to_item(product) for product in self.repo.all()]

    def get_product(self, product_id) -> Optional[CatalogItem]:
        product: Optional[Product] = self.repo.get_by_id(product_id)
        return self._to_item(product) if product else None

    def _to_item(self, product: Product) -> CatalogItem:
        # in another product app, the logic for getting the price could be different
        return CatalogItem(id=product.id, name=product.name, description=product.description, price=product.price)
