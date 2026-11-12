# repository for orders
from django.db import models


class OrderManager(models.Manager):
    def with_details(self) -> models.QuerySet:
        return self.get_queryset().prefetch_related("positions__ticket__seat__sector")
