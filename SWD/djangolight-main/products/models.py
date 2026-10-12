from decimal import Decimal

from django.db import models

from core.models import Product as CoreProduct
from products.managers import ProductManager


# this is the concrete model that we will use for the project
class Product(CoreProduct):
    objects: ProductManager = ProductManager()

    # we are adding a price field to the product model
    price: Decimal = models.DecimalField(max_digits=10, decimal_places=2)
