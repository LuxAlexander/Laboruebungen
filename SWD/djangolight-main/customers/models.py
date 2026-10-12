from decimal import Decimal

from django.db import models

from core.models import Customer as CoreCustomer
from customers.managers import CustomerManager


# this is the concrete model that we will use for the project.
# we are using the Customer model for authentication (settings.AUTH_USER_MODEL), so it builds on AbstractUser.
# for this project, only add nullable fields to the customer,
# otherwise you could run into trouble with creating superusers.
class Customer(CoreCustomer):
    objects: CustomerManager = CustomerManager()

    # we are adding a credit field to the customer model
    credit: Decimal = models.DecimalField(max_digits=10, decimal_places=2, null=True)
