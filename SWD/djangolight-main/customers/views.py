from django.contrib.auth import login
from django.contrib.auth.forms import UserCreationForm
from django.http import HttpRequest, HttpResponse
from django.shortcuts import redirect, render
from django.utils.http import url_has_allowed_host_and_scheme

from customers.models import Customer


class RegistrationForm(UserCreationForm):
    class Meta(UserCreationForm.Meta):
        model = Customer
        fields = ("username", "email", "first_name", "last_name")


def register(request: HttpRequest) -> HttpResponse:
    """Creates a customer account and logs it in."""
    next_url = request.POST.get("next") or request.GET.get("next") or ""
    if request.method == "POST":
        form = RegistrationForm(request.POST)
        if form.is_valid():
            login(request, form.save())
            if url_has_allowed_host_and_scheme(next_url, allowed_hosts={request.get_host()}):
                return redirect(next_url)
            return redirect("orders:match_list")
    else:
        form = RegistrationForm()
    return render(request, "customers/register.html", {"form": form, "next": next_url})
