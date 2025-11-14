FROM ubuntu:24.04

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        build-essential \
        libmosquitto-dev \
        libssl-dev \
        python3 \
        python3-pip \
        netcat \
    && rm -rf /var/lib/apt/lists/*

# Copy sources and tools
COPY ../../src/host_simulator.cpp /app/host_simulator.cpp
COPY ../requirements.txt /app/requirements.txt
COPY ../subscribe_simulator.py /app/subscribe_simulator.py
COPY run_test.sh /app/run_test.sh

# Build host simulator
RUN g++ -std=c++17 host_simulator.cpp -o host_simulator -lmosquitto -lpthread

RUN python3 -m pip install --no-cache-dir -r /app/requirements.txt

RUN chmod +x /app/run_test.sh

ENTRYPOINT ["/app/run_test.sh"]
