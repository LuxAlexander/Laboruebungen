from django.contrib.auth.models import AbstractUser


class Customer(AbstractUser):
    """
    Abstract base of the user model. customers.Customer is the concrete model (settings.AUTH_USER_MODEL);
    by inheriting from AbstractUser it keeps Django's login, logout and password handling.
    """

    class Meta:
        abstract = True

    def __str__(self) -> str:
        return self.username
