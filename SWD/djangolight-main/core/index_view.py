from django.http import HttpRequest, HttpResponseRedirect
from django.shortcuts import redirect


def index(request: HttpRequest) -> HttpResponseRedirect:
    """The site root has no content of its own, visitors are sent to the product catalog."""
    return redirect("products:index")
