#!/bin/sh

## initial database migration
## in a production environment this would typicalle not be put here. the database would be maintained seperately 
python /opt/django-app/manage.py migrate
python /opt/django-app/manage.py createsuperuser --noinput --username admin --email admin@localhost

python /opt/django-app/run_gunicorn.py -w 4 -b 0.0.0.0:8000 swd_django_demo.wsgi:application