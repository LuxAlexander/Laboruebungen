from core.models import Customer as CoreCustomer
from customers.managers import CustomerManager


class Customer(CoreCustomer):
    """
    A registered customer; also the Django user model (settings.AUTH_USER_MODEL), so staff are customers with
    is_staff set. Registration is required for season tickets and for buying more than one ticket online.
    Only add nullable fields here, otherwise `createsuperuser` gets into trouble.
    """
    objects: CustomerManager = CustomerManager()
