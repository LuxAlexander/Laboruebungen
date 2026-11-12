from django.contrib.auth.views import LoginView
from django.urls import path

from . import views

app_name = "customers"

urlpatterns = [
    path("login/", LoginView.as_view(template_name="customers/login.html"), name="login"),
    path("register/", views.register, name="register"),
]
