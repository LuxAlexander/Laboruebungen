from django.contrib import admin

from .models import Product

"""

This is a Django admin file. 
It imports the Product model from the models module and registers it with the Django admin site. 
This allows administrators to create, read, update and delete Product objects through the Django admin interface.
"""
admin.site.register(Product)