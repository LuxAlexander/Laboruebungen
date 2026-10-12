from typing import TYPE_CHECKING, Optional

from django.contrib.auth.models import UserManager

if TYPE_CHECKING:
    from customers.models import Customer


# UserManager (not BaseUserManager) provides create_user/create_superuser for `manage.py createsuperuser`
class CustomerManager(UserManager):
    def get_by_username(self, username: str) -> Optional["Customer"]:
        return self.get_queryset().filter(username=username).first()
