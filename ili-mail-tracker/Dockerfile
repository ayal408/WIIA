FROM python:3.12-slim
WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt && useradd --create-home tracker && mkdir /data && chown tracker:tracker /data
COPY app.py .
USER tracker
ENV TRACKER_DB=/data/tracker.sqlite3
EXPOSE 8080
CMD ["gunicorn", "--bind", "0.0.0.0:8080", "--workers", "1", "--threads", "4", "app:from_env()"]
