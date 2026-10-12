from django.contrib.auth.views import LogoutView
from django.urls import path

from . import views
from .index_view import index

app_name = "core"

# Map URLs to views
urlpatterns = [
    # the site root "/" redirects to the orders app
    path("", index, name="index"),
    path("health", views.health_check, name="health_check"),
    # logout must be a POST (Django >= 5); the navigation renders a small form for it
    path("logout", LogoutView.as_view(next_page="products:index"), name="logout"),
]
