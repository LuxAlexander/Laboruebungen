from django.contrib import admin
from django.contrib.auth.admin import UserAdmin
from .models import Customer


class CustomerAdmin(UserAdmin):
    model = Customer
    # If you have any custom fields in your Customer model, make sure to add them here
    fieldsets = UserAdmin.fieldsets + (
        (None, {'fields': ('credit',)}),  # Example for custom fields
    )
    # in case the UserAdmin has additional fields
    #add_fieldsets = UserAdmin.add_fieldsets + (
    #    (None, {'fields': ('custom_field',)}),  # Example for custom fields
    #)


# Register your custom admin with the Customer model
admin.site.register(Customer, CustomerAdmin)
