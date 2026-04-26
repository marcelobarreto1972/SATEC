FROM python:3.12-alpine
 
WORKDIR /app
 
# libpq is required at runtime by psycopg's pure-Python implementation

RUN apk update && \
    apk add --virtual --no-cache build-deps gcc python3-dev musl-dev && \
    apk add --no-cache postgresql-dev
 
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt
 
COPY subscriber.py .
 
CMD ["python", "-u", "subscriber.py"]
 