from django.db import models

"""
As is, the custom manager doesn't add any functionality to the default manager. 
The purpose of creating a custom manager is to extend the default manager with additional functionality 
or to customize the behavior of the manager.
"""


class CoreProductManager(models.Manager):
    pass
