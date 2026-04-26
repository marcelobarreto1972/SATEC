FROM python:3.12-alpine

WORKDIR /app

RUN apk update && \
    apk add --virtual build-deps gcc python3-dev musl-dev && \
    apk add --no-cache postgresql-dev

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

RUN apk del build-deps

COPY subscriber.py .

CMD ["python", "-u", "subscriber.py"]