"""URL configuration of the ticketing system: one URL prefix per app."""

from django.contrib import admin
from django.contrib.staticfiles.urls import staticfiles_urlpatterns
from django.urls import include, path

urlpatterns = [
    path("", include("core.urls")),  # site root redirects to the matches
    path("tickets/", include("orders.urls")),  # UC 4.1 single-match ticket sale
    path("gate/", include("access.urls")),  # UC 4.2 access control (gate simulation, alerts)
    path("stadium/", include("stadiums.urls")),  # statistics
    path("accounts/", include("customers.urls")),  # login, register
    path("admin/", admin.site.urls),  # master data maintenance, match status
]

urlpatterns += staticfiles_urlpatterns()
