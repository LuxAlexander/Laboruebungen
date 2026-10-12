"""swd_django_demo URL Configuration

The `urlpatterns` list routes URLs to views. For more information please see:
    https://docs.djangoproject.com/en/4.1/topics/http/urls/
Examples:
Function views
    1. Add an import:  from my_app import views
    2. Add a URL to urlpatterns:  path('', views.home, name='home')
Class-based views
    1. Add an import:  from other_app.views import Home
    2. Add a URL to urlpatterns:  path('', Home.as_view(), name='home')
Including another URLconf
    1. Import the include() function: from django.urls import include, path
    2. Add a URL to urlpatterns:  path('blog/', include('blog.urls'))
"""

from django.contrib import admin
from django.contrib.staticfiles.urls import staticfiles_urlpatterns
from django.urls import include, path

# Map URLs to views
urlpatterns = [
    # Map the "/" URL to the "core" application's URLs (the site root redirects to the catalog)
    path("", include("core.urls")),
    # Map the "products/" URL to the product catalog
    path("products/", include("products.urls")),
    # Map the "orders/" URL to the "orders" application's URLs
    path("orders/", include("orders.urls")),
    # Map the "admin/" URL to the Django admin site
    path("admin/", admin.site.urls),
]

# Serve static files in development
urlpatterns += staticfiles_urlpatterns()
