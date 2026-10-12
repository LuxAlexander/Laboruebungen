# this can be seen as our repository for our model
from decimal import Decimal
from typing import TYPE_CHECKING, Optional

from django.db import models

from core.managers import CoreProductManager

if TYPE_CHECKING:
    from products.models import Product


class ProductManager(CoreProductManager):
    def get_by_id(self, id: int) -> Optional["Product"]:
        return self.get_queryset().filter(id=id).first()

    def get_by_name(self, name: str) ->  Optional["Product"]:
        return self.get_queryset().filter(name=name).first()

    def get_by_price_range(self, min_price: Decimal, max_price: Decimal) -> models.QuerySet["Product"]:
        # Query for Product objects by price range
        return self.get_queryset().filter(price__gte=min_price, price__lte=max_price)
