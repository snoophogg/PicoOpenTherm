FROM python:3.11-slim

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY ../requirements.txt /app/requirements.txt
COPY ../subscribe_simulator.py /app/subscribe_simulator.py
COPY publisher.py /app/publisher.py
COPY run_test.sh /app/run_test.sh

RUN python3 -m pip install --no-cache-dir -r /app/requirements.txt

RUN chmod +x /app/run_test.sh

ENTRYPOINT ["/app/run_test.sh"]
