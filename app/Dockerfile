
FROM python:3.12-alpine
 
WORKDIR /app
 
RUN apk add --no-cache gcc musl-dev libpq-dev
 
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt
 
COPY subscriber.py .
 
CMD ["python", "-u", "subscriber.py"]